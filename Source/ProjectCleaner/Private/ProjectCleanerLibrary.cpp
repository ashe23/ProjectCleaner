// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleaner/Public/ProjectCleanerLibrary.h"
#include "ProjectCleanerConstants.h"
// Engine Headers
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "EditorUtilityBlueprint.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "IContentBrowserSingleton.h"
#include "FileHelpers.h"
#include "ProjectCleanerScanSettings.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"
#include "Engine/MapBuildDataRegistry.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"

bool UProjectCleanerLibrary::IsAssetRegistryWorking()
{
	return FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get().IsLoadingAssets();
}

bool UProjectCleanerLibrary::IsUnderFolder(const FString& InFolderPathAbs, const FString& RootFolder)
{
	if (InFolderPathAbs.IsEmpty() || RootFolder.IsEmpty()) return false;

	return InFolderPathAbs.Equals(RootFolder) || FPaths::IsUnderDirectory(InFolderPathAbs, RootFolder);
}

bool UProjectCleanerLibrary::IsUnderForbiddenFolders(const FString& InFolderPathAbs, const TSet<FString>& ForbiddenFolders)
{
	if (InFolderPathAbs.IsEmpty() || ForbiddenFolders.Num() == 0) return false;

	for (const auto& ForbiddenFolder : ForbiddenFolders)
	{
		if (IsUnderFolder(InFolderPathAbs, ForbiddenFolder)) return true;
	}

	return false;
}

bool UProjectCleanerLibrary::IsEngineFileExtension(const FString& Extension)
{
	TSet<FString> EngineExtensions;
	EngineExtensions.Reserve(3);
	EngineExtensions.Add(TEXT("uasset"));
	EngineExtensions.Add(TEXT("umap"));
	EngineExtensions.Add(TEXT("collection"));

	return EngineExtensions.Contains(Extension.ToLower());
}

bool UProjectCleanerLibrary::IsCorruptedEngineFile(const FString& InFilePathAbs)
{
	// here we got absolute path "C:/MyProject/Content/material.uasset"
	// we must first convert that path to In Engine Internal Path like "/Game/material.uasset"
	const FString RelativePath = PathConvertToRel(InFilePathAbs);
	// Converting file path to object path (This is for searching in AssetRegistry)
	// example "/Game/Name.uasset" => "/Game/Name.Name"
	FString ObjectPath = RelativePath;
	ObjectPath.RemoveFromEnd(FPaths::GetExtension(RelativePath, true));
	ObjectPath.Append(TEXT(".") + FPaths::GetBaseFilename(RelativePath));

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	const FAssetData AssetData = ModuleAssetRegistry.Get().GetAssetByObjectPath(FName{*ObjectPath});

	// if its does not exist in asset registry, then something wrong with asset
	return !AssetData.IsValid();
}

// === REFACTOR===
void UProjectCleanerLibrary::GetSubFolders(const FString& InDirPath, const bool bRecursive, TSet<FString>& SubFolders)
{
	if (InDirPath.IsEmpty()) return;
	if (!FPaths::DirectoryExists(InDirPath)) return;

	SubFolders.Reset();

	class FFindFoldersVisitor final : public IPlatformFile::FDirectoryVisitor
	{
	public:
		TSet<FString>& Folders;

		explicit FFindFoldersVisitor(TSet<FString>& InFolders) : FDirectoryVisitor(EDirectoryVisitorFlags::None), Folders(InFolders)
		{
		}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (bIsDirectory)
			{
				Folders.Add(FilenameOrDirectory);
			}
			return true;
		}
	};

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FFindFoldersVisitor FindFoldersVisitor(SubFolders);

	if (bRecursive)
	{
		PlatformFile.IterateDirectoryRecursively(*InDirPath, FindFoldersVisitor);
	}
	else
	{
		PlatformFile.IterateDirectory(*InDirPath, FindFoldersVisitor);
	}
}

int32 UProjectCleanerLibrary::GetSubFoldersNum(const FString& InDirPath, const bool bRecursive)
{
	TSet<FString> SubFolders;
	GetSubFolders(InDirPath, bRecursive, SubFolders);

	return SubFolders.Num();
}

