// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "Core/ProjectCleanerDataManager.h"
#include "Core/ProjectCleanerUtility.h"
// Engine Headers
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"
#include "Engine/AssetManagerSettings.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/PlatformFilemanager.h"
#include "Internationalization/Regex.h"


void ProjectCleanerDataManager::GetAllAssetsByPath(const FName& InPath, TArray<FAssetData>& AllAssets)
{
	AllAssets.Empty();
	const auto& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistry.Get().GetAssetsByPath(InPath, AllAssets, true);
}

void ProjectCleanerDataManager::GetInvalidFilesByPath(
	const FString& InPath,
	const TArray<FAssetData>& AllAssets,
	TSet<FName>& CorruptedAssets,
	TSet<FName>& NonEngineFiles)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*InPath)) return;

	CorruptedAssets.Empty();
	NonEngineFiles.Empty();

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

void ProjectCleanerDataManager::GetIndirectAssetsByPath(
	const FString& InPath,
	TMap<FAssetData, FIndirectAsset>& IndirectlyUsedAssets,
	const TArray<FAssetData>& AllAssets
)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*InPath)) return;

	IndirectlyUsedAssets.Empty();

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
		
		if (!HasIndirectlyUsedAssets(FileContent)) continue;

		static FRegexPattern Pattern(TEXT(R"(\/Game(.*)\b)"));
		FRegexMatcher Matcher(Pattern, FileContent);
		while (Matcher.FindNext())
		{
			const FName FoundedAssetObjectPath =  FName{Matcher.GetCaptureGroup(0)};
			if (!FoundedAssetObjectPath.IsValid()) continue;

			const FAssetData* AssetData = AllAssets.FindByPredicate([&] (const FAssetData& Elem)
			{
				return
					Elem.ObjectPath.IsEqual(FoundedAssetObjectPath) ||
					Elem.PackageName.IsEqual(FoundedAssetObjectPath);
			});

			if (!AssetData) continue;
			
			// if founded asset is ok, we loading file by lines to determine on what line its used
			TArray<FString> Lines;
			FFileHelper::LoadFileToStringArray(Lines, *File);
			for (int32 i = 0; i < Lines.Num(); ++i)
			{
				if (!Lines.IsValidIndex(i)) continue;
				if (!Lines[i].Contains(FoundedAssetObjectPath.ToString())) continue;
			
				FIndirectAsset IndirectAsset;
				IndirectAsset.File = FPaths::ConvertRelativePathToFull(File);
				IndirectAsset.RelativePath = AssetData->PackagePath;
				IndirectAsset.Line = i + 1;
				IndirectlyUsedAssets.Add(*AssetData, IndirectAsset);
			}
		}
	}
}

void ProjectCleanerDataManager::GetEmptyFolders(const FString& InPath, TSet<FName>& EmptyFolders, const bool bScanDevelopersContent)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*InPath)) return;

	EmptyFolders.Empty();
	
	FindEmptyFolders(InPath / TEXT("*"), EmptyFolders);

	const FString CollectionsFolder = InPath + TEXT("Collections/");
	const FString DevelopersFolder = InPath + TEXT("Developers/");
	const FString UserDir = DevelopersFolder + FPaths::GameUserDeveloperFolderName() + TEXT("/");
	const FString UserCollectionsDir = UserDir + TEXT("Collections/");
	
	EmptyFolders.Remove(FName{*CollectionsFolder});
	EmptyFolders.Remove(FName{*DevelopersFolder});
	EmptyFolders.Remove(FName{*UserDir});
	EmptyFolders.Remove(FName{*UserCollectionsDir});

	if (!bScanDevelopersContent)
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

