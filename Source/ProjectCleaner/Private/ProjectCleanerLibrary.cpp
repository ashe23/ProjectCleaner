// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerLibrary.h"
#include "Core/ProjectCleanerDataManager.h"
#include "Core/ProjectCleanerUtility.h"
// Engine Headers
#include "Misc/FileHelper.h"

TArray<FAssetData> UProjectCleanerLibrary::GetUnusedAssets(const FProjectCleanerConfigs& CleanerConfigs)
{
	FProjectCleanerDataManager DataManager;

	FillCleanerConfigs(DataManager, CleanerConfigs);

	return DataManager.GetUnusedAssets();
}

TArray<FString> UProjectCleanerLibrary::GetUnusedAssetsPaths(const FProjectCleanerConfigs& CleanerConfigs, EProjectCleanerPathReturnType PathType)
{
	const TArray<FAssetData> UnusedAssets = GetUnusedAssets(CleanerConfigs);
	TArray<FString> Paths;
	Paths.Reserve(UnusedAssets.Num());

	for (const auto& Asset : UnusedAssets)
	{
		Paths.Add(GetAssetPathByPathType(Asset, PathType));
	}

	return Paths;
}

FString UProjectCleanerLibrary::GetAssetPathByPathType(const FAssetData& AssetData, EProjectCleanerPathReturnType PathType)
{
	if (!AssetData.IsValid()) return FString{};

	if (PathType == EProjectCleanerPathReturnType::EPT_Game)
	{
		// Returns ObjectPath in format => /Game/MyFolder/NewMaterial.NewMaterial
		return AssetData.ObjectPath.ToString();
	}

	FString FinalPath{};
	// Converts /Game/MyFolder/NewMaterial.NewMaterial => C:/dev/MyProject/Content/MyFolder/NewMaterial.NewMaterial
	if (!FPackageName::TryConvertGameRelativePackagePathToLocalPath(AssetData.PackagePath.ToString(), FinalPath))
	{
		return FinalPath;
	}

	FinalPath = FPaths::ConvertRelativePathToFull(FinalPath);
	// Converts C:/dev/MyProject/Content/MyFolder/NewMaterial.NewMaterial => C:/dev/MyProject/Content/MyFolder/NewMaterial.uasset
	FinalPath.Append(TEXT("/"));
	FinalPath.Append(AssetData.AssetName.ToString());
	FinalPath.Append(TEXT(".uasset"));

	if (PathType == EProjectCleanerPathReturnType::EPT_Relative)
	{
		// Converts C:/dev/MyProject/Content/MyFolder/NewMaterial.uasset => Content/MyFolder/NewMaterial.uasset
		FinalPath.RemoveFromStart(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()));
	}

	FPaths::RemoveDuplicateSlashes(FinalPath);

	return FinalPath;
}

TArray<FString> UProjectCleanerLibrary::GetEmptyFolders(const FProjectCleanerConfigs& CleanerConfigs)
{
	FProjectCleanerDataManager DataManager;

	FillCleanerConfigs(DataManager, CleanerConfigs);

	const TSet<FName>& EmptyFoldersSet = DataManager.GetEmptyFolders();

	TArray<FString> EmptyFolders;
	EmptyFolders.Reserve(EmptyFoldersSet.Num());

	for (const auto& Folder : EmptyFoldersSet)
	{
		EmptyFolders.Add(FPaths::ConvertRelativePathToFull(Folder.ToString()));
	}

	return EmptyFolders;
}

TArray<FString> UProjectCleanerLibrary::GetNonEngineFiles(const FProjectCleanerConfigs& CleanerConfigs)
{
	FProjectCleanerDataManager DataManager;

	FillCleanerConfigs(DataManager, CleanerConfigs);

	const TSet<FName>& NonEngineFilesSet = DataManager.GetNonEngineFiles();

	TArray<FString> NonEngineFiles;
	NonEngineFiles.Reserve(NonEngineFiles.Num());

	for (const auto& File : NonEngineFilesSet)
	{
		NonEngineFiles.Add(File.ToString());
	}

	return NonEngineFiles;
}

