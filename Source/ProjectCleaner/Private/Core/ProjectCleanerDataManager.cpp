// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "Core/ProjectCleanerDataManager.h"
#include "Core/ProjectCleanerUtility.h"
// Engine Headers
#include "Settings/ContentBrowserSettings.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/World.h"
#include "Engine/AssetManager.h"
#include "Engine/AssetManagerSettings.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/PlatformFilemanager.h"
#include "Internationalization/Regex.h"


void ProjectCleanerDataManagerV2::GetAllAssetsByPath(const FName& InPath, TArray<FAssetData>& AllAssets)
{
	const auto& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistry.Get().GetAssetsByPath(InPath, AllAssets, true);
}

void ProjectCleanerDataManagerV2::GetInvalidFilesByPath(
	const FString& InPath,
	const TArray<FAssetData>& AllAssets,
	TSet<FName>& CorruptedAssets,
	TSet<FName>& NonEngineFiles)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*InPath)) return;

	struct ProjectCleanerDirVisitor : IPlatformFile::FDirectoryVisitor
	{
		ProjectCleanerDirVisitor(
			const TArray<FAssetData>& Assets,
			TSet<FName>& NewCorruptedAssets,
			TSet<FName>& NewNonEngineFiles
		) :
		AllAssets(Assets),
		CorruptedAssets(NewCorruptedAssets),
		NonEngineFiles(NewNonEngineFiles) {}
		
		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			const FString FullPath = FPaths::ConvertRelativePathToFull(FilenameOrDirectory);
			if (!bIsDirectory)
			{
				if (ProjectCleanerUtility::IsEngineExtension(FPaths::GetExtension(FullPath, false)))
				{
					// here we got absolute path "C:/MyProject/Content/material.uasset"
					// we must first convert that path to In Engine Internal Path like "/Game/material.uasset"
					const FString InternalFilePath = ProjectCleanerUtility::ConvertAbsolutePathToInternal(FullPath);
					// Converting file path to object path (This is for searching in AssetRegistry)
					// example "/Game/Name.uasset" => "/Game/Name.Name"
					FString ObjectPath = InternalFilePath;
					ObjectPath.RemoveFromEnd(FPaths::GetExtension(InternalFilePath, true));
					ObjectPath.Append(TEXT(".") + FPaths::GetBaseFilename(InternalFilePath));

					const FName ObjectPathName = FName{*ObjectPath};
					const bool IsInAssetRegistry = AllAssets.ContainsByPredicate([&] (const FAssetData& Elem)
					{
						return Elem.ObjectPath.IsEqual(ObjectPathName);
					});
					if (!IsInAssetRegistry)
					{
						CorruptedAssets.Add(ObjectPathName);
					}
				}
				else
				{
					NonEngineFiles.Add(FName{FullPath});
				}
			}

			return true;
		}
		const TArray<FAssetData>& AllAssets;
		TSet<FName>& CorruptedAssets;
		TSet<FName>& NonEngineFiles;
	};

	ProjectCleanerDirVisitor Visitor{AllAssets, CorruptedAssets, NonEngineFiles};
	FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryRecursively(*InPath, Visitor);
}

void ProjectCleanerDataManagerV2::GetIndirectAssetsByPath(const FString& InPath, TMap<FName, FIndirectAsset>& IndirectlyUsedAssets)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	if (!PlatformFile.DirectoryExists(*InPath)) return;

	const FString SourceDir = InPath + TEXT("Source/");
	const FString ConfigDir = InPath + TEXT("Config/");
	const FString PluginsDir = InPath + TEXT("Plugins/");
	
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

	for (const auto& File : Files)
	{
		if (!PlatformFile.FileExists(*File)) continue;
	
		FString FileContent;
		FFileHelper::LoadFileToString(FileContent, *File);
	
		// search any sub string that has asset package path in it
		static FRegexPattern Pattern(TEXT(R"(\/Game(.*)\b)"));
		FRegexMatcher Matcher(Pattern, FileContent);
		while (Matcher.FindNext())
		{
			const FName FoundedAssetPackageName =  FName{Matcher.GetCaptureGroup(0)};
			if (!FoundedAssetPackageName.IsValid()) continue;

			// if founded asset is ok, we loading file by lines to determine on what line its used
			TArray<FString> Lines;
			FFileHelper::LoadFileToStringArray(Lines, *File);
			for (int32 i = 0; i < Lines.Num(); ++i)
			{
				if (!Lines.IsValidIndex(i)) continue;
				if (!Lines[i].Contains(FoundedAssetPackageName.ToString())) continue;
			
				FIndirectAsset IndirectAsset;
				IndirectAsset.File = FPaths::ConvertRelativePathToFull(File);
				IndirectAsset.Line = i + 1;
				IndirectlyUsedAssets.Add(FoundedAssetPackageName, IndirectAsset);
			}
		}
	}
}

