// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcSubsystem.h"
#include "SourceControlHelpers.h"
#include "UObject/CoreRedirects.h"
#include "PjcCookInfo.h"
#include "PjcFormat.h"
#include "PjcConstants.h"
#include "PjcShim.h"
#include "PjcSettings.h"
#include "Pjc.h"
// Engine Headers
#include "AssetManagerEditorModule.h"
#include "BlueprintActionDatabase.h"
#include "AnimationModifier.h"
#include "EditorUtilityBlueprint.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "FileHelpers.h"
#include "ISourceControlModule.h"
#if ENGINE_MAJOR_VERSION >= 5
#include "HAL/PlatformFileManager.h"
#include "UObject/ObjectSaveContext.h"
#else
#include "HAL/PlatformFilemanager.h"
#endif
#include "ObjectTools.h"
#include "PackageTools.h"
#include "ShaderCompiler.h"
#include "Misc/ScopeExit.h"
#include "Settings/ContentBrowserSettings.h"
#include "Settings/ProjectPackagingSettings.h"
#include "Engine/AssetManager.h"
#include "Engine/PrimaryAssetLabel.h"
#include "GameplayTagContainer.h"
#include "GameplayTagsManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/ScopedSlowTask.h"
#include "Editor.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "UObject/ObjectRedirector.h"
#include "UObject/Linker.h"
#include "UObject/LinkerLoad.h"
#include "UObject/ReferencerFinder.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Docking/TabManager.h"
#include "HAL/PlatformProcess.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "PropertyEditorModule.h"

namespace
{
	struct FPjcScanCache
	{
		int32 Depth = 0;
		TOptional<TArray<FAssetData>> AssetsAll;
		TOptional<TArray<FAssetData>> AssetsUsed;
		TOptional<TArray<FAssetData>> AssetsUnused;
		TOptional<TArray<FAssetData>> AssetsIndirect;
		TOptional<TArray<FPjcAssetIndirectInfo>> AssetsIndirectInfos;
		TOptional<TArray<FAssetData>> AssetsExcluded;
		TOptional<TArray<FAssetData>> AssetsEngineReferenced;
		TOptional<TArray<FAssetData>> AssetsFolderLabelOnly;
		TOptional<TMap<FName, FPjcAssetUsage>> AssetsUsage;
		TOptional<TArray<FString>> PlatformsCookAll;
		TOptional<TArray<FString>> FoldersUsedByProject;
		TOptional<TSet<FString>> SourceAndConfigWords;

		void Reset() {
			AssetsAll.Reset();
			AssetsUsed.Reset();
			AssetsUnused.Reset();
			AssetsIndirect.Reset();
			AssetsIndirectInfos.Reset();
			AssetsExcluded.Reset();
			AssetsEngineReferenced.Reset();
			AssetsFolderLabelOnly.Reset();
			AssetsUsage.Reset();
			PlatformsCookAll.Reset();
			FoldersUsedByProject.Reset();
			SourceAndConfigWords.Reset();
		}
	};

	FPjcScanCache ScanCache;

	// progress of the running scan, for the Scan window
	FPjcScanProgress* PjcScanProgress = nullptr;
	EPjcScanStep PjcScanStepNow = EPjcScanStep::Registry;
	double PjcScanProgressTime = 0.0;

	void PjcScanStep(const EPjcScanStep Step, const FString& Current = FString {}) {
		PjcScanStepNow = Step;
		PjcScanProgressTime = FPlatformTime::Seconds();
		if (PjcScanProgress && PjcScanProgress->OnStep) {
			PjcScanProgress->OnStep(Step, Current);
		}
	}

	void PjcScanFound(const EPjcScanStep Step, const FString& Found) {
		if (PjcScanProgress && PjcScanProgress->OnFound) {
			PjcScanProgress->OnFound(Step, Found);
		}
	}

	// the item being read, at most 20 times a second
	void PjcScanCurrent(const FString& Current) {
		if (!PjcScanProgress || FPlatformTime::Seconds() - PjcScanProgressTime < 0.05) return;

		PjcScanStep(PjcScanStepNow, Current);
	}

	// what kept each asset last cleanup could not delete, shown by the Inspector and Assets Unused
	TMap<FName, FString> LastCleanupKept;

	// last Scan, shared by every tab
	FPjcScanResult LastScan;
	UPjcSubsystem::FPjcDelegateScanFinished ScanFinished;
	FPjcProjectChanges ProjectChanges;

	// detail of the cinematic reason, then the class
	const FString PjcCinematicDetail = TEXT("Cinematic ");

	template<typename T>
	bool ScanCacheGet(const TOptional<T>& Cached, T& Out) {
		if (ScanCache.Depth == 0 || !Cached.IsSet()) return false;

		Out = Cached.GetValue();
		return true;
	}

	template<typename T>
	void ScanCacheSet(TOptional<T>& Cached, const T& Value) {
		if (ScanCache.Depth == 0) return;

		Cached = Value;
	}

	// the real asset at the end of a chain (redirector to redirector to asset), null when the chain ends nowhere
	UObject* GetFinalDestination(const UObjectRedirector* Redirector) {
		UObject* Destination = Redirector ? Redirector->DestinationObject : nullptr;
		for (int32 Step = 0; Step < 64; ++Step) {
			const UObjectRedirector* Next = Cast<UObjectRedirector>(Destination);
			if (!Next) return Destination;

			Destination = Next->DestinationObject;
		}
		return nullptr;
	}
}	// namespace

FPjcScanCacheScope::FPjcScanCacheScope() {
	++ScanCache.Depth;
}

FPjcScanCacheScope::~FPjcScanCacheScope() {
	if (--ScanCache.Depth == 0) {
		ScanCache.Reset();
	}
}

void UPjcSubsystem::Initialize(FSubsystemCollectionBase& Collection) {
	Super::Initialize(Collection);

	MoveExclusionsToProject();

	IAssetRegistry& Registry = GetModuleAssetRegistry().Get();
	AssetAddedHandle = Registry.OnAssetAdded().AddUObject(this, &UPjcSubsystem::OnAssetAddedEvent);
	AssetRemovedHandle = Registry.OnAssetRemoved().AddUObject(this, &UPjcSubsystem::OnAssetRemovedEvent);
	AssetRenamedHandle = Registry.OnAssetRenamed().AddUObject(this, &UPjcSubsystem::OnAssetRenamedEvent);
#if ENGINE_MAJOR_VERSION >= 5
	const auto OnSaved = [this](const FString& Filename, UPackage* Package, FObjectPostSaveContext) {
		OnPackageSavedEvent(Filename, Package);
	};
	PackageSavedHandle = UPackage::PackageSavedWithContextEvent.AddWeakLambda(this, OnSaved);
#else
	PackageSavedHandle = UPackage::PackageSavedEvent.AddUObject(this, &UPjcSubsystem::OnPackageSavedEvent);
#endif
}

int32 UPjcSubsystem::MoveExclusionsToProject() {
	if (!GConfig) return 0;

	const int32 NumFilesMoved = MoveFileExclusionsToProject();

	const FString Section = UPjcAssetExcludeSettings::StaticClass()->GetPathName();
	TArray<FString> Folders;
	TArray<FString> Classes;
	TArray<FString> Assets;
	GConfig->GetArray(*Section, TEXT("ExcludedFolders"), Folders, GEditorPerProjectIni);
	GConfig->GetArray(*Section, TEXT("ExcludedClasses"), Classes, GEditorPerProjectIni);
	GConfig->GetArray(*Section, TEXT("ExcludedAssets"), Assets, GEditorPerProjectIni);
	if (Folders.Num() + Classes.Num() + Assets.Num() == 0) return NumFilesMoved;

	const auto Unquote = [](FString Value) {
		Value.TrimStartAndEndInline();
		Value.TrimQuotesInline();
		return Value;
	};

	UPjcAssetExcludeSettings* Settings = GetMutableDefault<UPjcAssetExcludeSettings>();
	int32 NumMoved = 0;
	for (const auto& Entry : Folders) {
		FString Path;
		if (!FParse::Value(*Entry, TEXT("Path="), Path)) continue;

		Path.RemoveFromEnd(TEXT(")"));
		Path = Unquote(Path);
		if (Path.IsEmpty() || Settings->ExcludedFolders.ContainsByPredicate([&Path](const FDirectoryPath& Dir) {
				return Dir.Path == Path;
			}))
			continue;

		Settings->ExcludedFolders.Add(FDirectoryPath {Path});
		++NumMoved;
	}
	for (const auto& Entry : Classes) {
		const TSoftClassPtr<UObject> Class {FSoftObjectPath {Unquote(Entry)}};
		if (Class.IsNull() || Settings->ExcludedClasses.Contains(Class)) continue;

		Settings->ExcludedClasses.Add(Class);
		++NumMoved;
	}
	for (const auto& Entry : Assets) {
		const TSoftObjectPtr<UObject> Asset {FSoftObjectPath {Unquote(Entry)}};
		if (Asset.IsNull() || Settings->ExcludedAssets.Contains(Asset)) continue;

		Settings->ExcludedAssets.Add(Asset);
		++NumMoved;
	}

	PjcShim::UpdateDefaultConfigFile(Settings);
	GConfig->EmptySection(*Section, GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);

	const TCHAR* Where = TEXT("the project settings (Config/DefaultEditor.ini)");
	UE_LOG(LogProjectCleaner, Display, TEXT("Moved %d exclusion(s) from your editor preferences to %s, shared with the team"), NumMoved, Where);
	return NumMoved + NumFilesMoved;
}

int32 UPjcSubsystem::MoveFileExclusionsToProject() {
	const FString Section = UPjcFileExcludeSettings::StaticClass()->GetPathName();
	TArray<FString> Files;
	TArray<FString> Extensions;
	GConfig->GetArray(*Section, TEXT("ExcludedFiles"), Files, GEditorPerProjectIni);
	GConfig->GetArray(*Section, TEXT("ExcludedExtensions"), Extensions, GEditorPerProjectIni);
	if (Files.Num() + Extensions.Num() == 0) return 0;

	UPjcFileExcludeSettings* Settings = GetMutableDefault<UPjcFileExcludeSettings>();
	int32 NumMoved = 0;
	for (const auto& Entry : Files) {
		FString Path;
		if (!FParse::Value(*Entry, TEXT("FilePath="), Path)) continue;

		Path.RemoveFromEnd(TEXT(")"));
		Path.TrimStartAndEndInline();
		Path.TrimQuotesInline();
		if (Path.IsEmpty() || Settings->ExcludedFiles.ContainsByPredicate([&Path](const FFilePath& File) {
				return File.FilePath == Path;
			}))
			continue;

		FFilePath FilePath;
		FilePath.FilePath = Path;
		Settings->ExcludedFiles.Add(FilePath);
		++NumMoved;
	}
	for (const auto& Entry : Extensions) {
		const FString Extension = Entry.TrimStartAndEnd().TrimQuotes();
		if (Extension.IsEmpty() || Settings->ExcludedExtensions.Contains(Extension)) continue;

		Settings->ExcludedExtensions.Add(Extension);
		++NumMoved;
	}

	PjcShim::UpdateDefaultConfigFile(Settings);
	GConfig->EmptySection(*Section, GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
	return NumMoved;
}

void UPjcSubsystem::Deinitialize() {
	if (FModuleManager::Get().IsModuleLoaded(TEXT("AssetRegistry"))) {
		IAssetRegistry& Registry = GetModuleAssetRegistry().Get();
		Registry.OnAssetAdded().Remove(AssetAddedHandle);
		Registry.OnAssetRemoved().Remove(AssetRemovedHandle);
		Registry.OnAssetRenamed().Remove(AssetRenamedHandle);
	}
#if ENGINE_MAJOR_VERSION >= 5
	UPackage::PackageSavedWithContextEvent.Remove(PackageSavedHandle);
#else
	UPackage::PackageSavedEvent.Remove(PackageSavedHandle);
#endif

	Super::Deinitialize();
}

const FPjcProjectChanges& UPjcSubsystem::GetChangesSinceScan() {
	return ProjectChanges;
}

bool UPjcSubsystem::IsProjectChange(const FName Package) {
	// before the first scan and while the registry discovers assets nothing is compared yet
	if (!LastScan.IsValid() || GetModuleAssetRegistry().Get().IsLoadingAssets()) return false;

	return PathIsInsideProject(Package.ToString());
}

void UPjcSubsystem::OnAssetAddedEvent(const FAssetData& Asset) {
	if (IsProjectChange(Asset.PackageName)) {
		ProjectChanges.Add(ProjectChanges.NumAdded, Asset.PackageName);
	}
}

void UPjcSubsystem::OnAssetRemovedEvent(const FAssetData& Asset) {
	if (IsProjectChange(Asset.PackageName)) {
		ProjectChanges.Add(ProjectChanges.NumRemoved, Asset.PackageName);
	}
}

void UPjcSubsystem::OnAssetRenamedEvent(const FAssetData& Asset, const FString& OldObjectPath) {
	if (IsProjectChange(Asset.PackageName)) {
		ProjectChanges.Add(ProjectChanges.NumRenamed, Asset.PackageName);
	}
}

void UPjcSubsystem::OnPackageSavedEvent(const FString& Filename, UObject* Package) {
	const FName PackageName = Package ? Package->GetOutermost()->GetFName() : NAME_None;
	if (!PackageName.IsNone() && IsProjectChange(PackageName)) {
		ProjectChanges.Add(ProjectChanges.NumSaved, PackageName);
	}
}

#if WITH_EDITOR
void UPjcSubsystem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) {
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();
}
#endif

void UPjcSubsystem::GetAssetsAll(TArray<FAssetData>& Assets) {
	if (GetModuleAssetRegistry().Get().IsLoadingAssets()) return;

	GetAssetsScanned(Assets);

	// redirectors are not content: GetProjectRedirectors lists them and only fixing them removes them
	TSet<FName> PackagesRedirector;
	GetPackagesRedirector(PackagesRedirector);
	Assets.RemoveAll([&PackagesRedirector](const FAssetData& Asset) {
		return PackagesRedirector.Contains(Asset.PackageName);
	});
}

void UPjcSubsystem::GetAssetsScanned(TArray<FAssetData>& Assets) {
	if (GetModuleAssetRegistry().Get().IsLoadingAssets()) return;
	if (ScanCacheGet(ScanCache.AssetsAll, Assets)) return;

	Assets.Reset();

	for (const auto& RootPath : GetProjectRootPaths()) {
		TArray<FAssetData> TempAssets;
		GetModuleAssetRegistry().Get().GetAssetsByPath(FName {*RootPath}, TempAssets, true);
		Assets.Append(TempAssets);
	}

	// Registry loaded from disk (commandlets) lists more than one asset per package, for example
	// BlueprintGeneratedClass next to its Blueprint. Everything here works per package, so keep
	// one entry per package: the main asset if there is one.
	TSet<FName> PackagesWithMainAsset;
	for (const auto& Asset : Assets) {
		if (Asset.IsUAsset()) {
			PackagesWithMainAsset.Add(Asset.PackageName);
		}
	}

	TSet<FName> PackagesAdded;
	PjcShim::RemoveAllSwapArray(Assets, [&](const FAssetData& Asset) {
		// World Partition actors and objects ('__ExternalActors__', '__ExternalObjects__') belong to their map
		if (FolderIsExternal(Asset.PackagePath.ToString())) return true;
		if (PathIsLeftOut(Asset.PackagePath.ToString())) return true;
		if (PackagesWithMainAsset.Contains(Asset.PackageName) && !Asset.IsUAsset()) return true;

		bool bAlreadyAdded = false;
		PackagesAdded.Add(Asset.PackageName, &bAlreadyAdded);
		return bAlreadyAdded;
	});

	ScanCacheSet(ScanCache.AssetsAll, Assets);
}

void UPjcSubsystem::GetAssetsUsed(TArray<FAssetData>& Assets, const bool bShowSlowTask) {
	if (GetModuleAssetRegistry().Get().IsLoadingAssets()) return;
	if (ScanCacheGet(ScanCache.AssetsUsed, Assets)) return;

	TArray<FAssetData> AssetsFolderLabelOnly;
	TMap<FName, FPjcAssetUsage> AssetsUsage;
	ComputeAssetsUsed(Assets, AssetsFolderLabelOnly, AssetsUsage, bShowSlowTask);

	ScanCacheSet(ScanCache.AssetsUsed, Assets);
	ScanCacheSet(ScanCache.AssetsFolderLabelOnly, AssetsFolderLabelOnly);
	ScanCacheSet(ScanCache.AssetsUsage, AssetsUsage);
}

void UPjcSubsystem::GetAssetsFolderLabelOnly(TArray<FAssetData>& Assets, const bool bShowSlowTask) {
	if (GetModuleAssetRegistry().Get().IsLoadingAssets()) return;
	if (ScanCacheGet(ScanCache.AssetsFolderLabelOnly, Assets)) return;

	TArray<FAssetData> AssetsUsed;
	TMap<FName, FPjcAssetUsage> AssetsUsage;
	ComputeAssetsUsed(AssetsUsed, Assets, AssetsUsage, bShowSlowTask);

	ScanCacheSet(ScanCache.AssetsUsed, AssetsUsed);
	ScanCacheSet(ScanCache.AssetsFolderLabelOnly, Assets);
	ScanCacheSet(ScanCache.AssetsUsage, AssetsUsage);
}

bool UPjcSubsystem::Scan(const bool bShowSlowTask, FPjcScanProgress* Progress) {
	if (GetModuleAssetRegistry().Get().IsLoadingAssets()) return false;

	const double StartTime = FPlatformTime::Seconds();
	PjcScanProgress = Progress;
	ON_SCOPE_EXIT {
		PjcScanProgress = nullptr;
	};
	// a cancel ends the scan between steps, the last result stays
	const auto IsCanceled = [Progress]() {
		if (Progress && Progress->bCancelRequested) {
			Progress->bCanceled = true;
		}
		return Progress && Progress->bCanceled;
	};
	PjcScanStep(EPjcScanStep::Registry);
	PjcScanFound(EPjcScanStep::Registry, TEXT("ready"));

	// the queries below share their intermediate results (references, source and config search)
	FPjcScanCacheScope ScanCacheScope;

	FPjcScanResult Result;
	PjcScanStep(EPjcScanStep::Assets);
	GetAssetsAll(Result.Assets);
	TSet<FName> Folders;
	for (const auto& Asset : Result.Assets) {
		Folders.Add(Asset.PackagePath);
	}
	const FString NumAssets = PjcFormat::Count(Result.Assets.Num());
	PjcScanFound(EPjcScanStep::Assets, FString::Printf(TEXT("%s assets \u00B7 %s folders"), *NumAssets, *PjcFormat::Count(Folders.Num())));
	if (IsCanceled()) return false;

	GetAssetsUsage(Result.Usage, bShowSlowTask);
	if (IsCanceled()) return false;

	// every place source and config files name an asset (the usage keeps the first), from the files the usage read
	{
		TArray<FAssetData> AssetsIndirect;
		TArray<FPjcAssetIndirectInfo> AssetsIndirectInfos;
		GetAssetsIndirect(AssetsIndirect, AssetsIndirectInfos, false);
		const FString ProjectDir = PathNormalize(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir())) + TEXT("/");
		for (const auto& Info : AssetsIndirectInfos) {
			FString File = PathNormalize(Info.FilePath);
			FPaths::MakePathRelativeTo(File, *ProjectDir);
			FPjcSourceRef& Ref = Result.SourceRefs.FindOrAdd(Info.Asset.PackageName).AddDefaulted_GetRef();
			Ref.File = File;
			Ref.Line = Info.FileNum;
		}
	}

	GetAssetsUsed(Result.Used, bShowSlowTask);
	GetAssetsUnused(Result.Unused, bShowSlowTask);
	GetAssetsFolderLabelOnly(Result.FolderLabelOnly, bShowSlowTask);
	const FString NumUsed = PjcFormat::Count(Result.Used.Num());
	PjcScanFound(EPjcScanStep::References, FString::Printf(TEXT("%s used \u00B7 %s unused"), *NumUsed, *PjcFormat::Count(Result.Unused.Num())));
	if (IsCanceled()) return false;

	// redirectors are not content (none of the lists above has them): one per package, its main asset
	TArray<FAssetData> Redirectors;
	GetProjectRedirectors(Redirectors);
	TMap<FName, FAssetData> RedirectorByPackage;
	for (const auto& Redirector : Redirectors) {
		const FAssetData* Found = RedirectorByPackage.Find(Redirector.PackageName);
		if (!Found || (!Found->IsUAsset() && Redirector.IsUAsset())) {
			RedirectorByPackage.Add(Redirector.PackageName, Redirector);
		}
	}
	RedirectorByPackage.GenerateValueArray(Result.Redirectors);

	Result.Sizes.Reserve(Result.Assets.Num());
	for (const auto& Asset : Result.Assets) {
		Result.Sizes.Add(Asset.PackageName, GetAssetSize(Asset));
	}

	// the game uses what ships: roots that are not editor assets, assets the Asset Manager cooks by its rules (folders
	// of labels among them), and what those reach through the references the cooker follows (game hard and soft
	// references). Labels are left out with their references: they cook nothing themselves, and editor only label
	// types ignore AlwaysCook rules, so what a label lists ships only through a cook rule or another reference. What
	// only editor assets, labels or editor-only references reach is editor only.
	TSet<FName> Labels;
	for (const auto& Asset : Result.Assets) {
		const UClass* AssetClass = Asset.GetClass();
		if (AssetClass && AssetClass->IsChildOf(UPrimaryAssetLabel::StaticClass())) {
			Labels.Add(Asset.PackageName);
		}
	}

	// a label whose cook rule packages its folder is cooked itself. Unused assets other folder labels list: their cook
	// rule does not package them
	TSet<FName> LabelsPackaged;
	for (const auto& Label : Labels) {
		const UPrimaryAssetLabel* FolderLabel = GetFolderLabel(Label);
		if (!FolderLabel) continue;
		if (FolderLabelIsPackaged(*FolderLabel)) {
			LabelsPackaged.Add(Label);
			continue;
		}

		const FString LabelPath = FPackageName::GetLongPackagePath(Label.ToString());
		for (const auto& Asset : Result.Unused) {
			const FString AssetPath = Asset.PackagePath.ToString();
			if (AssetPath == LabelPath || AssetPath.StartsWith(LabelPath + TEXT("/"))) {
				Result.FolderLabelUnused.Add(Asset.PackageName);
			}
		}
	}

	const UAssetManager* AssetManager = PjcShim::IsAssetManagerValid() ? &UAssetManager::Get() : nullptr;
	const auto IsCookedByRule = [AssetManager](const FName Package) {
		if (!AssetManager) return false;

		const EPrimaryAssetCookRule CookRule = AssetManager->GetPackageCookRule(Package);
		return CookRule == EPrimaryAssetCookRule::AlwaysCook || CookRule == EPrimaryAssetCookRule::DevelopmentAlwaysCook;
	};

	TSet<FName> GameUsed;
	TArray<FName> Stack;
	for (const auto& Pair : Result.Usage) {
		const FPjcAssetUsage& AssetUsage = Pair.Value;
		if (!AssetUsage.bUsed) continue;
		if (Labels.Contains(Pair.Key)) {
			if (LabelsPackaged.Contains(Pair.Key)) {
				GameUsed.Add(Pair.Key);
			}
			continue;
		}

		const bool bEditorRoot = AssetUsage.Reason == EPjcUsageReason::Editor || AssetUsage.Reason == EPjcUsageReason::Splash;
		const bool bRoot = AssetUsage.Parent.IsNone() && !bEditorRoot;
		const bool bLabelPackaged = AssetUsage.Reason == EPjcUsageReason::FolderLabel;
		if (bRoot || bLabelPackaged || IsCookedByRule(Pair.Key)) {
			GameUsed.Add(Pair.Key);
			Stack.Add(Pair.Key);
		}
	}

	TArray<FName> Deps;
	IAssetRegistry& Registry = GetModuleAssetRegistry().Get();
	const auto WalkGameReferences = [&]() {
		while (Stack.Num() > 0) {
			const FName Current = PjcShim::PopArray(Stack);

			Deps.Reset();
			Registry.GetDependencies(Current, Deps, UE::AssetRegistry::EDependencyCategory::Package, UE::AssetRegistry::EDependencyQuery::Game);
			for (const auto& Dep : Deps) {
				if (Labels.Contains(Dep) || !PathIsInsideProject(Dep.ToString())) continue;

				bool bAlreadyUsed = false;
				GameUsed.Add(Dep, &bAlreadyUsed);
				if (!bAlreadyUsed) {
					Stack.Add(Dep);
				}
			}
		}
	};
	WalkGameReferences();

	// localized copies ship with their source, the cooker adds them
	for (const auto& Pair : Result.Usage) {
		const FPjcAssetUsage& AssetUsage = Pair.Value;
		const bool bLocalizedUsed = AssetUsage.Reason == EPjcUsageReason::Localized && AssetUsage.bUsed;
		if (bLocalizedUsed && GameUsed.Contains(AssetUsage.Parent) && !GameUsed.Contains(Pair.Key)) {
			GameUsed.Add(Pair.Key);
			Stack.Add(Pair.Key);
		}
	}
	WalkGameReferences();

	Result.Status.Reserve(Result.Assets.Num());
	for (const auto& Asset : Result.Assets) {
		const bool bReview = FPjcScanResult::IsReviewReason(Result.GetChainReason(Asset.PackageName));
		const EPjcAssetStatus GameStatus = GameUsed.Contains(Asset.PackageName) ? EPjcAssetStatus::Used : EPjcAssetStatus::EditorOnly;
		const EPjcAssetStatus UsedStatus = bReview ? EPjcAssetStatus::Review : GameStatus;
		Result.Status.Add(Asset.PackageName, Result.IsUsed(Asset.PackageName) ? UsedStatus : EPjcAssetStatus::Unused);
	}

	// exclusion rules: what each keeps, and whether it is used without the rule (a used referencer outside exclusions)
	TSet<FName> ClassNamesUserExcluded;
	TSet<FName> ClassNamesUsed;
	TSet<FName> ClassNamesCinematic;
	GetClassNamesExcludedBy(ClassNamesUserExcluded, ClassNamesUsed, ClassNamesCinematic);
	TArray<FName> Referencers;
	for (const auto& Asset : Result.Assets) {
		FPjcExclusion Exclusion;
		if (!GetExclusion(Asset, Exclusion)) continue;

		Exclusion.bUsedAnyway = Result.GetChainReason(Asset.PackageName) != EPjcUsageReason::Excluded;
		Referencers.Reset();
		Registry.GetReferencers(Asset.PackageName, Referencers);
		for (const auto& Referencer : Referencers) {
			const bool bUsedOutside = Result.IsUsed(Referencer) && Result.GetChainReason(Referencer) != EPjcUsageReason::Excluded;
			Exclusion.bUsedAnyway |= bUsedOutside && Referencer != Asset.PackageName;
		}
		Result.Excluded.Add(Asset.PackageName, Exclusion);
	}

	// cook check: what the cooks saw of the unused assets
	TArray<const FPjcCookInfo*> Cooks = FPjcCookInfo::GetAll();
	for (const auto& Cook : Cooks) {
		Result.CookCheck.Platforms.Add(Cook->Platform);
	}
	Result.CookCheck.Time = Cooks.Num() > 0 ? Cooks[0]->Time : FDateTime {0};
	ScanCacheGet(ScanCache.PlatformsCookAll, Result.CookCheck.PlatformsCookAll);
	Cooks.RemoveAll([&Result](const FPjcCookInfo* Cook) {
		return Result.CookCheck.PlatformsCookAll.Contains(Cook->Platform);
	});
	for (const auto& Pair : Result.Usage) {
		Result.CookCheck.NumPackaged += Pair.Value.Reason == EPjcUsageReason::CookPackaged ? 1 : 0;
	}
	// a cook lists only the packages it visited: an unused asset nothing references is not in it, and the cook left it
	// out. Checked unless saved or added after the cook
	for (const auto& Asset : Result.Unused) {
		const auto IsChecked = [&Asset](const FPjcCookInfo* Cook) {
			return Cook->IsValid() && !Cook->IsSavedSince(Asset.PackageName);
		};
		const bool bChecked = Cooks.ContainsByPredicate(IsChecked);
		Result.CookCheck.NumChecked += bChecked ? 1 : 0;
		Result.CookCheck.NumNotChecked += bChecked || Cooks.Num() == 0 ? 0 : 1;
	}
	Result.CookCheck.NumChecked += Result.CookCheck.NumPackaged;

	// files that are not assets, and empty folders
	PjcScanStep(EPjcScanStep::Files);
	TArray<FString> FilesExternal;
	GetFilesExternalAll(FilesExternal);
	for (auto& File : FilesExternal) {
		FString Reason = GetFileKeptReason(File);
		if (Reason.IsEmpty()) {
			Result.FilesExternal.Add(MoveTemp(File));
		}
		else {
			Result.FilesExternalKept.Add(MoveTemp(File));
			Result.FilesExternalKeptReasons.Add(MoveTemp(Reason));
		}
	}
	GetFilesCorrupted(Result.FilesCorrupted, bShowSlowTask);
	GetFoldersEmpty(Result.FoldersEmpty);
	const FString NumExternal = PjcFormat::Count(Result.FilesExternal.Num());
	const FString NumCorrupted = PjcFormat::Count(Result.FilesCorrupted.Num());
	const FString NumEmpty = PjcFormat::Count(Result.FoldersEmpty.Num());
	const FString FilesFound = FString::Printf(TEXT("%s external \u00B7 %s corrupted \u00B7 %s empty"), *NumExternal, *NumCorrupted, *NumEmpty);
	PjcScanFound(EPjcScanStep::Files, FilesFound);
	if (Result.CookCheck.IsFound()) {
		PjcScanStep(EPjcScanStep::Cook);
		const FString Packaged = PjcFormat::Count(Result.CookCheck.NumPackaged) + TEXT(" packaged \u00B7 in Review");
		const FString Checked = PjcFormat::Count(Result.CookCheck.NumChecked) + TEXT(" unused the cook left out");
		const TCHAR* CookAll = TEXT("cook took nearly all assets \u00B7 not compared");
		PjcScanFound(EPjcScanStep::Cook, Result.CookCheck.IsCookAll() ? FString {CookAll} : (Result.CookCheck.NumPackaged > 0 ? Packaged : Checked));
	}
	if (IsCanceled()) return false;

	Result.Time = FDateTime::UtcNow();
	Result.Seconds = FPlatformTime::Seconds() - StartTime;
	LastScan = MoveTemp(Result);
	ProjectChanges = FPjcProjectChanges {};

	ScanFinished.Broadcast();
	return true;
}

