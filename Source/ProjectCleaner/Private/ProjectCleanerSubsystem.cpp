// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerSubsystem.h"
#include "ProjectCleanerConstants.h"
#include "Libs/ProjectCleanerLibPath.h"
#include "Settings/ProjectCleanerExcludeSettings.h"
// Engine Headers
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "EditorUtilityBlueprint.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "FileHelpers.h"
#include "Engine/AssetManager.h"
#include "Engine/MapBuildDataRegistry.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"

UProjectCleanerSubsystem::UProjectCleanerSubsystem()
	: ModuleAssetRegistry(&FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName)),
	  ModuleAssetTools(&FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"))),
	  PlatformFile(&FPlatformFileManager::Get().GetPlatformFile())
{
}

void UProjectCleanerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UProjectCleanerSubsystem::Deinitialize()
{
	Super::Deinitialize();

	ContainersEmpty();
	DelegateScanFinished.RemoveAll(this);
}

#if WITH_EDITOR
void UProjectCleanerSubsystem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();
}
#endif

void UProjectCleanerSubsystem::ProjectScan()
{
	CheckEditorState();

	if (EditorState != EProjectCleanerEditorState::Idle || ScanState != EProjectCleanerScanState::Idle)
	{
		return;
	}

	ScanState = EProjectCleanerScanState::Scanning;

	FixupRedirectors();
	FEditorFileUtils::SaveDirtyPackages(false, true, true, false, false, false);

	FScopedSlowTask SlowTask{3.0f, FText::FromString(ProjectCleanerConstants::MsgScanning)};
	SlowTask.MakeDialog(false, false);

	ContainersReset();

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(ProjectCleanerConstants::MsgScanningContentFolder));

	ScanContentFolder();

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(ProjectCleanerConstants::MsgLoadingAssetsBlacklist));

	FindAssetsBlacklisted();

	SlowTask.EnterProgressFrame(1.0f, FText::FromString(ProjectCleanerConstants::MsgScanningAssetsUnused));

	FindAssetsAll();
	FindAssetsPrimary();
	FindAssetsIndirect();
	FindAssetsExcluded();
	FindAssetsWithExternalRefs();
	FindAssetsUsed();
	FindAssetsUnused();

	ContainersShrink();

	ScanState = EProjectCleanerScanState::Idle;

	if (DelegateScanFinished.IsBound())
	{
		DelegateScanFinished.Broadcast();
	}
}

void UProjectCleanerSubsystem::CheckEditorState()
{
	if (!GEditor) return;

	if (GEditor->PlayWorld || GIsPlayInEditorWorld)
	{
		EditorState = EProjectCleanerEditorState::PlayMode;
		return;
	}

	if (ModuleAssetRegistry->Get().IsLoadingAssets())
	{
		EditorState = EProjectCleanerEditorState::AssetRegistryWorking;
		return;
	}

	EditorState = EProjectCleanerEditorState::Idle;
}

void UProjectCleanerSubsystem::GetLinkedAssets(const TArray<FAssetData>& Assets, TArray<FAssetData>& LinkedAssets) const
{
	LinkedAssets.Reset();

	TSet<FName> UsedAssetsDeps;
	TArray<FName> Stack;
	for (const auto& Asset : Assets)
	{
		UsedAssetsDeps.Add(Asset.PackageName);
		Stack.Add(Asset.PackageName);

		TArray<FName> Deps;
		while (Stack.Num() > 0)
		{
			const auto CurrentPackageName = Stack.Pop(false);
			Deps.Reset();

			ModuleAssetRegistry->Get().GetDependencies(CurrentPackageName, Deps);

			Deps.RemoveAllSwap([&](const FName& Dep)
			{
				return !Dep.ToString().StartsWith(*ProjectCleanerConstants::PathRelRoot.ToString());
			}, false);

			for (const auto& Dep : Deps)
			{
				bool bIsAlreadyInSet = false;
				UsedAssetsDeps.Add(Dep, &bIsAlreadyInSet);
				if (!bIsAlreadyInSet)
				{
					Stack.Add(Dep);
				}
			}
		}
	}

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(FName{*ProjectCleanerConstants::PathRelRoot.ToString()});

	for (const auto& Dep : UsedAssetsDeps)
	{
		Filter.PackageNames.Add(Dep);
	}

	ModuleAssetRegistry->Get().GetAssets(Filter, LinkedAssets);
}