ProjectCleanerDataManager::ProjectCleanerDataManager() :
	TotalProjectSize(0),
	TotalUnusedAssetsSize(0),
	AssetRegistry(nullptr),
	DirectoryWatcher(nullptr),
	CleanerConfigs(nullptr),
	ExcludeOptions(nullptr),
	ContentBrowserSettings(nullptr)
{
	AssetRegistry = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	DirectoryWatcher = &FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));

	CleanerConfigs = GetMutableDefault<UCleanerConfigs>();
	ExcludeOptions = GetMutableDefault<UExcludeOptions>();
	ContentBrowserSettings = GetMutableDefault<UContentBrowserSettings>();

	ensure(CleanerConfigs);
	ensure(ExcludeOptions);
	ensure(AssetRegistry);
	ensure(DirectoryWatcher);

	
	ContentDir_Absolute = FPaths::ProjectContentDir();
	ContentDir_Relative = TEXT("/Game");
}

const TArray<FAssetData>& ProjectCleanerDataManager::GetAllAssets() const
{
	return AllAssets;
}

const TSet<FName>& ProjectCleanerDataManager::GetUsedAssets() const
{
	return UsedAssets;
}

const TArray<FAssetData>& ProjectCleanerDataManager::GetUnusedAssets() const
{
	return UnusedAssets;
}

const TSet<FName>& ProjectCleanerDataManager::GetAllFiles() const
{
	return AllFiles;
}

const TSet<FName>& ProjectCleanerDataManager::GetNonEngineFiles() const
{
	return NonEngineFiles;
}

const TSet<FName>& ProjectCleanerDataManager::GetCorruptedAssets() const
{
	return CorruptedAssets;
}

const TSet<FName>& ProjectCleanerDataManager::GetEmptyFolders() const
{
	return EmptyFolders;
}

const TSet<FName>& ProjectCleanerDataManager::GetPrimaryAssetClasses() const
{
	return PrimaryAssetClasses;
}

const TMap<FAssetData, FIndirectAsset>& ProjectCleanerDataManager::GetIndirectlyUsedAssets() const
{
	return IndirectlyUsedAssets;
}

const TArray<FAssetData>& ProjectCleanerDataManager::GetUserExcludedAssets() const
{
	return UserExcludedAssets;
}

const TArray<FAssetData>& ProjectCleanerDataManager::GetExcludedAssets() const
{
	return ExcludedAssets;
}

const TArray<FAssetData>& ProjectCleanerDataManager::GetLinkedAssets() const
{
	return LinkedAssets1;
}

UCleanerConfigs* ProjectCleanerDataManager::GetCleanerConfigs() const
{
	return CleanerConfigs;
}

UExcludeOptions* ProjectCleanerDataManager::GetExcludeOptions() const
{
	return ExcludeOptions;
}

FString ProjectCleanerDataManager::GetAbsoluteContentDir() const
{
	return ContentDir_Absolute;
}

FName ProjectCleanerDataManager::GetRelativeContentDir() const
{
	return ContentDir_Relative;
}

void ProjectCleanerDataManager::Empty()
{
	AllAssets.Empty();
	UsedAssets.Empty();
	UnusedAssets.Empty();
	AllFiles.Empty();
	NonEngineFiles.Empty();
	CorruptedAssets.Empty();
	EmptyFolders.Empty();
	IndirectlyUsedAssets.Empty();
	PrimaryAssetClasses.Empty();
	ExcludedAssets.Empty();
	LinkedAssets1.Empty();
}