const FPjcScanResult& UPjcSubsystem::GetScanResult() {
	return LastScan;
}

UPjcSubsystem::FPjcDelegateScanFinished& UPjcSubsystem::OnScanFinished() {
	return ScanFinished;
}

void UPjcSubsystem::GetAssetsUsage(TMap<FName, FPjcAssetUsage>& Usage, const bool bShowSlowTask) {
	if (GetModuleAssetRegistry().Get().IsLoadingAssets()) return;
	if (ScanCacheGet(ScanCache.AssetsUsage, Usage)) return;

	TArray<FAssetData> AssetsUsed;
	TArray<FAssetData> AssetsFolderLabelOnly;
	ComputeAssetsUsed(AssetsUsed, AssetsFolderLabelOnly, Usage, bShowSlowTask);

	ScanCacheSet(ScanCache.AssetsUsed, AssetsUsed);
	ScanCacheSet(ScanCache.AssetsFolderLabelOnly, AssetsFolderLabelOnly);
	ScanCacheSet(ScanCache.AssetsUsage, Usage);
}

FString UPjcSubsystem::GetAssetHolder(const FAssetData& InAsset) {
	UObject* Object = FindObject<UObject>(nullptr, *PjcShim::GetObjectPathString(InAsset));
	if (!Object) return FString {};

	TArray<UObject*> Objects {Object};
	TMap<FString, FString> Holders;
	RemoveAssetsHeldOutside(Objects, &Holders);

	return Holders.FindRef(Object->GetPathName());
}

const TMap<FName, FString>& UPjcSubsystem::GetLastCleanupKept() {
	return LastCleanupKept;
}

FString UPjcSubsystem::GetUsageReasonText(const FPjcAssetUsage& Usage) {
	switch (Usage.Reason) {
		case EPjcUsageReason::Primary :			 return Usage.Detail.IsEmpty() ? TEXT("Primary asset") : TEXT("Primary asset ") + Usage.Detail;
		case EPjcUsageReason::PrimaryClass :	 return TEXT("Primary class, not registered by the Asset Manager");
		case EPjcUsageReason::EngineReferenced : return TEXT("Used by engine or plugins");
		case EPjcUsageReason::Indirect :		 return TEXT("Named in ") + Usage.Detail;
		case EPjcUsageReason::Excluded :		 return Usage.Detail.IsEmpty() ? TEXT("Excluded") : Usage.Detail;
		case EPjcUsageReason::Editor :			 return TEXT("Editor asset");
		case EPjcUsageReason::GameplayCue :		 return TEXT("GameplayCue notify ") + Usage.Detail;
		case EPjcUsageReason::Megascans :		 return TEXT("Megascans preset");
		case EPjcUsageReason::Localized :		 return TEXT("Localized copy of ") + FPackageName::GetShortName(Usage.Parent);
		case EPjcUsageReason::ConfigFolder :	 return TEXT("In folder ") + Usage.Detail;
		case EPjcUsageReason::FolderLabelAsset : return TEXT("Listed by folder label ") + FPackageName::GetShortName(Usage.Parent);
		case EPjcUsageReason::Dependency :
			// placed in a level by its actor file (World Partition, One File Per Actor)
			return (FolderIsExternal(Usage.Detail) ? TEXT("Placed in ") : TEXT("Referenced by ")) + FPackageName::GetShortName(Usage.Parent);
		case EPjcUsageReason::FolderLabel :			return TEXT("In folder of label ") + FPackageName::GetShortName(Usage.Parent);
		case EPjcUsageReason::CookPackaged :		return TEXT("Packaged by the last cook (") + Usage.Detail + TEXT(")");
		case EPjcUsageReason::GameplayCueNotFired : return TEXT("GameplayCue notify nothing fires: ") + Usage.Detail;
		case EPjcUsageReason::Cinematic :			return Usage.Detail;
		case EPjcUsageReason::Splash :				return TEXT("Splash content");
		case EPjcUsageReason::None :
		default :									return TEXT("Nothing uses it");
	}
}

// The cook took nearly every asset of the project, and nearly every asset nothing uses: the cooker was told to take
// everything, or not told what to take. Such a cook says nothing about what the game needs. Few unused assets cannot show it
static bool IsCookOfEverything(const FPjcCookInfo& Cook, const TArray<FAssetData>& Assets, const TMap<FName, FPjcAssetUsage>& Usage) {
	if (!Cook.IsValid()) return false;

	int32 NumCooked = 0;
	int32 NumUnused = 0;
	int32 NumUnusedCooked = 0;
	for (const auto& Asset : Assets) {
		const bool bCooked = Cook.Cooked.Contains(Asset.PackageName);
		NumCooked += bCooked ? 1 : 0;
		if (Usage.Contains(Asset.PackageName) || Cook.IsSavedSince(Asset.PackageName)) continue;

		++NumUnused;
		NumUnusedCooked += bCooked ? 1 : 0;
	}
	return NumUnused >= 10 && NumUnusedCooked * 10 >= NumUnused * 9 && NumCooked * 10 >= Assets.Num() * 9;
}

void UPjcSubsystem::ComputeAssetsUsed(
	TArray<FAssetData>& OutUsed,
	TArray<FAssetData>& OutFolderLabelOnly,
	TMap<FName, FPjcAssetUsage>& OutUsage,
	const bool bShowSlowTask
) {
	const UPjcScanSettings* ScanSettings = GetDefault<UPjcScanSettings>();

	TSet<FName> ClassNamesPrimary;
	TSet<FName> ClassNamesEditor;
	TArray<FAssetData> AssetsAll;
	TArray<FAssetData> AssetsIndirect;
	TArray<FAssetData> AssetsExcluded;
	TArray<FAssetData> AssetsMegascans;
	TArray<FAssetData> AssetsEngineReferenced;
	TArray<FPjcAssetIndirectInfo> AssetsIndirectInfos;

	GetAssetsScanned(AssetsAll);
	// the registry gives assets and references in another order each editor session: walked in name order, every
	// package gets the same parent and chain start in every scan
	AssetsAll.Sort([](const FAssetData& A, const FAssetData& B) {
		return A.PackageName.LexicalLess(B.PackageName);
	});
	GetAssetsEngineReferenced(AssetsEngineReferenced, bShowSlowTask);
	const TSet<FAssetData> AssetsEngineReferencedSet {AssetsEngineReferenced};
	PjcScanStep(EPjcScanStep::SourceConfig);
	GetAssetsIndirect(AssetsIndirect, AssetsIndirectInfos, bShowSlowTask);
	TSet<FName> PackagesNamed;
	for (const auto& Info : AssetsIndirectInfos) {
		PackagesNamed.Add(Info.Asset.PackageName);
	}
	for (const auto& Asset : AssetsIndirect) {
		PackagesNamed.Add(Asset.PackageName);
	}
	PjcScanFound(EPjcScanStep::SourceConfig, PjcFormat::Count(PackagesNamed.Num()) + TEXT(" assets named"));
	PjcScanStep(EPjcScanStep::Rules);
	GetAssetsExcluded(AssetsExcluded, bShowSlowTask);
	GetClassNamesPrimary(ClassNamesPrimary);
	GetClassNamesEditor(ClassNamesEditor);
	TSet<FName> PackagesPrimary;
	GetPackagesPrimary(PackagesPrimary);

	if (FModuleManager::Get().IsModuleLoaded(PjcConstants::ModuleMegascans)) {
		GetModuleAssetRegistry().Get().GetAssetsByPath(PjcConstants::PathMSPresets, AssetsMegascans, true);
	}

	// GameplayCue notifies (package -> tag, folder)
	TMap<FName, FName> CueNotifies;
	TMap<FName, FString> CueNotifyPaths;

	// every used package remembers why: its own reason, or the used package that references it (closest to a root)
	TMap<FName, FPjcAssetUsage> Usage;
	TArray<FName> Queue;
	const auto AddUsed = [&](const FName& PackageName, const EPjcUsageReason Reason, const FName& Parent, const FString& Detail) -> FPjcAssetUsage* {
		if (Usage.Contains(PackageName)) return nullptr;

		FPjcAssetUsage& AssetUsage = Usage.Add(PackageName);
		AssetUsage.bUsed = true;
		AssetUsage.Reason = Reason;
		AssetUsage.Parent = Parent;
		AssetUsage.Detail = Detail;
		Queue.Add(PackageName);

		return &AssetUsage;
	};

	FScopedSlowTask SlowTask {
	  static_cast<float>(AssetsAll.Num()), FText::FromString(TEXT("Searching used assets...")), bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	// first reason found is the one reported, most telling first
	for (const auto& Asset : AssetsAll) {
		PjcScanCurrent(Asset.PackageName.ToString());
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(Asset.GetFullName()));

		const FName AssetClassName = PjcShim::GetAssetClassName(Asset);
		const FName AssetExactClassName = GetAssetExactClassName(Asset);

		if (PackagesPrimary.Contains(Asset.PackageName)) {
			AddUsed(Asset.PackageName, EPjcUsageReason::Primary, NAME_None, GetPrimaryDetail(Asset));
		}
		else if (AssetsEngineReferencedSet.Contains(Asset)) {
			AddUsed(Asset.PackageName, EPjcUsageReason::EngineReferenced, NAME_None, GetEngineReferencedDetail(Asset));
		}
		else if (ClassNamesEditor.Contains(AssetClassName) || ClassNamesEditor.Contains(AssetExactClassName)) {
			AddUsed(Asset.PackageName, EPjcUsageReason::Editor, NAME_None, FString {});
		}
		else if (AssetIsLoadedByTag(Asset)) {
			// fired by tag: decided after what references them
			FString GameplayCueName;
			Asset.GetTagValue(FName {TEXT("GameplayCueName")}, GameplayCueName);
			CueNotifies.Add(Asset.PackageName, FName {*GameplayCueName});
			CueNotifyPaths.Add(Asset.PackageName, Asset.PackagePath.ToString());
		}
	}

	const FString ProjectDir = PathNormalize(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir())) + TEXT("/");
	for (const auto& Info : AssetsIndirectInfos) {
		FString File = PathNormalize(Info.FilePath);
		FPaths::MakePathRelativeTo(File, *ProjectDir);
		AddUsed(Info.Asset.PackageName, EPjcUsageReason::Indirect, NAME_None, FString::Printf(TEXT("%s, line %d"), *File, Info.FileNum));
	}
	for (const auto& Asset : AssetsIndirect) {
		AddUsed(Asset.PackageName, EPjcUsageReason::Indirect, NAME_None, FString {});
	}

	if (AssetsExcluded.Num() > 0) {
		TSet<FName> ClassNamesUserExcluded;
		TSet<FName> ClassNamesUsedClasses;
		TSet<FName> ClassNamesCinematic;
		GetClassNamesExcludedBy(ClassNamesUserExcluded, ClassNamesUsedClasses, ClassNamesCinematic);

		for (const auto& Asset : AssetsExcluded) {
			if (Usage.Contains(Asset.PackageName)) continue;

			const FString ExcludedDetail = GetExcludedDetail(Asset, ClassNamesUserExcluded, ClassNamesUsedClasses, ClassNamesCinematic);
			const bool bCinematic = ExcludedDetail.StartsWith(PjcCinematicDetail);
			AddUsed(Asset.PackageName, bCinematic ? EPjcUsageReason::Cinematic : EPjcUsageReason::Excluded, NAME_None, ExcludedDetail);
		}
	}

	for (const auto& Asset : AssetsMegascans) {
		AddUsed(Asset.PackageName, EPjcUsageReason::Megascans, NAME_None, FString {});
	}

	// the Splash folder holds the editor and game splash images (Content/Splash/EdSplash.png, Splash.png). The engine
	// shows the image files; the editor imports texture copies of them, and imports them again after a delete
	const FString SplashFolder = TEXT("/Game/Splash");
	for (const auto& Asset : AssetsAll) {
		const FString AssetPath = Asset.PackagePath.ToString();
		if (AssetPath == SplashFolder || AssetPath.StartsWith(SplashFolder + TEXT("/"))) {
			AddUsed(Asset.PackageName, EPjcUsageReason::Splash, NAME_None, FString {});
		}
	}

	// assets in the collection a used Primary Asset Label names are labeled like its explicit assets
	for (const auto& Asset : AssetsAll) {
		const UClass* AssetClass = Asset.GetClass();
		if (!AssetClass || !AssetClass->IsChildOf(UPrimaryAssetLabel::StaticClass()) || !Usage.Contains(Asset.PackageName)) continue;

		const UPrimaryAssetLabel* Label = Cast<UPrimaryAssetLabel>(Asset.GetAsset());
		if (!Label || Label->AssetCollection.CollectionName.IsNone()) continue;

		TArray<FName> CollectionPackages;
		PjcShim::GetPackagesInCollection(Label->AssetCollection.CollectionName, CollectionPackages);
		for (const auto& CollectionPackage : CollectionPackages) {
			const FString InCollection = TEXT("in collection ") + Label->AssetCollection.CollectionName.ToString();
			AddUsed(CollectionPackage, EPjcUsageReason::Dependency, Asset.PackageName, InCollection);
		}
	}

	// folders the cook always takes whole: every asset inside is used
	TMap<FString, FString> ConfigFolders;
	GetConfigFolders(ConfigFolders);
	if (ConfigFolders.Num() > 0) {
		for (const auto& Asset : AssetsAll) {
			const FString AssetPath = Asset.PackagePath.ToString();
			for (const auto& Folder : ConfigFolders) {
				if (AssetPath == Folder.Key || AssetPath.StartsWith(Folder.Key + TEXT("/"))) {
					AddUsed(Asset.PackageName, EPjcUsageReason::ConfigFolder, NAME_None, Folder.Value + TEXT(" ") + Folder.Key);
					break;
				}
			}
		}
	}

	// folder labels reference every asset in their folder. That is a cook rule, not usage: their references are never
	// followed. A label whose cook rule packages its folder keeps every asset in it used, added last so what only labels
	// keep is told apart. Other folder labels only sort assets into chunks, their folder assets follow their references
	TSet<FName> FolderLabels;
	TSet<FName> FolderLabelsPackaged;
	TMap<FName, FName> LabelAssets;
	GetFolderLabels(AssetsAll, Usage, FolderLabels, FolderLabelsPackaged, LabelAssets);
	LabelAssets.KeySort(FNameLexicalLess {});
	for (const auto& LabelAsset : LabelAssets) {
		AddUsed(LabelAsset.Key, EPjcUsageReason::FolderLabelAsset, LabelAsset.Value, FString {});
	}

	TSet<FName> PackagesScanned;
	PackagesScanned.Reserve(AssetsAll.Num());
	for (const auto& Asset : AssetsAll) {
		PackagesScanned.Add(Asset.PackageName);
	}

	// breadth first, so parent chains are short
	int32 QueueHead = 0;
	TArray<FName> Deps;
	const auto FollowDependencies = [&](const bool bFolderLabelOnly) {
		while (QueueHead < Queue.Num()) {
			const FName PackageName = Queue[QueueHead++];
			if (FolderLabels.Contains(PackageName)) continue;

			PjcScanCurrent(PackageName.ToString());

			// packages that are not scanned assets (e.g. actors saved in their own files) pass their parent on
			FName Parent = PackageName;
			FString Detail;
			if (!PackagesScanned.Contains(PackageName)) {
				const FPjcAssetUsage* ParentUsage = Usage.Find(PackageName);
				if (ParentUsage && !ParentUsage->Parent.IsNone()) {
					Parent = ParentUsage->Parent;
					Detail = TEXT("through ") + PackageName.ToString();
				}
			}

			Deps.Reset();
			GetModuleAssetRegistry().Get().GetDependencies(PackageName, Deps);
			Deps.Sort(FNameLexicalLess {});

			for (const auto& Dep : Deps) {
				if (!PathIsInsideProject(Dep.ToString())) continue;

				FPjcAssetUsage* DepUsage = AddUsed(Dep, EPjcUsageReason::Dependency, Parent, Detail);
				if (DepUsage && bFolderLabelOnly) {
					DepUsage->bFolderLabelOnly = true;
				}
			}
		}
	};

	// localized copies (Content/L10N/<culture>/...) are loaded in place of their source for that culture: they are
	// used as their source is, also when only a folder label keeps it
	const auto AddLocalized = [&]() {
		for (const auto& Asset : AssetsAll) {
			const FString PackageName = Asset.PackageName.ToString();
			if (Usage.Contains(Asset.PackageName) || !FPackageName::IsLocalizedPackage(PackageName)) continue;

			const FName Source {*FPackageName::GetSourcePackagePath(PackageName)};
			const FPjcAssetUsage* SourceUsage = Usage.Find(Source);
			if (!SourceUsage) continue;

			const bool bSourceUsed = SourceUsage->bUsed;
			const bool bSourceFolderLabelOnly = SourceUsage->bFolderLabelOnly;
			FPjcAssetUsage* CopyUsage = AddUsed(Asset.PackageName, EPjcUsageReason::Localized, Source, FString {});
			if (CopyUsage) {
				CopyUsage->bUsed = bSourceUsed;
				CopyUsage->bFolderLabelOnly = bSourceFolderLabelOnly;
			}
		}
	};

	// roots by their own reason, then what they reach
	TMap<EPjcUsageReason, int32> NumRoots;
	for (const auto& Pair : Usage) {
		NumRoots.FindOrAdd(Pair.Value.Reason)++;
	}
	const FString NumPrimary = PjcFormat::Count(NumRoots.FindRef(EPjcUsageReason::Primary));
	const FString NumEditor = PjcFormat::Count(NumRoots.FindRef(EPjcUsageReason::Editor) + NumRoots.FindRef(EPjcUsageReason::Splash));
	const FString NumExcluded = PjcFormat::Count(NumRoots.FindRef(EPjcUsageReason::Excluded) + NumRoots.FindRef(EPjcUsageReason::Cinematic));
	PjcScanFound(EPjcScanStep::Rules, FString::Printf(TEXT("%s primary \u00B7 %s editor \u00B7 %s excluded"), *NumPrimary, *NumEditor, *NumExcluded));
	PjcScanStep(EPjcScanStep::References);
	FollowDependencies(false);

	// assets of a primary class the Asset Manager did not register (outside the type's folders, or in a game feature
	// plugin that is not registered yet) are uncertain, so used. After the references above, which tell more, and
	// before folder labels, so labels keep only what nothing else does
	for (const auto& Asset : AssetsAll) {
		if (!ScanSettings->bUnregisteredPrimaryUsed) break;
		if (Usage.Contains(Asset.PackageName)) continue;

		const FName AssetClassName = PjcShim::GetAssetClassName(Asset);
		const FName AssetExactClassName = GetAssetExactClassName(Asset);
		if (ClassNamesPrimary.Contains(AssetClassName) || ClassNamesPrimary.Contains(AssetExactClassName)) {
			AddUsed(Asset.PackageName, EPjcUsageReason::PrimaryClass, NAME_None, AssetExactClassName.ToString());
		}
	}
	FollowDependencies(false);
	AddLocalized();
	FollowDependencies(false);

	// GameplayCue notifies are loaded by their tag from the cue notify paths, nothing references them. A tag fires the
	// notify of that tag, or of its nearest parent tag that has one. Tags in assets are searchable names in the registry
	TMap<FName, TArray<FName>> NotifiesByTag;
	for (const auto& Notify : CueNotifies) {
		NotifiesByTag.FindOrAdd(Notify.Value).Add(Notify.Key);
	}
	const auto FindNotifyTag = [&NotifiesByTag](FString Tag) {
		while (!Tag.IsEmpty()) {
			const FName TagName {*Tag};
			if (NotifiesByTag.Contains(TagName)) return TagName;

			int32 Dot = INDEX_NONE;
			if (!Tag.FindLastChar(TEXT('.'), Dot)) break;
			Tag.LeftInline(Dot);
		}
		return FName {};
	};

	// notify tag -> (referencing package, tag it references); a notify references its own tag
	TMap<FName, TArray<TPair<FName, FName>>> CueReferencers;
	TMap<FName, FName> CueNamed;
	if (CueNotifies.Num() > 0) {
		TSet<FName> Tags;
		NotifiesByTag.GetKeys(Tags);
		FGameplayTagContainer AllTags;
		UGameplayTagsManager::Get().RequestAllGameplayTags(AllTags, false);
		for (const auto& Tag : AllTags) {
			Tags.Add(Tag.GetTagName());
		}
		TArray<FName> TagsSorted = Tags.Array();
		TagsSorted.Sort(FNameLexicalLess {});

		TArray<FAssetIdentifier> TagReferencers;
		for (const auto& Tag : TagsSorted) {
			const FName NotifyTag = FindNotifyTag(Tag.ToString());
			if (NotifyTag.IsNone()) continue;

			TagReferencers.Reset();
			const FAssetIdentifier TagId {FGameplayTag::StaticStruct(), Tag};
			GetModuleAssetRegistry().Get().GetReferencers(TagId, TagReferencers, UE::AssetRegistry::EDependencyCategory::SearchableName);
			for (const auto& Referencer : TagReferencers) {
				const FName* ReferencerTag = CueNotifies.Find(Referencer.PackageName);
				if (ReferencerTag && *ReferencerTag == NotifyTag) continue;

				CueReferencers.FindOrAdd(NotifyTag).Emplace(Referencer.PackageName, Tag);
			}
		}

		TSet<FName> NamedTags;
		GetSourceAndConfigTagNames(NamedTags);
		TArray<FName> NamedSorted = NamedTags.Array();
		NamedSorted.Sort(FNameLexicalLess {});
		for (const auto& Named : NamedSorted) {
			const FName NotifyTag = FindNotifyTag(Named.ToString());
			if (!NotifyTag.IsNone() && !CueNamed.Contains(NotifyTag)) {
				CueNamed.Add(NotifyTag, Named);
			}
		}
		for (auto& Pair : CueReferencers) {
			Pair.Value.Sort([](const TPair<FName, FName>& A, const TPair<FName, FName>& B) {
				return A.Key.LexicalLess(B.Key);
			});
		}
	}

	TArray<FString> CuePaths;
	if (CueNotifies.Num() > 0) {
		GetGameplayCueNotifyPaths(CuePaths);
	}
	const auto IsInCuePaths = [&CuePaths, &CueNotifyPaths](const FName Notify) {
		const FString& NotifyPath = CueNotifyPaths.FindRef(Notify);
		return CuePaths.ContainsByPredicate([&NotifyPath](const FString& CuePath) {
			return NotifyPath == CuePath || NotifyPath.StartsWith(CuePath + TEXT("/"));
		});
	};

	// until nothing more fires: fired notifies may fire others
	const auto FireCues = [&](const bool bFolderLabelOnly) {
		bool bFired = CueNotifies.Num() > 0;
		while (bFired) {
			bFired = false;
			for (const auto& Notify : CueNotifies) {
				if (Usage.Contains(Notify.Key) || !IsInCuePaths(Notify.Key)) continue;

				FString Fired;
				if (const FName* Named = CueNamed.Find(Notify.Value)) {
					Fired = Named->ToString() + TEXT(", named in source or config");
				}
				for (const auto& Referencer : CueReferencers.FindRef(Notify.Value)) {
					const FPjcAssetUsage* ReferencerUsage = Usage.Find(Referencer.Key);
					if (!Fired.IsEmpty() || !ReferencerUsage || !ReferencerUsage->bUsed) continue;

					Fired = Referencer.Value.ToString() + TEXT(", fired by ") + FPackageName::GetShortName(Referencer.Key);
				}
				if (Fired.IsEmpty()) continue;

				FPjcAssetUsage* CueUsage = AddUsed(Notify.Key, EPjcUsageReason::GameplayCue, NAME_None, Fired);
				if (CueUsage) {
					CueUsage->bFolderLabelOnly = bFolderLabelOnly;
				}
				bFired = true;
			}
			if (bFired) {
				FollowDependencies(bFolderLabelOnly);
				AddLocalized();
				FollowDependencies(bFolderLabelOnly);
			}
		}
	};
	FireCues(false);

	// the cook takes the whole folder as it is now (the label finds it again when it loads), not the list it saved
	TArray<FName> FolderLabelsPackagedSorted = FolderLabelsPackaged.Array();
	FolderLabelsPackagedSorted.Sort(FNameLexicalLess {});
	for (const auto& FolderLabel : FolderLabelsPackagedSorted) {
		const FString LabelPath = FPackageName::GetLongPackagePath(FolderLabel.ToString());
		for (const auto& Asset : AssetsAll) {
			const FString AssetPath = Asset.PackagePath.ToString();
			if (AssetPath != LabelPath && !AssetPath.StartsWith(LabelPath + TEXT("/"))) continue;

			FPjcAssetUsage* LabelUsage = AddUsed(Asset.PackageName, EPjcUsageReason::FolderLabel, FolderLabel, FString {});
			if (LabelUsage) {
				LabelUsage->bFolderLabelOnly = true;
			}
		}
	}
	if (FolderLabelsPackaged.Num() > 0) {
		FollowDependencies(true);
		AddLocalized();
		FollowDependencies(true);
		FireCues(true);
	}

	// notifies nothing fires, or outside the cue notify paths: tags can be built at runtime, so kept for review
	for (const auto& Notify : CueNotifies) {
		if (Usage.Contains(Notify.Key)) continue;

		const FString Outside = IsInCuePaths(Notify.Key) ? FString {} : TEXT(", outside the GameplayCue notify paths");
		AddUsed(Notify.Key, EPjcUsageReason::GameplayCueNotFired, NAME_None, Notify.Value.ToString() + Outside);
	}
	FollowDependencies(false);
	AddLocalized();
	FollowDependencies(false);

	// unused assets the last cook of a platform packaged: the game may load them in a way the scan cannot follow, so
	// they are kept for review with what they use. Assets saved since that cook are not checked
	TArray<const FPjcCookInfo*> Cooks = FPjcCookInfo::GetAll();
	TArray<FString> PlatformsCookAll;
	Cooks.RemoveAll([&AssetsAll, &Usage, &PlatformsCookAll](const FPjcCookInfo* Cook) {
		const bool bCookAll = IsCookOfEverything(*Cook, AssetsAll, Usage);
		if (bCookAll) {
			PlatformsCookAll.Add(Cook->Platform);
		}
		return bCookAll;
	});
	ScanCacheSet(ScanCache.PlatformsCookAll, PlatformsCookAll);
	for (const auto& Asset : AssetsAll) {
		if (Usage.Contains(Asset.PackageName) || Cooks.Num() == 0) continue;

		for (const auto& Cook : Cooks) {
			if (!Cook->IsValid() || !Cook->Cooked.Contains(Asset.PackageName) || Cook->IsSavedSince(Asset.PackageName)) continue;

			AddUsed(Asset.PackageName, EPjcUsageReason::CookPackaged, NAME_None, Cook->Platform);
			break;
		}
	}
	FollowDependencies(false);
	AddLocalized();
	FollowDependencies(false);

	// assets placed in a level that saves each actor in its own file (World Partition, One File Per Actor) are used
	// because their map is: they show as its dependencies. Only the reason changes, never whether an
	// asset is used: they stay used as above. The map does not reference its actor files (the files point to the map),
	// so following references from the map would never reach them. Kept as outside references when any other outside
	// package references them, or a map of their actor files is not scanned or not used (actor files of a deleted map)
	{
		TMap<FName, TArray<FName>> ActorFilesOf;
#if ENGINE_MAJOR_VERSION < 5
		// UE4 registry has no dependencies of actor packages: read from the files
		TSet<FName> ActorFileDependencies;
		GetExternalActorsDependencies(ActorFileDependencies, &ActorFilesOf);
#endif
		IAssetRegistry& Registry = GetModuleAssetRegistry().Get();
		for (auto& Pair : Usage) {
			FPjcAssetUsage& AssetUsage = Pair.Value;
			if (!AssetUsage.bUsed || AssetUsage.Reason != EPjcUsageReason::EngineReferenced || !AssetUsage.Parent.IsNone()) continue;

			TArray<FName> ActorFiles;
			bool bOtherOutside = false;
			TArray<FName> Referencers;
			Registry.GetReferencers(Pair.Key, Referencers);
			for (const auto& Referencer : Referencers) {
				if (FolderIsExternal(Referencer.ToString())) {
					ActorFiles.AddUnique(Referencer);
				}
				else if (!PathIsInsideProject(Referencer.ToString())) {
					bOtherOutside = true;
				}
			}
			if (const TArray<FName>* Read = ActorFilesOf.Find(Pair.Key)) {
				for (const auto& ActorFile : *Read) {
					ActorFiles.AddUnique(ActorFile);
				}
			}
			if (bOtherOutside || ActorFiles.Num() == 0) continue;

			// every actor file belongs to a scanned, used map whose chain does not go through this asset
			FName Map;
			bool bAllMapsUsed = true;
			for (const auto& ActorFile : ActorFiles) {
				const FName Owner = GetExternalOwner(ActorFile);
				const FPjcAssetUsage* MapUsage = Owner.IsNone() ? nullptr : Usage.Find(Owner);
				if (!MapUsage || !MapUsage->bUsed || Owner == Pair.Key) {
					bAllMapsUsed = false;
					break;
				}
				const FPjcAssetUsage* Step = MapUsage;
				for (int32 NumSteps = 0; Step && !Step->Parent.IsNone() && NumSteps < 1024; ++NumSteps) {
					if (Step->Parent == Pair.Key) {
						bAllMapsUsed = false;
						break;
					}
					Step = Usage.Find(Step->Parent);
				}
				if (!bAllMapsUsed) break;
				Map = Map.IsNone() || Owner.LexicalLess(Map) ? Owner : Map;
			}
			if (!bAllMapsUsed || Map.IsNone()) continue;

			AssetUsage.Reason = EPjcUsageReason::Dependency;
			AssetUsage.Parent = Map;
			AssetUsage.Detail = ActorFiles[0].ToString();
		}
	}

	// dependencies come per package, map them back to scanned assets so used + unused = all. Redirectors keep their
	// usage (chains go through them), but are neither used nor unused assets
	TSet<FName> PackagesRedirector;
	GetPackagesRedirector(PackagesRedirector);
	OutUsed.Reset(AssetsAll.Num());
	OutFolderLabelOnly.Reset();
	OutUsage.Reset();
	OutUsage.Reserve(AssetsAll.Num());
	for (const auto& Asset : AssetsAll) {
		const FPjcAssetUsage* AssetUsage = Usage.Find(Asset.PackageName);
		OutUsage.Add(Asset.PackageName, AssetUsage ? *AssetUsage : FPjcAssetUsage {});
		if (!AssetUsage || PackagesRedirector.Contains(Asset.PackageName)) continue;

		if (AssetUsage->bUsed) {
			OutUsed.Emplace(Asset);
		}
		if (AssetUsage->bFolderLabelOnly) {
			OutFolderLabelOnly.Emplace(Asset);
		}
	}
	OutUsed.Shrink();

	// tell what each folder label decides, e.g. Lyra DefaultGame_Label covers all of /Game but packages nothing
	for (const auto& FolderLabel : FolderLabels) {
		const FString LabelPath = FPackageName::GetLongPackagePath(FolderLabel.ToString());
		int32 NumOnlyByLabel = 0;
		int32 NumUnused = 0;
		for (const auto& Asset : AssetsAll) {
			const FString AssetPath = Asset.PackagePath.ToString();
			if (AssetPath != LabelPath && !AssetPath.StartsWith(LabelPath + TEXT("/"))) continue;

			const FPjcAssetUsage* AssetUsage = Usage.Find(Asset.PackageName);
			NumOnlyByLabel += AssetUsage && AssetUsage->Reason == EPjcUsageReason::FolderLabel && AssetUsage->Parent == FolderLabel ? 1 : 0;
			NumUnused += AssetUsage ? 0 : 1;
		}

		const bool bPackaged = FolderLabelsPackaged.Contains(FolderLabel);
		if (!bPackaged && NumUnused == 0) continue;

		const FString Packaged = FString::Printf(TEXT("packages its folder, %d asset(s) used only by it"), NumOnlyByLabel);
		const FString NotPackaged = FString::Printf(TEXT("does not package its folder, %d asset(s) in it are unused"), NumUnused);
		const FString& LabelDetail = bPackaged ? Packaged : NotPackaged;
		UE_LOG(LogProjectCleaner, Display, TEXT("Folder label %s: its cook rule %s"), *FolderLabel.ToString(), *LabelDetail);
	}
}

