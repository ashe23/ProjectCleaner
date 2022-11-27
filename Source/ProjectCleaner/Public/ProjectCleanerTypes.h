// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.generated.h"

USTRUCT(BlueprintType)
struct FProjectCleanerIndirectAsset
{
	GENERATED_BODY()
	
	FAssetData AssetData;
	int32 LineNum;
	FString FilePath;
};