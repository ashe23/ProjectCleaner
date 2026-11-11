// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Commandlets/PjcCommandlet.h"

#include "PjcSettings.h"
#include "PjcShim.h"
#include "PjcSubsystem.h"
#include "PjcTypes.h"
#include "Dom/JsonObject.h"
#include "Engine/AssetManager.h"
#include "JsonObjectConverter.h"
#include "Misc/App.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

DEFINE_LOG_CATEGORY_STATIC(LogProjectCleanerCLI, Display, All);

namespace
{
	TArray<TSharedPtr<FJsonValue>> JsonStrings(const TArray<FString>& Items) {
		TArray<FString> Sorted = Items;
		Sorted.Sort();

		TArray<TSharedPtr<FJsonValue>> Values;
		Values.Reserve(Sorted.Num());
		for (const auto& Item : Sorted) {
			Values.Emplace(MakeShared<FJsonValueString>(Item));
		}
		return Values;
	}

	TArray<TSharedPtr<FJsonValue>> JsonAssets(const TArray<FAssetData>& Assets) {
		TArray<FString> Paths;
		Paths.Reserve(Assets.Num());
		for (const auto& Asset : Assets) {
			Paths.Emplace(PjcShim::GetObjectPathString(Asset));
		}
		return JsonStrings(Paths);
	}

	TSharedRef<FJsonObject> JsonSettings(const UObject* Settings) {
		TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		FJsonObjectConverter::UStructToJsonObject(Settings->GetClass(), Settings, Object, CPF_Config, 0);
		return Object;
	}

	// shared scan result: status counts, redirectors apart, editor only assets and what starts the chains of used ones
	TSharedRef<FJsonObject> JsonScanResult() {
		UPjcSubsystem::Scan(false);
		const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();

		int32 NumUsed = 0;
		int32 NumUnused = 0;
		TArray<FAssetData> EditorOnly;
		TArray<FAssetData> Review;
		TMap<FString, int32> ChainReasons;
		for (const auto& Asset : Result.Assets) {
			const EPjcAssetStatus Status = Result.GetStatus(Asset.PackageName);
			if (Status == EPjcAssetStatus::Unused) {
				++NumUnused;
				continue;
			}

			NumUsed += Status == EPjcAssetStatus::Used ? 1 : 0;
			if (Status == EPjcAssetStatus::EditorOnly) {
				EditorOnly.Add(Asset);
			}
			if (Status == EPjcAssetStatus::Review) {
				Review.Add(Asset);
			}
			ChainReasons.FindOrAdd(UEnum::GetValueAsString(Result.GetChainReason(Asset.PackageName)))++;
		}

		const FPjcCookCheck& Check = Result.CookCheck;
		const TSharedRef<FJsonObject> CookCheck = MakeShared<FJsonObject>();
		CookCheck->SetArrayField(TEXT("platforms"), JsonStrings(Check.Platforms));
		CookCheck->SetNumberField(TEXT("checked"), Check.NumChecked);
		CookCheck->SetNumberField(TEXT("not_checked"), Check.NumNotChecked);
		CookCheck->SetNumberField(TEXT("packaged"), Check.NumPackaged);
		CookCheck->SetArrayField(TEXT("platforms_cook_all"), JsonStrings(Check.PlatformsCookAll));

		const TSharedRef<FJsonObject> Reasons = MakeShared<FJsonObject>();
		for (const auto& Pair : ChainReasons) {
			Reasons->SetNumberField(Pair.Key, Pair.Value);
		}

		const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetNumberField(TEXT("assets"), Result.Assets.Num());
		Object->SetNumberField(TEXT("used"), NumUsed);
		Object->SetNumberField(TEXT("editor_only"), EditorOnly.Num());
		Object->SetNumberField(TEXT("review"), Review.Num());
		Object->SetNumberField(TEXT("unused"), NumUnused);
		Object->SetNumberField(TEXT("redirectors"), Result.Redirectors.Num());
		Object->SetNumberField(TEXT("files_external"), Result.FilesExternal.Num());
		Object->SetNumberField(TEXT("files_external_kept"), Result.FilesExternalKept.Num());
		Object->SetNumberField(TEXT("files_corrupted"), Result.FilesCorrupted.Num());
		Object->SetNumberField(TEXT("folders_empty"), Result.FoldersEmpty.Num());
		Object->SetNumberField(TEXT("seconds"), Result.Seconds);
		Object->SetObjectField(TEXT("chain_reasons"), Reasons);
		Object->SetArrayField(TEXT("assets_editor_only"), JsonAssets(EditorOnly));
		Object->SetArrayField(TEXT("assets_review"), JsonAssets(Review));
		Object->SetObjectField(TEXT("cook_check"), CookCheck);
		Object->SetArrayField(TEXT("redirectors_list"), JsonAssets(Result.Redirectors));
		return Object;
	}
}	// namespace