void UProjectCleanerLibrary::GetEmptyFolders(const FString& InDirPath, TSet<FString>& EmptyFolders)
{
	EmptyFolders.Reset();

	class FFindEmptyFoldersVisitor final : public IPlatformFile::FDirectoryVisitor
	{
	public:
		TSet<FString>& EmptyFolders;

		explicit FFindEmptyFoldersVisitor(TSet<FString>& InEmptyFolders) : FDirectoryVisitor(EDirectoryVisitorFlags::None), EmptyFolders(InEmptyFolders)
		{
		}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			const FString FullPath = FPaths::ConvertRelativePathToFull(FilenameOrDirectory);

			if (bIsDirectory)
			{
				TArray<FString> Files;
				IFileManager::Get().FindFilesRecursive(Files, *FullPath, TEXT("*.*"), true, false);

				if (Files.Num() == 0)
				{
					EmptyFolders.Add(FullPath);
				}
			}

			return true;
		}
	};

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FFindEmptyFoldersVisitor EmptyFoldersVisitor(EmptyFolders);
	PlatformFile.IterateDirectoryRecursively(*InDirPath, EmptyFoldersVisitor);
}

int32 UProjectCleanerLibrary::GetEmptyFoldersNum(const FString& InDirPath)
{
	TSet<FString> EmptyFolders;
	GetEmptyFolders(InDirPath, EmptyFolders);

	return EmptyFolders.Num();
}

bool UProjectCleanerLibrary::IsEmptyFolder(const FString& InDirPath)
{
	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *InDirPath,TEXT("*.*"), true, false);

	return Files.Num() == 0;
}

void UProjectCleanerLibrary::GetNonEngineFiles(TSet<FString>& FilesNonEngine)
{
	FilesNonEngine.Reset();

	class FFindNonEngineFilesVisitor final : public IPlatformFile::FDirectoryVisitor
	{
	public:
		TSet<FString>& Files;

		explicit FFindNonEngineFilesVisitor(TSet<FString>& InFiles) : FDirectoryVisitor(EDirectoryVisitorFlags::None), Files(InFiles)
		{
		}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			const FString FullPath = FPaths::ConvertRelativePathToFull(FilenameOrDirectory);

			if (!bIsDirectory && !IsEngineFileExtension(FPaths::GetExtension(FullPath, false)))
			{
				Files.Add(FullPath);
			}


			return true;
		}
	};

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FFindNonEngineFilesVisitor FindNonEngineFilesVisitor(FilesNonEngine);
	PlatformFile.IterateDirectoryRecursively(*FPaths::ProjectContentDir(), FindNonEngineFilesVisitor);
}

void UProjectCleanerLibrary::GetForbiddenFolders(TSet<FString>& ForbiddenFolders)
{
	ForbiddenFolders.Reset();

	// following folders will never be scanned nor deleted
	ForbiddenFolders.Add(FPaths::ProjectContentDir() / TEXT("Collections"));
	ForbiddenFolders.Add(FPaths::GameUserDeveloperDir() / TEXT("Collections"));

	const UProjectCleanerScanSettings* ScanSettings = GetDefault<UProjectCleanerScanSettings>();
	if (!ScanSettings) return;

	if (!ScanSettings->bScanDeveloperContents)
	{
		ForbiddenFolders.Add(FPaths::ProjectContentDir() / TEXT("Developers"));
	}
}

void UProjectCleanerLibrary::GetAssetsByPath(const FString& InDirPathAbs, const bool bRecursive, TArray<FAssetData>& Assets)
{
	if (InDirPathAbs.IsEmpty()) return;
	if (!FPaths::DirectoryExists(InDirPathAbs)) return;

	Assets.Reset();

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	ModuleAssetRegistry.Get().GetAssetsByPath(FName{*PathConvertToRel(InDirPathAbs)}, Assets, bRecursive);
	TSet<FString> ForbiddenFolders;
	GetForbiddenFolders(ForbiddenFolders);

	FARFilter Filter;
	Filter.bRecursivePaths = true;

	for (const auto& ForbiddenFolder : ForbiddenFolders)
	{
		Filter.PackagePaths.Add(FName{*PathConvertToRel(ForbiddenFolder)});
	}

	ModuleAssetRegistry.Get().UseFilterToExcludeAssets(Assets, Filter);
}