FString UProjectCleanerSubsystem::GetAssetClassName(const FAssetData& AssetData) const
{
	if (!AssetData.IsValid()) return {};

	if (AssetData.AssetClass.IsEqual("Blueprint"))
	{
		const auto GeneratedClassName = AssetData.TagsAndValues.FindTag(TEXT("GeneratedClass")).GetValue();
		const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(*GeneratedClassName);
		return FPackageName::ObjectPathToObjectName(ClassObjectPath);
	}

	return AssetData.AssetClass.ToString();
}

const TArray<FAssetData>& UProjectCleanerSubsystem::GetAssetsAll() const
{
	return AssetsAll;
}

const TArray<FAssetData>& UProjectCleanerSubsystem::GetAssetsIndirect() const
{
	return AssetsIndirect;
}

const TArray<FAssetData>& UProjectCleanerSubsystem::GetAssetsExcluded() const
{
	return AssetsExcluded;
}

const TArray<FAssetData>& UProjectCleanerSubsystem::GetAssetsUnused() const
{
	return AssetsUnused;
}

int64 UProjectCleanerSubsystem::GetAssetsTotalSize(const TArray<FAssetData>& Assets) const
{
	int64 Size = 0;

	for (const auto& Asset : Assets)
	{
		const auto AssetPackageData = ModuleAssetRegistry->Get().GetAssetPackageData(Asset.PackageName);
		if (!AssetPackageData) continue;
		Size += AssetPackageData->DiskSize;
	}

	return Size;
}

const TArray<FProjectCleanerIndirectAsset>& UProjectCleanerSubsystem::GetIndirectAssetsInfo() const
{
	return IndirectAssetsInfo;
}

const TSet<FString>& UProjectCleanerSubsystem::GetFilesNonEngine() const
{
	return FilesNonEngine;
}

const TSet<FString>& UProjectCleanerSubsystem::GetFilesCorrupted() const
{
	return FilesCorrupted;
}

const TSet<FString>& UProjectCleanerSubsystem::GetFoldersEmpty() const
{
	return FoldersEmpty;
}

bool UProjectCleanerSubsystem::IsFolderEmpty(const FString& InFolderPathAbs) const
{
	return FoldersEmpty.Contains(InFolderPathAbs);
}

bool UProjectCleanerSubsystem::IsFolderExcluded(const FString& InFolderPathAbs) const
{
	if (InFolderPathAbs.IsEmpty()) return false;
	if (!FPaths::DirectoryExists(InFolderPathAbs)) return false;

	for (const auto& ExcludedFolder : GetDefault<UProjectCleanerExcludeSettings>()->ExcludedFolders)
	{
		if (ExcludedFolder.Path.IsEmpty()) continue;

		const FString ExcludedAbsPath = UProjectCleanerLibPath::Convert(ExcludedFolder.Path, EProjectCleanerPathType::Absolute);
		if (!FPaths::DirectoryExists(ExcludedAbsPath)) continue;
		if (UProjectCleanerLibPath::IsUnderFolder(InFolderPathAbs, ExcludedAbsPath))
		{
			return true;
		}
	}

	return false;
}

int64 UProjectCleanerSubsystem::GetSizeTotal(const FString& InFolderPathAbs) const
{
	return GetSizeFor(InFolderPathAbs, AssetsAll);
}

int64 UProjectCleanerSubsystem::GetSizeUnused(const FString& InFolderPathAbs) const
{
	return GetSizeFor(InFolderPathAbs, AssetsUnused);
}

int32 UProjectCleanerSubsystem::GetAssetTotalNum(const FString& InFolderPathAbs) const
{
	return GetNumFor(InFolderPathAbs, AssetsAll);
}

int32 UProjectCleanerSubsystem::GetAssetUnusedNum(const FString& InFolderPathAbs) const
{
	return GetNumFor(InFolderPathAbs, AssetsUnused);
}

int32 UProjectCleanerSubsystem::GetFoldersTotalNum(const FString& InFolderPathAbs) const
{
	if (InFolderPathAbs.IsEmpty()) return 0;
	if (!FPaths::DirectoryExists(InFolderPathAbs)) return 0;

	TArray<FString> AllFolders;
	IFileManager::Get().FindFilesRecursive(AllFolders, *InFolderPathAbs, TEXT("*.*"), false, true);

	int Num = 0;
	for (const auto& Folder : AllFolders)
	{
		if (UProjectCleanerLibPath::IsUnderFolders(Folder, FoldersBlacklisted)) continue;

		++Num;
	}

	return Num;
}

