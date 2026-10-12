// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcSubsystem.h"
#include "PjcConstants.h"
#include "PjcShim.h"
#include "PjcSettings.h"
#include "Pjc.h"
// Engine Headers
#include "AssetManagerEditorModule.h"
#include "BlueprintActionDatabase.h"
#include "EditorUtilityBlueprint.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "FileHelpers.h"
#include "ISourceControlModule.h"
#include "HAL/PlatformFileManager.h"
#include "ObjectTools.h"
#include "ShaderCompiler.h"
#include "Misc/ScopeExit.h"
#include "Settings/ContentBrowserSettings.h"
#include "Settings/ProjectPackagingSettings.h"
#include "Engine/AssetManager.h"
#include "Engine/PrimaryAssetLabel.h"
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
			FoldersUsedByProject.Reset();
			SourceAndConfigWords.Reset();
		}
	};

	FPjcScanCache ScanCache;

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

	bFirstScan = true;
}

void UPjcSubsystem::Deinitialize() {
	Super::Deinitialize();
}

#if WITH_EDITOR
void UPjcSubsystem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) {
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();
}
#endif

void UPjcSubsystem::GetAssetsAll(TArray<FAssetData>& Assets) {
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
	ComputeAssetsUsed(Assets, AssetsFolderLabelOnly, bShowSlowTask);

	ScanCacheSet(ScanCache.AssetsUsed, Assets);
	ScanCacheSet(ScanCache.AssetsFolderLabelOnly, AssetsFolderLabelOnly);
}

void UPjcSubsystem::GetAssetsFolderLabelOnly(TArray<FAssetData>& Assets, const bool bShowSlowTask) {
	if (GetModuleAssetRegistry().Get().IsLoadingAssets()) return;
	if (ScanCacheGet(ScanCache.AssetsFolderLabelOnly, Assets)) return;

	TArray<FAssetData> AssetsUsed;
	ComputeAssetsUsed(AssetsUsed, Assets, bShowSlowTask);

	ScanCacheSet(ScanCache.AssetsUsed, AssetsUsed);
	ScanCacheSet(ScanCache.AssetsFolderLabelOnly, Assets);
}