void UPjcSubsystem::GetAssetsUnused(TArray<FAssetData>& Assets, const bool bShowSlowTask) {
	if (GetModuleAssetRegistry().Get().IsLoadingAssets()) return;
	if (ScanCacheGet(ScanCache.AssetsUnused, Assets)) return;

	TArray<FAssetData> AssetsAll;
	TArray<FAssetData> AssetsUsed;

	GetAssetsAll(AssetsAll);
	GetAssetsUsed(AssetsUsed, bShowSlowTask);

	const TSet<FAssetData> AssetsUsedSet {AssetsUsed};
	Assets.Reset(AssetsAll.Num());

	FScopedSlowTask SlowTask {
	  static_cast<float>(AssetsAll.Num()), FText::FromString(TEXT("Searching unused assets...")), bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	for (const auto& Asset : AssetsAll) {
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(Asset.GetFullName()));

		if (!AssetsUsedSet.Contains(Asset)) {
			Assets.Emplace(Asset);
		}
	}
	Assets.Shrink();

	ScanCacheSet(ScanCache.AssetsUnused, Assets);
}

void UPjcSubsystem::GetAssetsPrimary(TArray<FAssetData>& Assets, const bool bShowSlowTask) {
	if (GetModuleAssetRegistry().Get().IsLoadingAssets()) return;

	TArray<FAssetData> AssetsAll;
	GetAssetsScanned(AssetsAll);

	TSet<FName> PackagesPrimary;
	GetPackagesPrimary(PackagesPrimary);

	Assets.Reset(AssetsAll.Num());
	for (const auto& Asset : AssetsAll) {
		if (PackagesPrimary.Contains(Asset.PackageName)) {
			Assets.Emplace(Asset);
		}
	}

	Assets.Shrink();
}

