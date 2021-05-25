// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistryModule.h"

struct FCleaningStats
{
	int32 UnusedAssetsNum;
	int64 UnusedAssetsTotalSize;
	int32 NonUassetFilesNum;
	int32 SourceCodeAssetsNum;
	int32 CorruptedFilesNum;
	int32 EmptyFolders;
	
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
		SourceCodeAssetsNum = 0;
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
	FNode(const FAssetData& Data) : AssetData(Data) {}
	
	// FName Asset; // todo:ashe23 remove this
	const FAssetData& AssetData;
	
	TArray<FName> Refs; // todo:ashe23 for now its TArray just for debugging, on release it should be TSet
	TArray<FName> Deps;
	TArray<FName> LinkedAssets;

	bool operator==(const FNode& Other) const
	{
		if (!AssetData.IsValid() || !Other.AssetData.IsValid()) return false;
		
		return AssetData == Other.AssetData;
	}

	bool operator!=(const FNode& Other) const
	{
		return !(AssetData == Other.AssetData);
	}

	bool HasLinkedAssetsOutsideGameFolder() const
	{
		if (Refs.Num() == 0) return false;

		for (const auto& Ref : Refs)
		{
			if (!Ref.ToString().StartsWith("/Game")) return true;
			
			// FString PackageFileName;
			// FString PackageFile;
			// if (
			// 	FPackageName::TryConvertLongPackageNameToFilename(Ref.ToString(), PackageFileName) &&
			// 	FPackageName::FindPackageFileWithoutExtension(PackageFileName, PackageFile)
			// )
			// {
			// 	const FString FilePathOnDisk = FPaths::ConvertRelativePathToFull(PackageFile);
			// 	const bool UnderEnginePluginDir = FPaths::IsUnderDirectory(FilePathOnDisk, FPaths::EnginePluginsDir());
			// 	const bool UnderProjectPluginDir = FPaths::IsUnderDirectory(FilePathOnDisk, FPaths::ProjectPluginsDir());
			// 	if (UnderEnginePluginDir || UnderProjectPluginDir)
			// 	{
			// 		return true;
			// 	}
			// }
		}

		return false;
	}

	bool IsCircular() const
	{
		for (const auto& Ref : Refs)
		{
			if (Deps.Contains(Ref))
			{
				return true;
			}
		}

		return false;
	}

	void Reset()
	{
		Refs.Reset();
		Deps.Reset();
		LinkedAssets.Reset();
	}

	void Empty()
	{
		Refs.Empty();
		Deps.Empty();
		LinkedAssets.Empty();
	}
};

struct FAssetsRelationalGraph
{
	TArray<FNode> Nodes;
	TArray<FNode> NodesWithExternalDependencies;

	void Reset()
	{
		Nodes.Reset();
		NodesWithExternalDependencies.Reset();
	}

	void Empty()
	{
		Nodes.Empty();
		NodesWithExternalDependencies.Empty();
	}

	bool Contains(const FAssetData& Data) const
	{
		return Nodes.Contains(Data);
	}

	void Add(const FAssetData& Data)
	{
		if (Contains(Data)) return;
		
		// Creating new node
		FNode Node{Data};

		TArray<FName> Refs;
		TArray<FName> Deps;
		
		// finding current assets Dependencies and Referencers (1 level depth)
		GetLinkedAssets(Data.PackageName, ERelatedAssetType::Reference, Refs);
		GetLinkedAssets(Data.PackageName, ERelatedAssetType::Dependency, Deps);

		for (const auto& Ref : Refs)
		{
			Node.Refs.AddUnique(Ref); // todo:ashe23 change to just Add()
			Node.LinkedAssets.AddUnique(Ref);
		}
		for (const auto& Dep : Deps)
		{
			Node.Deps.AddUnique(Dep); // todo:ashe23 change to just Add()
			Node.LinkedAssets.AddUnique(Dep);
		}

		Nodes.Add(Node);
	}

	FNode* Find(const FName& Asset)
	{
		return Nodes.FindByPredicate([&](const FNode& Elem)
		{
			return Elem.AssetData.AssetName.IsEqual(Asset);
		});
	}

	void GetLinkedAssetsChain(const FName& Asset, TArray<FName>& LinkedAssets)
	{
		const auto Node = Find(Asset);
		if(!Node) return;

		// LinkedAssets.Append(Node->LinkedAssets);
		//
		// for (const auto& LinkedAsset : LinkedAssets)
		// {
		// 	GetLinkedAssetsChain(LinkedAsset, LinkedAssets);
		// }
	}

	void FindAssetsWithExternalDependencies()
	{
		NodesWithExternalDependencies.Reserve(Nodes.Num());
		for (const auto& Node : Nodes)
		{
			if (Node.HasLinkedAssetsOutsideGameFolder())
			{
				NodesWithExternalDependencies.AddUnique(Node);
			}
		}
	}
private:
	enum class ERelatedAssetType
	{
		Reference,
		Dependency
	};
	void GetLinkedAssets(const FName& Asset, ERelatedAssetType AssetType, TArray<FName>& RelatedAssets) const
	{
		FAssetRegistryModule& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");

		if (AssetType == ERelatedAssetType::Reference)
		{
			AssetRegistry.Get().GetReferencers(Asset, RelatedAssets);
		}
		else
		{
			AssetRegistry.Get().GetDependencies(Asset, RelatedAssets);
		}

		// todo:ashe23 maybe remove also assets that are outside "/Game" folder
		RelatedAssets.RemoveAll([&] (const FName& Elem)
		{
			return Elem.IsEqual(Asset);
		});
	}
};

struct FSourceCodeFile
{
	FName Name;
	FString AbsoluteFilePath;
	FString RelativeFilePath;
	FString Content;
};