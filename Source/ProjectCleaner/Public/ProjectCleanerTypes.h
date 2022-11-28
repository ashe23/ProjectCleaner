// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.generated.h"

USTRUCT(BlueprintType)
struct FProjectCleanerIndirectAsset
{
	GENERATED_BODY()

	bool operator==(const FProjectCleanerIndirectAsset& Other) const
	{
		return LineNum == Other.LineNum && FilePath.Equals(Other.FilePath);
	}

	bool operator!=(const FProjectCleanerIndirectAsset& Other) const
	{
		return LineNum != Other.LineNum || !FilePath.Equals(Other.FilePath);
	}

	FAssetData AssetData;
	int32 LineNum;
	FString FilePath;
};