UPjcCommandlet::UPjcCommandlet() {
	IsServer = false;
	LogToConsole = false;
	ShowErrorCount = false;
	ShowProgress = false;
}

int32 UPjcCommandlet::Main(const FString& Params) {
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("===================================="));
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("======  Project Cleaner CLI  ======="));
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("===================================="));

	ParseCommandLinesArguments(Params);

	// cli arguments
	//- scan_only
	//- full_cleanup
	//- delete_assets_unused
	//- delete_folders_empty
	//- delete_files_external
	//- delete_files_corrupted
	//- fix_redirectors
	//- list
	//- report=<file.json>

	// asset discovery can still run in background, every query below needs complete registry
	UPjcSubsystem::GetModuleAssetRegistry().Get().SearchAllAssets(true);

	if (!ReportPath.IsEmpty()) {
		ReportInit();
	}

	// redirectors are not assets: scans and cleanups leave them, only fixing them removes them
	if (bFixRedirectors && UPjcSubsystem::ProjectHasRedirectors()) {
		TArray<FAssetData> Redirectors;
		UPjcSubsystem::GetProjectRedirectors(Redirectors);
		UE_LOG(LogProjectCleanerCLI, Display, TEXT("Fixing %d redirectors..."), Redirectors.Num());
		UPjcSubsystem::FixProjectRedirectors(Redirectors, false);

		UPjcSubsystem::GetProjectRedirectors(Redirectors);
		for (const auto& Redirector : Redirectors) {
			UE_LOG(LogProjectCleanerCLI, Warning, TEXT("Redirector not fixed: %s"), *PjcShim::GetObjectPathString(Redirector));
		}
	}

	const FCleanupStats StatsBefore = StatsCollect(bList, TEXT("before"));

	UE_LOG(LogProjectCleanerCLI, Display, TEXT("======================================"));
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("========     Project Stats    ========"));
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("======================================"));
	StatsPrint(StatsBefore);

	if (bScanOnly) {
		ReportWrite(TEXT("scanned"));
		return 0;
	}

	bool bSuccess = true;

	if (bFullCleanup || bDeleteAssetsUnused) {
		bSuccess &= UPjcSubsystem::DeleteAssetsUnused(false, false);
	}

	if (bFullCleanup || bDeleteFilesExternal) {
		bSuccess &= UPjcSubsystem::DeleteFilesExternal(false, false);
	}

	if (bFullCleanup || bDeleteFilesCorrupted) {
		bSuccess &= UPjcSubsystem::DeleteFilesCorrupted(false, false);
	}

	if (bFullCleanup || bDeleteFoldersEmpty) {
		bSuccess &= UPjcSubsystem::DeleteFoldersEmpty(false, false);
	}

	const bool bDeletes = bFullCleanup || bDeleteAssetsUnused || bDeleteFilesExternal || bDeleteFilesCorrupted || bDeleteFoldersEmpty;
	if (bDeletes) {
		const FCleanupStats StatsAfter = StatsCollect(bList, TEXT("after"));

		UE_LOG(LogProjectCleanerCLI, Display, TEXT("======================================"));
		UE_LOG(LogProjectCleanerCLI, Display, TEXT("========  Stats After Cleanup ========"));
		UE_LOG(LogProjectCleanerCLI, Display, TEXT("======================================"));
		StatsPrint(StatsAfter);
	}

	if (!bSuccess) {
		UE_LOG(LogProjectCleanerCLI, Error, TEXT("Cleanup finished with errors, see log above."));
	}
	else if (const int32 NumKept = UPjcSubsystem::GetLastCleanupKept().Num()) {
		// kept on purpose, not an error: the exit code stays 0
		UE_LOG(LogProjectCleanerCLI, Warning, TEXT("Kept %d asset(s) that are still referenced, see warnings above."), NumKept);
	}

	// without a delete switch nothing was cleaned: the run only scanned (and fixed redirectors when asked)
	ReportWrite(!bSuccess ? TEXT("failed") : (bDeletes ? TEXT("cleaned") : TEXT("scanned")));

	return bSuccess ? 0 : 1;
}