void UPjcSubsystem::GetAssetsIndirect(TArray<FAssetData>& Assets, TArray<FPjcAssetIndirectInfo>& AssetsIndirectInfos, const bool bShowSlowTask) {
	if (GetModuleAssetRegistry().Get().IsLoadingAssets()) return;
	if (ScanCacheGet(ScanCache.AssetsIndirect, Assets) && ScanCacheGet(ScanCache.AssetsIndirectInfos, AssetsIndirectInfos)) return;

	Assets.Reset();
	AssetsIndirectInfos.Reset();

	TSet<FString> ScanFiles;
	GetSourceAndConfigFiles(ScanFiles);

	FScopedSlowTask SlowTask {
	  static_cast<float>(ScanFiles.Num()),
	  FText::FromString(TEXT("Searching Indirectly used assets...")),
	  bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	// asset paths of every scanned root, e.g. "/Game/Maps/Main.Main" or "/ShooterCore/Weapons/Rifle"
	TArray<FString> RootPatterns;
	for (const auto& RootPath : GetProjectRootPaths()) {
		RootPatterns.Add(RootPath.Replace(TEXT("/"), TEXT("\\/")));
	}
	// everything allowed in package names (e.g. "ButtonStyle-Primary-M"), stops at quotes, separators and brackets
	const FRegexPattern Pattern {FString::Printf(TEXT(R"((%s)\/[^\s"'`,:;=*?<>|&!~@#\\()\[\]{}]+)"), *FString::Join(RootPatterns, TEXT("|")))};

	TArray<FString> Lines;
	for (const auto& File : ScanFiles) {
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(File));
		PjcScanCurrent(File);

		Lines.Reset();
		FFileHelper::LoadFileToStringArray(Lines, *File);
		const FString FilePathAbs = FPaths::ConvertRelativePathToFull(File);
		// the plugin's own settings name assets without using them: exclusions added with Exclude are not Source & config
		const bool bConfig = FPaths::GetExtension(File).Equals(TEXT("ini"), ESearchCase::IgnoreCase);
		bool bPluginSection = false;

		for (int32 i = 0; i < Lines.Num(); ++i) {
			if (bConfig) {
				const FString Trimmed = Lines[i].TrimStartAndEnd();
				if (Trimmed.StartsWith(TEXT("[")) && Trimmed.EndsWith(TEXT("]"))) {
					bPluginSection = Trimmed.StartsWith(TEXT("[/Script/Pjc."));
					continue;
				}
				if (bPluginSection) continue;
			}
			FRegexMatcher Matcher(Pattern, Lines[i]);
			while (Matcher.FindNext()) {
				FString Match = Matcher.GetCaptureGroup(0);
				while (Match.EndsWith(TEXT(".")) || Match.EndsWith(TEXT("/"))) {
					Match.LeftChopInline(1);
				}
				// package file names count as package names, e.g. "/Game/Maps/Main.umap" in a map list
				for (const FString& Extension : {FPackageName::GetAssetPackageExtension(), FPackageName::GetMapPackageExtension()}) {
					if (Match.EndsWith(Extension)) {
						Match.LeftChopInline(Extension.Len());
						break;
					}
				}
				FString ObjectPath = PathConvertToObjectPath(Match);

				// package names are common in configs too, e.g. "/Game/Maps/Main" instead of "/Game/Maps/Main.Main"
				if (ObjectPath.IsEmpty() && !Match.Contains(TEXT("."))) {
					ObjectPath = PathConvertToObjectPath(Match + TEXT(".") + FPackageName::GetShortName(Match));
				}

				if (ObjectPath.IsEmpty()) continue;

				const FAssetData AssetData = PjcShim::GetAssetByObjectPath(ObjectPath);
				if (!AssetData.IsValid() || FolderIsExternal(AssetData.PackagePath.ToString())) continue;

				AssetsIndirectInfos.AddUnique(FPjcAssetIndirectInfo {AssetData, FilePathAbs, i + 1});
				Assets.AddUnique(AssetData);
			}
		}
	}

	ScanCacheSet(ScanCache.AssetsIndirect, Assets);
	ScanCacheSet(ScanCache.AssetsIndirectInfos, AssetsIndirectInfos);
}

void UPjcSubsystem::GetAssetsCircular(TArray<FAssetData>& Assets, const bool bShowSlowTask) {
	if (GetModuleAssetRegistry().Get().IsLoadingAssets()) return;

	TArray<FAssetData> AssetsAll;
	GetAssetsScanned(AssetsAll);

	Assets.Reset(AssetsAll.Num());

	FScopedSlowTask SlowTask {
	  static_cast<float>(AssetsAll.Num()),
	  FText::FromString(TEXT("Searching circular assets...")),
	  bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	for (const auto& Asset : AssetsAll) {
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(Asset.GetFullName()));

		if (AssetIsCircular(Asset)) {
			Assets.Emplace(Asset);
		}
	}

	Assets.Shrink();
}

void UPjcSubsystem::GetAssetsEditor(TArray<FAssetData>& Assets, const bool bShowSlowTask) {
	if (GetModuleAssetRegistry().Get().IsLoadingAssets()) return;

	TArray<FAssetData> AssetsAll;
	GetAssetsScanned(AssetsAll);

	TSet<FName> ClassNamesEditor;
	GetClassNamesEditor(ClassNamesEditor);

	Assets.Reset(AssetsAll.Num());

	FScopedSlowTask SlowTask {
	  static_cast<float>(AssetsAll.Num()), FText::FromString(TEXT("Searching editor assets...")), bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	for (const auto& Asset : AssetsAll) {
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(Asset.GetFullName()));

		if (ClassNamesEditor.Contains(PjcShim::GetAssetClassName(Asset)) || ClassNamesEditor.Contains(GetAssetExactClassName(Asset))) {
			Assets.Emplace(Asset);
		}
	}

	Assets.Shrink();
}

void UPjcSubsystem::GetAssetsExcluded(TArray<FAssetData>& Assets, const bool bShowSlowTask) {
	if (GetModuleAssetRegistry().Get().IsLoadingAssets()) return;
	if (ScanCacheGet(ScanCache.AssetsExcluded, Assets)) return;

	Assets.Reset();

	const UPjcAssetExcludeSettings* AssetExcludeSettings = GetDefault<UPjcAssetExcludeSettings>();
	if (!AssetExcludeSettings) return;

	FScopedSlowTask SlowTask {4.0f, FText::FromString(TEXT("Searching excluded assets...")), bShowSlowTask && GIsEditor && !IsRunningCommandlet()};
	SlowTask.MakeDialog(false, false);
	SlowTask.EnterProgressFrame(1.0f);

	TArray<FAssetData> AssetsAll;
	GetAssetsScanned(AssetsAll);

	TSet<FAssetData> AssetsExcluded;
	AssetsExcluded.Reserve(AssetsAll.Num());

	SlowTask.EnterProgressFrame(1.0f);
	// assets excluded by class names (user excluded classes and project used classes)
	{
		TSet<FName> ClassNamesExcluded;
		GetClassNamesExcluded(ClassNamesExcluded);

		for (const auto& Asset : AssetsAll) {
			if (ClassNamesExcluded.Contains(PjcShim::GetAssetClassName(Asset)) || ClassNamesExcluded.Contains(GetAssetExactClassName(Asset))) {
				AssetsExcluded.Emplace(Asset);
			}
		}
	}

	SlowTask.EnterProgressFrame(1.0f);
	// assets excluded by package paths
	{
		FARFilter Filter;

		Filter.bRecursivePaths = true;
		Filter.PackagePaths.Reserve(AssetExcludeSettings->ExcludedFolders.Num());

		for (const auto& ExcludedFolder : AssetExcludeSettings->ExcludedFolders) {
			if (!PathIsInsideProject(ExcludedFolder.Path)) continue;

			Filter.PackagePaths.Emplace(ExcludedFolder.Path);
		}

		if (Filter.PackagePaths.Num() > 0) {
			TArray<FAssetData> AssetsExcludedByPackagePaths;
			GetModuleAssetRegistry().Get().GetAssets(Filter, AssetsExcludedByPackagePaths);

			AssetsExcluded.Append(AssetsExcludedByPackagePaths);
		}
	}

	SlowTask.EnterProgressFrame(1.0f);
	// assets excluded by object paths
	{
		FARFilter Filter;

		PjcShim::ReserveFilterObjectPaths(Filter, AssetExcludeSettings->ExcludedAssets.Num());

		for (const auto& ExcludedAsset : AssetExcludeSettings->ExcludedAssets) {
			if (!ExcludedAsset.LoadSynchronous()) continue;

			FAssetData AssetData(ExcludedAsset.Get());
			PjcShim::AddFilterObjectPath(Filter, AssetData);
		}

		if (PjcShim::HasFilterObjectPaths(Filter)) {
			TArray<FAssetData> AssetsExcludedByObjectPaths;
			GetModuleAssetRegistry().Get().GetAssets(Filter, AssetsExcludedByObjectPaths);

			AssetsExcluded.Append(AssetsExcludedByObjectPaths);
		}
	}

	// registry queries above may return extra assets of same packages, keep scanned ones only
	TSet<FName> PackagesExcluded;
	for (const auto& Asset : AssetsExcluded) {
		PackagesExcluded.Add(Asset.PackageName);
	}

	Assets.Reset(PackagesExcluded.Num());
	for (const auto& Asset : AssetsAll) {
		if (PackagesExcluded.Contains(Asset.PackageName)) {
			Assets.Emplace(Asset);
		}
	}

	ScanCacheSet(ScanCache.AssetsExcluded, Assets);
}

void UPjcSubsystem::GetAssetsEngineReferenced(TArray<FAssetData>& Assets, const bool bShowSlowTask) {
	if (GetModuleAssetRegistry().Get().IsLoadingAssets()) return;
	if (ScanCacheGet(ScanCache.AssetsEngineReferenced, Assets)) return;

	TArray<FAssetData> AssetsAll;
	GetAssetsScanned(AssetsAll);

	Assets.Reset(AssetsAll.Num());

	TSet<FName> ExternalActorsDependencies;
	GetExternalActorsDependencies(ExternalActorsDependencies);

	FScopedSlowTask SlowTask {
	  static_cast<float>(AssetsAll.Num()),
	  FText::FromString(TEXT("Searching assets with external referencers...")),
	  bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	for (const auto& Asset : AssetsAll) {
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(Asset.GetFullName()));

		if (AssetIsEngineReferenced(Asset) || ExternalActorsDependencies.Contains(Asset.PackageName)) {
			Assets.Emplace(Asset);
		}
	}

	Assets.Shrink();

	ScanCacheSet(ScanCache.AssetsEngineReferenced, Assets);
}

void UPjcSubsystem::GetClassNamesPrimary(TSet<FName>& ClassNames) {
	ClassNames.Empty();

	// getting list of primary asset classes that are defined in AssetManager
	if (!PjcShim::IsAssetManagerValid()) return;

	const auto& AssetManager = UAssetManager::Get();

	TSet<FName> ClassNamesPrimaryBase;
	TArray<FPrimaryAssetTypeInfo> AssetTypeInfos;
	AssetManager.Get().GetPrimaryAssetTypeInfoList(AssetTypeInfos);
	ClassNamesPrimaryBase.Reserve(AssetTypeInfos.Num());

	for (const auto& AssetTypeInfo : AssetTypeInfos) {
		if (!AssetTypeInfo.AssetBaseClassLoaded) continue;

		// type registered without real base class (e.g. GameFeature plugins in Lyra like projects)
		// would make every asset used (#58)
		if (AssetTypeInfo.AssetBaseClassLoaded == UObject::StaticClass()) {
			const FString TypeName = AssetTypeInfo.PrimaryAssetType.ToString();
			UE_LOG(LogProjectCleaner, Verbose, TEXT("Primary asset type %s has UObject base class, ignored"), *TypeName);
			continue;
		}

		ClassNamesPrimaryBase.Emplace(PjcShim::GetClassName(AssetTypeInfo.AssetBaseClassLoaded));
	}

	// getting list of primary assets classes that are derived from main primary assets
	PjcShim::GetDerivedClassNames(ClassNamesPrimaryBase.Array(), TSet<FName> {}, ClassNames);
}

void UPjcSubsystem::GetPackagesPrimary(TSet<FName>& Packages) {
	Packages.Reset();

	// levels are opened by name, so every map is primary, not only the ones the Asset Manager registers (e.g. Lyra TransitionMap)
	TArray<FAssetData> AssetsAll;
	GetAssetsScanned(AssetsAll);
	const FName ClassNameWorld = PjcShim::GetClassName(UWorld::StaticClass());
	for (const auto& Asset : AssetsAll) {
		if (PjcShim::GetAssetClassName(Asset) == ClassNameWorld) {
			Packages.Add(Asset.PackageName);
		}
	}

	if (!PjcShim::IsAssetManagerValid()) return;

	const UAssetManager& AssetManager = UAssetManager::Get();
	TArray<FPrimaryAssetTypeInfo> TypeInfos;
	AssetManager.GetPrimaryAssetTypeInfoList(TypeInfos);
	for (const auto& TypeInfo : TypeInfos) {
		TArray<FPrimaryAssetId> Ids;
		AssetManager.GetPrimaryAssetIdList(TypeInfo.PrimaryAssetType, Ids);
		for (const auto& Id : Ids) {
			const FString ObjectPath = AssetManager.GetPrimaryAssetPath(Id).ToString();
			if (!ObjectPath.IsEmpty()) {
				Packages.Add(FName {*FPackageName::ObjectPathToPackageName(ObjectPath)});
			}
		}
	}
}

void UPjcSubsystem::GetConfigFolders(TMap<FString, FString>& Folders) {
	Folders.Reset();

	// full package paths (/Game/Folder, /PluginName/Folder) or relative to /Game, as the cooker reads them
	const auto AddFolder = [&Folders](FString Path, const TCHAR* Setting) {
		Path = Path.TrimStartAndEnd().Replace(TEXT("\\"), TEXT("/"));
		Path.RemoveFromEnd(TEXT("/"));
		if (Path.IsEmpty()) return;

		if (!Path.StartsWith(TEXT("/"))) {
			Path = TEXT("/Game/") + Path;
		}
		Folders.Add(Path, Setting);
	};

	for (const auto& Dir : GetDefault<UProjectPackagingSettings>()->DirectoriesToAlwaysCook) {
		AddFolder(Dir.Path, TEXT("Additional Asset Directories to Cook"));
	}

	// Data Registry plugin may not be enabled or loaded: its setting is read from config
	TArray<FString> RegistryDirs;
	GConfig->GetArray(TEXT("/Script/DataRegistry.DataRegistrySettings"), TEXT("DirectoriesToScan"), RegistryDirs, GGameIni);
	for (const auto& Entry : RegistryDirs) {
		FString Path;
		if (FParse::Value(*Entry, TEXT("Path="), Path)) {
			Path.RemoveFromEnd(TEXT(")"));
			AddFolder(Path, TEXT("Data Registry Directories to Scan"));
		}
	}
}

void UPjcSubsystem::GetClassNamesEditor(TSet<FName>& ClassNames) {
	ClassNames.Empty();

	// animation modifiers are editor tools too: applied to animations in the editor, never used by the game
	TArray<FName> ClassNamesEditorBase {
	  PjcShim::GetClassName(UEditorUtilityWidget::StaticClass()),
	  PjcShim::GetClassName(UEditorUtilityBlueprint::StaticClass()),
	  PjcShim::GetClassName(UEditorUtilityWidgetBlueprint::StaticClass()),
	  PjcShim::GetClassName(UAnimationModifier::StaticClass())
	};
	for (const auto& ClassPath : UPjcScanSettings::GetEditorClassPaths()) {
		// no loading: classes of disabled plugins or other engine versions are just skipped
		const UClass* EditorClass = FSoftClassPath {ClassPath}.ResolveClass();
		if (EditorClass) {
			ClassNamesEditorBase.Emplace(PjcShim::GetClassName(EditorClass));
		}
	}

	PjcShim::GetDerivedClassNames(ClassNamesEditorBase, TSet<FName> {}, ClassNames);
}

void UPjcSubsystem::GetClassNamesExcluded(TSet<FName>& ClassNames) {
	TSet<FName> ClassNamesUserExcluded;
	TSet<FName> ClassNamesUsedClasses;
	TSet<FName> ClassNamesCinematic;
	GetClassNamesExcludedBy(ClassNamesUserExcluded, ClassNamesUsedClasses, ClassNamesCinematic);

	ClassNames.Empty();
	ClassNames.Append(ClassNamesUserExcluded);
	ClassNames.Append(ClassNamesUsedClasses);
	ClassNames.Append(ClassNamesCinematic);
}

void UPjcSubsystem::GetClassNamesExcludedBy(TSet<FName>& OutUserExcluded, TSet<FName>& OutUsedClasses, TSet<FName>& OutCinematic) {
	OutUserExcluded.Empty();
	OutUsedClasses.Empty();
	OutCinematic.Empty();

	// user excluded classes match exactly
	const UPjcAssetExcludeSettings* AssetExcludeSettings = GetDefault<UPjcAssetExcludeSettings>();
	for (const auto& ExcludedClass : AssetExcludeSettings->ExcludedClasses) {
		if (!ExcludedClass.LoadSynchronous() || ExcludedClass.IsNull()) continue;

		OutUserExcluded.Emplace(PjcShim::GetClassName(ExcludedClass.Get()));
	}

	// project used classes match with derived classes
	const UPjcScanSettings* ScanSettings = GetDefault<UPjcScanSettings>();
	TArray<FName> ClassNamesUsedBase;

	for (const auto& UsedClass : ScanSettings->UsedClasses) {
		if (!UsedClass.LoadSynchronous() || UsedClass.IsNull()) continue;

		ClassNamesUsedBase.Emplace(PjcShim::GetClassName(UsedClass.Get()));
	}

	if (ClassNamesUsedBase.Num() > 0) {
		PjcShim::GetDerivedClassNames(ClassNamesUsedBase, TSet<FName> {}, OutUsedClasses);
	}

	if (ScanSettings->bCinematicAssetsUsed) {
		TArray<FName> ClassNamesCinematicBase;
		for (const auto& ClassPath : UPjcScanSettings::GetCinematicClassPaths()) {
			// no loading: classes of disabled plugins or other engine versions are just skipped
			const UClass* CinematicClass = FSoftClassPath {ClassPath}.ResolveClass();
			if (!CinematicClass) continue;

			ClassNamesCinematicBase.Emplace(PjcShim::GetClassName(CinematicClass));
		}

		if (ClassNamesCinematicBase.Num() > 0) {
			PjcShim::GetDerivedClassNames(ClassNamesCinematicBase, TSet<FName> {}, OutCinematic);
		}
	}
}

FString UPjcSubsystem::GetExcludedDetail(
	const FAssetData& Asset,
	const TSet<FName>& UserExcluded,
	const TSet<FName>& UsedClasses,
	const TSet<FName>& Cinematic
) {
	const UPjcAssetExcludeSettings* AssetExcludeSettings = GetDefault<UPjcAssetExcludeSettings>();

	for (const auto& ExcludedAsset : AssetExcludeSettings->ExcludedAssets) {
		if (ExcludedAsset.ToSoftObjectPath().GetLongPackageName() == Asset.PackageName.ToString()) {
			return TEXT("Excluded asset");
		}
	}

	const FString PackagePath = Asset.PackagePath.ToString();
	for (const auto& ExcludedFolder : AssetExcludeSettings->ExcludedFolders) {
		if (PackagePath == ExcludedFolder.Path || PackagePath.StartsWith(ExcludedFolder.Path + TEXT("/"))) {
			return TEXT("In excluded folder ") + ExcludedFolder.Path;
		}
	}

	const FName ClassName = PjcShim::GetAssetClassName(Asset);
	const FName ExactClassName = GetAssetExactClassName(Asset);
	const auto HasClass = [&](const TSet<FName>& ClassNames) {
		return ClassNames.Contains(ClassName) || ClassNames.Contains(ExactClassName);
	};

	if (HasClass(UserExcluded)) return TEXT("Excluded class ") + ExactClassName.ToString();
	if (HasClass(UsedClasses)) return TEXT("Always used class ") + ExactClassName.ToString();
	if (HasClass(Cinematic)) return PjcCinematicDetail + ExactClassName.ToString();

	return TEXT("Excluded");
}

bool UPjcSubsystem::GetExclusion(const FAssetData& Asset, FPjcExclusion& OutExclusion) {
	const UPjcAssetExcludeSettings* AssetExcludeSettings = GetDefault<UPjcAssetExcludeSettings>();

	for (const auto& ExcludedAsset : AssetExcludeSettings->ExcludedAssets) {
		if (ExcludedAsset.ToSoftObjectPath().GetLongPackageName() == Asset.PackageName.ToString()) {
			OutExclusion.Kind = EPjcExclusionKind::Asset;
			OutExclusion.Value = ExcludedAsset.ToSoftObjectPath().ToString();
			return true;
		}
	}

	const FString PackagePath = Asset.PackagePath.ToString();
	for (const auto& ExcludedFolder : AssetExcludeSettings->ExcludedFolders) {
		if (PackagePath == ExcludedFolder.Path || PackagePath.StartsWith(ExcludedFolder.Path + TEXT("/"))) {
			OutExclusion.Kind = EPjcExclusionKind::Folder;
			OutExclusion.Value = ExcludedFolder.Path;
			return true;
		}
	}

	// classes match exactly, as the scan excludes them (loaded by GetClassNamesExcludedBy)
	const FName ClassName = PjcShim::GetAssetClassName(Asset);
	const FName ExactClassName = GetAssetExactClassName(Asset);
	for (const auto& ExcludedClass : AssetExcludeSettings->ExcludedClasses) {
		const UClass* Class = ExcludedClass.Get();
		const FName ExcludedName = Class ? PjcShim::GetClassName(Class) : NAME_None;
		if (!ExcludedName.IsNone() && (ExcludedName == ClassName || ExcludedName == ExactClassName)) {
			OutExclusion.Kind = EPjcExclusionKind::Class;
			OutExclusion.Value = ExcludedClass.ToSoftObjectPath().ToString();
			return true;
		}
	}

	return false;
}

FString UPjcSubsystem::GetPrimaryDetail(const FAssetData& Asset) {
	if (PjcShim::IsAssetManagerValid()) {
		const FPrimaryAssetId PrimaryAssetId = UAssetManager::Get().GetPrimaryAssetIdForData(Asset);
		if (PrimaryAssetId.IsValid()) {
			return PrimaryAssetId.ToString();
		}
	}

	return GetAssetExactClassName(Asset).ToString();
}

FString UPjcSubsystem::GetEngineReferencedDetail(const FAssetData& Asset) {
	TArray<FName> Refs;
	GetModuleAssetRegistry().Get().GetReferencers(Asset.PackageName, Refs);

	for (const auto& Ref : Refs) {
		if (!PathIsInsideProject(Ref.ToString()) || FolderIsExternal(Ref.ToString())) {
			return Ref.ToString();
		}
	}

	// UE4 registry has no dependencies of actor packages, they are read from package files
	return TEXT("actor placed in a level (one file per actor)");
}

void UPjcSubsystem::GetFiles(const FString& InSearchPath, const bool bSearchRecursive, TArray<FString>& OutFiles) {
	OutFiles.Empty();

	struct FFindFilesVisitor : IPlatformFile::FDirectoryVisitor
	{
		TArray<FString>& Files;

		explicit FFindFilesVisitor(TArray<FString>& InFiles) : Files(InFiles) {}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override {
			if (!bIsDirectory) {
				Files.Emplace(FPaths::ConvertRelativePathToFull(FilenameOrDirectory));
			}

			return true;
		}
	};

	FFindFilesVisitor FindFilesVisitor {OutFiles};

	if (bSearchRecursive) {
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryRecursively(*InSearchPath, FindFilesVisitor);
	}
	else {
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*InSearchPath, FindFilesVisitor);
	}
}

void UPjcSubsystem::GetFilesByExt(
	const FString& InSearchPath,
	const bool bSearchRecursive,
	const bool bExtSearchInvert,
	const TSet<FString>& InExtensions,
	TArray<FString>& OutFiles
) {
	OutFiles.Empty();

	struct FFindFilesVisitor : IPlatformFile::FDirectoryVisitor
	{
		const bool bSearchInvert;
		TArray<FString>& Files;
		const TSet<FString>& Extensions;

		explicit FFindFilesVisitor(const bool bInSearchInvert, TArray<FString>& InFiles, const TSet<FString>& InExtensions) :
			bSearchInvert(bInSearchInvert),
			Files(InFiles),
			Extensions(InExtensions) {}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override {
			if (!bIsDirectory) {
				const FString FullPath = FPaths::ConvertRelativePathToFull(FilenameOrDirectory);

				if (Extensions.Num() == 0) {
					Files.Emplace(FullPath);
					return true;
				}

				const FString Ext = FPaths::GetExtension(FullPath, false);
				const bool bExistsInSearchList = Extensions.Contains(Ext);

				if ((bExistsInSearchList && !bSearchInvert) || (!bExistsInSearchList && bSearchInvert)) {
					Files.Emplace(FullPath);
				}
			}

			return true;
		}
	};

	TSet<FString> ExtensionsNormalized;
	ExtensionsNormalized.Reserve(InExtensions.Num());

	for (const auto& Ext : InExtensions) {
		const FString ExtNormalized = Ext.Replace(TEXT("."), TEXT(""));
		ExtensionsNormalized.Emplace(ExtNormalized);
	}

	FFindFilesVisitor FindFilesVisitor {bExtSearchInvert, OutFiles, ExtensionsNormalized};
	if (bSearchRecursive) {
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryRecursively(*InSearchPath, FindFilesVisitor);
	}
	else {
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*InSearchPath, FindFilesVisitor);
	}
}

void UPjcSubsystem::GetFilesExternalAll(TArray<FString>& Files) {
	Files.Reset();

	TArray<FString> RootFiles;
	for (const auto& ContentDir : GetProjectContentDirs()) {
		GetFilesByExt(ContentDir, true, true, PjcConstants::EngineFileExtensions, RootFiles);
		Files.Append(RootFiles);
	}
	PjcShim::RemoveAllSwapArray(Files, [](const FString& File) {
		return PathIsLeftOut(PathConvertToRelative(FPaths::GetPath(File)));
	});
}

void UPjcSubsystem::GetFilesExternalFiltered(TArray<FString>& Files, const bool bShowSlowTask) {
	TArray<FString> FilesExternalAll;
	GetFilesExternalAll(FilesExternalAll);

	Files.Reset(FilesExternalAll.Num());

	FPjcScanCacheScope ScanCacheScope;
	FScopedSlowTask SlowTask(
		static_cast<float>(FilesExternalAll.Num()),
		FText::FromString(TEXT("Searching for external files...")),
		bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	);
	SlowTask.MakeDialog(false, false);

	for (const auto& File : FilesExternalAll) {
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(File));

		if (FileIsExcluded(File)) continue;

		Files.Emplace(File);
	}

	Files.Shrink();
}

void UPjcSubsystem::GetFilesExternalExcluded(TArray<FString>& Files, const bool bShowSlowTask) {
	TArray<FString> FilesExternalAll;
	GetFilesExternalAll(FilesExternalAll);

	Files.Reset(FilesExternalAll.Num());

	FPjcScanCacheScope ScanCacheScope;
	FScopedSlowTask SlowTask(
		static_cast<float>(FilesExternalAll.Num()),
		FText::FromString(TEXT("Searching for external files...")),
		bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	);
	SlowTask.MakeDialog(false, false);

	for (const auto& File : FilesExternalAll) {
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(File));

		if (FileIsExcluded(File)) {
			Files.Emplace(File);
		}
	}

	Files.Shrink();
}

void UPjcSubsystem::GetFilesCorrupted(TArray<FString>& Files, const bool bShowSlowTask) {
	Files.Reset();

	// files are corrupted when registry does not know them, so incomplete registry would report every asset
	if (GetModuleAssetRegistry().Get().IsLoadingAssets()) return;

	TArray<FString> FileAssets;
	TArray<FString> RootFiles;

	for (const auto& ContentDir : GetProjectContentDirs()) {
		GetFilesByExt(ContentDir, true, false, PjcConstants::EngineFileExtensions, RootFiles);
		FileAssets.Append(RootFiles);
	}

	Files.Reset(FileAssets.Num());

	FScopedSlowTask SlowTask(
		static_cast<float>(FileAssets.Num()),
		FText::FromString(TEXT("Searching for corrupted asset files...")),
		bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	);
	SlowTask.MakeDialog(false, false);

	for (const auto& File : FileAssets) {
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(File));

		const FString FileFolder = PathConvertToRelative(FPaths::GetPath(File));
		if (FolderIsExternal(FileFolder) || PathIsLeftOut(FileFolder)) continue;

		const FString Path = PathConvertToObjectPath(File);
		if (Path.IsEmpty()) continue;
		if (PjcShim::GetAssetByObjectPath(Path).IsValid()) continue;

		Files.Emplace(File);
	}

	Files.Shrink();
}

void UPjcSubsystem::GetFolders(const FString& InSearchPath, const bool bSearchRecursive, TArray<FString>& OutFolders) {
	OutFolders.Empty();

	struct FFindFoldersVisitor : IPlatformFile::FDirectoryVisitor
	{
		TArray<FString>& Folders;

		explicit FFindFoldersVisitor(TArray<FString>& InFolders) : Folders(InFolders) {}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override {
			if (bIsDirectory) {
				Folders.Emplace(FPaths::ConvertRelativePathToFull(FilenameOrDirectory));
			}

			return true;
		}
	};

	FFindFoldersVisitor FindFoldersVisitor {OutFolders};
	if (bSearchRecursive) {
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryRecursively(*InSearchPath, FindFoldersVisitor);
	}
	else {
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*InSearchPath, FindFoldersVisitor);
	}
}

void UPjcSubsystem::GetFoldersEmpty(TArray<FString>& Folders) {
	TArray<FString> FoldersAll;

	for (const auto& RootPath : GetProjectRootPaths()) {
		TArray<FString> RootFolders;
		GetFolders(PathConvertToAbsolute(RootPath), true, RootFolders);
		FoldersAll.Append(RootFolders);
	}

	Folders.Reset(FoldersAll.Num());

	for (const auto& Folder : FoldersAll) {
		if (FolderIsEmpty(Folder) && !FolderIsEngineGenerated(Folder) && !FolderIsExcluded(Folder) && !PathIsLeftOut(PathConvertToRelative(Folder))) {
			Folders.Emplace(Folder);
		}
	}

	Folders.Shrink();
}

bool UPjcSubsystem::DeleteAssetsUnused(const bool bShowSlowTask, const bool bShowEditorNotification) {
	// saved first: unsaved edits could reference assets that look unused on disk
	if (!PrepareDelete()) return false;

	TArray<FAssetData> AssetsUnused;
	GetAssetsUnused(AssetsUnused);

	if (AssetsUnused.Num() == 0) {
		UE_LOG(LogProjectCleaner, Display, TEXT("The project currently contains no unused assets, thus there are no items to delete."));
		return true;
	}

	return DeleteAssets(AssetsUnused, bShowSlowTask, bShowEditorNotification);
}

bool UPjcSubsystem::PrepareDelete() {
	if (GetModuleAssetRegistry().Get().IsLoadingAssets()) {
		UE_LOG(LogProjectCleaner, Warning, TEXT("Failed to delete unused assets, because AssetRegistry still discovering assets."));
		UE_LOG(LogProjectCleaner, Warning, TEXT("Please wait until it finished and then try again."));
		return false;
	}

	if (GEditor && !GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllAssetEditors()) {
		UE_LOG(LogProjectCleaner, Warning, TEXT("Failed to delete unused assets because some editor windows are still open."));
		// clang-format off
		UE_LOG(LogProjectCleaner, Warning, TEXT("Please try again, check the Output Log for reasons why certain windows haven't closed, ")
			TEXT("or try to manually close all editor windows."));
		// clang-format on
		return false;
	}

	// unsaved edits could reference assets that look unused on disk. Commandlets have no user edits, packages there
	// only get dirty when loading updates them (e.g. Niagara systems after redirector fixup) and nobody can answer a prompt
	if (!IsRunningCommandlet() && !FEditorFileUtils::SaveDirtyPackages(true, true, true, false, false, false)) {
		UE_LOG(LogProjectCleaner, Warning, TEXT("Failed to delete unused assets because project contains unsaved assets."));
		UE_LOG(LogProjectCleaner, Warning, TEXT("Please save those assets and then try again. Check the Output Log for more information"));
		return false;
	}

	return true;
}