TArray<FString> UProjectCleanerLibrary::GetCorruptedFiles(const FProjectCleanerConfigs& CleanerConfigs)
{
	FProjectCleanerDataManager DataManager;

	FillCleanerConfigs(DataManager, CleanerConfigs);

	const TSet<FName>& CorruptedAssetsSet = DataManager.GetCorruptedAssets();

	TArray<FString> CorruptedAssets;
	CorruptedAssets.Reserve(CorruptedAssetsSet.Num());

	for (const auto& Asset : CorruptedAssetsSet)
	{
		CorruptedAssets.Add(ProjectCleanerUtility::ConvertInternalToAbsolutePath(Asset.ToString()));
	}

	return CorruptedAssets;
}

TArray<FString> UProjectCleanerLibrary::GetIndirectlyUsedAssets(const FProjectCleanerConfigs& CleanerConfigs)
{
	FProjectCleanerDataManager DataManager;

	FillCleanerConfigs(DataManager, CleanerConfigs);

	const auto& IndirectlyUsedAssetsMap = DataManager.GetIndirectAssets();

	TArray<FString> IndirectAssets;
	IndirectAssets.Reserve(IndirectlyUsedAssetsMap.Num());

	for (const auto& Asset : IndirectlyUsedAssetsMap)
	{
		IndirectAssets.Add(GetAssetPathByPathType(Asset.Key, EProjectCleanerPathReturnType::EPT_Game));
	}

	return IndirectAssets;
}

bool UProjectCleanerLibrary::ExportToFile(const TArray<FString>& List, const FString& FileName)
{
	if (FileName.IsEmpty())
	{
		UE_LOG(LogProjectCleaner, Error, TEXT("Empty FileName"));
		return false;
	}

	const FString SaveDir = FPaths::ProjectSavedDir() + TEXT("/ProjectCleaner/");
	const FString SavePath = SaveDir + FileName;
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	// check if Saved/ProjectCleaner directory exists and create one if not
	if (!PlatformFile.DirectoryExists(*SaveDir))
	{
		if (!PlatformFile.CreateDirectory(*SaveDir))
		{
			UE_LOG(LogProjectCleaner, Error, TEXT("Failed to create %s directory"), *SaveDir);
			return false;
		}
	}

	// check if file already exists?
	if (PlatformFile.FileExists(*SavePath))
	{
		if (!PlatformFile.DeleteFile(*SavePath))
		{
			UE_LOG(LogProjectCleaner, Error, TEXT("Failed to delete %s file"), *SavePath);
			return false;
		}
	}

	// Saving file
	if (!FFileHelper::SaveStringArrayToFile(List, *SavePath))
	{
		UE_LOG(LogProjectCleaner, Error, TEXT("Failed to create %s file"), *SavePath);
		return false;
	}

	return true;
}

void UProjectCleanerLibrary::FillCleanerConfigs(FProjectCleanerDataManager& DataManager, const FProjectCleanerConfigs& CleanerConfigs)
{
	TArray<FString> ExcludedAssets;
	TArray<FString> ExcludedPaths;
	TArray<FString> ExcludedClasses;

	ExcludedAssets.Reserve(CleanerConfigs.ExcludedAssets.Num());
	ExcludedPaths.Reserve(CleanerConfigs.ExcludedPaths.Num());
	ExcludedClasses.Reserve(CleanerConfigs.ExcludedClasses.Num());

	for (const auto& Asset : CleanerConfigs.ExcludedAssets)
	{
		if (!Asset || !Asset->IsValidLowLevel()) continue;

		ExcludedAssets.AddUnique(Asset->GetPathName());
	}

	for (const auto& DirPath : CleanerConfigs.ExcludedPaths)
	{
		ExcludedPaths.AddUnique(DirPath.Path);
	}

	for (const auto& ExcludedClass : CleanerConfigs.ExcludedClasses)
	{
		if (!ExcludedClass ||!ExcludedClass->IsValidLowLevel()) continue;

		ExcludedClasses.AddUnique(ExcludedClass->GetFName().ToString());
	}

	// todo:ashe23 add scandevfolder option
	DataManager.SetSilentMode(true);
	DataManager.SetUserExcludedAssets(ExcludedAssets);
	DataManager.SetExcludePaths(ExcludedPaths);
	DataManager.SetExcludeClasses(ExcludedClasses);
	DataManager.SetScanDeveloperContents(CleanerConfigs.bScanDeveloperContents);

	DataManager.AnalyzeProject();
}