void UPjcCommandlet::ParseCommandLinesArguments(const FString& Params) {
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> Parameters;
	ParseCommandLine(*Params, Tokens, Switches, Parameters);

	for (const auto& Switch : Switches) {
		if (Switch.Equals(TEXT("scan_only"))) {
			bScanOnly = true;
		}

		if (Switch.Equals(TEXT("full_cleanup"))) {
			bFullCleanup = true;
		}

		if (Switch.Equals(TEXT("delete_assets_unused"))) {
			bDeleteAssetsUnused = true;
		}

		if (Switch.Equals(TEXT("delete_folders_empty"))) {
			bDeleteFoldersEmpty = true;
		}

		if (Switch.Equals(TEXT("delete_files_external"))) {
			bDeleteFilesExternal = true;
		}

		if (Switch.Equals(TEXT("delete_files_corrupted"))) {
			bDeleteFilesCorrupted = true;
		}

		if (Switch.Equals(TEXT("fix_redirectors"))) {
			bFixRedirectors = true;
		}

		if (Switch.Equals(TEXT("list"))) {
			bList = true;
		}
	}

	if (const FString* Path = Parameters.Find(TEXT("report"))) {
		ReportPath = Path->TrimQuotes();
	}
}

UPjcCommandlet::FCleanupStats UPjcCommandlet::StatsCollect(const bool bPrintLists, const FString& ReportField) {
	FPjcScanCacheScope ScanCacheScope;

	TArray<FAssetData> AssetsAll;
	TArray<FAssetData> AssetsUsed;
	TArray<FAssetData> AssetsUnused;
	TArray<FString> FoldersEmpty;
	TArray<FString> FilesExternal;
	TArray<FString> FilesCorrupted;

	UPjcSubsystem::GetAssetsAll(AssetsAll);
	UPjcSubsystem::GetAssetsUsed(AssetsUsed, false);
	UPjcSubsystem::GetAssetsUnused(AssetsUnused, false);
	UPjcSubsystem::GetFoldersEmpty(FoldersEmpty);
	UPjcSubsystem::GetFilesExternalFiltered(FilesExternal, false);
	UPjcSubsystem::GetFilesCorrupted(FilesCorrupted, false);

	if (bPrintLists) {
		ListPrint(TEXT("Assets Unused"), AssetsUnused);
		ListPrint(TEXT("Folders Empty"), FoldersEmpty);
		ListPrint(TEXT("Files External"), FilesExternal);
		ListPrint(TEXT("Files Corrupted"), FilesCorrupted);
	}

	const FCleanupStats Stats {AssetsAll.Num(), AssetsUsed.Num(), AssetsUnused.Num(), FoldersEmpty.Num(), FilesExternal.Num(), FilesCorrupted.Num()};

	if (Report.IsValid()) {
		TArray<FAssetData> AssetsPrimary;
		TArray<FAssetData> AssetsEditor;
		TArray<FAssetData> AssetsIndirect;
		TArray<FAssetData> AssetsExcluded;
		TArray<FAssetData> AssetsEngineReferenced;
		TArray<FAssetData> AssetsFolderLabelOnly;
		TArray<FAssetData> Redirectors;
		TArray<FPjcAssetIndirectInfo> AssetsIndirectInfos;
		TArray<FString> FilesExternalExcluded;

		UPjcSubsystem::GetAssetsPrimary(AssetsPrimary, false);
		UPjcSubsystem::GetAssetsEditor(AssetsEditor, false);
		UPjcSubsystem::GetAssetsIndirect(AssetsIndirect, AssetsIndirectInfos, false);
		UPjcSubsystem::GetAssetsExcluded(AssetsExcluded, false);
		UPjcSubsystem::GetAssetsEngineReferenced(AssetsEngineReferenced, false);
		UPjcSubsystem::GetAssetsFolderLabelOnly(AssetsFolderLabelOnly, false);
		UPjcSubsystem::GetFilesExternalExcluded(FilesExternalExcluded, false);
		UPjcSubsystem::GetProjectRedirectors(Redirectors);

		const TSharedRef<FJsonObject> StatsJson = MakeShared<FJsonObject>();
		StatsJson->SetNumberField(TEXT("assets_all"), Stats.NumAssetsAll);
		StatsJson->SetNumberField(TEXT("assets_used"), Stats.NumAssetsUsed);
		StatsJson->SetNumberField(TEXT("assets_unused"), Stats.NumAssetsUnused);
		StatsJson->SetNumberField(TEXT("folders_empty"), Stats.NumFoldersEmpty);
		StatsJson->SetNumberField(TEXT("files_external"), Stats.NumFilesExternal);
		StatsJson->SetNumberField(TEXT("files_corrupted"), Stats.NumFilesCorrupted);

		const TSharedRef<FJsonObject> Block = MakeShared<FJsonObject>();
		Block->SetObjectField(TEXT("stats"), StatsJson);
		Block->SetArrayField(TEXT("assets_unused"), JsonAssets(AssetsUnused));
		Block->SetArrayField(TEXT("assets_primary"), JsonAssets(AssetsPrimary));
		Block->SetArrayField(TEXT("assets_editor"), JsonAssets(AssetsEditor));
		Block->SetArrayField(TEXT("assets_indirect"), JsonAssets(AssetsIndirect));
		Block->SetArrayField(TEXT("assets_excluded"), JsonAssets(AssetsExcluded));
		Block->SetArrayField(TEXT("assets_engine_referenced"), JsonAssets(AssetsEngineReferenced));
		Block->SetArrayField(TEXT("assets_folder_label_only"), JsonAssets(AssetsFolderLabelOnly));
		Block->SetArrayField(TEXT("folders_empty"), JsonStrings(FoldersEmpty));
		Block->SetArrayField(TEXT("files_external"), JsonStrings(FilesExternal));
		Block->SetArrayField(TEXT("files_external_excluded"), JsonStrings(FilesExternalExcluded));
		Block->SetArrayField(TEXT("files_corrupted"), JsonStrings(FilesCorrupted));
		Block->SetArrayField(TEXT("redirectors"), JsonAssets(Redirectors));
		Block->SetObjectField(TEXT("scan_result"), JsonScanResult());
		Report->SetObjectField(ReportField, Block);
	}

	return Stats;
}

