#pragma once

#include "AssetRegistry/Public/AssetData.h"
#include "CoreMinimal.h"

struct FAssetChunk
{
	TArray<FAssetData> Dependencies;
};

struct FCleaningStats
{
	int32 UnusedAssetsNum;
	int32 EmptyFolders;
	int64 UnusedAssetsTotalSize;
	int32 DeleteChunkSize;
	// This is for progressbar calculations
	int32 DeletedAssetCount;
	int32 TotalAssetNum;

	FCleaningStats()
	{
		UnusedAssetsNum = 0;
		EmptyFolders = 0;
		UnusedAssetsTotalSize = 0;
		DeleteChunkSize = 100;
		DeletedAssetCount = 0;
		TotalAssetNum = 0;
	}
};