int32 UProjectCleanerSubsystem::GetFoldersEmptyNum(const FString& InFolderPathAbs) const
{
	if (InFolderPathAbs.IsEmpty()) return 0;
	if (!FPaths::DirectoryExists(InFolderPathAbs)) return 0;

	int32 Num = 0;
	for (const auto& EmptyFolder : FoldersEmpty)
	{
		if (EmptyFolder.Equals(InFolderPathAbs)) continue;
		if (UProjectCleanerLibPath::IsUnderFolder(EmptyFolder, InFolderPathAbs))
		{
			++Num;
		}
	}

	return Num;
}

void UProjectCleanerSubsystem::GetSubFolders(const FString& InFolderPathAbs, TSet<FString>& SubFolders) const
{
	TArray<FString> Folders;
	IFileManager::Get().FindFiles(Folders, *(InFolderPathAbs / TEXT("*")), false, true);

	for (const auto& Folder : Folders)
	{
		const FString FolderAbsPath = InFolderPathAbs / Folder;
		if (UProjectCleanerLibPath::IsUnderFolders(FolderAbsPath, FoldersBlacklisted)) continue;

		SubFolders.Add(FolderAbsPath);
	}
}

EProjectCleanerEditorState UProjectCleanerSubsystem::GetEditorState() const
{
	return EditorState;
}

EProjectCleanerScanState UProjectCleanerSubsystem::GetScanState() const
{
	return ScanState;
}

FProjectCleanerDelegateScanFinished& UProjectCleanerSubsystem::OnScanFinished()
{
	return DelegateScanFinished;
}

void UProjectCleanerSubsystem::FixupRedirectors() const
{
	FScopedSlowTask FixRedirectorsTask{
		1.0f,
		FText::FromString(ProjectCleanerConstants::MsgFixingRedirectors)
	};
	FixRedirectorsTask.MakeDialog();

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Emplace(ProjectCleanerConstants::PathRelRoot);
	Filter.ClassNames.Emplace(UObjectRedirector::StaticClass()->GetFName());

	TArray<FAssetData> AssetList;
	ModuleAssetRegistry->Get().GetAssets(Filter, AssetList);

	if (AssetList.Num() == 0) return;

	FScopedSlowTask FixRedirectorsLoadingTask(
		AssetList.Num(),
		FText::FromString(ProjectCleanerConstants::MsgLoadingRedirectors)
	);
	FixRedirectorsLoadingTask.MakeDialog();

	TArray<UObjectRedirector*> Redirectors;
	Redirectors.Reserve(AssetList.Num());

	for (const auto& Asset : AssetList)
	{
		FixRedirectorsLoadingTask.EnterProgressFrame();

		UObject* AssetObj = Asset.GetAsset();
		if (!AssetObj) continue;

		UObjectRedirector* Redirector = CastChecked<UObjectRedirector>(AssetObj);
		if (!Redirector) continue;

		Redirectors.Add(Redirector);
	}

	Redirectors.Shrink();

	ModuleAssetTools->Get().FixupReferencers(Redirectors);

	FixRedirectorsTask.EnterProgressFrame(1.0f);
}

void UProjectCleanerSubsystem::FindAssetsAll()
{
	ModuleAssetRegistry->Get().GetAssetsByPath(ProjectCleanerConstants::PathRelRoot, AssetsAll, true);
}