void UPjcCommandlet::ReportInit() {
	Report = MakeShared<FJsonObject>();
	Report->SetStringField(TEXT("engine"), FEngineVersion::Current().ToString());
	Report->SetStringField(TEXT("project"), FApp::GetProjectName());
	Report->SetArrayField(TEXT("roots"), JsonStrings(UPjcSubsystem::GetProjectRootPaths()));
	Report->SetArrayField(TEXT("content_dirs"), JsonStrings(UPjcSubsystem::GetProjectContentDirs()));
	Report->SetObjectField(TEXT("scan_settings"), JsonSettings(GetDefault<UPjcScanSettings>()));
	Report->SetObjectField(TEXT("asset_exclude_settings"), JsonSettings(GetDefault<UPjcAssetExcludeSettings>()));
	Report->SetObjectField(TEXT("file_exclude_settings"), JsonSettings(GetDefault<UPjcFileExcludeSettings>()));

	// what the engine itself considers primary assets, independent of plugin classification (used by tests)
	TArray<FString> PrimaryAssets;
	if (PjcShim::IsAssetManagerValid()) {
		const UAssetManager& AssetManager = UAssetManager::Get();
		TArray<FPrimaryAssetTypeInfo> TypeInfos;
		AssetManager.GetPrimaryAssetTypeInfoList(TypeInfos);
		for (const auto& TypeInfo : TypeInfos) {
			TArray<FPrimaryAssetId> Ids;
			AssetManager.GetPrimaryAssetIdList(TypeInfo.PrimaryAssetType, Ids);
			for (const auto& Id : Ids) {
				PrimaryAssets.Emplace(Id.ToString() + TEXT(" ") + AssetManager.GetPrimaryAssetPath(Id).ToString());
			}
		}
	}
	Report->SetArrayField(TEXT("asset_manager_primary_assets"), JsonStrings(PrimaryAssets));
}

void UPjcCommandlet::ReportWrite(const FString& Result) const {
	if (!Report.IsValid()) return;

	Report->SetStringField(TEXT("result"), Result);

	FString Json;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
	FJsonSerializer::Serialize(Report.ToSharedRef(), Writer);

	if (FFileHelper::SaveStringToFile(Json, *ReportPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)) {
		UE_LOG(LogProjectCleanerCLI, Display, TEXT("Report written to %s"), *ReportPath);
	}
	else {
		UE_LOG(LogProjectCleanerCLI, Error, TEXT("Failed to write report to %s"), *ReportPath);
	}
}

void UPjcCommandlet::StatsPrint(const FCleanupStats& Stats) {
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("Assets All - %d"), Stats.NumAssetsAll);
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("Assets Used - %d"), Stats.NumAssetsUsed);
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("Assets Unused - %d"), Stats.NumAssetsUnused);
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("================="));
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("Files External - %d"), Stats.NumFilesExternal);
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("Files Corrupted - %d"), Stats.NumFilesCorrupted);
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("Folders Empty - %d"), Stats.NumFoldersEmpty);
}

void UPjcCommandlet::ListPrint(const FString& Title, const TArray<FAssetData>& Assets) {
	TArray<FString> Paths;
	Paths.Reserve(Assets.Num());
	for (const auto& Asset : Assets) {
		Paths.Add(PjcShim::GetObjectPathString(Asset));
	}

	ListPrint(Title, Paths);
}

void UPjcCommandlet::ListPrint(const FString& Title, const TArray<FString>& Items) {
	TArray<FString> Sorted = Items;
	Sorted.Sort();

	UE_LOG(LogProjectCleanerCLI, Display, TEXT("%s:"), *Title);
	for (const auto& Item : Sorted) {
		UE_LOG(LogProjectCleanerCLI, Display, TEXT("	%s"), *Item);
	}
}