void ProjectCleanerDataManager::Update()
{
	Empty();

	// all assets
	AssetRegistry->Get().GetAssetsByPath(ContentDir_Relative, AllAssets, true);

	// invalid assets
	FindInvalidFiles();

	// used assets
	FindIndirectlyUsedAssets(); // todo:ashe23 this should be in used assets function maybe?
	FindEmptyFolders(ContentDir_Absolute / TEXT("*"));
	RemoveForbiddenFolders();
	
	FindPrimaryAssetClasses();
	FindUsedAssets();

	// filtered assets

	// unused assets
	FindUnusedAssets();


	TotalProjectSize = ProjectCleanerUtility::GetTotalSize(AllAssets);
	TotalUnusedAssetsSize = ProjectCleanerUtility::GetTotalSize(UnusedAssets);

	// registering delegates
	// AssetRegistry->Get().OnAssetAdded().AddRaw(this, &ProjectCleanerDataManager::OnAssetAdded);
	// AssetRegistry->Get().OnAssetRemoved().AddRaw(this, &FProjectCleanerModule::OnAssetRemoved);
	// AssetRegistry->Get().OnAssetRenamed().AddRaw(this, &FProjectCleanerModule::OnAssetRenamed);
	// AssetRegistry->Get().OnAssetUpdated().AddRaw(this, &FProjectCleanerModule::OnAssetUpdated);
}

int64 ProjectCleanerDataManager::GetTotalProjectSize() const
{
	return TotalProjectSize;
}

int64 ProjectCleanerDataManager::GetTotalUnusedAssetsSize() const
{
	return TotalUnusedAssetsSize;
}

void ProjectCleanerDataManager::FindPrimaryAssetClasses()
{
	const auto& AssetManager = UAssetManager::Get();
	if (!AssetManager.IsValid()) return;

	PrimaryAssetClasses.Reserve(10);
	
	const UAssetManagerSettings& Settings = AssetManager.GetSettings();
	TArray<FPrimaryAssetId> Ids;
	for (const auto& Type : Settings.PrimaryAssetTypesToScan)
	{
		AssetManager.Get().GetPrimaryAssetIdList(Type.PrimaryAssetType, Ids);
		for(const auto& Id : Ids)
		{
			FAssetData Data;
			AssetManager.Get().GetPrimaryAssetData(Id, Data);
			if(!Data.IsValid()) continue;
			PrimaryAssetClasses.Add(Data.AssetClass);
		}
		Ids.Reset();
	}

	PrimaryAssetClasses.Shrink();
}

void ProjectCleanerDataManager::FindUsedAssets()
{
	UsedAssets.Reserve(AllAssets.Num());
	
	FARFilter Filter;
	Filter.ClassNames.Add(UWorld::StaticClass()->GetFName());
	Filter.ClassNames.Append(PrimaryAssetClasses.Array());
	Filter.PackagePaths.Add(ContentDir_Relative);
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	
	TArray<FName> PackageNamesToProcess;
	{
		TArray<FAssetData> FoundAssets;
		AssetRegistry->Get().GetAssets(Filter, FoundAssets);
		for (const FAssetData& AssetData : FoundAssets)
		{
			PackageNamesToProcess.Add(AssetData.PackageName);
			UsedAssets.Add(AssetData.PackageName);
		}
	}

	TArray<FAssetIdentifier> AssetDependencies;
	while (PackageNamesToProcess.Num() > 0)
	{
		const FName PackageName = PackageNamesToProcess.Pop(false);
		AssetDependencies.Reset();
		AssetRegistry->Get().GetDependencies(FAssetIdentifier(PackageName), AssetDependencies);
		for (const FAssetIdentifier& Dependency : AssetDependencies)
		{
			bool bIsAlreadyInSet = false;
			if (!Dependency.PackageName.ToString().StartsWith(ContentDir_Relative.ToString())) continue;
			
			UsedAssets.Add(Dependency.PackageName, &bIsAlreadyInSet);
			if (!bIsAlreadyInSet && Dependency.IsValid())
			{
				PackageNamesToProcess.Add(Dependency.PackageName);
			}
		}
	}
}

