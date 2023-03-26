// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcTypes.generated.h"

UENUM(BlueprintType)
enum class EPjcCleanupMethod : uint8
{
	None UMETA(Hidden),
	Full UMETA(ToolTip="Remove both unused assets and empty folders"),
	UnusedAssetsOnly UMETA(Tooltip="Remove unused assets only"),
	EmptyFoldersOnly UMETA(Tooltip="Remove empty folders only"),
};

UENUM(BlueprintType)
enum class EPjcScanResult : uint8
{
	None UMETA(Hidden),
	Success,
	Fail
};

UENUM(BlueprintType)
enum class EPjcScannerState : uint8
{
	Idle,
	Scanning,
	Cleaning
};

struct FPjcAssetUsageInfo
{
	int32 FileLine;
	FString FilePathAbs;

	explicit FPjcAssetUsageInfo(const int32 InFileLine, const FString& InFilePath) : FileLine(InFileLine), FilePathAbs(InFilePath) {}

	bool operator==(const FPjcAssetUsageInfo& Other) const
	{
		return FileLine == Other.FileLine && FilePathAbs.Equals(Other.FilePathAbs, ESearchCase::CaseSensitive);
	}

	bool operator!=(const FPjcAssetUsageInfo& Other) const
	{
		return !(FileLine == Other.FileLine && FilePathAbs.Equals(Other.FilePathAbs, ESearchCase::CaseSensitive));
	}
};