int32 UProjectCleanerLibrary::GetAssetsByPathNum(const FString& InDirPathAbs, const bool bRecursive, TArray<FAssetData>& Assets)
{
	GetAssetsByPath(InDirPathAbs, bRecursive, Assets);

	return Assets.Num();
}

void UProjectCleanerLibrary::GetAssetsCorrupted(TSet<FString>& FilesCorrupted)
{
	FilesCorrupted.Reset();

	class FFindCorruptedFilesVisitor final : public IPlatformFile::FDirectoryVisitor
	{
	public:
		TSet<FString>& CorruptedFiles;

		explicit FFindCorruptedFilesVisitor(TSet<FString>& InCorruptedFiles)
			: FDirectoryVisitor(EDirectoryVisitorFlags::None),
			  CorruptedFiles(InCorruptedFiles)
		{
		}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (!bIsDirectory)
			{
				const FString FullPath = FPaths::ConvertRelativePathToFull(FilenameOrDirectory);
				if (IsEngineFileExtension(FPaths::GetExtension(FullPath, false)))
				{
					// here we got absolute path "C:/MyProject/Content/material.uasset"
					// we must first convert that path to In Engine Internal Path like "/Game/material.uasset"
					const FString InternalFilePath = PathConvertToRel(FullPath);
					// Converting file path to object path (This is for searching in AssetRegistry)
					// example "/Game/Name.uasset" => "/Game/Name.Name"
					FString ObjectPath = InternalFilePath;
					ObjectPath.RemoveFromEnd(FPaths::GetExtension(InternalFilePath, true));
					ObjectPath.Append(TEXT(".") + FPaths::GetBaseFilename(InternalFilePath));

					const FName ObjectPathName = FName{*ObjectPath};

					const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

					// if its does not exist in asset registry, then something wrong with asset
					const FAssetData AssetData = ModuleAssetRegistry.Get().GetAssetByObjectPath(ObjectPathName);
					if (!AssetData.IsValid())
					{
						CorruptedFiles.Add(FullPath);
					}
				}
			}

			return true;
		}
	};

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FFindCorruptedFilesVisitor FindCorruptedFilesVisitor(FilesCorrupted);
	PlatformFile.IterateDirectoryRecursively(*FPaths::ProjectContentDir(), FindCorruptedFilesVisitor);
}

void UProjectCleanerLibrary::GetAssetsWithExternalRefs(TArray<FAssetData>& Assets)
{
	Assets.Reset();

	TArray<FAssetData> AllAssets;

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	ModuleAssetRegistry.Get().GetAssetsByPath(FName{ProjectCleanerConstants::PathRelRoot}, AllAssets, true);

	TArray<FName> Refs;
	for (const auto& Asset : AllAssets)
	{
		ModuleAssetRegistry.Get().GetReferencers(Asset.PackageName, Refs);

		const bool HasExternalRefs = Refs.ContainsByPredicate([](const FName& Ref)
		{
			return !Ref.ToString().StartsWith(ProjectCleanerConstants::PathRelRoot.ToString());
		});

		if (HasExternalRefs)
		{
			Assets.AddUnique(Asset);
		}

		Refs.Reset();
	}
}

void UProjectCleanerLibrary::GetPrimaryAssetClasses(TArray<FName>& PrimaryAssetClasses, const bool bIncludeDerivedClasses)
{
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	PrimaryAssetClasses.Reset();

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

	if (bIncludeDerivedClasses)
	{
		// getting list of primary assets classes that are derived from main primary assets
		TSet<FName> DerivedFromPrimaryAssets;
		{
			const TSet<FName> ExcludedClassNames;
			ModuleAssetRegistry.Get().GetDerivedClassNames(PrimaryAssetClasses, ExcludedClassNames, DerivedFromPrimaryAssets);
		}

		for (const auto& DerivedClassName : DerivedFromPrimaryAssets)
		{
			PrimaryAssetClasses.AddUnique(DerivedClassName);
		}
	}
}