bool UPjcSubsystem::DeleteAssets(
	const TArray<FAssetData>& Assets,
	const bool bShowSlowTask,
	const bool bShowEditorNotification,
	FPjcDeleteProgress* Progress
) {
	if (!PrepareDelete()) return false;
	if (Assets.Num() == 0) return true;

	TArray<FAssetData> AssetsUnused = Assets;

	// asset delete handlers (e.g. RigVM) create it on first use, building it while assets are half deleted asserts (UE 5.4)
	FBlueprintActionDatabase::Get();

	LastCleanupKept.Reset();

	constexpr int32 BucketSize = PjcConstants::BucketSize;
	const TArray<FAssetData> AssetsToDelete = AssetsUnused;
	TSet<FName> PackagesUnused;
	for (const auto& Asset : AssetsUnused) {
		PackagesUnused.Add(Asset.PackageName);
	}
	const int32 NumAssetsTotal = AssetsUnused.Num();
	int32 NumAssetsDeleted = 0;

	// an unreferenced redirector to an asset that goes would be left broken and hold that asset: it goes first.
	// Redirectors are no unused assets, only this and fixing remove them
	TArray<FAssetData> RedirectorsStale;
	GetRedirectorsStale(PackagesUnused, RedirectorsStale);
	if (RedirectorsStale.Num() > 0) {
		FPjcRedirectorFixOptions Options;
		Options.bCheckNeeded = false;
		const FPjcRedirectorFixResult Fixed = FixRedirectorsWithReport(RedirectorsStale, false, Options);
		UE_LOG(LogProjectCleaner, Display, TEXT("Deleted %d redirector(s) to deleted assets"), Fixed.Deleted.Num());
	}

	// Engine asks before loading many assets (UE 5.4+). In commandlets and unattended runs nobody can answer,
	// default answer cancels loading and deletion aborts. Users can skip it too, they already confirmed deletion (#60).
	UContentBrowserSettings* ContentBrowserSettings = GetMutableDefault<UContentBrowserSettings>();
	const int32 NumObjectsToLoadBeforeWarning = ContentBrowserSettings->NumObjectsToLoadBeforeWarning;
	if (IsRunningCommandlet() || FApp::IsUnattended() || GetDefault<UPjcEditorSettings>()->bSkipAssetLoadWarning) {
		ContentBrowserSettings->NumObjectsToLoadBeforeWarning = MAX_int32;
	}
	ON_SCOPE_EXIT {
		ContentBrowserSettings->NumObjectsToLoadBeforeWarning = NumObjectsToLoadBeforeWarning;
	};

	TArray<FAssetData> Bucket;
	TArray<UObject*> LoadedAssets;
	LoadedAssets.Reserve(BucketSize);
	Bucket.Reserve(BucketSize);

	// Assets must be loaded first when deleting, which can cause a lot of unnecessary shader compilation work.
	// Therefore, we disable shader compilation during this stage for faster deletion and then enable it afterward.
	ShaderCompilationDisable();

	FScopedSlowTask SlowTask(
		AssetsUnused.Num(), FText::FromString(TEXT("Deleting unused assets...")), bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	);
	SlowTask.MakeDialog(false, false);

	bool bErrors = false;
	int32 NumProgress = 0;
	// what the batches tried: after a stop only these are looked at
	TArray<FAssetData> AssetsTried;

	while (AssetsUnused.Num() > 0) {
		BucketFill(AssetsUnused, Bucket, BucketSize, PackagesUnused);

		if (Bucket.Num() == 0) {
			break;
		}
		AssetsTried.Append(Bucket);

		if (!BucketPrepare(Bucket, LoadedAssets)) {
			bErrors = true;
			UE_LOG(LogProjectCleaner, Error, TEXT("Failed to load some assets. Aborting."));
			break;
		}

		NumAssetsDeleted += BucketDelete(LoadedAssets);
		const FString DeletedText = PjcFormat::Count(NumAssetsDeleted);
		const FString ProgressMsg = FString::Printf(TEXT("Deleted %s of %s assets"), *DeletedText, *PjcFormat::Count(NumAssetsTotal));
		const int32 ProgressWork = FMath::Min(Bucket.Num(), NumAssetsTotal - NumProgress);
		NumProgress += ProgressWork;
		SlowTask.EnterProgressFrame(ProgressWork, FText::FromString(ProgressMsg));

		if (Progress) {
			// assets taken off the list, each once
			Progress->NumTried = NumAssetsTotal - AssetsUnused.Num();
			if (Progress->OnBatch) {
				Progress->OnBatch(Progress->NumTried, NumAssetsTotal, Bucket.Last().AssetName.ToString());
			}
			if (Progress->bStopRequested && AssetsUnused.Num() > 0) {
				Progress->bStopped = true;
				Bucket.Reset();
				LoadedAssets.Reset();
				break;
			}
		}

		Bucket.Reset();
		LoadedAssets.Reset();
	}

	// stopped: what was not tried stays as it is
	const TArray<FAssetData> AssetsDone = Progress && Progress->bStopped ? AssetsTried : AssetsToDelete;

	// packages left empty by this deletion only: registry "empty" list also holds packages of other loaded assets
	const TSet<FName> EmptyPackages = PjcShim::GetCachedEmptyPackages();
	TArray<UPackage*> AssetPackages;
	for (const auto& Asset : AssetsDone) {
		if (!EmptyPackages.Contains(Asset.PackageName)) continue;

		UPackage* Package = FindPackage(nullptr, *Asset.PackageName.ToString());
		if (Package && Package->IsValidLowLevel()) {
			AssetPackages.AddUnique(Package);
		}
	}

	if (AssetPackages.Num() > 0) {
		ObjectTools::CleanupAfterSuccessfulDelete(AssetPackages);
	}

	DeleteAssetsReferencedByFolderLabels(AssetsDone);
	DeleteAssetsHeldByEachOther(AssetsDone);

	ShaderCompilationEnable();

	// assets can be deleted together with other assets, so count what is really gone
	TArray<UObject*> AssetsRemaining;
	int32 NumNotLoaded = 0;
	for (const auto& Asset : AssetsDone) {
		if (!FPackageName::DoesPackageExist(Asset.PackageName.ToString())) continue;

		if (UObject* Object = FindObject<UObject>(nullptr, *PjcShim::GetObjectPathString(Asset))) {
			AssetsRemaining.Add(Object);
		}
		else {
			UE_LOG(LogProjectCleaner, Warning, TEXT("Delete failed: %s"), *PjcShim::GetObjectPathString(Asset));
			LastCleanupKept.Add(Asset.PackageName, TEXT("could not be loaded"));
			++NumNotLoaded;
		}
	}

	// tell who keeps each of them, a live reference means something uses it even when no saved asset does
	TMap<FString, FString> Holders;
	TArray<UObject*> HeldByEachOther = AssetsRemaining;
	RemoveAssetsHeldOutside(HeldByEachOther, &Holders);
	for (const UObject* Asset : AssetsRemaining) {
		const FString* Holder = Holders.Find(Asset->GetPathName());
		const FString HeldBy = Holder ? *Holder : GetKeptReason(Asset->GetOutermost()->GetFName());
		// the line CI scripts and polygon read: keep its start as it is
		UE_LOG(LogProjectCleaner, Warning, TEXT("Delete failed, still referenced: %s (%s)"), *Asset->GetPathName(), *HeldBy);
		LastCleanupKept.Add(Asset->GetOutermost()->GetFName(), HeldBy);
	}
	int32 NumAssetsRemaining = 0;
	for (const auto& Asset : AssetsDone) {
		NumAssetsRemaining += FPackageName::DoesPackageExist(Asset.PackageName.ToString()) ? 1 : 0;
	}
	NumAssetsDeleted = AssetsDone.Num() - NumAssetsRemaining;

	const FString Msg = FString::Printf(TEXT("Deleted %s of %s assets"), *PjcFormat::Count(NumAssetsDeleted), *PjcFormat::Count(NumAssetsTotal));
	UE_LOG(LogProjectCleaner, Display, TEXT("%s"), *Msg);

	if (bErrors) {
		UE_LOG(LogProjectCleaner, Error, TEXT("There were some errors while deleting assets. Please check the Output Log for more information."));
	}

	if (bShowEditorNotification && GEditor) {
		if (NumAssetsDeleted == NumAssetsTotal) {
			ShowNotification(Msg, SNotificationItem::CS_Success, 3.0f);
		}
		else {
			ShowNotificationWithOutputLog(Msg, SNotificationItem::CS_Fail, 5.0f);
		}
	}

	// kept on purpose is no failure: something still holds those assets, deleting them would break it
	return !bErrors && NumNotLoaded == 0;
}

int32 UPjcSubsystem::DeleteFiles(const TArray<FString>& Files) {
	int32 NumDeleted = 0;
	for (const auto& File : Files) {
		if (IFileManager::Get().Delete(*File, false, true, true) && !FPaths::FileExists(File)) {
			++NumDeleted;
		}
		else {
			UE_LOG(LogProjectCleaner, Error, TEXT("Failed to delete file: %s"), *File);
		}
	}
	return NumDeleted;
}

int32 UPjcSubsystem::DeleteFolders(const TArray<FString>& Folders) {
	int32 NumDeleted = 0;
	for (const auto& Folder : Folders) {
		// gone with an empty parent folder deleted before it
		if (!IFileManager::Get().DirectoryExists(*Folder)) {
			++NumDeleted;
			GetModuleAssetRegistry().Get().RemovePath(PathConvertToRelative(Folder));
			continue;
		}
		if (!FolderIsEmpty(Folder) || !IFileManager::Get().DeleteDirectory(*Folder, true, true)) {
			UE_LOG(LogProjectCleaner, Error, TEXT("Failed to delete folder: %s"), *Folder);
			continue;
		}
		++NumDeleted;
		GetModuleAssetRegistry().Get().RemovePath(PathConvertToRelative(Folder));
	}
	return NumDeleted;
}

bool UPjcSubsystem::DeleteFoldersEmpty(const bool bShowSlowTask, const bool bShowEditorNotification) {
	TArray<FString> FoldersEmpty;
	GetFoldersEmpty(FoldersEmpty);

	if (FoldersEmpty.Num() == 0) {
		UE_LOG(LogProjectCleaner, Display, TEXT("The project currently contains no empty folders, thus there are no items to delete."));
		return true;
	}

	FScopedSlowTask SlowTaskMain(1.0f, FText::FromString(TEXT("Deleting empty folders...")), bShowSlowTask && GIsEditor && !IsRunningCommandlet());
	SlowTaskMain.MakeDialog(false, false);
	SlowTaskMain.EnterProgressFrame(1.0f);

	FScopedSlowTask SlowTask(FoldersEmpty.Num(), FText::FromString(TEXT(" ")), bShowSlowTask && GIsEditor && !IsRunningCommandlet());
	SlowTask.MakeDialog(false, false);

	bool bErrors = false;

	const int32 NumFoldersTotal = FoldersEmpty.Num();
	int32 NumFoldersDeleted = 0;

	for (const auto& Folder : FoldersEmpty) {
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(Folder));

		if (!IFileManager::Get().DeleteDirectory(*Folder, true, true)) {
			bErrors = true;
			UE_LOG(LogProjectCleaner, Error, TEXT("Failed to delete folder: %s"), *Folder);
			continue;
		}

		++NumFoldersDeleted;

		// if folder deleted successfully, we must remove it from cached AssetRegistry paths also.
		GetModuleAssetRegistry().Get().RemovePath(PathConvertToRelative(Folder));
	}

	const FString DeletedText = PjcFormat::Count(NumFoldersDeleted);
	const FString Msg = FString::Printf(TEXT("Deleted %s of %s empty folders"), *DeletedText, *PjcFormat::Count(NumFoldersTotal));
	UE_LOG(LogProjectCleaner, Display, TEXT("%s"), *Msg);

	if (bErrors) {
		UE_LOG(LogProjectCleaner, Error, TEXT("Failed to delete some folders. Please check the Output Log for more information"));
	}

	if (bShowEditorNotification && GEditor) {
		if (NumFoldersDeleted == NumFoldersTotal) {
			ShowNotification(Msg, SNotificationItem::CS_Success, 3.0f);
		}
		else {
			ShowNotificationWithOutputLog(Msg, SNotificationItem::CS_Fail, 5.0f);
		}
	}

	return !bErrors;
}

bool UPjcSubsystem::DeleteFilesExternal(const bool bShowSlowTask, const bool bShowEditorNotification) {
	TArray<FString> FilesExternalFiltered;
	GetFilesExternalFiltered(FilesExternalFiltered);

	if (FilesExternalFiltered.Num() == 0) {
		UE_LOG(LogProjectCleaner, Display, TEXT("The project currently contains no external files, thus there are no items to delete."));
		return true;
	}

	FScopedSlowTask SlowTask(
		FilesExternalFiltered.Num(), FText::FromString(TEXT("Deleting external files ...")), bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	);
	SlowTask.MakeDialog(false, false);

	bool bErrors = false;

	const int32 NumFilesTotal = FilesExternalFiltered.Num();
	int32 NumFilesDeleted = 0;

	for (const auto& File : FilesExternalFiltered) {
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(File));

		if (!IFileManager::Get().Delete(*File, true)) {
			bErrors = true;
			UE_LOG(LogProjectCleaner, Error, TEXT("Failed to delete file: %s"), *File);
			continue;
		}

		++NumFilesDeleted;
	}

	const FString DeletedText = PjcFormat::Count(NumFilesDeleted);
	const FString Msg = FString::Printf(TEXT("Deleted %s of %s external files"), *DeletedText, *PjcFormat::Count(NumFilesTotal));
	UE_LOG(LogProjectCleaner, Display, TEXT("%s"), *Msg);

	if (bErrors) {
		UE_LOG(LogProjectCleaner, Error, TEXT("Failed to delete some files. Please check the Output Log for more information"));
	}

	if (bShowEditorNotification && GEditor) {
		if (NumFilesDeleted == NumFilesTotal) {
			ShowNotification(Msg, SNotificationItem::CS_Success, 3.0f);
		}
		else {
			ShowNotificationWithOutputLog(Msg, SNotificationItem::CS_Fail, 5.0f);
		}
	}

	return !bErrors;
}

bool UPjcSubsystem::DeleteFilesCorrupted(const bool bShowSlowTask, const bool bShowEditorNotification) {
	if (GetModuleAssetRegistry().Get().IsLoadingAssets()) {
		UE_LOG(LogProjectCleaner, Warning, TEXT("Failed to delete corrupted files, because AssetRegistry still discovering assets."));
		return false;
	}

	TArray<FString> FilesCorrupted;
	GetFilesCorrupted(FilesCorrupted);

	if (FilesCorrupted.Num() == 0) {
		UE_LOG(LogProjectCleaner, Display, TEXT("The project currently contains no corrupted files, thus there are no items to delete."));
		return true;
	}

	FScopedSlowTask SlowTask(
		FilesCorrupted.Num(), FText::FromString(TEXT("Deleting corrupted asset files ...")), bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	);
	SlowTask.MakeDialog(false, false);

	bool bErrors = false;

	const int32 NumFilesTotal = FilesCorrupted.Num();
	int32 NumFilesDeleted = 0;

	for (const auto& File : FilesCorrupted) {
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(File));

		if (!IFileManager::Get().Delete(*File, true)) {
			bErrors = true;
			UE_LOG(LogProjectCleaner, Error, TEXT("Failed to delete file: %s"), *File);
			continue;
		}

		++NumFilesDeleted;
	}

	const FString DeletedText = PjcFormat::Count(NumFilesDeleted);
	const FString Msg = FString::Printf(TEXT("Deleted %s of %s corrupted files"), *DeletedText, *PjcFormat::Count(NumFilesTotal));
	UE_LOG(LogProjectCleaner, Display, TEXT("%s"), *Msg);

	if (bErrors) {
		UE_LOG(LogProjectCleaner, Error, TEXT("Failed to delete some files. Please check the Output Log for more information"));
	}

	if (bShowEditorNotification && GEditor) {
		if (NumFilesDeleted == NumFilesTotal) {
			ShowNotification(Msg, SNotificationItem::CS_Success, 3.0f);
		}
		else {
			ShowNotificationWithOutputLog(Msg, SNotificationItem::CS_Fail, 5.0f);
		}
	}

	return !bErrors;
}

void UPjcSubsystem::GetProjectRedirectors(TArray<FAssetData>& Redirectors) {
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	for (const auto& RootPath : GetProjectRootPaths()) {
		Filter.PackagePaths.Emplace(FName {*RootPath});
	}
	PjcShim::AddFilterClass(Filter, UObjectRedirector::StaticClass());

	Redirectors.Reset();
	IAssetRegistry& AssetRegistry = GetModuleAssetRegistry().Get();
	AssetRegistry.GetAssets(Filter, Redirectors);
	PjcShim::RemoveAllSwapArray(Redirectors, [](const FAssetData& Redirector) {
		return PathIsLeftOut(Redirector.PackagePath.ToString());
	});

	// a redirector next to a real asset (e.g. of a renamed Director Blueprint inside a Level Sequence) is part of that
	// asset: fixing cannot delete it, only resaving that asset does
	const FName RedirectorClass = PjcShim::GetClassName(UObjectRedirector::StaticClass());
	TMap<FName, bool> PackagesRedirectorOnly;
	TArray<FAssetData> PackageAssets;
	for (const auto& Redirector : Redirectors) {
		if (PackagesRedirectorOnly.Contains(Redirector.PackageName)) continue;

		PackageAssets.Reset();
		AssetRegistry.GetAssetsByPackageName(Redirector.PackageName, PackageAssets);
		const bool bHasOtherAsset = PackageAssets.ContainsByPredicate([&RedirectorClass](const FAssetData& Asset) {
			return PjcShim::GetAssetClassName(Asset) != RedirectorClass;
		});
		PackagesRedirectorOnly.Add(Redirector.PackageName, !bHasOtherAsset);
	}
	Redirectors.RemoveAll([&PackagesRedirectorOnly](const FAssetData& Redirector) {
		return !PackagesRedirectorOnly.FindRef(Redirector.PackageName);
	});
}

void UPjcSubsystem::GetRedirectorsStale(const TSet<FName>& PackagesDeleted, TArray<FAssetData>& Redirectors) {
	GetProjectRedirectors(Redirectors);

	// the package each redirector points to
	IAssetRegistry& AssetRegistry = GetModuleAssetRegistry().Get();
	TMap<FName, FName> TargetOf;
	TArray<FName> Links;
	for (const auto& Redirector : Redirectors) {
		Links.Reset();
		AssetRegistry.GetDependencies(Redirector.PackageName, Links);
		Links.Sort(FNameLexicalLess {});
		for (const FName Dependency : Links) {
			if (Dependency.ToString().StartsWith(TEXT("/Script/")) || Dependency == Redirector.PackageName) continue;

			TargetOf.Add(Redirector.PackageName, Dependency);
			break;
		}
	}

	// candidates: the chain ends at a deleted package (redirector to redirector to asset)
	TSet<FName> Stale;
	for (const auto& Pair : TargetOf) {
		FName End = Pair.Value;
		for (int32 Step = 0; Step < 64 && TargetOf.Contains(End); ++Step) {
			End = TargetOf[End];
		}
		if (PackagesDeleted.Contains(End)) {
			Stale.Add(Pair.Key);
		}
	}
	// a candidate something staying still points through stays, until nothing changes: only deleted packages and other
	// stale redirectors may use one (redirectors used only by unused assets, chains)
	for (bool bChanged = true; bChanged;) {
		bChanged = false;
		for (const FName Package : Stale.Array()) {
			Links.Reset();
			AssetRegistry.GetReferencers(Package, Links);
			const bool bUsed = Links.ContainsByPredicate([&](const FName Referencer) {
				return !PackagesDeleted.Contains(Referencer) && !Stale.Contains(Referencer);
			});
			if (bUsed) {
				Stale.Remove(Package);
				bChanged = true;
			}
		}
	}
	Redirectors.RemoveAll([&Stale](const FAssetData& Redirector) {
		return !Stale.Contains(Redirector.PackageName);
	});
}

void UPjcSubsystem::AddLocalizedRedirectors(TArray<FAssetData>& Redirectors) {
	// "/Game/L10N/fr/UI/Old" is the French copy of "/Game/UI/Old"
	TSet<FName> Sources;
	for (const auto& Redirector : Redirectors) {
		Sources.Add(Redirector.PackageName);
	}
	TArray<FAssetData> All;
	GetProjectRedirectors(All);
	for (const auto& Redirector : All) {
		const FString Path = Redirector.PackageName.ToString();
		const int32 L10N = Path.Find(TEXT("/L10N/"), ESearchCase::IgnoreCase);
		if (L10N == INDEX_NONE || Sources.Contains(Redirector.PackageName)) continue;

		const int32 CultureEnd = Path.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromStart, L10N + 6);
		if (CultureEnd == INDEX_NONE) continue;

		const FName Source {*(Path.Left(L10N) + Path.Mid(CultureEnd))};
		if (Sources.Contains(Source)) {
			Redirectors.Add(Redirector);
		}
	}
}

void UPjcSubsystem::GetRedirectorsNeededOutside(
	const TArray<FAssetData>& Redirectors,
	TMap<FName, FString>& OutNamedInFiles,
	TMap<FName, FString>& OutUsedOutside
) {
	OutNamedInFiles.Reset();
	OutUsedOutside.Reset();
	TSet<FName> Packages;
	for (const auto& Redirector : Redirectors) {
		Packages.Add(Redirector.PackageName);
	}
	if (Packages.Num() == 0) return;

	// files that name the old path: C++, Python, config
	{
		FPjcScanCacheScope ScanCacheScope;
		TArray<FAssetData> AssetsIndirect;
		TArray<FPjcAssetIndirectInfo> AssetsIndirectInfos;
		GetAssetsIndirect(AssetsIndirect, AssetsIndirectInfos, false);
		const FString ProjectDir = PathNormalize(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir())) + TEXT("/");
		for (const auto& Info : AssetsIndirectInfos) {
			if (!Packages.Contains(Info.Asset.PackageName) || OutNamedInFiles.Contains(Info.Asset.PackageName)) continue;

			FString File = PathNormalize(Info.FilePath);
			FPaths::MakePathRelativeTo(File, *ProjectDir);
			OutNamedInFiles.Add(Info.Asset.PackageName, FString::Printf(TEXT("Named in %s, line %d"), *File, Info.FileNum));
		}
	}

	// content the scan leaves out: engine, other plugins, Developers folders
	IAssetRegistry& AssetRegistry = GetModuleAssetRegistry().Get();
	TArray<FName> Referencers;
	for (const FName Package : Packages) {
		Referencers.Reset();
		AssetRegistry.GetReferencers(Package, Referencers);
		const FName* Outside = Referencers.FindByPredicate([&Packages](const FName Referencer) {
			return !Packages.Contains(Referencer) && !Referencer.ToString().StartsWith(TEXT("/Script/"))
				&& !PathIsInsideProject(Referencer.ToString());
		});
		if (Outside) {
			OutUsedOutside.Add(Package, FString::Printf(TEXT("Used by %s, which the scan leaves out"), *Outside->ToString()));
		}
	}
#if ENGINE_MAJOR_VERSION < 5
	// UE 4.27: a level's actor files are not in the asset registry, fixing cannot find and resave them
	TSet<FName> ActorFileDependencies;
	GetExternalActorsDependencies(ActorFileDependencies);
	for (const FName Package : Packages) {
		if (ActorFileDependencies.Contains(Package) && !OutUsedOutside.Contains(Package)) {
			OutUsedOutside.Add(Package, TEXT("Placed in a level by its actor files, which UE 4.27 cannot resave here"));
		}
	}
#endif
}

int32 UPjcSubsystem::AddCoreRedirects(const TArray<UObjectRedirector*>& Redirectors) {
	// the old package, the old object (when renamed) and a Blueprint's old class name resolve to the new ones
	TArray<FString> Lines;
	TArray<FCoreRedirect> Redirects;
	int32 NumRedirected = 0;
	for (const UObjectRedirector* Redirector : Redirectors) {
		const UObject* Destination = GetFinalDestination(Redirector);
		if (!Destination) continue;

		const FString OldPackage = Redirector->GetOutermost()->GetName();
		const FString NewPackage = Destination->GetOutermost()->GetName();
		const FString OldObject = Redirector->GetPathName();
		const FString NewObject = Destination->GetPathName();
		const int32 Before = Lines.Num();
		if (OldPackage != NewPackage) {
			Lines.Add(FString::Printf(TEXT("+PackageRedirects=(OldName=\"%s\",NewName=\"%s\")"), *OldPackage, *NewPackage));
			Redirects.Emplace(ECoreRedirectFlags::Type_Package, OldPackage, NewPackage);
		}
		if (OldObject != NewObject) {
			Lines.Add(FString::Printf(TEXT("+ObjectRedirects=(OldName=\"%s\",NewName=\"%s\")"), *OldObject, *NewObject));
			Redirects.Emplace(ECoreRedirectFlags::Type_Object, OldObject, NewObject);
			if (Cast<UBlueprint>(Destination)) {
				Lines.Add(FString::Printf(TEXT("+ClassRedirects=(OldName=\"%s_C\",NewName=\"%s_C\")"), *OldObject, *NewObject));
				Redirects.Emplace(ECoreRedirectFlags::Type_Class, OldObject + TEXT("_C"), NewObject + TEXT("_C"));
			}
		}
		NumRedirected += Lines.Num() > Before ? 1 : 0;
	}
	if (Lines.Num() == 0) return 0;

	// Config/DefaultEngine.ini, in its [CoreRedirects] section (made when missing), lines already there not again
	const FString File = FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir() / TEXT("DefaultEngine.ini"));
	if (ISourceControlModule::Get().IsEnabled()) {
		USourceControlHelpers::CheckOutOrAddFile(File);
	}
	if (IFileManager::Get().IsReadOnly(*File)) {
		FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*File, false);
	}
	TArray<FString> Text;
	FFileHelper::LoadFileToStringArray(Text, *File);
	Lines.RemoveAll([&Text](const FString& Line) {
		return Text.Contains(Line);
	});
	if (Lines.Num() > 0) {
		const int32 Section = Text.IndexOfByPredicate([](const FString& Line) {
			return Line.TrimStartAndEnd().Equals(TEXT("[CoreRedirects]"), ESearchCase::IgnoreCase);
		});
		if (Section == INDEX_NONE) {
			if (Text.Num() > 0 && !Text.Last().TrimStartAndEnd().IsEmpty()) {
				Text.Add(FString {});
			}
			Text.Add(TEXT("[CoreRedirects]"));
			Text.Append(Lines);
		}
		else {
			int32 End = Section + 1;
			while (End < Text.Num() && !Text[End].TrimStart().StartsWith(TEXT("["))) {
				++End;
			}
			while (End > Section + 1 && Text[End - 1].TrimStartAndEnd().IsEmpty()) {
				--End;
			}
			Text.Insert(Lines, End);
		}
		if (!FFileHelper::SaveStringArrayToFile(Text, *File)) {
			UE_LOG(LogProjectCleaner, Error, TEXT("Could not write redirects to %s"), *File);
			return 0;
		}
	}
	// this session too: the old paths resolve without a restart
	FCoreRedirects::AddRedirectList(Redirects, TEXT("ProjectCleaner"));
	UE_LOG(LogProjectCleaner, Display, TEXT("Added redirects for %d redirector(s) to %s"), NumRedirected, *File);
	return NumRedirected;
}

