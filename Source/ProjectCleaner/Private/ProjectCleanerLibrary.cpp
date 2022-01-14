// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.


#include "ProjectCleanerLibrary.h"
#include "Core/ProjectCleanerDataManager.h"
#include "Core/ProjectCleanerUtility.h"

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

	DataManager.AnalyzeProject();
}