void UProjectCleanerLibrary::GetLinkedAssets(const TArray<FAssetData>& Assets, TArray<FAssetData>& LinkedAssets)
{
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

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

			ModuleAssetRegistry.Get().GetDependencies(CurrentPackageName, Deps);

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

	ModuleAssetRegistry.Get().GetAssets(Filter, LinkedAssets);
}

void UProjectCleanerLibrary::GetAssetsUsed(TArray<FAssetData>& AssetsUsed)
{
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	const UProjectCleanerScanSettings* ScanSettings = GetDefault<UProjectCleanerScanSettings>();
	if (!ScanSettings) return;

	AssetsUsed.Reset();

	TArray<FAssetData> AssetsPrimary;
	TArray<FAssetData> AssetsIndirect;
	TArray<FAssetData> AssetsWithExternalRefs;
	TArray<FAssetData> AssetsInDeveloperFolder;
	TArray<FAssetData> AssetsInMSPresets;
	TArray<FAssetData> AssetsBlackListed;

	GetAssetsPrimary(AssetsPrimary, true);
	GetAssetsIndirect(AssetsIndirect);
	GetAssetsWithExternalRefs(AssetsWithExternalRefs);
	// todo:ashe23 excluded assets
	// todo:ashe23 Megascans plugin assets
	// todo:ashe23 for ue5 add __ExternalObject__ and __ExternalActors__ folders

	// finding all blacklisted assets that are considered used
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	Filter.PackagePaths.Add(FName{ProjectCleanerConstants::PathRelRoot});
	Filter.ClassNames.Add(UEditorUtilityWidget::StaticClass()->GetFName());
	Filter.ClassNames.Add(UEditorUtilityBlueprint::StaticClass()->GetFName());
	Filter.ClassNames.Add(UEditorUtilityWidgetBlueprint::StaticClass()->GetFName());
	Filter.ClassNames.Add(UMapBuildDataRegistry::StaticClass()->GetFName());
	ModuleAssetRegistry.Get().GetAssets(Filter, AssetsBlackListed);

	if (!ScanSettings->bScanDeveloperContents)
	{
		ModuleAssetRegistry.Get().GetAssetsByPath(ProjectCleanerConstants::PathRelDevelopers, AssetsInDeveloperFolder, true);
	}

	if (FModuleManager::Get().IsModuleLoaded(ProjectCleanerConstants::PluginMegascans))
	{
		ModuleAssetRegistry.Get().GetAssetsByPath(ProjectCleanerConstants::PathRelMegascansPresets, AssetsInMSPresets, true);
	}

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
	for (const auto& Asset : AssetsInDeveloperFolder)
	{
		AssetsUsed.AddUnique(Asset);
	}
	for (const auto& Asset : AssetsInMSPresets)
	{
		AssetsUsed.AddUnique(Asset);
	}
	for (const auto& Asset : AssetsBlackListed)
	{
		AssetsUsed.AddUnique(Asset);
	}

	TArray<FAssetData> LinkedAssets;
	GetLinkedAssets(AssetsUsed, LinkedAssets);

	for (const auto& Asset : LinkedAssets)
	{
		AssetsUsed.AddUnique(Asset);
	}
}

void UProjectCleanerLibrary::GetAssetsUnused(TArray<FAssetData>& AssetsUnused)
{
	AssetsUnused.Reset();

	TArray<FAssetData> UsedAssets;
	GetAssetsUsed(UsedAssets);

	TArray<FAssetData> AllAssets;
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	ModuleAssetRegistry.Get().GetAssetsByPath(ProjectCleanerConstants::PathRelRoot, AllAssets, true);

	AssetsUnused.Reserve(UsedAssets.Num());

	for (const auto& Asset : AllAssets)
	{
		if (UsedAssets.Contains(Asset)) continue;

		AssetsUnused.AddUnique(Asset);
	}

	AssetsUnused.Shrink();
}

void UProjectCleanerLibrary::GetAssetsIndirect(TArray<FAssetData>& AssetsIndirect)
{
	AssetsIndirect.Reset();

	TArray<FProjectCleanerIndirectAsset> IndirectAssets;
	GetAssetsIndirectAdvanced(IndirectAssets);

	AssetsIndirect.Reserve(IndirectAssets.Num());
	for (const auto& Info : IndirectAssets)
	{
		AssetsIndirect.AddUnique(Info.AssetData);
	}
}

