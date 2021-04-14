#pragma once

#include "CoreMinimal.h"
// #include "StructsContainer.generated.h"

struct FCleaningStats
{
	int32 UnusedAssetsNum;
	int32 EmptyFolders;
	int64 UnusedAssetsTotalSize;
	int32 NonUassetFilesNum;
	int32 AssetsUsedInSourceCodeFilesNum;
	int32 CorruptedFilesNum;
	int32 DeletedAssetCount;
	int32 TotalAssetNum;

	FCleaningStats()
	{
		Reset();
	}

	int32 GetPercentage() const
	{
		if (TotalAssetNum == 0) return 0;
		return (DeletedAssetCount * 100.0f) / TotalAssetNum;
	}

	void Reset()
	{
		UnusedAssetsNum = 0;
		EmptyFolders = 0;
		UnusedAssetsTotalSize = 0;
		NonUassetFilesNum = 0;
		AssetsUsedInSourceCodeFilesNum = 0;
		CorruptedFilesNum = 0;
		DeletedAssetCount = 0;
		TotalAssetNum = 0;
	}
};

struct FStandardCleanerText
{
	FText AssetsDeleteWindowTitle;
	FText AssetsDeleteWindowContent;
	FText EmptyFolderWindowTitle;
	FText EmptyFolderWindowContent;
	FText StartingCleanup;
	FText NoAssetsToDelete;
	FText NoEmptyFolderToDelete;
	FText NonUAssetFilesFound;
	FText SearchingEmptyFolders;

	FStandardCleanerText()
	{
		AssetsDeleteWindowTitle = FText::FromString("Confirm deletion");
		AssetsDeleteWindowContent = FText::FromString("Are you sure you want to permanently delete unused assets?");
		EmptyFolderWindowTitle = FText::FromString("Confirm deletion of empty folders");
		EmptyFolderWindowContent = FText::FromString("Are you sure you want to delete all empty folders in project?");
		StartingCleanup = FText::FromString("Starting Cleanup. This could take some time, please wait");
		NoAssetsToDelete = FText::FromString("There are no assets to delete!");
		NoEmptyFolderToDelete = FText::FromString("There are no empty folders to delete!");
		NonUAssetFilesFound = FText::FromString("Project contains non .uasset files. Check Output Log for more info.");
		SearchingEmptyFolders = FText::FromString("Searching empty folders...");
	}
};


/**
 * @brief Adjacency List Node
 */
struct FNode
{
	FName Asset;

	/**
	 * @brief List of all Dependecies and References for given asset
	 */
	TSet<FName> LinkedAssets;
	TSet<FName> Refs;
	TSet<FName> Deps;

	bool HasLinkedAssetsOutsideGameFolder() const
	{
		if (Refs.Num() == 0) return false;

		for (const auto& Ref : Refs)
		{
			FString PackageFileName;
			FString PackageFile;
			if (
				FPackageName::TryConvertLongPackageNameToFilename(Ref.ToString(), PackageFileName) &&
				FPackageName::FindPackageFileWithoutExtension(PackageFileName, PackageFile)
			)
			{
				const FString FilePathOnDisk = FPaths::ConvertRelativePathToFull(PackageFile);
				const bool UnderEnginePluginDir = FPaths::IsUnderDirectory(FilePathOnDisk, FPaths::EnginePluginsDir());

				if (UnderEnginePluginDir)
				{
					return true;
				}
			}
		}

		return false;
	}

	bool IsCircular() const
	{
		for(const auto& Ref : Refs)
		{
			if(Deps.Contains(Ref))
			{
				return true;
			}
		}

		return false;
	}
};

struct FSourceCodeFile
{
	FName Name;
	FString AbsoluteFilePath;
	FString RelativeFilePath;
	FString Content;
};
