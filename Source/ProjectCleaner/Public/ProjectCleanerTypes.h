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

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Indirect Asset")
	FAssetData AssetData;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Indirect Asset")
	int32 LineNum;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Indirect Asset")
	FString FilePath;
};