void UProjectCleanerSubsystem::FindAssetsPrimary()
{
	TArray<FName> PrimaryAssetClasses;

	// getting list of primary asset classes that are defined in AssetManager
	const auto& AssetManager = UAssetManager::Get();
	if (!AssetManager.IsValid()) return;

	TArray<FPrimaryAssetTypeInfo> AssetTypeInfos;
	AssetManager.Get().GetPrimaryAssetTypeInfoList(AssetTypeInfos);
	PrimaryAssetClasses.Reserve(AssetTypeInfos.Num());

	for (const auto& AssetTypeInfo : AssetTypeInfos)
	{
		if (!AssetTypeInfo.AssetBaseClassLoaded) continue;

		PrimaryAssetClasses.AddUnique(AssetTypeInfo.AssetBaseClassLoaded->GetFName());
	}

	// getting list of primary assets classes that are derived from main primary assets
	TSet<FName> DerivedFromPrimaryAssets;
	{
		const TSet<FName> ExcludedClassNames;
		ModuleAssetRegistry->Get().GetDerivedClassNames(PrimaryAssetClasses, ExcludedClassNames, DerivedFromPrimaryAssets);
	}

	for (const auto& DerivedClassName : DerivedFromPrimaryAssets)
	{
		PrimaryAssetClasses.AddUnique(DerivedClassName);
	}

	FARFilter Filter;
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(FName{ProjectCleanerConstants::PathRelRoot});

	for (const auto& ClassName : PrimaryAssetClasses)
	{
		Filter.ClassNames.Add(ClassName);
	}
	ModuleAssetRegistry->Get().GetAssets(Filter, AssetsPrimary);

	FARFilter FilterBlueprint;
	FilterBlueprint.bRecursivePaths = true;
	FilterBlueprint.bRecursiveClasses = true;
	FilterBlueprint.PackagePaths.Add(FName{ProjectCleanerConstants::PathRelRoot});
	FilterBlueprint.ClassNames.Add(UBlueprint::StaticClass()->GetFName());

	TArray<FAssetData> BlueprintAssets;
	ModuleAssetRegistry->Get().GetAssets(FilterBlueprint, BlueprintAssets);

	for (const auto& BlueprintAsset : BlueprintAssets)
	{
		const FName BlueprintClass = FName{*GetAssetClassName(BlueprintAsset)};
		if (PrimaryAssetClasses.Contains(BlueprintClass))
		{
			AssetsPrimary.AddUnique(BlueprintAsset);
		}
	}
}

void UProjectCleanerSubsystem::FindAssetsExcluded()
{
	for (const auto& Asset : AssetsAll)
	{
		// excluded by path
		const FString PackagePathAbs = UProjectCleanerLibPath::Convert(Asset.PackagePath.ToString(), EProjectCleanerPathType::Absolute);
		for (const auto& ExcludedFolder : GetDefault<UProjectCleanerExcludeSettings>()->ExcludedFolders)
		{
			const FString ExcludedFolderPathAbs = UProjectCleanerLibPath::Convert(ExcludedFolder.Path, EProjectCleanerPathType::Absolute);

			if (UProjectCleanerLibPath::IsUnderFolder(PackagePathAbs, ExcludedFolderPathAbs))
			{
				AssetsExcluded.AddUnique(Asset);
			}
		}

		// excluded by class
		const FString AssetClassName = GetAssetClassName(Asset);
		for (const auto& ExcludedClass : GetDefault<UProjectCleanerExcludeSettings>()->ExcludedClasses)
		{
			if (!ExcludedClass.LoadSynchronous()) continue;

			const FString ExcludedClassName = ExcludedClass->GetName();

			if (ExcludedClassName.Equals(AssetClassName))
			{
				AssetsExcluded.AddUnique(Asset);
			}
		}
	}

	for (const auto& ExcludedAsset : GetDefault<UProjectCleanerExcludeSettings>()->ExcludedAssets)
	{
		if (!ExcludedAsset.LoadSynchronous()) continue;

		const FName AssetObjectPath = ExcludedAsset.ToSoftObjectPath().GetAssetPathName();
		const FAssetData AssetData = ModuleAssetRegistry->Get().GetAssetByObjectPath(AssetObjectPath);
		if (!AssetData.IsValid()) continue;

		AssetsExcluded.AddUnique(AssetData);
	}
}

void UProjectCleanerSubsystem::FindAssetsWithExternalRefs()
{
	TArray<FName> Refs;
	for (const auto& Asset : AssetsAll)
	{
		ModuleAssetRegistry->Get().GetReferencers(Asset.PackageName, Refs);

		const bool HasExternalRefs = Refs.ContainsByPredicate([](const FName& Ref)
		{
			return !Ref.ToString().StartsWith(ProjectCleanerConstants::PathRelRoot.ToString());
		});

		if (HasExternalRefs)
		{
			AssetsWithExternalRefs.AddUnique(Asset);
		}

		Refs.Reset();
	}
}