FString UPjcSubsystem::RepointRedirector(const FAssetData& Redirector, const FAssetData& Target) {
	UObjectRedirector* Object = Cast<UObjectRedirector>(Redirector.GetAsset());
	if (!Object) return TEXT("The redirector could not be loaded");
	UObject* TargetObject = Target.GetAsset();
	if (!TargetObject || Cast<UObjectRedirector>(TargetObject)) return TEXT("The asset could not be loaded");

	UPackage* Package = Object->GetOutermost();
	FString Filename;
	if (!PjcShim::DoesPackageExist(Package->GetName(), &Filename)) return TEXT("The redirector has no file");
	Filename = FPaths::ConvertRelativePathToFull(Filename);
	if (ISourceControlModule::Get().IsEnabled()) {
		FEditorFileUtils::CheckoutPackages({Package}, nullptr, false);
	}
	if (IFileManager::Get().IsReadOnly(*Filename)) return TEXT("Its file is read-only (not checked out)");

	Object->DestinationObject = TargetObject;
	Package->MarkPackageDirty();
	if (!UEditorLoadingAndSavingUtils::SavePackages({Package}, false)) return TEXT("Saving it failed, see the Output Log");
	IAssetRegistry& AssetRegistry = GetModuleAssetRegistry().Get();
	AssetRegistry.ScanFilesSynchronous({Filename}, true);
	PjcShim::RegisterRedirection(Object, TargetObject->GetPathName());

	// loaded assets that use it hold the empty reference they loaded: loaded again, they find the new asset
	TArray<FName> Referencers;
	AssetRegistry.GetReferencers(Package->GetFName(), Referencers);
	TArray<UPackage*> Loaded;
	for (const FName Referencer : Referencers) {
		if (UPackage* User = FindPackage(nullptr, *Referencer.ToString())) {
			Loaded.Add(User);
		}
	}
	FText Error;
	if (Loaded.Num() > 0 && !UPackageTools::ReloadPackages(Loaded, Error, EReloadPackagesInteractionMode::AssumePositive)) {
		return Error.IsEmpty() ? FString {TEXT("The assets that use it could not be loaded again")} : Error.ToString();
	}
	return FString {};
}

UClass* UPjcSubsystem::GetRedirectorTargetClass(const FAssetData& Redirector) {
	// the import of its target keeps the class the target had
	const UObject* Object = Redirector.GetAsset();
	FLinkerLoad* Linker = Object ? FLinkerLoad::FindExistingLinkerForPackage(Object->GetOutermost()) : nullptr;
	if (!Linker) return nullptr;

	for (const FObjectImport& Import : Linker->ImportMap) {
		if (!Import.OuterIndex.IsImport()) continue;

		const FObjectImport& Outer = Linker->Imp(Import.OuterIndex);
		if (Outer.ClassName != NAME_Package || Outer.ObjectName.ToString().StartsWith(TEXT("/Script/"))) continue;

		const FString ClassPath = Import.ClassPackage.ToString() + TEXT(".") + Import.ClassName.ToString();
		if (UClass* Class = FSoftClassPath {ClassPath}.ResolveClass()) return Class;
	}
	return nullptr;
}

FString UPjcSubsystem::GetRedirectorDeleteBlock(const FName Package, FString& OutHint) {
	if (!ISourceControlModule::Get().IsEnabled()) return FString {};

	FString Filename;
	if (!PjcShim::DoesPackageExist(Package.ToString(), &Filename)) return FString {};

	ISourceControlProvider& Provider = ISourceControlModule::Get().GetProvider();
	const FSourceControlStatePtr State = Provider.GetState(FPaths::ConvertRelativePathToFull(Filename), EStateCacheUsage::ForceUpdate);
	if (!State.IsValid()) return FString {};
	if (State->IsCheckedOutOther()) {
		OutHint = TEXT("Fix it when they are done");
		return TEXT("Checked out by someone else");
	}
	if (State->IsSourceControlled() && !State->IsCurrent()) {
		OutHint = TEXT("Get the latest first");
		return TEXT("Not at the latest revision");
	}
	return FString {};
}

FString UPjcSubsystem::GetCaseMismatch(const FName Package) {
	// the file is there, but its name differs in letter case from the package (a case-only rename): it does not load
	const FString Expected = FPackageName::LongPackageNameToFilename(Package.ToString(), FPackageName::GetAssetPackageExtension());
	const FString Directory = FPaths::GetPath(Expected);
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *Directory, *FPackageName::GetAssetPackageExtension());
	const FString Wanted = FPaths::GetCleanFilename(Expected);
	for (const auto& File : Files) {
		if (File.Equals(Wanted, ESearchCase::IgnoreCase) && !File.Equals(Wanted, ESearchCase::CaseSensitive)) {
			return FString::Printf(TEXT("Its file is named %s on disk"), *File);
		}
	}
	return FString {};
}

void UPjcSubsystem::GetPackagesRedirector(TSet<FName>& Packages) {
	TArray<FAssetData> Redirectors;
	GetProjectRedirectors(Redirectors);

	Packages.Reset();
	for (const auto& Redirector : Redirectors) {
		Packages.Add(Redirector.PackageName);
	}
}

bool UPjcSubsystem::ProjectHasRedirectors() {
	TArray<FAssetData> Redirectors;
	GetProjectRedirectors(Redirectors);

	return Redirectors.Num() > 0;
}

void UPjcSubsystem::FixProjectRedirectors(const TArray<FAssetData>& Redirectors, const bool bShowSlowTask) {
	if (Redirectors.Num() == 0) return;

	// same fix as Issues: keeps what files or content outside the scan still need
	const FPjcRedirectorFixResult Result = FixRedirectorsWithReport(Redirectors, bShowSlowTask);
	for (const auto& Pair : Result.Kept) {
		const FString* Hint = Result.KeptHints.Find(Pair.Key);
		UE_LOG(LogProjectCleaner, Warning, TEXT("Redirector %s kept: %s. %s"), *Pair.Key.ToString(), *Pair.Value, Hint ? **Hint : TEXT(""));
	}
}

FPjcRedirectorFixResult
UPjcSubsystem::FixRedirectorsWithReport(const TArray<FAssetData>& InRedirectors, const bool bShowSlowTask, const FPjcRedirectorFixOptions& Options) {
	FPjcRedirectorFixResult Result;
	IAssetRegistry& AssetRegistry = GetModuleAssetRegistry().Get();
	const auto Keep = [&Result](const FName Package, const FString& Why, const FString& Hint) {
		Result.Kept.Add(Package, Why);
		Result.KeptHints.Add(Package, Hint);
	};

	// every referencer must be known before a redirector goes
	if (AssetRegistry.IsLoadingAssets()) {
		for (const auto& Redirector : InRedirectors) {
			Keep(Redirector.PackageName, TEXT("The asset registry is still loading"), TEXT("Fix again when it is done"));
		}
		return Result;
	}

	// localized copies (L10N) go with their redirector
	TArray<FAssetData> Redirectors = InRedirectors;
	AddLocalizedRedirectors(Redirectors);

	// what still needs the old path stays: files that name it, unless a redirect keeps it working, and
	// content the scan leaves out, which fixing cannot resave
	TSet<FName> Redirected;
	if (Options.bCheckNeeded) {
		TMap<FName, FString> NamedInFiles;
		TMap<FName, FString> UsedOutside;
		GetRedirectorsNeededOutside(Redirectors, NamedInFiles, UsedOutside);
		Redirectors.RemoveAll([&](const FAssetData& Redirector) {
			if (const FString* Why = UsedOutside.Find(Redirector.PackageName)) {
				Keep(Redirector.PackageName, *Why, TEXT("Fixing cannot resave that content: keep it, or scan that content too"));
				return true;
			}
			if (const FString* Why = NamedInFiles.Find(Redirector.PackageName)) {
				if (Options.bAddRedirects) {
					Redirected.Add(Redirector.PackageName);
					return false;
				}
				Keep(Redirector.PackageName, *Why, TEXT("Deleting it breaks that file: fix it with the redirect option on"));
				return true;
			}
			return false;
		});
	}

	FScopedSlowTask SlowTask {3.0f, FText::FromString(TEXT("Fixing redirectors...")), bShowSlowTask && GIsEditor && !IsRunningCommandlet()};
	SlowTask.MakeDialog(false, false);
	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Loading redirectors")));

	TArray<UObjectRedirector*> RedirectorObjects;
	for (const auto& Redirector : Redirectors) {
		if (UObjectRedirector* RedirectorObject = Cast<UObjectRedirector>(Redirector.GetAsset())) {
			RedirectorObjects.Add(RedirectorObject);
		}
		else {
			const FString CaseMismatch = GetCaseMismatch(Redirector.PackageName);
			if (CaseMismatch.IsEmpty()) {
				Keep(Redirector.PackageName, TEXT("Could not be loaded"), TEXT("See the Output Log"));
			}
			else {
				Keep(Redirector.PackageName, CaseMismatch, TEXT("Rename the asset back, or the file to match"));
			}
		}
	}

	// old paths that files name keep working through a redirect (Config/DefaultEngine.ini)
	if (Redirected.Num() > 0) {
		const TArray<UObjectRedirector*> ToRedirect = RedirectorObjects.FilterByPredicate([&Redirected](const UObjectRedirector* Object) {
			return Redirected.Contains(Object->GetOutermost()->GetFName());
		});
		Result.NumRedirects = AddCoreRedirects(ToRedirect);
	}

	// resaving rewrites soft references through registered redirections (not always registered before 5.5), and soft paths
	// of a Blueprint's class and defaults (_C, Default__) like the engine's fixup
	TSet<FName> RedirectorPackageNames;
	TMap<FSoftObjectPath, FSoftObjectPath> SoftPaths;
	for (const auto& RedirectorObject : RedirectorObjects) {
		RedirectorPackageNames.Add(RedirectorObject->GetOutermost()->GetFName());
		const UObject* Destination = GetFinalDestination(RedirectorObject);
		if (!Destination) {
			if (Options.bClearBroken) {
				SoftPaths.Add(FSoftObjectPath {RedirectorObject}, FSoftObjectPath {});
			}
			continue;
		}

		PjcShim::RegisterRedirection(RedirectorObject, Destination->GetPathName());
		const FSoftObjectPath OldPath {RedirectorObject};
		const FSoftObjectPath NewPath {Destination};
		SoftPaths.Add(OldPath, NewPath);
		if (Cast<UBlueprint>(Destination)) {
			SoftPaths.Add(FSoftObjectPath {OldPath.ToString() + TEXT("_C")}, FSoftObjectPath {NewPath.ToString() + TEXT("_C")});
			const FString OldDefault = FString::Printf(TEXT("%s.Default__%s_C"), *OldPath.GetLongPackageName(), *OldPath.GetAssetName());
			const FString NewDefault = FString::Printf(TEXT("%s.Default__%s_C"), *NewPath.GetLongPackageName(), *NewPath.GetAssetName());
			SoftPaths.Add(FSoftObjectPath {OldDefault}, FSoftObjectPath {NewDefault});
		}
	}

	// redirector to missing asset: resaving its referencers replaces the broken reference with null, both stay unless clearing
	TSet<FName> ReferencersToSkip;
	for (const auto& RedirectorObject : RedirectorObjects) {
		if (Options.bClearBroken || GetFinalDestination(RedirectorObject)) continue;

		TArray<FName> Referencers;
		AssetRegistry.GetReferencers(RedirectorObject->GetOutermost()->GetFName(), Referencers);
		ReferencersToSkip.Append(Referencers);
		UE_LOG(LogProjectCleaner, Warning, TEXT("Redirector %s points to missing asset, not fixed"), *RedirectorObject->GetPathName());
	}

	// referencers of the scan only: content it leaves out is never edited
	TSet<FName> ReferencerNames;
	for (const FName& PackageName : RedirectorPackageNames) {
		TArray<FName> Referencers;
		AssetRegistry.GetReferencers(PackageName, Referencers);
		for (const FName& Referencer : Referencers) {
			const bool bInside = PathIsInsideProject(Referencer.ToString());
			if (bInside && !RedirectorPackageNames.Contains(Referencer) && !ReferencersToSkip.Contains(Referencer)) {
				ReferencerNames.Add(Referencer);
			}
		}
	}

	// loading referencers resolves redirectors (loaded redirectors also fix soft references on save)
	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Resaving the assets that use them")));
	TArray<UPackage*> PackagesToSave;
	TArray<FString> PackageFiles;
	int32 NumLoaded = 0;
	TSet<FString> Worlds;
	for (const FName& ReferencerName : ReferencerNames) {
		FString Filename;
		if (!PjcShim::DoesPackageExist(ReferencerName.ToString(), &Filename)) continue;

		UPackage* Package = FindPackage(nullptr, *ReferencerName.ToString());
		if (!Package) {
			Package = LoadPackage(nullptr, *ReferencerName.ToString(), LOAD_None);
			NumLoaded += Package ? 1 : 0;
		}
		if (!Package) {
			Result.NotResaved.Add(ReferencerName, TEXT("could not be loaded"));
			continue;
		}
		if (Package->HasAnyPackageFlags(PKG_CompiledIn)) continue;

		// a redirector that points to one of these (a chain) points to the real asset from now on
		UObjectRedirector* Link = FindObject<UObjectRedirector>(Package, *FPackageName::GetShortName(ReferencerName));
		UObject* LinkFinal = Link ? GetFinalDestination(Link) : nullptr;
		if (LinkFinal && Link->DestinationObject != LinkFinal) {
			Link->DestinationObject = LinkFinal;
			Package->MarkPackageDirty();
		}

		PackagesToSave.Add(Package);
		PackageFiles.Add(FPaths::ConvertRelativePathToFull(Filename));
		// a level's actor files and the level itself: its loaders are reset before saving (the engine does it too)
		const FName Owner = FolderIsExternal(ReferencerName.ToString()) ? GetExternalOwner(ReferencerName) : NAME_None;
		if (!Owner.IsNone()) {
			Worlds.Add(Owner.ToString());
		}
	}
	for (const auto& World : Worlds) {
		PjcShim::ResetLoadersForWorld(World);
	}
	if (PackagesToSave.Num() > 0 && SoftPaths.Num() > 0) {
		GetModuleAssetTools().Get().RenameReferencingSoftObjectPaths(PackagesToSave, SoftPaths);
	}

	if (PackagesToSave.Num() > 0 && ISourceControlModule::Get().IsEnabled()) {
		FEditorFileUtils::CheckoutPackages(PackagesToSave, nullptr, false);
	}
	// one by one, so a failure names its asset
	TArray<FString> FilesToRescan;
	for (int32 Index = 0; Index < PackagesToSave.Num(); ++Index) {
		UPackage* Package = PackagesToSave[Index];
		if (IFileManager::Get().IsReadOnly(*PackageFiles[Index])) {
			Result.NotResaved.Add(Package->GetFName(), TEXT("file is read-only (not checked out)"));
			continue;
		}
		if (!UEditorLoadingAndSavingUtils::SavePackages({Package}, false)) {
			Result.NotResaved.Add(Package->GetFName(), TEXT("save failed, see Output Log"));
			continue;
		}
		++Result.NumResaved;
		FilesToRescan.Add(PackageFiles[Index]);
	}
	if (FilesToRescan.Num() > 0) {
		AssetRegistry.ScanFilesSynchronous(FilesToRescan, true);
	}

	// delete only redirectors nothing points to anymore (other redirectors do not count)
	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Deleting fixed redirectors")));
	TArray<UObject*> ObjectsToDelete;
	TArray<FName> PackagesToDelete;
	for (const auto& RedirectorObject : RedirectorObjects) {
		const FName PackageName = RedirectorObject->GetOutermost()->GetFName();
		TArray<FName> Referencers;
		AssetRegistry.GetReferencers(PackageName, Referencers);
		const FName* User = Referencers.FindByPredicate([&](const FName& Referencer) {
			return !RedirectorPackageNames.Contains(Referencer);
		});

		// a redirector to a missing asset goes when nothing uses it; assets that use it need a repair
		FString BlockHint;
		const FString Block = User ? FString {} : GetRedirectorDeleteBlock(PackageName, BlockHint);
		const FString UserName = User ? FPackageName::GetShortName(*User) : FString {};
		const FString* NotResaved = User ? Result.NotResaved.Find(*User) : nullptr;
		if (User && !GetFinalDestination(RedirectorObject) && !Options.bClearBroken) {
			const FString Missing = FString::Printf(TEXT("Its target is missing and %s uses it"), *UserName);
			Keep(PackageName, Missing, TEXT("Repair it: point it to another asset, or clear the references"));
		}
		else if (NotResaved) {
			const bool bReadOnly = NotResaved->Contains(TEXT("read-only"));
			const FString Why = FString::Printf(TEXT("%s was not resaved: %s"), *UserName, **NotResaved);
			Keep(PackageName, Why, bReadOnly ? TEXT("Check it out, then fix again") : TEXT("See the Output Log"));
		}
		else if (User) {
			Keep(PackageName, UserName + TEXT(" still uses it"), TEXT("Open it and check the reference"));
		}
		else if (!Block.IsEmpty()) {
			Keep(PackageName, Block, BlockHint);
		}
		else {
			ObjectsToDelete.Add(RedirectorObject);
			PackagesToDelete.Add(PackageName);
		}
	}

	if (ObjectsToDelete.Num() > 0) {
		// target path: its destination, or for a missing one the package it was saved with
		for (UObject* Object : ObjectsToDelete) {
			const UObjectRedirector* RedirectorObject = Cast<UObjectRedirector>(Object);
			const UObject* Destination = GetFinalDestination(RedirectorObject);
			FString TargetPath = Destination ? Destination->GetPathName() : FString {};
			if (TargetPath.IsEmpty()) {
				TArray<FName> Dependencies;
				AssetRegistry.GetDependencies(Object->GetOutermost()->GetFName(), Dependencies);
				for (const FName Dependency : Dependencies) {
					if (Dependency.ToString().StartsWith(TEXT("/Script/"))) continue;

					TargetPath = Dependency.ToString() + TEXT(".") + FPackageName::GetShortName(Dependency);
					break;
				}
			}
			PjcShim::RegisterRedirection(Object, TargetPath);
			// collections that list the redirector list its target instead
			if (!PjcShim::HandleCollectionRedirectorDeleted(Object)) {
				UE_LOG(LogProjectCleaner, Warning, TEXT("A collection listing %s could not be saved"), *Object->GetPathName());
			}
		}
		ObjectTools::DeleteObjects(ObjectsToDelete, false);
	}
	// gone from disk and from the asset registry (a package can stay in memory)
	TArray<FAssetData> Left;
	for (const FName& PackageName : PackagesToDelete) {
		Left.Reset();
		AssetRegistry.GetAssetsByPackageName(PackageName, Left);
		if (PjcShim::DoesPackageExist(PackageName.ToString(), nullptr) || Left.Num() > 0) {
			Keep(PackageName, TEXT("Could not be deleted"), TEXT("See the Output Log"));
		}
		else {
			Result.Deleted.Add(PackageName);
		}
	}

	// folders that held only the deleted redirectors
	if (Options.bDeleteEmptyFolders && Result.Deleted.Num() > 0) {
		TSet<FString> Folders;
		for (const FName& PackageName : Result.Deleted) {
			Folders.Add(FPackageName::GetLongPackagePath(PackageName.ToString()));
		}
		TArray<FString> Empty;
		for (const auto& Folder : Folders) {
			if (FolderIsEmpty(Folder)) {
				Empty.Add(PathConvertToAbsolute(Folder));
			}
		}
		Result.NumFoldersDeleted = Empty.Num() > 0 ? DeleteFolders(Empty) : 0;
	}

	// what fixing loaded leaves memory
	if (NumLoaded > 0) {
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	}
	return Result;
}

bool UPjcSubsystem::EditorIsInPlayMode() {
	return (GEditor && GEditor->PlayWorld) || GIsPlayInEditorWorld;
}

bool UPjcSubsystem::AssetIsBlueprint(const FAssetData& InAsset) {
	if (!InAsset.IsValid()) return false;

	const UClass* AssetClass = InAsset.GetClass();
	if (!AssetClass) return false;

	return AssetClass->IsChildOf(UBlueprint::StaticClass());
}

bool UPjcSubsystem::AssetIsEngineReferenced(const FAssetData& InAsset) {
	if (!InAsset.IsValid()) return false;

	TArray<FName> Refs;
	GetModuleAssetRegistry().Get().GetReferencers(InAsset.PackageName, Refs);

	return Refs.ContainsByPredicate([](const FName& Ref) {
		return !PathIsInsideProject(Ref.ToString()) || FolderIsExternal(Ref.ToString());
	});
}

bool UPjcSubsystem::AssetIsLoadedByTag(const FAssetData& InAsset) {
	// GameplayAbilities: notifies found in GameplayCueNotifyPaths are loaded by their cue tag, nothing references them.
	// Tag is written for every GameplayCueNotify_Static and GameplayCueNotify_Actor (blueprints too)
	static const FName GameplayCueNameTag {TEXT("GameplayCueName")};

	FString GameplayCueName;
	return InAsset.GetTagValue(GameplayCueNameTag, GameplayCueName) && !GameplayCueName.IsEmpty() && GameplayCueName != TEXT("None");
}

bool UPjcSubsystem::AssetIsCircular(const FAssetData& InAsset) {
	if (!InAsset.IsValid()) return false;

	TArray<FName> Refs;
	TArray<FName> Deps;

	GetModuleAssetRegistry().Get().GetReferencers(InAsset.PackageName, Refs);
	GetModuleAssetRegistry().Get().GetDependencies(InAsset.PackageName, Deps);

	for (const auto& Ref : Refs) {
		if (Deps.Contains(Ref)) {
			return true;
		}
	}

	return false;
}

FString UPjcSubsystem::PathNormalize(const FString& InPath) {
	if (InPath.IsEmpty()) return {};

	// Ensure the path don't start with a slash or a disk drive letter
	if (!(InPath.StartsWith(TEXT("/")) || InPath.StartsWith(TEXT("\\")) || (InPath.Len() > 2 && InPath[1] == ':'))) {
		return {};
	}

	// backslashes first: on Linux a path starting with one "\" counts as relative, and the engine dir gets prepended
	FString Path = FPaths::ConvertRelativePathToFull(InPath.Replace(TEXT("\\"), TEXT("/"))).TrimStartAndEnd();
	FPaths::RemoveDuplicateSlashes(Path);

	// Collapse any ".." or "." references in the path
	FPaths::CollapseRelativeDirectories(Path);

	if (FPaths::GetExtension(Path).IsEmpty()) {
		FPaths::NormalizeDirectoryName(Path);
	}
	else {
		FPaths::NormalizeFilename(Path);
	}

	// Ensure the path does not end with a trailing slash
	if (Path.EndsWith(TEXT("/")) || Path.EndsWith(TEXT("\\"))) {
		Path = Path.LeftChop(1);
	}

	return Path;
}

FString UPjcSubsystem::PathConvertToAbsolute(const FString& InPath) {
	const FString PathNormalized = PathNormalize(InPath);

	if (PathNormalized.IsEmpty()) return {};

	// trailing slash makes mount roots like "/Game" convertible too.
	// result is relative to engine binaries when project and engine are on same drive, so make it full.
	FString AbsolutePath;
	if (FPackageName::TryConvertLongPackageNameToFilename(PathNormalized + TEXT("/"), AbsolutePath)) {
		return PathNormalize(FPaths::ConvertRelativePathToFull(AbsolutePath));
	}

	return PathNormalized;
}

FString UPjcSubsystem::PathConvertToRelative(const FString& InPath) {
	const FString PathNormalized = PathNormalize(InPath);

	if (PathNormalized.IsEmpty()) return {};

	// files convert as is, folders (including content root itself) need trailing slash
	FString RelativePath;
	bool bConverted = FPackageName::TryConvertFilenameToLongPackageName(PathNormalized, RelativePath);
	if (!bConverted) bConverted = FPackageName::TryConvertFilenameToLongPackageName(PathNormalized + TEXT("/"), RelativePath);
	if (bConverted) {
		return PathNormalize(RelativePath);
	}

	return PathNormalized;
}

FString UPjcSubsystem::PathConvertToObjectPath(const FString& InPath) {
	if (FPaths::FileExists(InPath)) {
		const FString FileName = FPaths::GetBaseFilename(InPath);
		const FString AssetPath = PathConvertToRelative(FPaths::GetPath(InPath));

		return FString::Printf(TEXT("%s/%s.%s"), *AssetPath, *FileName, *FileName);
	}

	FString ObjectPath = FPackageName::ExportTextPathToObjectPath(InPath);
	ObjectPath.RemoveFromEnd(TEXT("_C"));	// we should remove _C prefix if its blueprint asset

	if (!PathIsInsideProject(ObjectPath)) return {};

	TArray<FString> Parts;
	ObjectPath.ParseIntoArray(Parts, TEXT("/"), true);

	if (Parts.Num() > 0) {
		FString Left;
		FString Right;
		Parts.Last().Split(TEXT("."), &Left, &Right);

		if (!Left.IsEmpty() && !Right.IsEmpty() && Left.Equals(*Right)) {
			return ObjectPath;
		}
	}

	return {};
}

