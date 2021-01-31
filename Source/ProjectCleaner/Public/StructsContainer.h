#pragma once

#include "AssetRegistry/Public/AssetData.h"
#include "CoreMinimal.h"

struct FAssetChunk
{
	TArray<FAssetData> Dependencies;
};