void UProjectCleanerSubsystem::FindAssetsIndirect()
{
	const FString SourceDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + TEXT("Source/"));
	const FString ConfigDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + TEXT("Config/"));
	const FString PluginsDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + TEXT("Plugins/"));

	TSet<FString> Files;
	Files.Reserve(200); // reserving some space

	// 1) finding all source files in main project "Source" directory (<yourproject>/Source/*)
	TArray<FString> FilesToScan;
	PlatformFile->FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".cs"));
	PlatformFile->FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".cpp"));
	PlatformFile->FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".h"));
	PlatformFile->FindFilesRecursively(FilesToScan, *ConfigDir, TEXT(".ini"));
	Files.Append(FilesToScan);

	// 2) we should find all source files in plugins folder (<yourproject>/Plugins/*)
	TArray<FString> ProjectPluginsFiles;
	// finding all installed plugins in "Plugins" directory
	struct FDirectoryVisitor : IPlatformFile::FDirectoryVisitor
	{
		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (bIsDirectory)
			{
				InstalledPlugins.Add(FilenameOrDirectory);
			}

			return true;
		}

		TArray<FString> InstalledPlugins;
	};

	FDirectoryVisitor Visitor;
	PlatformFile->IterateDirectory(*PluginsDir, Visitor);

	// 3) for every installed plugin we scanning only "Source" and "Config" folders
	for (const auto& Dir : Visitor.InstalledPlugins)
	{
		const FString PluginSourcePathDir = Dir + "/Source";
		const FString PluginConfigPathDir = Dir + "/Config";

		PlatformFile->FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".cs"));
		PlatformFile->FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".cpp"));
		PlatformFile->FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".h"));
		PlatformFile->FindFilesRecursively(ProjectPluginsFiles, *PluginConfigPathDir, TEXT(".ini"));
	}

	Files.Append(ProjectPluginsFiles);
	Files.Shrink();

	for (const auto& File : Files)
	{
		if (!PlatformFile->FileExists(*File)) continue;

		FString FileContent;
		FFileHelper::LoadFileToString(FileContent, *File);

		if (!UProjectCleanerLibPath::FileContainsIndirectAssets(FileContent)) continue;

		static FRegexPattern Pattern(TEXT(R"(\/Game([A-Za-z0-9_.\/]+)\b)"));
		FRegexMatcher Matcher(Pattern, FileContent);
		while (Matcher.FindNext())
		{
			FString FoundedAssetObjectPath = Matcher.GetCaptureGroup(0);


			// if ObjectPath ends with "_C" , then its probably blueprint, so we trim that
			if (FoundedAssetObjectPath.EndsWith("_C"))
			{
				FString TrimmedObjectPath = FoundedAssetObjectPath;
				TrimmedObjectPath.RemoveFromEnd("_C");

				FoundedAssetObjectPath = TrimmedObjectPath;
			}


			const FAssetData* AssetData = AssetsAll.FindByPredicate([&](const FAssetData& Elem)
			{
				return
					Elem.ObjectPath.ToString() == (FoundedAssetObjectPath) ||
					Elem.PackageName.ToString() == (FoundedAssetObjectPath);
			});

			if (!AssetData) continue;

			// if founded asset is ok, we loading file by lines to determine on what line its used
			TArray<FString> Lines;
			FFileHelper::LoadFileToStringArray(Lines, *File);
			for (int32 i = 0; i < Lines.Num(); ++i)
			{
				if (!Lines.IsValidIndex(i)) continue;
				if (!Lines[i].Contains(FoundedAssetObjectPath)) continue;

				FProjectCleanerIndirectAsset IndirectAsset;
				IndirectAsset.AssetData = *AssetData;
				IndirectAsset.FilePath = FPaths::ConvertRelativePathToFull(File);
				IndirectAsset.LineNum = i + 1;
				IndirectAssetsInfo.AddUnique(IndirectAsset);
				AssetsIndirect.AddUnique(*AssetData);
			}
		}
	}
}