void ProjectCleanerDataManager::GetPrimaryAssetClasses(TSet<FName>& PrimaryAssetClasses)
{
	const auto& AssetManager = UAssetManager::Get();
	if (!AssetManager.IsValid()) return;
	
	TArray<FPrimaryAssetTypeInfo> AssetTypeInfos;
	AssetManager.Get().GetPrimaryAssetTypeInfoList(AssetTypeInfos);

	for (const auto& AssetTypeInfo : AssetTypeInfos)
	{
		UClass* AssetTypeCLass = AssetTypeInfo.AssetBaseClassLoaded;
		if (!AssetTypeCLass) continue;
		FName ClassName = AssetTypeCLass->GetFName();
		PrimaryAssetClasses.Add(ClassName);
	}
}

void ProjectCleanerDataManager::GetAllAssetsWithExternalReferencers(TArray<FAssetData>& AssetsWithExternalRefs,
	const TArray<FAssetData>& AllAssets)
{
	const auto& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	
	AssetsWithExternalRefs.Empty();
	TArray<FName> Refs;
	for (const auto& Asset : AllAssets)
	{
		AssetRegistry.Get().GetReferencers(Asset.PackageName, Refs);

		const bool HasExternalRefs = Refs.ContainsByPredicate([](const FName& Ref)
		{
			return !Ref.ToString().StartsWith(TEXT("/Game"));
		});

		if (HasExternalRefs)
		{
			AssetsWithExternalRefs.AddUnique(Asset);
		}

		Refs.Reset();
	}
}

FName ProjectCleanerDataManager::GetClassName(const FAssetData& AssetData)
{
	FName ClassName;
	if (AssetData.AssetClass.IsEqual("Blueprint"))
	{
		const auto GeneratedClassName = AssetData.TagsAndValues.FindTag(TEXT("GeneratedClass")).GetValue();
		const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(*GeneratedClassName);
		ClassName = FName{*FPackageName::ObjectPathToObjectName(ClassObjectPath)};
	}
	else
	{
		ClassName = FName{*AssetData.AssetClass.ToString()};
	}

	return ClassName;
}

bool ProjectCleanerDataManager::IsUnderMegascansFolder(const FAssetData& AssetData)
{
	return AssetData.PackagePath.ToString().StartsWith(TEXT("/Game/MSPresets"));
}

bool ProjectCleanerDataManager::IsCircularAsset(const FAssetData& AssetData)
{
	const auto& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	TArray<FName> Refs;
	TArray<FName> Deps;
	
	AssetRegistry.Get().GetReferencers(AssetData.PackageName, Refs);
	AssetRegistry.Get().GetDependencies(AssetData.PackageName, Deps);

	Refs.RemoveAllSwap([&] (const FName& Ref)
	{
		return !Ref.ToString().StartsWith(TEXT("/Game")) || Ref.IsEqual(AssetData.PackageName);
	}, false);

	Deps.RemoveAllSwap([&] (const FName& Dep)
	{
		return !Dep.ToString().StartsWith(TEXT("/Game")) || Dep.IsEqual(AssetData.PackageName);
	}, false);

	Refs.Shrink();
	Deps.Shrink();

	for (const auto& Ref : Refs)
	{
		if (Deps.Contains(Ref))
		{
			return true;
		}
	}

	return false;
}

bool ProjectCleanerDataManager::IsRootAsset(const FAssetData& AssetData)
{
	const auto& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	
	TArray<FName> Refs;
	AssetRegistry.Get().GetReferencers(AssetData.PackageName, Refs);
	Refs.RemoveAllSwap([&] (const FName& Ref)
	{
		return !Ref.ToString().StartsWith(TEXT("/Game")) || Ref.IsEqual(AssetData.PackageName);
	});

	return Refs.Num() == 0;
}

bool ProjectCleanerDataManager::FindEmptyFolders(const FString& FolderPath, TSet<FName>& EmptyFolders)
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
		if (FindEmptyFolders(NewPath, EmptyFolders))
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

bool ProjectCleanerDataManager::HasIndirectlyUsedAssets(const FString& FileContent)
{
	if (FileContent.IsEmpty()) return false;
	
	// search any sub string that has asset package path in it
	static FRegexPattern Pattern(TEXT(R"(\/Game(.*)\b)"));
	FRegexMatcher Matcher(Pattern, FileContent);
	return Matcher.FindNext();
}