void ProjectCleanerDataManager::FindUnusedAssets()
{
	TSet<FName> FilteredAssets;
	FilteredAssets.Reserve(AllAssets.Num());

	for (const auto& IndirectAsset : IndirectlyUsedAssets)
	{
		FilteredAssets.Add(IndirectAsset.Key.PackageName);
	}
	
	for (const auto& AssetData : AllAssets)
	{
		if (
			HasExternalReferencers(AssetData.PackageName) ||
			ExcludedByPath(AssetData.PackagePath) ||
			ExcludedByClass(AssetData))
		{
			FilteredAssets.Add(AssetData.PackageName);
		}
	}

	FilteredAssets.Shrink();
	
	TSet<FName> RelatedAssets; // todo:ashe23
	FindLinkedAssets(FilteredAssets, RelatedAssets);
	
	for (const auto& AssetData : AllAssets)
	{
		if (UsedAssets.Contains(AssetData.PackageName)) continue;
		if (!AssetData.PackageName.ToString().StartsWith(ContentDir_Relative.ToString())) continue;
		if (IndirectlyUsedAssets.Contains(AssetData)) continue;
		if (RelatedAssets.Contains(AssetData.PackageName)) continue;

		UnusedAssets.Add(AssetData);
	}
}

void ProjectCleanerDataManager::FindIndirectlyUsedAssets()
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	TSet<FString> Files;
	Files.Reserve(200); // reserving some space
	
	// 1) finding all source files in main project "Source" directory (<yourproject>/Source/*)
	TArray<FString> FilesToScan;
	PlatformFile.FindFilesRecursively(FilesToScan, *FPaths::GameSourceDir(), TEXT(".cs"));
	PlatformFile.FindFilesRecursively(FilesToScan, *FPaths::GameSourceDir(), TEXT(".cpp"));
	PlatformFile.FindFilesRecursively(FilesToScan, *FPaths::GameSourceDir(), TEXT(".h"));
	PlatformFile.FindFilesRecursively(FilesToScan, *FPaths::ProjectConfigDir(), TEXT(".ini"));
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
	PlatformFile.IterateDirectory(*FPaths::ProjectPluginsDir(), Visitor);
	
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

	for (const auto& File : Files)
	{
		if (!PlatformFile.FileExists(*File)) continue;
	
		FString FileContent;
		FFileHelper::LoadFileToString(FileContent, *File);
	
		// search any sub string that has asset package path in it
		static FRegexPattern Pattern(TEXT(R"(\/Game(.*)\b)"));
		FRegexMatcher Matcher(Pattern, FileContent);
		if (!Matcher.FindNext()) continue;
		const FName FoundedAssetPackageName =  FName{Matcher.GetCaptureGroup(0)};
	
		// if founded asset exist in AssetRegistry database, then we finding line number where its used
		const FAssetData* AssetData = AllAssets.FindByPredicate([&] (const FAssetData& Elem)
		{
			return Elem.ObjectPath.IsEqual(FoundedAssetPackageName);
		});
		if (!AssetData) continue;
		// if (UsedAssets.Contains(AssetData->PackageName)) continue;
		
		TArray<FString> Lines;
		FFileHelper::LoadFileToStringArray(Lines, *File);
		for (int32 i = 0; i < Lines.Num(); ++i)
		{
			if (!Lines.IsValidIndex(i)) continue;
			if (!Lines[i].Contains(FoundedAssetPackageName.ToString())) continue;
	
			FIndirectAsset IndirectAsset;
			IndirectAsset.File = FPaths::ConvertRelativePathToFull(File);
			IndirectAsset.Line = i + 1;
			IndirectlyUsedAssets.Add(*AssetData, IndirectAsset);
		}
	}
}