void UProjectCleanerSubsystem::FindAssetsBlacklisted()
{
	// filling blacklisted folders
	FoldersBlacklisted.Add(UProjectCleanerLibPath::FolderCollections(EProjectCleanerPathType::Absolute));
	FoldersBlacklisted.Add(UProjectCleanerLibPath::FolderDeveloperCollections(EProjectCleanerPathType::Absolute));
	// todo:ashe23 for ue5 add __ExternalObject__ and __ExternalActors__ folders

	if (FModuleManager::Get().IsModuleLoaded(ProjectCleanerConstants::PluginNameMegascans))
	{
		FoldersBlacklisted.Add(UProjectCleanerLibPath::FolderMsPresets(EProjectCleanerPathType::Absolute));
	}

	// if (!ScanSettings->bScanDeveloperContents)
	// {
	// 	FoldersBlacklist.Add(UProjectCleanerLibrary::PathGetDevelopersFolder(true));
	// }

	// filling blacklisted assets
	FARFilter FilterBlacklistAssets;
	FilterBlacklistAssets.bRecursivePaths = true;
	FilterBlacklistAssets.bRecursiveClasses = true;
	FilterBlacklistAssets.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
	FilterBlacklistAssets.ClassNames.Add(UEditorUtilityWidget::StaticClass()->GetFName());
	FilterBlacklistAssets.ClassNames.Add(UEditorUtilityBlueprint::StaticClass()->GetFName());
	FilterBlacklistAssets.ClassNames.Add(UEditorUtilityWidgetBlueprint::StaticClass()->GetFName());
	FilterBlacklistAssets.ClassNames.Add(UMapBuildDataRegistry::StaticClass()->GetFName());
	ModuleAssetRegistry->Get().GetAssets(FilterBlacklistAssets, AssetsBlacklisted);
}

void UProjectCleanerSubsystem::FindAssetsUsed()
{
	AssetsUsed.Reserve(AssetsAll.Num());

	for (const auto& Asset : AssetsPrimary)
	{
		AssetsUsed.AddUnique(Asset);
	}

	for (const auto& Asset : AssetsIndirect)
	{
		AssetsUsed.AddUnique(Asset);
	}

	for (const auto& Asset : AssetsWithExternalRefs)
	{
		AssetsUsed.AddUnique(Asset);
	}

	for (const auto& Asset : AssetsBlacklisted)
	{
		AssetsUsed.AddUnique(Asset);
	}

	for (const auto& Asset : AssetsExcluded)
	{
		AssetsUsed.AddUnique(Asset);
	}

	TArray<FAssetData> LinkedAssets;
	GetLinkedAssets(AssetsUsed, LinkedAssets);
	for (const auto& LinkedAsset : LinkedAssets)
	{
		AssetsUsed.AddUnique(LinkedAsset);
	}
}


void UProjectCleanerSubsystem::FindAssetsUnused()
{
	AssetsUnused.Append(AssetsAll);

	FARFilter Filter;
	Filter.bRecursivePaths = true;

	for (const auto& FolderBlacklist : FoldersBlacklisted)
	{
		const FString FolderPathRel = UProjectCleanerLibPath::Convert(FolderBlacklist, EProjectCleanerPathType::Relative);
		Filter.PackagePaths.AddUnique(FName{*FolderPathRel});
	}

	for (const auto& Asset : AssetsUsed)
	{
		Filter.ObjectPaths.AddUnique(Asset.ObjectPath);
	}

	ModuleAssetRegistry->Get().UseFilterToExcludeAssets(AssetsUnused, Filter);
}

void UProjectCleanerSubsystem::ScanContentFolder()
{
	class FContentFolderVisitor final : public IPlatformFile::FDirectoryVisitor
	{
	public:
		TSet<FString>& FoldersEmpty;
		TSet<FString>& FilesCorrupted;
		TSet<FString>& FilesNonEngine;
		const TSet<FString>& FoldersBlacklist;

		FContentFolderVisitor(
			TSet<FString>& InFoldersEmpty,
			TSet<FString>& InFilesCorrupted,
			TSet<FString>& InFilesNonEngine,
			const TSet<FString>& InFoldersBlacklist
		)
			: FDirectoryVisitor(EDirectoryVisitorFlags::None),
			  FoldersEmpty(InFoldersEmpty),
			  FilesCorrupted(InFilesCorrupted),
			  FilesNonEngine(InFilesNonEngine),
			  FoldersBlacklist(InFoldersBlacklist)
		{
		}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			const FString FullPath = FPaths::ConvertRelativePathToFull(FilenameOrDirectory);

			if (bIsDirectory)
			{
				if (UProjectCleanerLibPath::IsUnderFolders(FullPath, FoldersBlacklist)) return true;

				TArray<FString> Files;
				IFileManager::Get().FindFilesRecursive(Files, *FullPath, TEXT("*.*"), true, false);

				if (Files.Num() == 0)
				{
					FoldersEmpty.Add(FullPath);
				}

				return true;
			}

			FString FileName;
			FString FileExtension;
			FString Path;
			FPaths::Split(FullPath, Path, FileName, FileExtension);

			if (UProjectCleanerLibPath::IsUnderFolders(Path, FoldersBlacklist)) return true;

			if (UProjectCleanerLibPath::FileHasEngineExtension(FileExtension))
			{
				if (UProjectCleanerLibPath::FileIsCorrupted(FullPath))
				{
					FilesCorrupted.Add(FullPath);
				}
			}
			else
			{
				FilesNonEngine.Add(FullPath);
			}

			return true;
		}
	};

	FContentFolderVisitor ContentFolderVisitor{
		FoldersEmpty,
		FilesCorrupted,
		FilesNonEngine,
		FoldersBlacklisted
	};
	PlatformFile->IterateDirectoryRecursively(*FPaths::ProjectContentDir(), ContentFolderVisitor);
}

