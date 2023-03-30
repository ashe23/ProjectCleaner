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

USTRUCT(BlueprintType)
struct FPjcScanSettings
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="ProjectCleaner")
	TArray<FString> ExcludedPaths;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="ProjectCleaner", DisplayName="ExcludedClasses")
	TArray<FName> ExcludedClassNames;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="ProjectCleaner", DisplayName="ExcludedAssets")
	TArray<FName> ExcludedObjectPaths;
};

USTRUCT(BlueprintType)
struct FPjcScanResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner")
	bool bSuccess = false;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner")
	FString ErrMsg;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner")
	TSet<FString> FilesTotal;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner")
	TSet<FString> FilesAsset;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner")
	TSet<FString> FilesNonAsset;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner")
	TSet<FString> FilesCorruptedAsset;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner")
	TSet<FString> FoldersTotal;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner")
	TSet<FString> FoldersEmpty;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner")
	TSet<FAssetData> AssetsAll;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner")
	TSet<FAssetData> AssetsPrimary;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner")
	TSet<FAssetData> AssetsIndirect;
	TMap<FAssetData, TArray<FPjcAssetUsageInfo>> AssetsIndirectInfos;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner")
	TSet<FAssetData> AssetsEditor;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner")
	TSet<FAssetData> AssetsExtReferenced;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner")
	TSet<FAssetData> AssetsExcluded;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner")
	TSet<FAssetData> AssetsUsed;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ProjectCleaner")
	TSet<FAssetData> AssetsUnused;

	void Clear()
	{
		bSuccess = false;
		ErrMsg.Empty();
		FilesTotal.Empty();
		FilesAsset.Empty();
		FilesNonAsset.Empty();
		FilesCorruptedAsset.Empty();
		FoldersTotal.Empty();
		FoldersEmpty.Empty();
		AssetsAll.Empty();
		AssetsPrimary.Empty();
		AssetsIndirect.Empty();
		AssetsIndirectInfos.Empty();
		AssetsEditor.Empty();
		AssetsExtReferenced.Empty();
		AssetsExcluded.Empty();
		AssetsUsed.Empty();
		AssetsUnused.Empty();
	}
};
