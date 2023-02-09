// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ProjectCleanerTypes.h"
#include "ProjectCleanerAssetInfo.generated.h"

USTRUCT(BlueprintType)
struct FProjectCleanerAssetInfo
{
	GENERATED_BODY()

	bool IsValid() const;
	FORCEINLINE bool operator==(const FProjectCleanerAssetInfo& Other) const;
	FORCEINLINE bool operator!=(const FProjectCleanerAssetInfo& Other) const;

	const FName& GetObjectPath() const;
	const FName& GetPackagePath() const;
	const FName& GetAssetName() const;
	const FName& GetAssetClassName() const;
	const UClass* GetAssetClass() const;
	EProjectCleanerAssetCategory GetAssetCategory() const;

private:
	bool bIsBlueprint = false;
	FName ObjectPath;
	FName PackagePath;
	FName AssetName;
	FName AssetClassName;
	UClass* AssetClass = nullptr;
	EProjectCleanerAssetCategory AssetCategory = EProjectCleanerAssetCategory::None;
};

FORCEINLINE uint32 GetTypeHash(const FProjectCleanerAssetInfo& AssetInfo)
{
	return FCrc::MemCrc32(&AssetInfo, sizeof(FProjectCleanerAssetInfo));
}