void UProjectCleanerSubsystem::ContainersReset()
{
	AssetsAll.Reset();
	AssetsPrimary.Reset();
	AssetsExcluded.Reset();
	AssetsUsed.Reset();
	AssetsUnused.Reset();
	AssetsIndirect.Reset();
	AssetsBlacklisted.Reset();
	IndirectAssetsInfo.Reset();
	AssetsWithExternalRefs.Reset();

	FilesNonEngine.Reset();
	FilesCorrupted.Reset();

	FoldersEmpty.Reset();
	FoldersBlacklisted.Reset();
}

void UProjectCleanerSubsystem::ContainersShrink()
{
	AssetsAll.Shrink();
	AssetsPrimary.Shrink();
	AssetsExcluded.Shrink();
	AssetsUsed.Shrink();
	AssetsUnused.Shrink();
	AssetsIndirect.Shrink();
	AssetsBlacklisted.Shrink();
	IndirectAssetsInfo.Shrink();
	AssetsWithExternalRefs.Shrink();

	FilesNonEngine.Shrink();
	FilesCorrupted.Shrink();

	FoldersEmpty.Shrink();
	FoldersBlacklisted.Shrink();
}

void UProjectCleanerSubsystem::ContainersEmpty()
{
	AssetsAll.Empty();
	AssetsPrimary.Empty();
	AssetsExcluded.Empty();
	AssetsUsed.Empty();
	AssetsUnused.Empty();
	AssetsIndirect.Empty();
	AssetsBlacklisted.Empty();
	IndirectAssetsInfo.Empty();
	AssetsWithExternalRefs.Empty();

	FilesNonEngine.Empty();
	FilesCorrupted.Empty();

	FoldersEmpty.Empty();
	FoldersBlacklisted.Empty();
}

int32 UProjectCleanerSubsystem::GetNumFor(const FString& InFolderPathAbs, const TArray<FAssetData>& Assets) const
{
	if (InFolderPathAbs.IsEmpty()) return 0;
	if (!FPaths::DirectoryExists(InFolderPathAbs)) return 0;

	int32 Num = 0;
	for (const auto& Asset : Assets)
	{
		const FString AssetPackagePathAbs = UProjectCleanerLibPath::Convert(Asset.PackagePath.ToString(), EProjectCleanerPathType::Absolute);
		if (!UProjectCleanerLibPath::IsUnderFolder(AssetPackagePathAbs, InFolderPathAbs)) continue;

		++Num;
	}

	return Num;
}

int64 UProjectCleanerSubsystem::GetSizeFor(const FString& InFolderPathAbs, const TArray<FAssetData>& Assets) const
{
	if (InFolderPathAbs.IsEmpty()) return 0;
	if (!FPaths::DirectoryExists(InFolderPathAbs)) return 0;

	int64 Size = 0;
	for (const auto& Asset : Assets)
	{
		const FString AssetPackagePathAbs = UProjectCleanerLibPath::Convert(Asset.PackagePath.ToString(), EProjectCleanerPathType::Absolute);
		if (!UProjectCleanerLibPath::IsUnderFolder(AssetPackagePathAbs, InFolderPathAbs)) continue;

		const auto AssetPackageData = ModuleAssetRegistry->Get().GetAssetPackageData(Asset.PackageName);
		if (!AssetPackageData) continue;
		Size += AssetPackageData->DiskSize;
	}

	return Size;
}