void ProjectCleanerDataManager::FindInvalidFiles()
{
	struct ProjectCleanerRootDirVisitor : IPlatformFile::FDirectoryVisitor
	{
		ProjectCleanerRootDirVisitor(
			TArray<FAssetData>& Assets,
			TSet<FName>& Files,
			TSet<FName>& NewNonEngineFiles,
			TSet<FName>& NewCorruptedAssets
		) :
		AllAssets(Assets),
		AllFiles(Files),
		NonEngineFiles(NewNonEngineFiles),
		CorruptedAssets(NewCorruptedAssets) {}
		
		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			const FString FullPath = FPaths::ConvertRelativePathToFull(FilenameOrDirectory);
			if (!bIsDirectory)
			{
				AllFiles.Add(FName{FullPath});
				
				if (ProjectCleanerUtility::IsEngineExtension(FPaths::GetExtension(FullPath, false)))
				{
					// here we got absolute path "C:/MyProject/Content/material.uasset"
					// we must first convert that path to In Engine Internal Path like "/Game/material.uasset"
					const FString InternalFilePath = ProjectCleanerUtility::ConvertAbsolutePathToInternal(FullPath);
					// Converting file path to object path (This is for searching in AssetRegistry)
					// example "/Game/Name.uasset" => "/Game/Name.Name"
					FString ObjectPath = InternalFilePath;
					ObjectPath.RemoveFromEnd(FPaths::GetExtension(InternalFilePath, true));
					ObjectPath.Append(TEXT(".") + FPaths::GetBaseFilename(InternalFilePath));

					const FName ObjectPathName = FName{*ObjectPath};
					const bool IsInAssetRegistry = AllAssets.ContainsByPredicate([&] (const FAssetData& Elem)
					{
						return Elem.ObjectPath.IsEqual(ObjectPathName);
					});
					if (!IsInAssetRegistry)
					{
						CorruptedAssets.Add(ObjectPathName);
					}
				}
				else
				{
					NonEngineFiles.Add(FName{FullPath});
				}
			}

			return true;
		}
		TArray<FAssetData>& AllAssets;
		TSet<FName>& AllFiles;
		TSet<FName>& NonEngineFiles;
		TSet<FName>& CorruptedAssets;
	};

	ProjectCleanerRootDirVisitor Visitor{AllAssets, AllFiles, NonEngineFiles, CorruptedAssets};
	FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryRecursively(*ContentDir_Absolute, Visitor);
}

bool ProjectCleanerDataManager::FindEmptyFolders(const FString& FolderPath)
{
	bool IsSubFoldersEmpty = true;
	TArray<FString> SubFolders;
	IFileManager::Get().FindFiles(SubFolders, *FolderPath, false, true);

	for (const auto& SubFolder : SubFolders)
	{
		// "*" needed for unreal`s IFileManager class, without it , its not working.
		auto NewPath = FolderPath;
		NewPath.RemoveFromEnd(TEXT("*"));
		NewPath += SubFolder / TEXT("*");
		if (FindEmptyFolders(NewPath))
		{
			NewPath.RemoveFromEnd(TEXT("*"));
			EmptyFolders.Add(*NewPath);
		}
		else
		{
			IsSubFoldersEmpty = false;
		}
	}

	TArray<FString> FilesInFolder;
	IFileManager::Get().FindFiles(FilesInFolder, *FolderPath, true, false);

	if (IsSubFoldersEmpty && FilesInFolder.Num() == 0)
	{
		return true;
	}

	return false;
}

void ProjectCleanerDataManager::RemoveForbiddenFolders()
{
	const FString CollectionsFolder = ContentDir_Absolute + TEXT("Collections/");
	const FString DevelopersFolder = ContentDir_Absolute + TEXT("Developers/");
	const FString UserDir = DevelopersFolder + FPaths::GameUserDeveloperFolderName() + TEXT("/");
	const FString UserCollectionsDir = UserDir + TEXT("Collections/");
	
	EmptyFolders.Remove(FName{*CollectionsFolder});
	EmptyFolders.Remove(FName{*DevelopersFolder});
	EmptyFolders.Remove(FName{*UserDir});
	EmptyFolders.Remove(FName{*UserCollectionsDir});

	if (!CleanerConfigs->bScanDeveloperContents)
	{
		// find all folders that are under developer folders
		TSet<FName> FilteredFolders;
		FilteredFolders.Reserve(EmptyFolders.Num());

		for (const auto& Folder : EmptyFolders)
		{
			if (
				FPaths::IsUnderDirectory(Folder.ToString(), CollectionsFolder) ||
				FPaths::IsUnderDirectory(Folder.ToString(), DevelopersFolder)
			)
			{
				FilteredFolders.Add(Folder);
			}
		}

		for (const auto& Folder : FilteredFolders)
		{
			EmptyFolders.Remove(Folder);
		}
	}
}