void UProjectCleanerLibrary::GetAssetsIndirectAdvanced(TArray<FProjectCleanerIndirectAsset>& AssetsIndirect)
{
	AssetsIndirect.Reset();

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	const FString SourceDir = FPaths::ProjectDir() + TEXT("Source/");
	const FString ConfigDir = FPaths::ProjectDir() + TEXT("Config/");
	const FString PluginsDir = FPaths::ProjectDir() + TEXT("Plugins/");

	TSet<FString> Files;
	Files.Reserve(200); // reserving some space

	// 1) finding all source files in main project "Source" directory (<yourproject>/Source/*)
	TArray<FString> FilesToScan;
	PlatformFile.FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".cs"));
	PlatformFile.FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".cpp"));
	PlatformFile.FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".h"));
	PlatformFile.FindFilesRecursively(FilesToScan, *ConfigDir, TEXT(".ini"));
	Files.Append(FilesToScan);

	// 2) we should find all source files in plugins folder (<yourproject>/Plugins/*)
	TArray<FString> ProjectPluginsFiles;
	// finding all installed plugins in "Plugins" directory
	struct DirectoryVisitor : public IPlatformFile::FDirectoryVisitor
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

	DirectoryVisitor Visitor;
	PlatformFile.IterateDirectory(*PluginsDir, Visitor);

	// 3) for every installed plugin we scanning only "Source" and "Config" folders
	for (const auto& Dir : Visitor.InstalledPlugins)
	{
		const FString PluginSourcePathDir = Dir + "/Source";
		const FString PluginConfigPathDir = Dir + "/Config";

		PlatformFile.FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".cs"));
		PlatformFile.FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".cpp"));
		PlatformFile.FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".h"));
		PlatformFile.FindFilesRecursively(ProjectPluginsFiles, *PluginConfigPathDir, TEXT(".ini"));
	}

	Files.Append(ProjectPluginsFiles);
	Files.Shrink();

	TArray<FAssetData> AllAssets;
	AllAssets.Reserve(ModuleAssetRegistry.Get().GetAllocatedSize());
	ModuleAssetRegistry.Get().GetAssetsByPath(ProjectCleanerConstants::PathRelRoot, AllAssets, true);

	for (const auto& File : Files)
	{
		if (!PlatformFile.FileExists(*File)) continue;

		FString FileContent;
		FFileHelper::LoadFileToString(FileContent, *File);

		if (!HasIndirectlyUsedAssets(FileContent)) continue;

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


			const FAssetData* AssetData = AllAssets.FindByPredicate([&](const FAssetData& Elem)
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
				AssetsIndirect.AddUnique(IndirectAsset);
			}
		}
	}
}

void UProjectCleanerLibrary::GetAssetsPrimary(TArray<FAssetData>& AssetsPrimary, const bool bIncludeDerivedClasses)
{
	AssetsPrimary.Reset();

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	TArray<FName> PrimaryAssetClasses;
	GetPrimaryAssetClasses(PrimaryAssetClasses, bIncludeDerivedClasses);

	FARFilter Filter;
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(FName{ProjectCleanerConstants::PathRelRoot});

	for (const auto& ClassName : PrimaryAssetClasses)
	{
		Filter.ClassNames.Add(ClassName);
	}
	ModuleAssetRegistry.Get().GetAssets(Filter, AssetsPrimary);

	FARFilter FilterBlueprint;
	FilterBlueprint.bRecursivePaths = true;
	FilterBlueprint.bRecursiveClasses = true;
	FilterBlueprint.PackagePaths.Add(FName{ProjectCleanerConstants::PathRelRoot});
	FilterBlueprint.ClassNames.Add(UBlueprint::StaticClass()->GetFName());

	TArray<FAssetData> BlueprintAssets;
	ModuleAssetRegistry.Get().GetAssets(FilterBlueprint, BlueprintAssets);

	for (const auto& BlueprintAsset : BlueprintAssets)
	{
		const FName BlueprintClass = FName{*GetAssetClassName(BlueprintAsset)};
		if (PrimaryAssetClasses.Contains(BlueprintClass))
		{
			AssetsPrimary.AddUnique(BlueprintAsset);
		}
	}
}

