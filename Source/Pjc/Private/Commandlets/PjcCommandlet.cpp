// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Commandlets/PjcCommandlet.h"

#include "PjcShim.h"
#include "PjcSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogProjectCleanerCLI, Display, All);

UPjcCommandlet::UPjcCommandlet() {
	IsServer = false;
	LogToConsole = false;
	ShowErrorCount = false;
	ShowProgress = false;
}

int32 UPjcCommandlet::Main(const FString& Params) {
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("===================================="));
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("=======  ProjectCleaner CLI  ======="));
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

	// asset discovery can still run in background, every query below needs complete registry
	UPjcSubsystem::GetModuleAssetRegistry().Get().SearchAllAssets(true);

	if (bFixRedirectors && UPjcSubsystem::ProjectHasRedirectors()) {
		TArray<FAssetData> Redirectors;
		UPjcSubsystem::GetProjectRedirectors(Redirectors);
		UE_LOG(LogProjectCleanerCLI, Display, TEXT("Fixing %d redirectors..."), Redirectors.Num());
		UPjcSubsystem::FixProjectRedirectors(Redirectors, false);
	}

	if (UPjcSubsystem::ProjectHasRedirectors()) {
		TArray<FAssetData> Redirectors;
		UPjcSubsystem::GetProjectRedirectors(Redirectors);

		UE_LOG(LogProjectCleanerCLI, Warning, TEXT("Project contains %d redirectors that must be fixed first:"), Redirectors.Num());
		for (const auto& Redirector : Redirectors) {
			UE_LOG(LogProjectCleanerCLI, Warning, TEXT("	%s"), *PjcShim::GetObjectPathString(Redirector));
		}
		UE_LOG(LogProjectCleanerCLI, Warning, TEXT("Run again with -fix_redirectors, or fix them with:"));
		UE_LOG(
			LogProjectCleanerCLI,
			Warning,
			TEXT("	%s <uproject> -run=ResavePackages -fixupredirects -autocheckout -projectonly -unattended"),
			ENGINE_MAJOR_VERSION >= 5 ? TEXT("UnrealEditor-Cmd") : TEXT("UE4Editor-Cmd")
		);

		return 1;
	}

	const FCleanupStats StatsBefore = StatsCollect(bList);

	UE_LOG(LogProjectCleanerCLI, Display, TEXT("======================================"));
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("========     Project Stats    ========"));
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("======================================"));
	StatsPrint(StatsBefore);

	if (bScanOnly) return 0;

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

	if (bFullCleanup || bDeleteAssetsUnused || bDeleteFilesExternal || bDeleteFilesCorrupted || bDeleteFoldersEmpty) {
		const FCleanupStats StatsAfter = StatsCollect(bList);

		UE_LOG(LogProjectCleanerCLI, Display, TEXT("======================================"));
		UE_LOG(LogProjectCleanerCLI, Display, TEXT("========  Stats After Cleanup ========"));
		UE_LOG(LogProjectCleanerCLI, Display, TEXT("======================================"));
		StatsPrint(StatsAfter);
	}

	if (!bSuccess) {
		UE_LOG(LogProjectCleanerCLI, Error, TEXT("Cleanup finished with errors, see log above."));
	}

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
}

UPjcCommandlet::FCleanupStats UPjcCommandlet::StatsCollect(const bool bPrintLists) {
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

	return FCleanupStats {AssetsAll.Num(), AssetsUsed.Num(), AssetsUnused.Num(), FoldersEmpty.Num(), FilesExternal.Num(), FilesCorrupted.Num()};
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
