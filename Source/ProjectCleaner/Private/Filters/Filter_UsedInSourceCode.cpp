#include "Filters/Filter_UsedInSourceCode.h"

#include "ProjectCleaner.h"
#include "ProjectCleanerUtility.h"
#include "StructsContainer.h"
#include "UI/ProjectCleanerAssetsUsedInSourceCodeUI.h"

#pragma optimize("", off)
Filter_UsedInSourceCode::Filter_UsedInSourceCode(TArray<FSourceCodeFile>& SourceFiles, TArray<FNode>& List, TArray<TWeakObjectPtr<UAssetsUsedInSourceCodeUIStruct>>& AssetsUsedInSourceCodeUIStructs)
{
	AdjacencyList = &List;
	this->SourceFiles = &SourceFiles;
	this->AssetsUsedInSourceCodeUIStructs = &AssetsUsedInSourceCodeUIStructs;
}

void Filter_UsedInSourceCode::Apply(TArray<FAssetData>& Assets)
{
	TArray<FName> UsedInSourceFilesRelatedAssets;
	UsedInSourceFilesRelatedAssets.Reserve(100);

	for (const auto& Asset : Assets)
	{
		// checking if current asset used in source files
		if (UsedInSourceFiles(Asset))
		{
			// finding him in adjacency list
			FNode* Node = AdjacencyList->FindByPredicate([&](const FNode& Elem)
			{
				return Elem.Asset == Asset.PackageName;
			});

			// and if its valid finding all related assets 
			if (Node)
			{
				ProjectCleanerUtility::FindAllRelatedAssets(*Node, UsedInSourceFilesRelatedAssets, *AdjacencyList);
			}
		}
	}

	if (UsedInSourceFilesRelatedAssets.Num() == 0) return;

	// removing all assets we found
	Assets.RemoveAll([&](const FAssetData& Val)
	{
		return UsedInSourceFilesRelatedAssets.Contains(Val.PackageName);
	});
}

bool Filter_UsedInSourceCode::UsedInSourceFiles(const FAssetData& Asset) const
{
	for (const auto& File : *SourceFiles)
	{
		// Wrapping in quotes AssetName => "AssetName"
		FString QuotedAssetName = Asset.AssetName.ToString();
		QuotedAssetName.InsertAt(0, TEXT("\""));
		QuotedAssetName.Append(TEXT("\""));
		
		if (
			File.Content.Contains(Asset.PackageName.ToString()) ||
			File.Content.Contains(QuotedAssetName)
		)
		{
			auto Obj = NewObject<UAssetsUsedInSourceCodeUIStruct>();
			Obj->AssetName = Asset.AssetName.ToString();
			Obj->AssetPath = Asset.PackageName.ToString();
			Obj->SourceCodePath = File.AbsoluteFilePath;
			AssetsUsedInSourceCodeUIStructs->Add(Obj);
			UE_LOG(LogProjectCleaner, Warning, TEXT("\"%s\" asset used in \"%s\" file"), *Asset.AssetName.ToString(), *File.AbsoluteFilePath);
			return true;
		}
	}

	return false;
}
#pragma optimize("", on)