int64 UProjectCleanerLibrary::GetAssetsTotalSize(const TArray<FAssetData>& Assets)
{
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	int64 Size = 0;
	for (const auto& Asset : Assets)
	{
		const auto AssetPackageData = ModuleAssetRegistry.Get().GetAssetPackageData(Asset.PackageName);
		if (!AssetPackageData) continue;
		Size += AssetPackageData->DiskSize;
	}

	return Size;
}

FString UProjectCleanerLibrary::PathConvertToAbs(const FString& InRelPath)
{
	FString Path = InRelPath;
	FPaths::NormalizeFilename(Path);
	const FString ProjectContentDirAbsPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
	return ConvertPathInternal(FString{"/Game/"}, ProjectContentDirAbsPath, Path);
}

FString UProjectCleanerLibrary::PathConvertToRel(const FString& InAbsPath)
{
	FString Path = InAbsPath;
	FPaths::NormalizeFilename(Path);
	const FString ProjectContentDirAbsPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
	return ConvertPathInternal(ProjectContentDirAbsPath, FString{"/Game/"}, Path);
}

FString UProjectCleanerLibrary::GetAssetClassName(const FAssetData& AssetData)
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

void UProjectCleanerLibrary::FixupRedirectors()
{
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	const FAssetToolsModule& ModuleAssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));

	FScopedSlowTask FixRedirectorsTask{
		1.0f,
		FText::FromString(ProjectCleanerConstants::MsgFixingRedirectors)
	};
	FixRedirectorsTask.MakeDialog();

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Emplace(ProjectCleanerConstants::PathRelRoot);
	Filter.ClassNames.Emplace(UObjectRedirector::StaticClass()->GetFName());

	// Getting all redirectors in given path
	TArray<FAssetData> AssetList;
	ModuleAssetRegistry.Get().GetAssets(Filter, AssetList);

	if (AssetList.Num() > 0)
	{
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

			const auto Redirector = CastChecked<UObjectRedirector>(AssetObj);
			if (!Redirector) continue;

			Redirectors.Add(Redirector);
		}

		Redirectors.Shrink();

		// Fix up all founded redirectors
		ModuleAssetTools.Get().FixupReferencers(Redirectors);
	}

	FixRedirectorsTask.EnterProgressFrame(1.0f);
}

void UProjectCleanerLibrary::SaveAllAssets(const bool bPromptUser)
{
	FEditorFileUtils::SaveDirtyPackages(
		bPromptUser,
		true,
		true,
		false,
		false,
		false
	);
}

void UProjectCleanerLibrary::UpdateAssetRegistry(const bool bSyncScan)
{
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	TArray<FString> ScanFolders;
	ScanFolders.Add(ProjectCleanerConstants::PathRelRoot.ToString());

	ModuleAssetRegistry.Get().ScanPathsSynchronous(ScanFolders, true);
	ModuleAssetRegistry.Get().SearchAllAssets(bSyncScan);
}

void UProjectCleanerLibrary::FocusOnDirectory(const FString& InRelPath)
{
	if (InRelPath.IsEmpty()) return;

	TArray<FString> FocusFolders;
	FocusFolders.Add(InRelPath);

	const FContentBrowserModule& ModuleContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ModuleContentBrowser.Get().SyncBrowserToFolders(FocusFolders);
}

bool UProjectCleanerLibrary::HasIndirectlyUsedAssets(const FString& FileContent)
{
	if (FileContent.IsEmpty()) return false;

	// search any sub string that has asset package path in it
	static FRegexPattern Pattern(TEXT(R"(\/Game([A-Za-z0-9_.\/]+)\b)"));
	FRegexMatcher Matcher(Pattern, FileContent);
	return Matcher.FindNext();
}

FString UProjectCleanerLibrary::ConvertPathInternal(const FString& From, const FString& To, const FString& Path)
{
	return Path.Replace(*From, *To, ESearchCase::IgnoreCase);
}
