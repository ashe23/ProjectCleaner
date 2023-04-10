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

USTRUCT(BlueprintType)
struct FPjcExcludeSettings
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ExcludeSettings")
	TArray<FString> ExcludedPaths;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ExcludeSettings")
	TArray<FName> ExcludedClassNames;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ExcludeSettings")
	TArray<FName> ExcludedAssetObjectPaths;

	void Clear()
	{
		ExcludedPaths.Empty();
		ExcludedClassNames.Empty();
		ExcludedAssetObjectPaths.Empty();
	}
};

USTRUCT(BlueprintType)
struct FPjcAssetIndirectInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|Indirect Asset Info")
	int32 FileLine = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|Indirect Asset Info")
	FAssetData AssetData;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|Indirect Asset Info")
	FString FilePath;

	bool operator==(const FPjcAssetIndirectInfo& Other) const
	{
		return AssetData == Other.AssetData && FilePath.Equals(Other.FilePath) && FileLine == Other.FileLine;
	}

	bool operator!=(const FPjcAssetIndirectInfo& Other) const
	{
		return !(AssetData == Other.AssetData && FilePath.Equals(Other.FilePath) && FileLine == Other.FileLine);
	}
};

USTRUCT(BlueprintType)
struct FPjcScanStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanStats")
	int64 SizeFilesTotal = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanStats")
	int64 SizeFilesAsset = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanStats")
	int64 SizeFilesNonAsset = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanStats")
	int64 SizeFilesCorrupted = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanStats")
	int64 SizeAssetsUsed = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanStats")
	int64 SizeAssetsUnused = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanStats")
	int64 SizeAssetsPrimary = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanStats")
	int64 SizeAssetsIndirect = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanStats")
	int64 SizeAssetsEditor = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanStats")
	int64 SizeAssetsExcluded = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanStats")
	int64 SizeAssetsExtReferenced = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanStats")
	int32 NumFilesTotal = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanStats")
	int32 NumFilesAsset = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanStats")
	int32 NumFilesNonAsset = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanStats")
	int32 NumFilesCorrupted = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanStats")
	int32 NumAssetsUsed = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanStats")
	int32 NumAssetsUnused = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanStats")
	int32 NumAssetsPrimary = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanStats")
	int32 NumAssetsIndirect = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanStats")
	int32 NumAssetsEditor = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanStats")
	int32 NumAssetsExcluded = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanStats")
	int32 NumAssetsExtReferenced = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanStats")
	int32 NumFoldersTotal = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanStats")
	int32 NumFoldersEmpty = 0;

	void Clear()
	{
		SizeFilesTotal = 0;
		SizeFilesAsset = 0;
		SizeFilesNonAsset = 0;
		SizeFilesCorrupted = 0;
		SizeAssetsUsed = 0;
		SizeAssetsUnused = 0;
		SizeAssetsPrimary = 0;
		SizeAssetsIndirect = 0;
		SizeAssetsEditor = 0;
		SizeAssetsExcluded = 0;
		SizeAssetsExtReferenced = 0;
		NumFilesTotal = 0;
		NumFilesAsset = 0;
		NumFilesNonAsset = 0;
		NumFilesCorrupted = 0;
		NumAssetsUsed = 0;
		NumAssetsUnused = 0;
		NumAssetsPrimary = 0;
		NumAssetsIndirect = 0;
		NumAssetsEditor = 0;
		NumAssetsExcluded = 0;
		NumAssetsExtReferenced = 0;
		NumFoldersTotal = 0;
		NumFoldersEmpty = 0;
	}
};

USTRUCT(BlueprintType)
struct FPjcScanData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FString> FilesNonAsset;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FString> FilesCorrupted;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FString> FoldersEmpty;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FAssetData> AssetsAll;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FAssetData> AssetsUsed;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FAssetData> AssetsUnused;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FAssetData> AssetsPrimary;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FAssetData> AssetsIndirect;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FPjcAssetIndirectInfo> AssetsIndirectInfos;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FAssetData> AssetsEditor;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FAssetData> AssetsExcluded;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanData")
	TArray<FAssetData> AssetsExtReferenced;

	void Clear()
	{
		FilesNonAsset.Empty();
		FilesCorrupted.Empty();
		FoldersEmpty.Empty();
		AssetsAll.Empty();
		AssetsUsed.Empty();
		AssetsUnused.Empty();
		AssetsPrimary.Empty();
		AssetsIndirect.Empty();
		AssetsEditor.Empty();
		AssetsExcluded.Empty();
		AssetsExtReferenced.Empty();
	}
};

USTRUCT(BlueprintType)
struct FPjcScanResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanResult")
	bool bScanSuccess = false;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanResult")
	FString ScanErrMsg;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanResult")
	FPjcScanStats ScanStats;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner|ScanResult")
	FPjcScanData ScanData;
};
