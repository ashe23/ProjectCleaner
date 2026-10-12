// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcSubsystem.h"
#include "PjcConstants.h"
#include "PjcShim.h"
#include "PjcSettings.h"
#include "Pjc.h"
// Engine Headers
#include "AssetManagerEditorModule.h"
#include "EditorUtilityBlueprint.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "FileHelpers.h"
#include "ObjectTools.h"
#include "ShaderCompiler.h"
#include "Misc/ScopeExit.h"
#include "Settings/ContentBrowserSettings.h"
#include "Engine/AssetManager.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/ScopedSlowTask.h"

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

		void Reset() {
			AssetsAll.Reset();
			AssetsUsed.Reset();
			AssetsUnused.Reset();
			AssetsIndirect.Reset();
			AssetsIndirectInfos.Reset();
			AssetsExcluded.Reset();
			AssetsEngineReferenced.Reset();
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
		const bool bIsUsed = bIsPrimary || bIsEditor || bIsEngineReferenced;

		if (bIsUsed) {
			AssetsUsed.Emplace(Asset);
		}
	}

	GetAssetsDependencies(AssetsUsed);

	// dependencies come per package, map them back to scanned assets so used + unused = all
	TSet<FName> PackagesUsed;
	PackagesUsed.Reserve(AssetsUsed.Num());
	for (const auto& Asset : AssetsUsed) {
		PackagesUsed.Add(Asset.PackageName);
	}

	Assets.Reset(AssetsAll.Num());
	for (const auto& Asset : AssetsAll) {
		if (PackagesUsed.Contains(Asset.PackageName)) {
			Assets.Emplace(Asset);
		}
	}
	Assets.Shrink();

	ScanCacheSet(ScanCache.AssetsUsed, Assets);
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
	const FRegexPattern Pattern {FString::Printf(TEXT(R"((%s)\/[A-Za-z0-9_.\/]+\b)"), *FString::Join(RootPatterns, TEXT("|")))};

	TArray<FString> Lines;
	for (const auto& File : ScanFiles) {
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(File));

		Lines.Reset();
		FFileHelper::LoadFileToStringArray(Lines, *File);
		const FString FilePathAbs = FPaths::ConvertRelativePathToFull(File);

		for (int32 i = 0; i < Lines.Num(); ++i) {
			FRegexMatcher Matcher(Pattern, Lines[i]);
			while (Matcher.FindNext()) {
				const FString Match = Matcher.GetCaptureGroup(0);
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

	FScopedSlowTask SlowTask {
	  static_cast<float>(AssetsAll.Num()),
	  FText::FromString(TEXT("Searching assets with external referencers...")),
	  bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);

	for (const auto& Asset : AssetsAll) {
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(Asset.GetFullName()));

		if (AssetIsEngineReferenced(Asset)) {
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

	TArray<FAssetData> Redirectors;
	GetProjectRedirectors(Redirectors);
	FixProjectRedirectors(Redirectors);

	if (ProjectHasRedirectors()) {
		UE_LOG(LogProjectCleaner, Warning, TEXT("Failed to delete unused assets because project contains redirectors that failed to fix."));
		UE_LOG(LogProjectCleaner, Warning, TEXT("Please fix redirectors first manually and then try again."));
		return false;
	}

	if (!FEditorFileUtils::SaveDirtyPackages(true, true, true, false, false, false)) {
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

	constexpr int32 BucketSize = PjcConstants::BucketSize;
	const TArray<FAssetData> AssetsToDelete = AssetsUnused;
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

	while (AssetsUnused.Num() > 0) {
		BucketFill(AssetsUnused, Bucket, BucketSize);

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
		SlowTask.EnterProgressFrame(Bucket.Num(), FText::FromString(ProgressMsg));

		Bucket.Reset();
		LoadedAssets.Reset();
	}

	const TSet<FName> EmptyPackages = PjcShim::GetCachedEmptyPackages();
	TArray<UPackage*> AssetPackages;
	for (const auto& EmptyPackage : EmptyPackages) {
		UPackage* Package = FindPackage(nullptr, *EmptyPackage.ToString());
		if (Package && Package->IsValidLowLevel()) {
			AssetPackages.Add(Package);
		}
	}

	if (AssetPackages.Num() > 0) {
		ObjectTools::CleanupAfterSuccessfulDelete(AssetPackages);
	}

	ShaderCompilationEnable();

	// assets can be deleted together with other assets, so count what is really gone
	int32 NumAssetsRemaining = 0;
	for (const auto& Asset : AssetsToDelete) {
		if (FPackageName::DoesPackageExist(Asset.PackageName.ToString())) {
			++NumAssetsRemaining;
		}
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

		UObjectRedirector* RedirectorObject = CastChecked<UObjectRedirector>(Redirector.GetAsset());
		if (!RedirectorObject) continue;

		RedirectorObjects.Emplace(RedirectorObject);
	}

	GetModuleAssetTools().Get().FixupReferencers(RedirectorObjects, false);
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

void UPjcSubsystem::GetAssetsDependencies(TSet<FAssetData>& Assets) {
	TSet<FName> UsedAssetsDeps;
	TArray<FName> Stack;
	for (const auto& Asset : Assets) {
		UsedAssetsDeps.Add(Asset.PackageName);
		Stack.Add(Asset.PackageName);

		TArray<FName> Deps;
		while (Stack.Num() > 0) {
			const auto CurrentPackageName = PjcShim::PopArray(Stack);
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

void UPjcSubsystem::BucketFill(TArray<FAssetData>& AssetsUnused, TArray<FAssetData>& Bucket, const int32 BucketSize) {
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

bool UPjcSubsystem::BucketPrepare(const TArray<FAssetData>& Bucket, TArray<UObject*>& LoadedAssets) {
	TArray<FString> ObjectPaths;
	ObjectPaths.Reserve(Bucket.Num());

	for (const auto& Asset : Bucket) {
		// package could be gone already, deleted together with assets of previous bucket
		if (!FPackageName::DoesPackageExist(Asset.PackageName.ToString())) continue;

		ObjectPaths.Add(PjcShim::GetObjectPathString(Asset));
	}

	if (ObjectPaths.Num() == 0) return true;

	return PjcShim::LoadAssetsIfNeeded(ObjectPaths, LoadedAssets, false, true);
}

int32 UPjcSubsystem::BucketDelete(const TArray<UObject*>& LoadedAssets) {
	int32 DeletedAssetsNum = ObjectTools::DeleteObjects(LoadedAssets, false);

	if (DeletedAssetsNum == 0) {
		DeletedAssetsNum = ObjectTools::ForceDeleteObjects(LoadedAssets, false);
	}

	return DeletedAssetsNum;
}