int64 UPjcSubsystem::GetAssetSize(const FAssetData& InAsset) {
	if (!InAsset.IsValid()) return 0;

	const FAssetPackageData AssetPackageData = PjcShim::GetAssetPackageData(InAsset.PackageName);

	return AssetPackageData.DiskSize;
}

int64 UPjcSubsystem::GetAssetsTotalSize(const TArray<FAssetData>& InAssets) {
	int64 Size = 0;

	for (const auto& Asset : InAssets) {
		if (!Asset.IsValid()) continue;

		const FAssetPackageData AssetPackageData = PjcShim::GetAssetPackageData(Asset.PackageName);
		Size += AssetPackageData.DiskSize;
	}

	return Size;
}

int64 UPjcSubsystem::GetFileSize(const FString& InFile) {
	if (InFile.IsEmpty() || !FPaths::FileExists(InFile)) return 0;

	return IFileManager::Get().FileSize(*InFile);
}

int64 UPjcSubsystem::GetFilesTotalSize(const TArray<FString>& Files) {
	int64 Size = 0;

	for (const auto& File : Files) {
		if (File.IsEmpty() || !FPaths::FileExists(File)) continue;

		Size += IFileManager::Get().FileSize(*File);
	}

	return Size;
}

FName UPjcSubsystem::GetAssetExactClassName(const FAssetData& InAsset) {
	if (!InAsset.IsValid()) return NAME_None;

	if (AssetIsBlueprint(InAsset)) {
		const FString GeneratedClassName = InAsset.TagsAndValues.FindTag(TEXT("GeneratedClass")).GetValue();
		const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(*GeneratedClassName);
		return PjcShim::GetClassNameFromObjectPath(ClassObjectPath);
	}

	return PjcShim::GetAssetClassName(InAsset);
}

TArray<FString> UPjcSubsystem::GetProjectRootPaths() {
	TArray<FString> Paths;
	Paths.Add(TEXT("/Game"));

	const UPjcScanSettings* ScanSettings = GetDefault<UPjcScanSettings>();
	if (!ScanSettings->bScanProjectPlugins) return Paths;

	for (const auto& Plugin : IPluginManager::Get().GetEnabledPluginsWithContent()) {
		if (Plugin->GetLoadedFrom() != EPluginLoadedFrom::Project) continue;
		if (Plugin->GetName().Equals(PjcConstants::ModulePjcName.ToString())) continue;
		if (ScanSettings->IgnoredPlugins.Contains(Plugin->GetName())) continue;

		FString MountedPath = Plugin->GetMountedAssetPath();
		MountedPath.RemoveFromEnd(TEXT("/"));
		if (!MountedPath.IsEmpty()) {
			Paths.AddUnique(MountedPath);
		}
	}

	return Paths;
}

TArray<FString> UPjcSubsystem::GetProjectContentDirs() {
	TArray<FString> Dirs;
	for (const auto& RootPath : GetProjectRootPaths()) {
		const FString Dir = PathConvertToAbsolute(RootPath);
		if (!Dir.IsEmpty() && FPaths::DirectoryExists(Dir)) {
			Dirs.Add(Dir);
		}
	}
	return Dirs;
}

bool UPjcSubsystem::FileIsExcluded(const FString& InFile) {
	if (FileIsUsedByProject(InFile)) return true;

	const UPjcFileExcludeSettings* FileExcludeSettings = GetDefault<UPjcFileExcludeSettings>();

	const FString File = PathNormalize(InFile);
	const bool bInExcludedFolder = FileExcludeSettings->ExcludedFolders.ContainsByPredicate([&File](const FDirectoryPath& Folder) {
		const FString FolderPath = Folder.Path.TrimStartAndEnd();
		if (FolderPath.IsEmpty()) return false;

		const FString FolderAbs = PathNormalize(PathConvertToAbsolute(FolderPath));
		return !FolderAbs.IsEmpty() && File.StartsWith(FolderAbs + TEXT("/"));
	});
	if (bInExcludedFolder) return true;

	const FString FileExt = FPaths::GetExtension(InFile, false);
	const bool bExcludedByExt = FileExcludeSettings->ExcludedExtensions.ContainsByPredicate([&](const FString& ExcludedExt) {
		FString Ext = ExcludedExt.TrimStartAndEnd();
		Ext.RemoveFromStart(TEXT("."));
		return Ext.Equals(FileExt, ESearchCase::IgnoreCase);
	});
	if (bExcludedByExt) return true;

	return FileExcludeSettings->ExcludedFiles.ContainsByPredicate([&](const FFilePath& InFilePath) {
		if (InFilePath.FilePath.IsEmpty()) return false;

		// picker stores paths relative to project dir, absolute ones are accepted too
		const FString Path = FPaths::IsRelative(InFilePath.FilePath) ? FPaths::ProjectDir() / InFilePath.FilePath : InFilePath.FilePath;
		const FString PathAbs = PathNormalize(FPaths::ConvertRelativePathToFull(Path));
		return !PathAbs.IsEmpty() && FPaths::IsSamePath(PathAbs, InFile);
	});
}

FString UPjcSubsystem::GetFileKeptReason(const FString& InFile) {
	if (!FileIsExcluded(InFile)) return FString {};

	const FString File = PathNormalize(InFile);
	for (const auto& ContentDir : GetProjectContentDirs()) {
		for (const TCHAR* FolderName : {TEXT("Localization"), TEXT("Movies"), TEXT("Splash"), TEXT("Python"), TEXT("Collections")}) {
			if (File.StartsWith(PathNormalize(ContentDir / FolderName) + TEXT("/"))) {
				return FString::Printf(TEXT("Engine folder (%s)"), FolderName);
			}
		}
		const FString DevelopersDir = PathNormalize(ContentDir / TEXT("Developers")) + TEXT("/");
		if (File.StartsWith(DevelopersDir) && File.RightChop(DevelopersDir.Len()).Contains(TEXT("/Collections/"))) {
			return TEXT("Engine folder (Collections)");
		}
	}
	if (!FileIsUsedByProject(InFile)) return TEXT("Excluded in settings");

	// used by the project: staged as is, or named in source or config
	TArray<FString> Folders;
	GetFoldersUsedByProject(Folders);
	const bool bStaged = Folders.ContainsByPredicate([&File](const FString& Folder) {
		return File.StartsWith(Folder + TEXT("/"));
	});
	return bStaged ? TEXT("Staged folder") : TEXT("Named in source or config");
}

bool UPjcSubsystem::FileIsUsedByProject(const FString& InFile) {
	const FString File = PathNormalize(InFile);
	if (File.IsEmpty()) return false;

	// called per file, rules come from scan cache when there is one (no copies of big word set)
	TArray<FString> FoldersLocal;
	TSet<FString> WordsLocal;
	if (ScanCache.Depth > 0 && !ScanCache.FoldersUsedByProject.IsSet()) {
		ScanCache.FoldersUsedByProject.Emplace();
		GetFoldersUsedByProject(ScanCache.FoldersUsedByProject.GetValue());
	}
	if (ScanCache.Depth == 0) {
		GetFoldersUsedByProject(FoldersLocal);
	}
	const TArray<FString>& Folders = ScanCache.Depth > 0 ? ScanCache.FoldersUsedByProject.GetValue() : FoldersLocal;

	for (const auto& Folder : Folders) {
		if (File.StartsWith(Folder + TEXT("/"))) return true;
	}

	// Developers/<User>/Collections
	for (const auto& ContentDir : GetProjectContentDirs()) {
		const FString DevelopersDir = PathNormalize(ContentDir / TEXT("Developers")) + TEXT("/");
		if (File.StartsWith(DevelopersDir) && File.RightChop(DevelopersDir.Len()).Contains(TEXT("/Collections/"))) return true;
	}

	// loaded by code or config by file name, e.g. FPaths::ProjectContentDir() / "Slate/Icon.svg"
	if (ScanCache.Depth > 0 && !ScanCache.SourceAndConfigWords.IsSet()) {
		ScanCache.SourceAndConfigWords.Emplace();
		GetSourceAndConfigWords(ScanCache.SourceAndConfigWords.GetValue());
	}
	if (ScanCache.Depth == 0) {
		GetSourceAndConfigWords(WordsLocal);
	}
	const TSet<FString>& Words = ScanCache.Depth > 0 ? ScanCache.SourceAndConfigWords.GetValue() : WordsLocal;

	return Words.Contains(FPaths::GetBaseFilename(File));
}

void UPjcSubsystem::GetFoldersUsedByProject(TArray<FString>& Folders) {
	Folders.Reset();

	// read by engine from every content root
	for (const auto& ContentDir : GetProjectContentDirs()) {
		for (const TCHAR* FolderName : {TEXT("Localization"), TEXT("Movies"), TEXT("Splash"), TEXT("Python"), TEXT("Collections")}) {
			Folders.Add(PathNormalize(ContentDir / FolderName));
		}
	}

	// staged as is with packaged builds, paths are relative to project Content folder
	const UProjectPackagingSettings* PackagingSettings = GetDefault<UProjectPackagingSettings>();
	TArray<FDirectoryPath> StagedDirs;
	StagedDirs.Append(PackagingSettings->DirectoriesToAlwaysStageAsUFS);
	StagedDirs.Append(PackagingSettings->DirectoriesToAlwaysStageAsNonUFS);
	StagedDirs.Append(PackagingSettings->DirectoriesToAlwaysStageAsUFSServer);
	StagedDirs.Append(PackagingSettings->DirectoriesToAlwaysStageAsNonUFSServer);

	for (const auto& StagedDir : StagedDirs) {
		FString Path = StagedDir.Path.TrimStartAndEnd();
		if (Path.IsEmpty()) continue;

		Path.RemoveFromStart(TEXT("/Game/"));
		Folders.Add(PathNormalize(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / Path)));
	}

	Folders.RemoveAll([](const FString& Folder) {
		return Folder.IsEmpty();
	});
}

void UPjcSubsystem::GetSourceAndConfigWords(TSet<FString>& Words) {
	Words.Reset();

	TSet<FString> Files;
	GetSourceAndConfigFiles(Files);

	FString Content;
	for (const auto& File : Files) {
		Content.Reset();
		if (!FFileHelper::LoadFileToString(Content, *File)) continue;

		int32 Start = INDEX_NONE;
		for (int32 i = 0; i <= Content.Len(); ++i) {
			const bool bWordChar = i < Content.Len() && (FChar::IsAlnum(Content[i]) || Content[i] == TCHAR('_') || Content[i] == TCHAR('-'));
			if (bWordChar && Start == INDEX_NONE) {
				Start = i;
			}
			else if (!bWordChar && Start != INDEX_NONE) {
				Words.Add(Content.Mid(Start, i - Start));
				Start = INDEX_NONE;
			}
		}
	}
}

void UPjcSubsystem::GetSourceAndConfigTagNames(TSet<FName>& Names) {
	Names.Reset();

	TSet<FString> Files;
	GetSourceAndConfigFiles(Files);

	// sections that define gameplay tags: a definition does not use the tag
	const TCHAR* TagSettings = TEXT("[/Script/GameplayTags.GameplayTagsSettings]");
	const TCHAR* TagList = TEXT("[/Script/GameplayTags.GameplayTagsList]");

	FString Content;
	TArray<FString> Lines;
	for (const auto& File : Files) {
		Content.Reset();
		if (!FFileHelper::LoadFileToString(Content, *File)) continue;

		const bool bConfig = FPaths::GetExtension(File).Equals(TEXT("ini"), ESearchCase::IgnoreCase);
		bool bTagDefinitions = false;
		Lines.Reset();
		Content.ParseIntoArrayLines(Lines, false);
		for (const auto& Line : Lines) {
			if (bConfig) {
				const FString Trimmed = Line.TrimStartAndEnd();
				if (Trimmed.StartsWith(TEXT("["))) {
					bTagDefinitions = Trimmed.Equals(TagSettings) || Trimmed.Equals(TagList);
					continue;
				}
				if (bTagDefinitions) continue;
			}

			// names of word parts joined by dots
			int32 Start = INDEX_NONE;
			for (int32 i = 0; i <= Line.Len(); ++i) {
				const bool bNameChar = i < Line.Len() && (FChar::IsAlnum(Line[i]) || Line[i] == TCHAR('_') || Line[i] == TCHAR('.'));
				if (bNameChar && Start == INDEX_NONE) {
					Start = i;
				}
				else if (!bNameChar && Start != INDEX_NONE) {
					FString Name = Line.Mid(Start, i - Start);
					Name.TrimCharInline(TCHAR('.'), nullptr);
					if (Name.Contains(TEXT("."))) {
						Names.Add(FName {*Name});
					}
					Start = INDEX_NONE;
				}
			}
		}
	}
}

void UPjcSubsystem::GetGameplayCueNotifyPaths(TArray<FString>& Paths) {
	Paths.Reset();

	// AbilitySystemGlobals (and project classes based on it) and GameplayAbilitiesDeveloperSettings
	if (GConfig) {
		TArray<FString> Sections;
		GConfig->GetSectionNames(GGameIni, Sections);
		for (const auto& Section : Sections) {
			TArray<FString> Values;
			GConfig->GetArray(*Section, TEXT("GameplayCueNotifyPaths"), Values, GGameIni);
			Paths.Append(Values);
		}
	}

	// game feature actions that add cue folders (e.g. Lyra AddGameplayCuePath): their directory lists, a folder that
	// is not a package path is inside the plugin (UGameFeaturesSubsystem::FixPluginPackagePath)
	UClass* FeatureDataClass = FindObject<UClass>(nullptr, TEXT("/Script/GameFeatures.GameFeatureData"));
	TArray<FAssetData> FeatureDatas;
	if (FeatureDataClass) {
		FARFilter Filter;
		PjcShim::AddFilterClass(Filter, FeatureDataClass);
		Filter.bRecursiveClasses = true;
		GetModuleAssetRegistry().Get().GetAssets(Filter, FeatureDatas);
	}
	for (const auto& FeatureData : FeatureDatas) {
		const UObject* Data = FeatureData.GetAsset();
		const FArrayProperty* ActionsProperty = Data ? FindFProperty<FArrayProperty>(Data->GetClass(), TEXT("Actions")) : nullptr;
		const FObjectPropertyBase* ActionProperty = ActionsProperty ? CastField<FObjectPropertyBase>(ActionsProperty->Inner) : nullptr;
		if (!ActionProperty) continue;

		const FString PackageName = FeatureData.PackageName.ToString();
		const int32 RootEnd = PackageName.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 1);
		const FString PluginRoot = RootEnd > 0 ? PackageName.Left(RootEnd) : PackageName;
		FScriptArrayHelper Actions {ActionsProperty, ActionsProperty->ContainerPtrToValuePtr<void>(Data)};
		for (int32 ActionIndex = 0; ActionIndex < Actions.Num(); ++ActionIndex) {
			const UObject* Action = ActionProperty->GetObjectPropertyValue(Actions.GetRawPtr(ActionIndex));
			if (!Action || !Action->GetClass()->GetName().Contains(TEXT("GameplayCue"))) continue;

			for (TFieldIterator<FArrayProperty> It {Action->GetClass()}; It; ++It) {
				const FStructProperty* DirectoryProperty = CastField<FStructProperty>(It->Inner);
				if (!DirectoryProperty || DirectoryProperty->Struct->GetFName() != FName {TEXT("DirectoryPath")}) continue;

				FScriptArrayHelper Directories {*It, It->ContainerPtrToValuePtr<void>(Action)};
				for (int32 DirectoryIndex = 0; DirectoryIndex < Directories.Num(); ++DirectoryIndex) {
					const FString& Path = reinterpret_cast<const FDirectoryPath*>(Directories.GetRawPtr(DirectoryIndex))->Path;
					Paths.Add(FPackageName::IsValidLongPackageName(Path) ? Path : PluginRoot / Path);
				}
			}
		}
	}

	if (Paths.Num() == 0) {
		Paths.Add(TEXT("/Game"));
	}
	for (auto& Path : Paths) {
		Path.RemoveFromEnd(TEXT("/"));
	}
}

bool UPjcSubsystem::PathIsInsideProject(const FString& InPath) {
	const TArray<FString> RootPaths = GetProjectRootPaths();
	for (const auto& RootPath : RootPaths) {
		// "/GameData" or "/MyPluginExtra" must not match "/Game" or "/MyPlugin"
		if (InPath.Equals(RootPath) || InPath.StartsWith(RootPath + TEXT("/"))) {
			return !PathIsLeftOut(InPath);
		}
	}
	return false;
}

bool UPjcSubsystem::PathIsLeftOut(const FString& InPath) {
	if (GetDefault<UPjcScanSettings>()->bScanDevelopers) return false;

	const FString Developers = PjcConstants::PathDevelopers.ToString();
	return InPath.Equals(Developers) || InPath.StartsWith(Developers + TEXT("/"));
}

bool UPjcSubsystem::AssetIsInsideProject(const FAssetData& InAsset) {
	if (!InAsset.IsValid()) return false;

	const FString PackagePath = InAsset.PackagePath.ToString();
	return PathIsInsideProject(PackagePath);
}

bool UPjcSubsystem::FolderIsEmpty(const FString& InPath) {
	if (InPath.IsEmpty()) return false;

	const FName PathRel = FName {*PathConvertToRelative(InPath)};
	if (GetModuleAssetRegistry().Get().HasAssets(PathRel, true)) {
		return false;
	}

	const FString PathAbs = PathConvertToAbsolute(InPath);
	if (PathAbs.IsEmpty()) return false;

	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *PathAbs, TEXT("*"), true, false);

	return Files.Num() == 0;
}

bool UPjcSubsystem::FolderIsExcluded(const FString& InPath) {
	const UPjcAssetExcludeSettings* EditorAssetExcludeSettings = GetDefault<UPjcAssetExcludeSettings>();
	if (!EditorAssetExcludeSettings) return false;

	const FString PathRel = PathConvertToRelative(InPath);
	if (PathRel.IsEmpty()) return false;

	for (const auto& ExcludedPath : EditorAssetExcludeSettings->ExcludedFolders) {
		const FString ExcludedPathRel = PathConvertToRelative(ExcludedPath.Path);
		if (ExcludedPathRel.IsEmpty()) continue;

		if (PathRel.Equals(ExcludedPathRel) || PathRel.StartsWith(ExcludedPathRel + TEXT("/"))) {
			return true;
		}
	}

	return false;
}

bool UPjcSubsystem::FolderIsEngineGenerated(const FString& InPath) {
	const FString PathDevelopers = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Developers"));
	const FString PathCollections = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()) / TEXT("Collections");
	const FString PathCurrentDeveloper = PathDevelopers / FPaths::GameUserDeveloperFolderName();
	const FString PathCurrentDeveloperCollections = PathCurrentDeveloper / TEXT("Collections");

	TSet<FString> EngineGeneratedPaths;
	EngineGeneratedPaths.Emplace(PathDevelopers);
	EngineGeneratedPaths.Emplace(PathCollections);
	EngineGeneratedPaths.Emplace(PathCurrentDeveloper);
	EngineGeneratedPaths.Emplace(PathCurrentDeveloperCollections);

	return EngineGeneratedPaths.Contains(InPath);
}

FName UPjcSubsystem::GetExternalOwner(const FName External) {
	FString Path = External.ToString();
	for (const auto& Folder : {PjcShim::GetExternalActorsFolderName(), PjcShim::GetExternalObjectsFolderName()}) {
		const FString Segment = TEXT("/") + Folder + TEXT("/");
		const int32 At = Folder.IsEmpty() ? INDEX_NONE : Path.Find(Segment, ESearchCase::CaseSensitive);
		if (At == INDEX_NONE) continue;

		Path = Path.Left(At) + TEXT("/") + Path.Mid(At + Segment.Len());
		break;
	}

	// the file sits a few hashed folders below the path of its map: the longest part that is a package
	IAssetRegistry& Registry = GetModuleAssetRegistry().Get();
	TArray<FAssetData> Assets;
	for (int32 Step = 0; Step < 4; ++Step) {
		int32 Slash = INDEX_NONE;
		if (!Path.FindLastChar(TEXT('/'), Slash) || Slash <= 0) break;

		Path = Path.Left(Slash);
		Assets.Reset();
		Registry.GetAssetsByPackageName(FName {*Path}, Assets);
		if (Assets.Num() > 0) return FName {*Path};
	}
	return NAME_None;
}

bool UPjcSubsystem::FolderIsExternal(const FString& InPath) {
	// World Partition external actors/objects live directly under any content root: /Game/__ExternalActors__, /MyPlugin/__ExternalObjects__
	for (const auto& FolderName : {PjcShim::GetExternalActorsFolderName(), PjcShim::GetExternalObjectsFolderName()}) {
		if (FolderName.IsEmpty()) continue;

		const FString Segment = TEXT("/") + FolderName;
		if (InPath.Contains(Segment + TEXT("/")) || InPath.EndsWith(Segment)) {
			return true;
		}
	}

	return false;
}

void UPjcSubsystem::GetSourceAndConfigFiles(TSet<FString>& Files) {
	const FString DirSrc = FPaths::ConvertRelativePathToFull(FPaths::GameSourceDir());
	const FString DirCfg = FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir());
	const FString DirPlg = FPaths::ConvertRelativePathToFull(FPaths::ProjectPluginsDir());

	TArray<FString> SourceFiles;
	TArray<FString> ConfigFiles;

	GetFilesByExt(DirSrc, true, false, PjcConstants::SourceFileExtensions, SourceFiles);
	GetFilesByExt(DirCfg, true, false, PjcConstants::ConfigFileExtensions, ConfigFiles);

	// platform extensions (Platforms/<Platform>/Config, /Source) and editor Python scripts (Content/Python, e.g. init_unreal.py)
	const auto AddPlatformAndPythonFiles = [&SourceFiles, &ConfigFiles](const FString& Root) {
		TArray<FString> Found;
		GetFilesByExt(Root / TEXT("Platforms"), true, false, PjcConstants::SourceFileExtensions, Found);
		SourceFiles.Append(Found);
		GetFilesByExt(Root / TEXT("Platforms"), true, false, PjcConstants::ConfigFileExtensions, Found);
		ConfigFiles.Append(Found);
		GetFilesByExt(Root / TEXT("Content/Python"), true, false, PjcConstants::PythonFileExtensions, Found);
		SourceFiles.Append(Found);
	};
	AddPlatformAndPythonFiles(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()));

	// every plugin in project Plugins folder, nested ones too (e.g. Plugins/GameFeatures/ShooterCore),
	// enabled or not: assets mentioned by them are kept even when their content is not scanned
	TArray<FString> PluginDescriptors;
	IFileManager::Get().FindFilesRecursive(PluginDescriptors, *DirPlg, TEXT("*.uplugin"), true, false);

	TArray<FString> PluginFiles;
	for (const auto& PluginDescriptor : PluginDescriptors) {
		if (FPaths::GetBaseFilename(PluginDescriptor).Equals(PjcConstants::ModulePjcName.ToString())) continue;

		const FString InstalledPlugin = FPaths::GetPath(PluginDescriptor);

		GetFilesByExt(InstalledPlugin / TEXT("Source"), true, false, PjcConstants::SourceFileExtensions, PluginFiles);
		SourceFiles.Append(PluginFiles);

		PluginFiles.Reset();

		GetFilesByExt(InstalledPlugin / TEXT("Config"), true, false, PjcConstants::ConfigFileExtensions, PluginFiles);
		ConfigFiles.Append(PluginFiles);

		PluginFiles.Reset();

		AddPlatformAndPythonFiles(InstalledPlugin);
	}

	Files.Reset();
	Files.Append(SourceFiles);
	Files.Append(ConfigFiles);
}

void UPjcSubsystem::GetFolderLabels(
	const TArray<FAssetData>& AssetsAll,
	const TMap<FName, FPjcAssetUsage>& Usage,
	TSet<FName>& OutFolderLabels,
	TSet<FName>& OutFolderLabelsPackaged,
	TMap<FName, FName>& OutLabelAssets
) {
	// "Label Assets In My Directory" saves a reference to every asset in label folder. That is a cook rule, not usage:
	// label stays used, only its explicit assets are followed instead of registry dependencies
	for (const auto& Asset : AssetsAll) {
		const UClass* AssetClass = Asset.GetClass();
		if (!AssetClass || !AssetClass->IsChildOf(UPrimaryAssetLabel::StaticClass()) || !Usage.Contains(Asset.PackageName)) continue;

		const UPrimaryAssetLabel* Label = Cast<UPrimaryAssetLabel>(Asset.GetAsset());
		if (!Label || !Label->bLabelAssetsInMyDirectory) continue;

		OutFolderLabels.Add(Asset.PackageName);
		if (FolderLabelIsPackaged(*Label)) {
			OutFolderLabelsPackaged.Add(Asset.PackageName);
		}

		TArray<FSoftObjectPath> ExplicitPaths;
		for (const auto& ExplicitAsset : Label->ExplicitAssets) {
			ExplicitPaths.Add(ExplicitAsset.ToSoftObjectPath());
		}
		for (const auto& ExplicitBlueprint : Label->ExplicitBlueprints) {
			ExplicitPaths.Add(ExplicitBlueprint.ToSoftObjectPath());
		}

		for (const auto& ExplicitPath : ExplicitPaths) {
			const FName ExplicitPackage {*ExplicitPath.GetLongPackageName()};
			TArray<FAssetData> ExplicitAssets;
			GetModuleAssetRegistry().Get().GetAssetsByPackageName(ExplicitPackage, ExplicitAssets);
			if (ExplicitAssets.Num() > 0 && !OutLabelAssets.Contains(ExplicitPackage)) {
				OutLabelAssets.Add(ExplicitPackage, Asset.PackageName);
			}
		}
	}
}