void ProjectCleanerDataManager::FindLinkedAssets(const TSet<FName>& FilteredAssets, TSet<FName>& LinkedAssets)
{
	TArray<FName> Stack;
	for (const auto& Asset : FilteredAssets)
	{
		FAssetNode Node;
		AssetRegistry->Get().GetReferencers(Asset, Node.Refs);
		AssetRegistry->Get().GetDependencies(Asset, Node.Deps);
	
		Node.Refs.Remove(Asset);
		Node.Deps.Remove(Asset);
	
		Stack.Add(Asset);
		
		TArray<FAssetIdentifier> AssetDependencies;
		TArray<FAssetIdentifier> AssetReferencers;
		
		while (Stack.Num() > 0)
		{
			const FName PackageName = Stack.Pop(false);
	
			AssetDependencies.Reset();
			AssetReferencers.Reset();
			AssetRegistry->Get().GetDependencies(FAssetIdentifier(PackageName), AssetDependencies);
			AssetRegistry->Get().GetReferencers(FAssetIdentifier(PackageName), AssetReferencers);
	
			AssetDependencies.RemoveAllSwap([&](const FAssetIdentifier& Elem)
			{
				return !Elem.PackageName.ToString().StartsWith(ContentDir_Relative.ToString());
			});
			AssetReferencers.RemoveAllSwap([&](const FAssetIdentifier& Elem)
			{
				return !Elem.PackageName.ToString().StartsWith(ContentDir_Relative.ToString());
			});
			
			for (const auto Dep : AssetDependencies)
			{
				bool bIsAlreadyInSet = false;
				LinkedAssets.Add(Dep.PackageName, &bIsAlreadyInSet);
				if (!bIsAlreadyInSet && Dep.IsValid())
				{
					Stack.Add(Dep.PackageName);
				}
			}
			for (const auto Ref : AssetReferencers)
			{
				bool bIsAlreadyInSet = false;
				LinkedAssets.Add(Ref.PackageName, &bIsAlreadyInSet);
				if (!bIsAlreadyInSet && Ref.IsValid())
				{
					Stack.Add(Ref.PackageName);
				}
			}
		}
	}
}

bool ProjectCleanerDataManager::HasExternalReferencers(const FName& PackageName)
{
	TArray<FName> Refs;
	AssetRegistry->Get().GetReferencers(PackageName, Refs);
	return Refs.ContainsByPredicate([&] (const FName& Elem)
	{
		return !Elem.ToString().StartsWith(ContentDir_Relative.ToString());
	});
}

bool ProjectCleanerDataManager::ExcludedByPath(const FName& PackagePath)
{
	return ExcludeOptions->Paths.ContainsByPredicate([&](const FDirectoryPath& DirectoryPath)
	{
		return PackagePath.ToString().StartsWith(DirectoryPath.Path);
	});
}

bool ProjectCleanerDataManager::ExcludedByClass(const FAssetData& AssetData)
{
	// todo:ashe23 load asset if class is blueprint
	const UBlueprint* BlueprintAsset = Cast<UBlueprint>(AssetData.GetAsset());
	const bool IsBlueprint = (BlueprintAsset != nullptr);

	FName ClassName;
	FName ParentClassName;

	if (IsBlueprint && BlueprintAsset->GeneratedClass && BlueprintAsset->ParentClass)
	{
		ClassName = BlueprintAsset->GeneratedClass->GetFName();
		ParentClassName = BlueprintAsset->ParentClass->GetFName();
	}
	else
	{
		ClassName = AssetData.AssetClass;
	}
	
	return ExcludeOptions->Classes.ContainsByPredicate([&](const UClass* ElemClass)
	{
		if (!ElemClass) return false;
		return ClassName.IsEqual(ElemClass->GetFName()) ||
		(IsBlueprint &&
			(
				ClassName.IsEqual(ElemClass->GetFName()) ||
				ParentClassName.IsEqual(ElemClass->GetFName()) ||
				ElemClass->GetFName().IsEqual(UBlueprint::StaticClass()->GetFName())
			)
		);
	});
}

// Automation testing functionality
void ProjectCleanerDataManager::EnableTestMode()
{
	ContentDir_Absolute = FPaths::ProjectContentDir() +  TEXT("AutomatationTests/");
	ContentDir_Relative = TEXT("/Game/AutomatationTests");
	SourceDir = FPaths::ProjectContentDir() + TEXT("AutomatationTests/Source/");
	ConfigDir = FPaths::ProjectContentDir() + TEXT("AutomatationTests/Config/");
	PluginsDir = FPaths::ProjectContentDir() + TEXT("AutomatationTests/Plugins/");
}