void UPjcSubsystem::ComputeAssetsUsed(TArray<FAssetData>& OutUsed, TArray<FAssetData>& OutFolderLabelOnly, const bool bShowSlowTask) {
	const UPjcScanSettings* ScanSettings = GetDefault<UPjcScanSettings>();

	TSet<FName> ClassNamesPrimary;
	TSet<FName> ClassNamesEditor;
	TArray<FAssetData> AssetsAll;
	TArray<FAssetData> AssetsIndirect;
	TArray<FAssetData> AssetsExcluded;
	TArray<FAssetData> AssetsMegascans;
	TArray<FAssetData> AssetsEngineReferenced;
	TArray<FPjcAssetIndirectInfo> AssetsIndirectInfos;

	GetAssetsAll(AssetsAll);
	GetAssetsEngineReferenced(AssetsEngineReferenced, bShowSlowTask);
	const TSet<FAssetData> AssetsEngineReferencedSet {AssetsEngineReferenced};
	if (ScanSettings->bIndirectAssetsUsed) {
		GetAssetsIndirect(AssetsIndirect, AssetsIndirectInfos, bShowSlowTask);
	}
	GetAssetsExcluded(AssetsExcluded, bShowSlowTask);
	GetClassNamesPrimary(ClassNamesPrimary);
	GetClassNamesEditor(ClassNamesEditor);

	if (FModuleManager::Get().IsModuleLoaded(PjcConstants::ModuleMegascans)) {
		GetModuleAssetRegistry().Get().GetAssetsByPath(PjcConstants::PathMSPresets, AssetsMegascans, true);
	}

	TSet<FAssetData> AssetsUsed;
	AssetsUsed.Append(AssetsIndirect);
	AssetsUsed.Append(AssetsExcluded);
	AssetsUsed.Append(AssetsMegascans);

	FScopedSlowTask SlowTask {
	  static_cast<float>(AssetsAll.Num()), FText::FromString(TEXT("Searching used assets...")), bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	for (const auto& Asset : AssetsAll) {
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(Asset.GetFullName()));

		const FName AssetExactClassName = GetAssetExactClassName(Asset);
		const bool bIsPrimary = ClassNamesPrimary.Contains(PjcShim::GetAssetClassName(Asset)) || ClassNamesPrimary.Contains(AssetExactClassName);
		const bool bIsEditor = ClassNamesEditor.Contains(PjcShim::GetAssetClassName(Asset)) || ClassNamesEditor.Contains(AssetExactClassName);
		const bool bIsEngineReferenced = AssetsEngineReferencedSet.Contains(Asset);
		const bool bIsUsed = bIsPrimary || bIsEditor || bIsEngineReferenced || AssetIsLoadedByTag(Asset);

		if (bIsUsed) {
			AssetsUsed.Emplace(Asset);
		}
	}

	// folder labels reference every asset in their folder: closure without following them tells which assets only they keep
	TSet<FAssetData> AssetsUsedStrict = AssetsUsed;
	TSet<FName> FolderLabels;
	if (ScanSettings->bPrimaryAssetsUsed) {
		GetFolderLabels(AssetsAll, AssetsUsedStrict, FolderLabels);
	}
	GetAssetsDependencies(AssetsUsedStrict, FolderLabels);

	TSet<FName> PackagesStrict;
	for (const auto& Asset : AssetsUsedStrict) {
		PackagesStrict.Add(Asset.PackageName);
	}

	TSet<FName> PackagesFull = PackagesStrict;
	if (FolderLabels.Num() > 0) {
		GetAssetsDependencies(AssetsUsed);
		for (const auto& Asset : AssetsUsed) {
			PackagesFull.Add(Asset.PackageName);
		}
	}

	// dependencies come per package, map them back to scanned assets so used + unused = all
	const TSet<FName>& PackagesUsed = ScanSettings->bFolderLabelsUsed ? PackagesFull : PackagesStrict;
	OutUsed.Reset(AssetsAll.Num());
	OutFolderLabelOnly.Reset();
	for (const auto& Asset : AssetsAll) {
		if (PackagesUsed.Contains(Asset.PackageName)) {
			OutUsed.Emplace(Asset);
		}
		if (PackagesFull.Contains(Asset.PackageName) && !PackagesStrict.Contains(Asset.PackageName)) {
			OutFolderLabelOnly.Emplace(Asset);
		}
	}
	OutUsed.Shrink();

	// tell which label decides how many assets, e.g. Lyra DefaultGame_Label keeps hundreds of otherwise unused assets
	for (const auto& FolderLabel : FolderLabels) {
		TArray<FName> Deps;
		GetModuleAssetRegistry().Get().GetDependencies(FolderLabel, Deps);
		int32 NumOnlyByLabel = 0;
		for (const auto& Asset : OutFolderLabelOnly) {
			NumOnlyByLabel += Deps.Contains(Asset.PackageName) ? 1 : 0;
		}
		if (NumOnlyByLabel == 0) continue;

		UE_LOG(
			LogProjectCleaner,
			Display,
			TEXT("Folder label %s: %d asset(s) referenced only by it are %s (Project Settings > Plugins > Project Cleaner > Folder Labels Used)"),
			*FolderLabel.ToString(),
			NumOnlyByLabel,
			ScanSettings->bFolderLabelsUsed ? TEXT("kept used") : TEXT("unused")
		);
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
	GetAssetsAll(AssetsAll);

	TSet<FName> ClassNamesPrimary;
	GetClassNamesPrimary(ClassNamesPrimary);

	Assets.Reset(AssetsAll.Num());

	FScopedSlowTask SlowTask {
	  static_cast<float>(AssetsAll.Num()),
	  FText::FromString(TEXT("Searching primary assets...")),
	  bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	for (const auto& Asset : AssetsAll) {
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(Asset.GetFullName()));

		if (ClassNamesPrimary.Contains(PjcShim::GetAssetClassName(Asset)) || ClassNamesPrimary.Contains(GetAssetExactClassName(Asset))) {
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

		Lines.Reset();
		FFileHelper::LoadFileToStringArray(Lines, *File);
		const FString FilePathAbs = FPaths::ConvertRelativePathToFull(File);

		for (int32 i = 0; i < Lines.Num(); ++i) {
			FRegexMatcher Matcher(Pattern, Lines[i]);
			while (Matcher.FindNext()) {
				FString Match = Matcher.GetCaptureGroup(0);
				while (Match.EndsWith(TEXT(".")) || Match.EndsWith(TEXT("/"))) {
					Match.LeftChopInline(1);
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
	GetAssetsAll(AssetsAll);

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
	GetAssetsAll(AssetsAll);

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
	GetAssetsAll(AssetsAll);

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
	GetAssetsAll(AssetsAll);

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

	const UPjcScanSettings* ScanSettings = GetDefault<UPjcScanSettings>();
	if (!ScanSettings->bPrimaryAssetsUsed) return;

	// getting list of primary asset classes that are defined in AssetManager
	if (!PjcShim::IsAssetManagerValid()) return;

	const auto& AssetManager = UAssetManager::Get();

	TSet<FName> ClassNamesPrimaryBase;
	TArray<FPrimaryAssetTypeInfo> AssetTypeInfos;
	AssetManager.Get().GetPrimaryAssetTypeInfoList(AssetTypeInfos);
	ClassNamesPrimaryBase.Reserve(AssetTypeInfos.Num());

	for (const auto& AssetTypeInfo : AssetTypeInfos) {
		if (!AssetTypeInfo.AssetBaseClassLoaded) continue;
		if (ScanSettings->IgnoredPrimaryAssetTypes.Contains(AssetTypeInfo.PrimaryAssetType)) continue;

		// type registered without real base class (e.g. GameFeature plugins in Lyra like projects)
		// would make every asset used (#58)
		if (AssetTypeInfo.AssetBaseClassLoaded == UObject::StaticClass()) {
			UE_LOG(
				LogProjectCleaner, Verbose, TEXT("Primary asset type %s has UObject base class, ignored"), *AssetTypeInfo.PrimaryAssetType.ToString()
			);
			continue;
		}

		ClassNamesPrimaryBase.Emplace(PjcShim::GetClassName(AssetTypeInfo.AssetBaseClassLoaded));
	}

	// getting list of primary assets classes that are derived from main primary assets
	PjcShim::GetDerivedClassNames(ClassNamesPrimaryBase.Array(), TSet<FName> {}, ClassNames);
}

void UPjcSubsystem::GetClassNamesEditor(TSet<FName>& ClassNames) {
	ClassNames.Empty();

	if (!GetDefault<UPjcScanSettings>()->bEditorAssetsUsed) return;

	const TArray<FName> ClassNamesEditorBase {
	  PjcShim::GetClassName(UEditorUtilityWidget::StaticClass()),
	  PjcShim::GetClassName(UEditorUtilityBlueprint::StaticClass()),
	  PjcShim::GetClassName(UEditorUtilityWidgetBlueprint::StaticClass())
	};

	PjcShim::GetDerivedClassNames(ClassNamesEditorBase, TSet<FName> {}, ClassNames);
}

void UPjcSubsystem::GetClassNamesExcluded(TSet<FName>& ClassNames) {
	ClassNames.Empty();

	// user excluded classes match exactly
	const UPjcAssetExcludeSettings* AssetExcludeSettings = GetDefault<UPjcAssetExcludeSettings>();
	for (const auto& ExcludedClass : AssetExcludeSettings->ExcludedClasses) {
		if (!ExcludedClass.LoadSynchronous() || ExcludedClass.IsNull()) continue;

		ClassNames.Emplace(PjcShim::GetClassName(ExcludedClass.Get()));
	}

	// project used classes match with derived classes
	const UPjcScanSettings* ScanSettings = GetDefault<UPjcScanSettings>();
	TArray<FName> ClassNamesUsedBase;

	for (const auto& UsedClass : ScanSettings->UsedClasses) {
		if (!UsedClass.LoadSynchronous() || UsedClass.IsNull()) continue;

		ClassNamesUsedBase.Emplace(PjcShim::GetClassName(UsedClass.Get()));
	}

	if (ScanSettings->bCinematicAssetsUsed) {
		for (const auto& ClassPath : UPjcScanSettings::GetCinematicClassPaths()) {
			// no loading: classes of disabled plugins or other engine versions are just skipped
			const UClass* CinematicClass = FSoftClassPath {ClassPath}.ResolveClass();
			if (!CinematicClass) continue;

			ClassNamesUsedBase.Emplace(PjcShim::GetClassName(CinematicClass));
		}
	}

	if (ClassNamesUsedBase.Num() > 0) {
		TSet<FName> ClassNamesUsed;
		PjcShim::GetDerivedClassNames(ClassNamesUsedBase, TSet<FName> {}, ClassNamesUsed);
		ClassNames.Append(ClassNamesUsed);
	}
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

		if (FolderIsExternal(PathConvertToRelative(FPaths::GetPath(File)))) continue;

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
		if (FolderIsEmpty(Folder) && !FolderIsEngineGenerated(Folder) && !FolderIsExcluded(Folder)) {
			Folders.Emplace(Folder);
		}
	}

	Folders.Shrink();
}

bool UPjcSubsystem::DeleteAssetsUnused(const bool bShowSlowTask, const bool bShowEditorNotification) {
	if (GetModuleAssetRegistry().Get().IsLoadingAssets()) {
		UE_LOG(LogProjectCleaner, Warning, TEXT("Failed to delete unused assets, because AssetRegistry still discovering assets."));
		UE_LOG(LogProjectCleaner, Warning, TEXT("Please wait until it finished and then try again."));
		return false;
	}

	if (GEditor && !GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllAssetEditors()) {
		UE_LOG(LogProjectCleaner, Warning, TEXT("Failed to delete unused assets because some editor windows are still open."));
		UE_LOG(
			LogProjectCleaner,
			Warning,
			TEXT("Please try again, check the OutputLog for reasons why certain windows haven't closed, or try to manually close all editor windows.")
		);
		return false;
	}

	// unsaved edits could reference assets that look unused on disk. Commandlets have no user edits, packages there
	// only get dirty when loading updates them (e.g. Niagara systems after redirector fixup) and nobody can answer a prompt
	if (!IsRunningCommandlet() && !FEditorFileUtils::SaveDirtyPackages(true, true, true, false, false, false)) {
		UE_LOG(LogProjectCleaner, Warning, TEXT("Failed to delete unused assets because project contains unsaved assets."));
		UE_LOG(LogProjectCleaner, Warning, TEXT("Please save those assets and then try again. Check OutputLog for more information"));
		return false;
	}

	TArray<FAssetData> AssetsUnused;
	GetAssetsUnused(AssetsUnused);

	if (AssetsUnused.Num() == 0) {
		UE_LOG(LogProjectCleaner, Display, TEXT("The project currently contains no unused assets, thus there are no items to delete."));
		return true;
	}

	// asset delete handlers (e.g. RigVM) create it on first use, building it while assets are half deleted asserts (UE 5.4)
	FBlueprintActionDatabase::Get();

	constexpr int32 BucketSize = PjcConstants::BucketSize;
	const TArray<FAssetData> AssetsToDelete = AssetsUnused;
	TSet<FName> PackagesUnused;
	for (const auto& Asset : AssetsUnused) {
		PackagesUnused.Add(Asset.PackageName);
	}
	const int32 NumAssetsTotal = AssetsUnused.Num();
	int32 NumAssetsDeleted = 0;

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

	while (AssetsUnused.Num() > 0) {
		BucketFill(AssetsUnused, Bucket, BucketSize, PackagesUnused);

		if (Bucket.Num() == 0) {
			break;
		}

		if (!BucketPrepare(Bucket, LoadedAssets)) {
			bErrors = true;
			UE_LOG(LogProjectCleaner, Error, TEXT("Failed to load some assets. Aborting."));
			break;
		}

		NumAssetsDeleted += BucketDelete(LoadedAssets);
		const FString ProgressMsg = FString::Printf(TEXT("Deleted %d of %d assets"), NumAssetsDeleted, NumAssetsTotal);
		const int32 ProgressWork = FMath::Min(Bucket.Num(), NumAssetsTotal - NumProgress);
		NumProgress += ProgressWork;
		SlowTask.EnterProgressFrame(ProgressWork, FText::FromString(ProgressMsg));

		Bucket.Reset();
		LoadedAssets.Reset();
	}

	// packages left empty by this deletion only: registry "empty" list also holds packages of other loaded assets
	const TSet<FName> EmptyPackages = PjcShim::GetCachedEmptyPackages();
	TArray<UPackage*> AssetPackages;
	for (const auto& Asset : AssetsToDelete) {
		if (!EmptyPackages.Contains(Asset.PackageName)) continue;

		UPackage* Package = FindPackage(nullptr, *Asset.PackageName.ToString());
		if (Package && Package->IsValidLowLevel()) {
			AssetPackages.AddUnique(Package);
		}
	}

	if (AssetPackages.Num() > 0) {
		ObjectTools::CleanupAfterSuccessfulDelete(AssetPackages);
	}

	DeleteAssetsReferencedByFolderLabels(AssetsToDelete);
	DeleteAssetsHeldByEachOther(AssetsToDelete);

	ShaderCompilationEnable();

	// assets can be deleted together with other assets, so count what is really gone
	TArray<UObject*> AssetsRemaining;
	for (const auto& Asset : AssetsToDelete) {
		if (!FPackageName::DoesPackageExist(Asset.PackageName.ToString())) continue;

		if (UObject* Object = FindObject<UObject>(nullptr, *PjcShim::GetObjectPathString(Asset))) {
			AssetsRemaining.Add(Object);
		}
		else {
			UE_LOG(LogProjectCleaner, Warning, TEXT("Not deleted: %s"), *PjcShim::GetObjectPathString(Asset));
		}
	}

	// tell who keeps each of them, a live reference means something uses it even when no saved asset does
	TMap<FString, FString> Holders;
	TArray<UObject*> HeldByEachOther = AssetsRemaining;
	RemoveAssetsHeldOutside(HeldByEachOther, &Holders);
	for (const UObject* Asset : AssetsRemaining) {
		const FString* Holder = Holders.Find(Asset->GetPathName());
		UE_LOG(
			LogProjectCleaner,
			Warning,
			TEXT("Not deleted, still referenced: %s (%s)"),
			*Asset->GetPathName(),
			Holder ? **Holder : TEXT("by asset that is kept")
		);
	}
	int32 NumAssetsRemaining = 0;
	for (const auto& Asset : AssetsToDelete) {
		NumAssetsRemaining += FPackageName::DoesPackageExist(Asset.PackageName.ToString()) ? 1 : 0;
	}
	NumAssetsDeleted = NumAssetsTotal - NumAssetsRemaining;

	const FString Msg = FString::Printf(TEXT("Deleted %d of %d assets"), NumAssetsDeleted, NumAssetsTotal);
	UE_LOG(LogProjectCleaner, Display, TEXT("%s"), *Msg);

	if (bErrors) {
		UE_LOG(LogProjectCleaner, Error, TEXT("There very some errors while deleting assets. Please check OutputLog for more information."));
	}

	if (bShowEditorNotification && GEditor) {
		if (NumAssetsDeleted == NumAssetsTotal) {
			ShowNotification(Msg, SNotificationItem::CS_Success, 3.0f);
		}
		else {
			ShowNotificationWithOutputLog(Msg, SNotificationItem::CS_Fail, 5.0f);
		}
	}

	return !bErrors && NumAssetsDeleted == NumAssetsTotal;
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

	const FString Msg = FString::Printf(TEXT("Deleted %d of %d empty folders"), NumFoldersDeleted, NumFoldersTotal);
	UE_LOG(LogProjectCleaner, Display, TEXT("%s"), *Msg);

	if (bErrors) {
		UE_LOG(LogProjectCleaner, Error, TEXT("Failed to delete some folders. Please check OutputLog for more information"));
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

	const FString Msg = FString::Printf(TEXT("Deleted %d of %d external files"), NumFilesDeleted, NumFilesTotal);
	UE_LOG(LogProjectCleaner, Display, TEXT("%s"), *Msg);

	if (bErrors) {
		UE_LOG(LogProjectCleaner, Error, TEXT("Failed to delete some files. Please check OutputLog for more information"));
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

	const FString Msg = FString::Printf(TEXT("Deleted %d of %d corrupted files"), NumFilesDeleted, NumFilesTotal);
	UE_LOG(LogProjectCleaner, Display, TEXT("%s"), *Msg);

	if (bErrors) {
		UE_LOG(LogProjectCleaner, Error, TEXT("Failed to delete some files. Please check OutputLog for more information"));
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
	GetModuleAssetRegistry().Get().GetAssets(Filter, Redirectors);
}

bool UPjcSubsystem::ProjectHasRedirectors() {
	TArray<FAssetData> Redirectors;
	GetProjectRedirectors(Redirectors);

	return Redirectors.Num() > 0;
}

void UPjcSubsystem::FixProjectRedirectors(const TArray<FAssetData>& Redirectors, const bool bShowSlowTask) {
	if (Redirectors.Num() == 0) return;

	FScopedSlowTask SlowTask {
	  static_cast<float>(Redirectors.Num()), FText::FromString(TEXT("Fixing redirectors...")), bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	TArray<UObjectRedirector*> RedirectorObjects;
	RedirectorObjects.Reserve(Redirectors.Num());

	for (const auto& Redirector : Redirectors) {
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(Redirector.GetFullName()));

		UObjectRedirector* RedirectorObject = Cast<UObjectRedirector>(Redirector.GetAsset());
		if (!RedirectorObject) continue;

		RedirectorObjects.Emplace(RedirectorObject);
	}

	// engine fixup ends with a modal report window (UE 5.4+), which can not exist without Slate (commandlets)
	if (IsRunningCommandlet() || !FSlateApplication::IsInitialized()) {
		FixRedirectorsWithoutUI(RedirectorObjects);
		return;
	}

	GetModuleAssetTools().Get().FixupReferencers(RedirectorObjects, false);
}

void UPjcSubsystem::FixRedirectorsWithoutUI(const TArray<UObjectRedirector*>& RedirectorObjects) {
	IAssetRegistry& AssetRegistry = GetModuleAssetRegistry().Get();

	TSet<FName> RedirectorPackageNames;
	for (const auto& RedirectorObject : RedirectorObjects) {
		RedirectorPackageNames.Add(RedirectorObject->GetOutermost()->GetFName());
	}

	// redirector to missing asset: resaving its referencers would replace the broken reference with null, keep both
	TSet<FName> ReferencersToSkip;
	for (const auto& RedirectorObject : RedirectorObjects) {
		if (RedirectorObject->DestinationObject) continue;

		TArray<FName> Referencers;
		AssetRegistry.GetReferencers(RedirectorObject->GetOutermost()->GetFName(), Referencers);
		ReferencersToSkip.Append(Referencers);
		UE_LOG(LogProjectCleaner, Warning, TEXT("Redirector %s points to missing asset, not fixed"), *RedirectorObject->GetPathName());
	}

	TSet<FName> ReferencerNames;
	for (const FName& PackageName : RedirectorPackageNames) {
		TArray<FName> Referencers;
		AssetRegistry.GetReferencers(PackageName, Referencers);
		for (const FName& Referencer : Referencers) {
			if (!RedirectorPackageNames.Contains(Referencer) && !ReferencersToSkip.Contains(Referencer)) {
				ReferencerNames.Add(Referencer);
			}
		}
	}

	// loading referencers resolves redirectors (loaded redirectors also fix soft references on save)
	TArray<UPackage*> PackagesToSave;
	TArray<FString> FilesToRescan;
	for (const FName& ReferencerName : ReferencerNames) {
		FString Filename;
		if (!PjcShim::DoesPackageExist(ReferencerName.ToString(), &Filename)) continue;

		UPackage* Package = FindPackage(nullptr, *ReferencerName.ToString());
		if (!Package) {
			Package = LoadPackage(nullptr, *ReferencerName.ToString(), LOAD_None);
		}

		if (!Package || Package->HasAnyPackageFlags(PKG_CompiledIn)) continue;

		PackagesToSave.Add(Package);
		FilesToRescan.Add(FPaths::ConvertRelativePathToFull(Filename));
	}

	if (PackagesToSave.Num() > 0) {
		if (ISourceControlModule::Get().IsEnabled()) {
			FEditorFileUtils::CheckoutPackages(PackagesToSave, nullptr, false);
		}
		UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, false);
		AssetRegistry.ScanFilesSynchronous(FilesToRescan, true);
	}

	// delete only redirectors nothing points to anymore (other redirectors do not count)
	TArray<UObject*> ObjectsToDelete;
	for (const auto& RedirectorObject : RedirectorObjects) {
		TArray<FName> Referencers;
		AssetRegistry.GetReferencers(RedirectorObject->GetOutermost()->GetFName(), Referencers);
		const bool bReferenced = Referencers.ContainsByPredicate([&](const FName& Referencer) {
			return !RedirectorPackageNames.Contains(Referencer);
		});

		if (!bReferenced && RedirectorObject->DestinationObject) {
			ObjectsToDelete.Add(RedirectorObject);
		}
	}

	if (ObjectsToDelete.Num() > 0) {
		ObjectTools::DeleteObjects(ObjectsToDelete, false);
	}
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

	FString Path = FPaths::ConvertRelativePathToFull(InPath).TrimStartAndEnd();
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
	if (FPackageName::TryConvertFilenameToLongPackageName(PathNormalized, RelativePath)
		|| FPackageName::TryConvertFilenameToLongPackageName(PathNormalized + TEXT("/"), RelativePath)) {
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

bool UPjcSubsystem::PathIsInsideProject(const FString& InPath) {
	const TArray<FString> RootPaths = GetProjectRootPaths();
	for (const auto& RootPath : RootPaths) {
		// "/GameData" or "/MyPluginExtra" must not match "/Game" or "/MyPlugin"
		if (InPath.Equals(RootPath) || InPath.StartsWith(RootPath + TEXT("/"))) {
			return true;
		}
	}
	return false;
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
	}

	Files.Reset();
	Files.Append(SourceFiles);
	Files.Append(ConfigFiles);
}

void UPjcSubsystem::GetFolderLabels(const TArray<FAssetData>& AssetsAll, TSet<FAssetData>& AssetsUsed, TSet<FName>& PackagesNotFollowed) {
	// "Label Assets In My Directory" saves a reference to every asset in label folder. That is a cook rule, not usage:
	// label stays used, only its explicit assets are followed instead of registry dependencies
	for (const auto& Asset : AssetsAll) {
		const UClass* AssetClass = Asset.GetClass();
		if (!AssetClass || !AssetClass->IsChildOf(UPrimaryAssetLabel::StaticClass()) || !AssetsUsed.Contains(Asset)) continue;

		const UPrimaryAssetLabel* Label = Cast<UPrimaryAssetLabel>(Asset.GetAsset());
		if (!Label || !Label->bLabelAssetsInMyDirectory || !Label->AssetCollection.CollectionName.IsNone()) continue;

		PackagesNotFollowed.Add(Asset.PackageName);

		TArray<FSoftObjectPath> ExplicitPaths;
		for (const auto& ExplicitAsset : Label->ExplicitAssets) {
			ExplicitPaths.Add(ExplicitAsset.ToSoftObjectPath());
		}
		for (const auto& ExplicitBlueprint : Label->ExplicitBlueprints) {
			ExplicitPaths.Add(ExplicitBlueprint.ToSoftObjectPath());
		}

		for (const auto& ExplicitPath : ExplicitPaths) {
			TArray<FAssetData> ExplicitAssets;
			GetModuleAssetRegistry().Get().GetAssetsByPackageName(FName {*ExplicitPath.GetLongPackageName()}, ExplicitAssets);
			AssetsUsed.Append(ExplicitAssets);
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
	Info.HyperlinkText = FText::FromString(TEXT("Show OutputLog..."));

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

			const FString ObjectPath = Ref.ToString() + TEXT(".") + FPaths::GetBaseFilename(*Ref.ToString());
			const FAssetData AssetData = PjcShim::GetAssetByObjectPath(ObjectPath);
			if (AssetData.IsValid()) {
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
	if (GetDefault<UPjcScanSettings>()->bFolderLabelsUsed) return 0;

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
		return LabelCache.Add(PackageName, PackageIsFolderLabel(PackageName));
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

	UE_LOG(
		LogProjectCleaner, Display, TEXT("Deleted %d asset(s) referenced only by folder labels, resaved %d label(s)"), NumDeleted, LabelPackages.Num()
	);
	return NumDeleted;
}

void UPjcSubsystem::GetExternalActorsDependencies(TSet<FName>& OutPackages) {
	OutPackages.Reset();

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
			FLinkerLoad* Linker = LoadPackageLinker(
				nullptr, *PackageName, LOAD_NoVerify | LOAD_Quiet | LOAD_NoWarn, nullptr, nullptr, nullptr, [&](FLinkerLoad* LoadedLinker) {
					if (!LoadedLinker) return;

					for (const FObjectImport& Import : LoadedLinker->ImportMap) {
						if (Import.OuterIndex.IsNull() && Import.ClassName == NAME_Package) {
							OutPackages.Add(Import.ObjectName);
						}
					}
				}
			);

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
			// engine refuses delete when anything outside holds asset or any of its subobjects (e.g. montage notifies)
			TArray<UObject*> Referencees;
			GetObjectsWithOuter(Assets[i], Referencees);
			Referencees.Add(Assets[i]);

			TArray<UObject*> Holders = FReferencerFinder::GetAllReferencers(Referencees, &ObjectsInSet);
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

bool UPjcSubsystem::PackageIsFolderLabel(const FName& PackageName) {
	TArray<FAssetData> Assets;
	GetModuleAssetRegistry().Get().GetAssetsByPackageName(PackageName, Assets);

	for (const auto& Asset : Assets) {
		const UClass* AssetClass = Asset.GetClass();
		if (!AssetClass || !AssetClass->IsChildOf(UPrimaryAssetLabel::StaticClass())) continue;

		const UPrimaryAssetLabel* Label = Cast<UPrimaryAssetLabel>(Asset.GetAsset());
		if (Label && Label->bLabelAssetsInMyDirectory && Label->AssetCollection.CollectionName.IsNone()) return true;
	}

	return false;
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