void UPjcSubsystem::GetAssetsDependencies(TSet<FAssetData>& Assets, const TSet<FName>& PackagesNotFollowed) {
	TSet<FName> UsedAssetsDeps;
	TArray<FName> Stack;
	for (const auto& Asset : Assets) {
		UsedAssetsDeps.Add(Asset.PackageName);
		Stack.Add(Asset.PackageName);

		TArray<FName> Deps;
		while (Stack.Num() > 0) {
			const auto CurrentPackageName = PjcShim::PopArray(Stack);
			if (PackagesNotFollowed.Contains(CurrentPackageName)) continue;

			Deps.Reset();

			GetModuleAssetRegistry().Get().GetDependencies(CurrentPackageName, Deps);

			PjcShim::RemoveAllSwapArray(Deps, [&](const FName& Dep) {
				return !PathIsInsideProject(Dep.ToString());
			});

			for (const auto& Dep : Deps) {
				bool bIsAlreadyInSet = false;
				UsedAssetsDeps.Add(Dep, &bIsAlreadyInSet);
				if (!bIsAlreadyInSet) {
					Stack.Add(Dep);
				}
			}
		}
	}

	// empty filter would return every asset in registry and make whole project used
	if (UsedAssetsDeps.Num() == 0) return;

	FARFilter Filter;

	for (const auto& Dep : UsedAssetsDeps) {
		Filter.PackageNames.Add(Dep);
	}

	TArray<FAssetData> TempContainer;
	GetModuleAssetRegistry().Get().GetAssets(Filter, TempContainer);

	Assets.Append(TempContainer);
}

bool UPjcSubsystem::ProjectHasVersionControl() {
	if (ISourceControlModule::Get().IsEnabled()) return true;

	// repository root can be above project folder
	FString Dir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	FPaths::NormalizeDirectoryName(Dir);
	while (!Dir.IsEmpty()) {
		for (const TCHAR* Marker : {TEXT(".git"), TEXT(".svn"), TEXT(".hg"), TEXT(".plastic"), TEXT(".p4config"), TEXT("p4config.txt")}) {
			if (IFileManager::Get().FileExists(*(Dir / Marker)) || IFileManager::Get().DirectoryExists(*(Dir / Marker))) return true;
		}

		const FString Parent = FPaths::GetPath(Dir);
		if (Parent.IsEmpty() || Parent == Dir) break;
		Dir = Parent;
	}

	return false;
}

FString UPjcSubsystem::GetDeleteWarning() {
	FString Warning = TEXT("Deleted files can not be restored by Project Cleaner. ")
		TEXT("Make sure your work is saved to version control (Git, Perforce, ...) or the project folder is backed up.");

	if (!ProjectHasVersionControl()) {
		Warning += TEXT("\n\nNo version control found for this project. Copy the project folder somewhere safe before deleting.");
	}

	return Warning;
}

void UPjcSubsystem::ShowNotification(const FString& Msg, const SNotificationItem::ECompletionState State, const float Duration) {
	FNotificationInfo Info {FText::FromString(Msg)};
	Info.Text = FText::FromString(Msg);
	Info.ExpireDuration = Duration;

	const auto NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
	if (!NotificationPtr.IsValid()) return;

	NotificationPtr.Get()->SetCompletionState(State);
}

void UPjcSubsystem::ShowNotificationWithOutputLog(const FString& Msg, const SNotificationItem::ECompletionState State, const float Duration) {
	FNotificationInfo Info {FText::FromString(Msg)};
	Info.Text = FText::FromString(Msg);
	Info.ExpireDuration = Duration;
	Info.Hyperlink = FSimpleDelegate::CreateLambda([]() {
		FGlobalTabmanager::Get()->TryInvokeTab(FName {TEXT("OutputLog")});
	});
	Info.HyperlinkText = FText::FromString(TEXT("Show Output Log..."));

	const auto NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
	if (!NotificationPtr.IsValid()) return;

	NotificationPtr.Get()->SetCompletionState(State);
}

void UPjcSubsystem::ShaderCompilationEnable() {
	if (!GShaderCompilingManager) return;

	GShaderCompilingManager->SkipShaderCompilation(false);
}

void UPjcSubsystem::ShaderCompilationDisable() {
	if (!GShaderCompilingManager) return;

	GShaderCompilingManager->SkipShaderCompilation(true);
}

void UPjcSubsystem::OpenPathInFileExplorer(const FString& InPath) {
	if (InPath.IsEmpty()) return;

	FPlatformProcess::ExploreFolder(*InPath);
}

void UPjcSubsystem::OpenAssetEditor(const FAssetData& InAsset) {
	PjcShim::OpenAssetEditor(InAsset);
}

void UPjcSubsystem::OpenSizeMapViewer(const TArray<FAssetData>& InAssets) {
	if (InAssets.Num() == 0) return;

	TArray<FName> PackageNames;
	PackageNames.Reserve(InAssets.Num());

	for (const auto& Asset : InAssets) {
		PackageNames.Emplace(Asset.PackageName);
	}

	IAssetManagerEditorModule::Get().OpenSizeMapUI(PackageNames);
}

void UPjcSubsystem::OpenReferenceViewer(const TArray<FAssetData>& InAssets) {
	if (InAssets.Num() == 0) return;

	TArray<FName> PackageNames;
	PackageNames.Reserve(InAssets.Num());

	for (const auto& Asset : InAssets) {
		PackageNames.Emplace(Asset.PackageName);
	}

	IAssetManagerEditorModule::Get().OpenReferenceViewerUI(PackageNames);
}

void UPjcSubsystem::OpenAssetAuditViewer(const TArray<FAssetData>& InAssets) {
	if (InAssets.Num() == 0) return;

	TArray<FName> PackageNames;
	PackageNames.Reserve(InAssets.Num());

	for (const auto& Asset : InAssets) {
		PackageNames.Emplace(Asset.PackageName);
	}

	IAssetManagerEditorModule::Get().OpenAssetAuditUI(PackageNames);
}

void UPjcSubsystem::TryOpenFile(const FString& InPath) {
	if (InPath.IsEmpty()) return;
	if (!FPaths::FileExists(InPath)) return;

	FPlatformProcess::LaunchFileInDefaultExternalApplication(*InPath);
}

FString UPjcSubsystem::GetPathExternalActors() {
	return PjcShim::GetPathExternalActors();
}
FString UPjcSubsystem::GetPathExternalObjects() {
	return PjcShim::GetPathExternalObjects();
}

FAssetRegistryModule& UPjcSubsystem::GetModuleAssetRegistry() {
	return FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistry);
}

FAssetToolsModule& UPjcSubsystem::GetModuleAssetTools() {
	return FModuleManager::LoadModuleChecked<FAssetToolsModule>(PjcConstants::ModuleAssetTools);
}

FContentBrowserModule& UPjcSubsystem::GetModuleContentBrowser() {
	return FModuleManager::LoadModuleChecked<FContentBrowserModule>(PjcConstants::ModuleContentBrowser);
}

FPropertyEditorModule& UPjcSubsystem::GetModulePropertyEditor() {
	return FModuleManager::LoadModuleChecked<FPropertyEditorModule>(PjcConstants::ModulePropertyEditor);
}

void UPjcSubsystem::BucketFill(
	TArray<FAssetData>& AssetsUnused,
	TArray<FAssetData>& Bucket,
	const int32 BucketSize,
	const TSet<FName>& PackagesUnused
) {
	// Searching Root assets (assets without referencers)
	int32 Index = 0;
	TArray<FName> Refs;
	while (Bucket.Num() < BucketSize && AssetsUnused.IsValidIndex(Index)) {
		const FAssetData CurrentAsset = AssetsUnused[Index];
		GetModuleAssetRegistry().Get().GetReferencers(CurrentAsset.PackageName, Refs);
		PjcShim::RemoveAllSwapArray(Refs, [&](const FName& Ref) {
			return !PathIsInsideProject(Ref.ToString()) || Ref.IsEqual(CurrentAsset.PackageName);
		});
		Refs.Shrink();

		if (Refs.Num() == 0) {
			Bucket.AddUnique(CurrentAsset);
			AssetsUnused.RemoveAt(Index);
		}
		else {
			++Index;
		}

		Refs.Reset();
	}

	if (Bucket.Num() > 0) {
		return;
	}

	// if root assets not found, we are deleting assets single by finding its referencers
	if (AssetsUnused.Num() == 0) {
		return;
	}

	TArray<FAssetData> Stack;
	Stack.Add(AssetsUnused[0]);

	while (Stack.Num() > 0) {
		const FAssetData Current = PjcShim::PopArray(Stack);
		Bucket.AddUnique(Current);
		AssetsUnused.Remove(Current);

		GetModuleAssetRegistry().Get().GetReferencers(Current.PackageName, Refs);

		PjcShim::RemoveAllSwapArray(Refs, [&](const FName& Ref) {
			return !PathIsInsideProject(Ref.ToString()) || Ref.IsEqual(Current.PackageName);
		});
		Refs.Shrink();

		for (const auto& Ref : Refs) {
			// only unused referencers go with it, a used one (e.g. folder label) must never be deleted
			if (!PackagesUnused.Contains(Ref)) continue;

			// and only those not tried yet: one a batch before could not delete is held in memory, trying it again in
			// every later batch only costs time (and counted it twice)
			const FString ObjectPath = Ref.ToString() + TEXT(".") + FPaths::GetBaseFilename(*Ref.ToString());
			const FAssetData AssetData = PjcShim::GetAssetByObjectPath(ObjectPath);
			if (AssetData.IsValid() && AssetsUnused.Contains(AssetData)) {
				if (!Bucket.Contains(AssetData)) {
					Stack.Add(AssetData);
				}

				Bucket.AddUnique(AssetData);
				AssetsUnused.Remove(AssetData);
			}
		}

		Refs.Reset();
	}
}

int32 UPjcSubsystem::DeleteAssetsReferencedByFolderLabels(const TArray<FAssetData>& Assets) {
	// engine never deletes assets with referencers on disk. Folder labels reference every asset in their folder (cook
	// rule, not usage), so unused assets there are left over. Delete those whose only referencers are folder labels or
	// other assets deleted together, then resave labels so they stop referencing them.
	TMap<FName, FAssetData> Candidates;
	for (const auto& Asset : Assets) {
		if (FPackageName::DoesPackageExist(Asset.PackageName.ToString())) {
			Candidates.Add(Asset.PackageName, Asset);
		}
	}

	TMap<FName, TArray<FName>> Referencers;
	for (const auto& Pair : Candidates) {
		GetModuleAssetRegistry().Get().GetReferencers(Pair.Key, Referencers.Add(Pair.Key));
	}

	TMap<FName, bool> LabelCache;
	auto IsFolderLabel = [&](const FName& PackageName) {
		if (const bool* bCached = LabelCache.Find(PackageName)) return *bCached;
		return LabelCache.Add(PackageName, GetFolderLabel(PackageName) != nullptr);
	};

	// assets referenced by anything that stays (on disk) are not deleted, repeat until nothing changes
	TSet<FName> Labels;
	bool bChanged = true;
	while (bChanged) {
		bChanged = false;
		Labels.Reset();
		for (auto It = Candidates.CreateIterator(); It; ++It) {
			bool bKeep = false;
			for (const auto& Ref : Referencers[It.Key()]) {
				if (Ref == It.Key() || Candidates.Contains(Ref) || !PathIsInsideProject(Ref.ToString())) continue;
				if (IsFolderLabel(Ref)) {
					Labels.Add(Ref);
					continue;
				}
				bKeep = true;
				break;
			}
			if (bKeep) {
				It.RemoveCurrent();
				bChanged = true;
			}
		}
	}

	if (Candidates.Num() == 0 || Labels.Num() == 0) return 0;

	TArray<FString> ObjectPaths;
	for (const auto& Pair : Candidates) {
		ObjectPaths.Add(PjcShim::GetObjectPathString(Pair.Value));
	}

	TArray<UObject*> LoadedAssets;
	PjcShim::LoadAssetsIfNeeded(ObjectPaths, LoadedAssets, false, false);
	PjcShim::FinishAssetCompilation();
	PjcShim::ReleaseEditorHelpers(LoadedAssets);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);

	// same for references in memory: something live outside this set uses the asset, keep it
	RemoveAssetsHeldOutside(LoadedAssets);

	if (LoadedAssets.Num() == 0) return 0;

	// nothing outside the set references them: no reference replacing (force delete edits referencers, can crash).
	// Editor systems drop their references first, same as engine delete
	FEditorDelegates::OnAssetsPreDelete.Broadcast(LoadedAssets);
	const int32 NumDeleted = ObjectTools::DeleteObjectsUnchecked(LoadedAssets);

	TArray<UPackage*> LabelPackages;
	for (const auto& Label : Labels) {
		if (UPackage* Package = LoadPackage(nullptr, *Label.ToString(), LOAD_None)) {
			Package->MarkPackageDirty();
			LabelPackages.Add(Package);
		}
	}
	if (LabelPackages.Num() > 0) {
		// saving updates label asset bundles, deleted assets are not referenced anymore
		UEditorLoadingAndSavingUtils::SavePackages(LabelPackages, false);
	}

	const int32 NumLabels = LabelPackages.Num();
	UE_LOG(LogProjectCleaner, Display, TEXT("Deleted %d asset(s) referenced only by folder labels, resaved %d label(s)"), NumDeleted, NumLabels);
	return NumDeleted;
}

void UPjcSubsystem::GetExternalActorsDependencies(TSet<FName>& OutPackages, TMap<FName, TArray<FName>>* OutActorFiles) {
	OutPackages.Reset();
	if (OutActorFiles) {
		OutActorFiles->Reset();
	}

#if ENGINE_MAJOR_VERSION < 5
	// UE4 asset registry does not read one file per actor packages (levels with external actors, 4.26+), so assets
	// placed only there have no referencers. Read imports of those packages directly, only file tables are loaded.
	for (const auto& ContentDir : GetProjectContentDirs()) {
		const FString Dir = ContentDir / PjcShim::GetExternalActorsFolderName();
		if (!IFileManager::Get().DirectoryExists(*Dir)) continue;

		TArray<FString> Files;
		IFileManager::Get().FindFilesRecursive(Files, *Dir, *(TEXT("*") + FPackageName::GetAssetPackageExtension()), true, false);

		for (const auto& File : Files) {
			FString PackageName;
			if (!FPackageName::TryConvertFilenameToLongPackageName(File, PackageName)) continue;

			const bool bWasLoaded = FindPackage(nullptr, *PackageName) != nullptr;
			const auto OnLinker = [&](FLinkerLoad* LoadedLinker) {
				if (!LoadedLinker) return;

				for (const FObjectImport& Import : LoadedLinker->ImportMap) {
					if (Import.OuterIndex.IsNull() && Import.ClassName == NAME_Package) {
						OutPackages.Add(Import.ObjectName);
						if (OutActorFiles) {
							OutActorFiles->FindOrAdd(Import.ObjectName).AddUnique(FName {*PackageName});
						}
					}
				}
			};
			const uint32 LoadFlags = LOAD_NoVerify | LOAD_Quiet | LOAD_NoWarn;
			FLinkerLoad* Linker = LoadPackageLinker(nullptr, *PackageName, LoadFlags, nullptr, nullptr, nullptr, OnLinker);

			// release file handles of packages opened only for this
			if (Linker && Linker->LinkerRoot && !bWasLoaded) {
				ResetLoaders(Linker->LinkerRoot);
			}
		}
	}
#endif
}

int32 UPjcSubsystem::DeleteAssetsHeldByEachOther(const TArray<FAssetData>& Assets) {
	// unused assets can hold each other in memory without registry knowing (e.g. montage and its sequence through
	// notifies), deleting them one by one never works and engine deletes all or nothing: delete those together
	TArray<FString> ObjectPaths;
	for (const auto& Asset : Assets) {
		if (FPackageName::DoesPackageExist(Asset.PackageName.ToString())) {
			ObjectPaths.Add(PjcShim::GetObjectPathString(Asset));
		}
	}
	if (ObjectPaths.Num() < 2) return 0;

	TArray<UObject*> LoadedAssets;
	PjcShim::LoadAssetsIfNeeded(ObjectPaths, LoadedAssets, false, false);
	PjcShim::FinishAssetCompilation();
	PjcShim::ReleaseEditorHelpers(LoadedAssets);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);

	RemoveAssetsHeldOutside(LoadedAssets);
	if (LoadedAssets.Num() == 0) return 0;

	// groups of assets referencing each other, so one asset engine refuses does not block all others
	TMap<FName, UObject*> ByPackage;
	for (UObject* Asset : LoadedAssets) {
		ByPackage.Add(Asset->GetOutermost()->GetFName(), Asset);
	}

	int32 NumDeleted = 0;
	TSet<FName> Visited;
	for (const auto& Pair : ByPackage) {
		if (Visited.Contains(Pair.Key)) continue;

		TArray<UObject*> Group;
		TArray<FName> Stack {Pair.Key};
		Visited.Add(Pair.Key);
		while (Stack.Num() > 0) {
			const FName Current = PjcShim::PopArray(Stack);
			Group.Add(ByPackage[Current]);

			TArray<FName> Linked;
			GetModuleAssetRegistry().Get().GetDependencies(Current, Linked);
			TArray<FName> Referencers;
			GetModuleAssetRegistry().Get().GetReferencers(Current, Referencers);
			Linked.Append(Referencers);

			for (const auto& Other : Linked) {
				if (ByPackage.Contains(Other) && !Visited.Contains(Other)) {
					Visited.Add(Other);
					Stack.Add(Other);
				}
			}
		}

		// regular delete: it still refuses when a kept asset references any of them on disk
		NumDeleted += ObjectTools::DeleteObjects(Group, false);
	}

	return NumDeleted;
}

FString UPjcSubsystem::GetKeptReason(const FName& PackageName) {
	// nothing live holds it: a saved asset that stays references it
	TArray<FName> Referencers;
	GetModuleAssetRegistry().Get().GetReferencers(PackageName, Referencers);
	for (const auto& Referencer : Referencers) {
		if (Referencer == PackageName || !PathIsInsideProject(Referencer.ToString())) continue;
		if (!FPackageName::DoesPackageExist(Referencer.ToString())) continue;

		return TEXT("referenced by ") + Referencer.ToString();
	}
	return TEXT("the editor refused to delete it, see the Output Log");
}

void UPjcSubsystem::RemoveAssetsHeldOutside(TArray<UObject*>& Assets, TMap<FString, FString>* OutHolders) {
	// objects of other assets in the set (and own subobjects) do not count, repeat until nothing changes
	bool bChanged = true;
	while (bChanged && Assets.Num() > 0) {
		bChanged = false;

		TSet<UObject*> ObjectsInSet;
		for (const UObject* Asset : Assets) {
			TArray<UObject*> Inner;
			GetObjectsWithOuter(Asset->GetOutermost(), Inner);
			ObjectsInSet.Append(Inner);
			ObjectsInSet.Add(Asset->GetOutermost());
		}

		for (int32 i = Assets.Num() - 1; i >= 0; --i) {
			// engine refuses delete when anything outside holds asset or any of its subobjects (e.g. montage notifies), or
			// a blueprint's generated class. Weak caches do not hold it (a macro library's or parent's dependents)
			TArray<UObject*> Referencees;
			GetObjectsWithOuter(Assets[i], Referencees);
			Referencees.Add(Assets[i]);
			const UBlueprint* Blueprint = Cast<UBlueprint>(Assets[i]);
			if (Blueprint && Blueprint->GeneratedClass) {
				Referencees.Add(Blueprint->GeneratedClass);
			}

			TArray<UObject*> Holders = PjcShim::GetDeleteBlockers(Referencees, &ObjectsInSet);
			Holders.RemoveAll([](const UObject* Holder) {
				return !IsValid(Holder);
			});
			if (Holders.Num() == 0) continue;

			if (OutHolders) {
				OutHolders->Add(Assets[i]->GetPathName(), TEXT("in memory by ") + Holders[0]->GetFullName());
			}
			Assets.RemoveAt(i);
			bChanged = true;
		}
	}
}

const UPrimaryAssetLabel* UPjcSubsystem::GetFolderLabel(const FName& PackageName) {
	TArray<FAssetData> Assets;
	GetModuleAssetRegistry().Get().GetAssetsByPackageName(PackageName, Assets);

	for (const auto& Asset : Assets) {
		const UClass* AssetClass = Asset.GetClass();
		if (!AssetClass || !AssetClass->IsChildOf(UPrimaryAssetLabel::StaticClass())) continue;

		const UPrimaryAssetLabel* Label = Cast<UPrimaryAssetLabel>(Asset.GetAsset());
		if (Label && Label->bLabelAssetsInMyDirectory) return Label;
	}

	return nullptr;
}

bool UPjcSubsystem::FolderLabelIsPackaged(const UPrimaryAssetLabel& Label) {
	// UAssetManager::ModifyCook cooks the bundles of a registered label, its folder among them, when the label type is
	// not editor only and the label's rule is Always Cook or a Development Always rule. The engine default label type is
	// editor only: such labels only sort assets into chunks
	if (!PjcShim::IsAssetManagerValid()) return false;

	const UAssetManager& AssetManager = UAssetManager::Get();
	const FPrimaryAssetId LabelId = Label.GetPrimaryAssetId();
	FPrimaryAssetTypeInfo TypeInfo;
	FAssetData LabelData;
	if (!LabelId.IsValid() || !AssetManager.GetPrimaryAssetTypeInfo(LabelId.PrimaryAssetType, TypeInfo) || TypeInfo.bIsEditorOnly) return false;
	if (!AssetManager.GetPrimaryAssetData(LabelId, LabelData)) return false;

	// a loaded label sets its own rule over its type's rule, unless it leaves it Unknown
	const EPrimaryAssetCookRule LabelRule = Label.Rules.CookRule;
	const EPrimaryAssetCookRule TypeRule = AssetManager.GetPrimaryAssetRules(LabelId).CookRule;
	return PjcShim::IsCookRuleAlways(LabelRule != EPrimaryAssetCookRule::Unknown ? LabelRule : TypeRule);
}

bool UPjcSubsystem::BucketPrepare(const TArray<FAssetData>& Bucket, TArray<UObject*>& LoadedAssets) {
	TArray<FString> ObjectPaths;
	ObjectPaths.Reserve(Bucket.Num());

	for (const auto& Asset : Bucket) {
		// package could be gone already, deleted together with assets of previous bucket
		if (!FPackageName::DoesPackageExist(Asset.PackageName.ToString())) continue;

		ObjectPaths.Add(PjcShim::GetObjectPathString(Asset));
	}

	if (ObjectPaths.Num() == 0) return true;

	// unused redirector must delete itself, following it would load (and delete) its target
	const bool bLoaded = PjcShim::LoadAssetsIfNeeded(ObjectPaths, LoadedAssets, false, false);

	// async compilation started by loading (meshes, textures, animations, Niagara) holds assets, engine refuses to delete them
	PjcShim::FinishAssetCompilation();

	// same for helpers assets keep for themselves (animation data controllers), engine counts them as outside references
	if (PjcShim::ReleaseEditorHelpers(LoadedAssets)) {
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	}

	return bLoaded;
}

int32 UPjcSubsystem::BucketDelete(const TArray<UObject*>& LoadedAssets) {
	const int32 NumDeleted = ObjectTools::DeleteObjects(LoadedAssets, false);
	if (NumDeleted > 0 || LoadedAssets.Num() < 2) return NumDeleted;

	// engine deletes all or nothing: one asset still referenced in memory blocks the whole bucket. Retry one by one,
	// never force delete, a live reference means something uses that asset even when no saved asset does
	TArray<TWeakObjectPtr<UObject>> Remaining;
	for (UObject* Asset : LoadedAssets) {
		Remaining.Add(Asset);
	}

	// leftovers of assets deleted before (and their finished async jobs) must not count as references
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);

	int32 NumDeletedSingle = 0;
	bool bProgress = true;
	while (bProgress && Remaining.Num() > 0) {
		bProgress = false;
		for (int32 i = Remaining.Num() - 1; i >= 0; --i) {
			UObject* Asset = Remaining[i].Get();
			if (Asset && ObjectTools::DeleteObjects({Asset}, false) == 0) continue;

			// deleted, or gone together with an asset deleted before
			Remaining.RemoveAt(i);
			NumDeletedSingle += Asset ? 1 : 0;
			bProgress = true;
		}
	}

	return NumDeletedSingle;
}
