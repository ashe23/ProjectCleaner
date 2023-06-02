// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcTypes.generated.h"

UCLASS(Config = EditorPerProjectUserSettings)
class UPjcAssetExcludeSettings : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Config, Category="AssetExcludeSettings", meta=(ContentDir, ToolTip="Consider assets in specified folders as used"))
	TArray<FDirectoryPath> ExcludedFolders;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Config, Category="AssetExcludeSettings", meta=(ToolTip="Consider assets of specified classes as used"))
	TArray<TSoftClassPtr<UObject>> ExcludedClasses;

	UPROPERTY(Config)
	TArray<TSoftObjectPtr<UObject>> ExcludedAssets;

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override
	{
		Super::PostEditChangeProperty(PropertyChangedEvent);

		SaveConfig();
	}
#endif
};

UCLASS(Config = EditorPerProjectUserSettings)
class UPjcFileExcludeSettings : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Config, Category="FileExcludeSettings", meta=(RelativeToGameDir, ToolTip="Exclude specified files from scanning"))
	TArray<FFilePath> ExcludedFiles;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Config, Category="FileExcludeSettings", meta=(ToolTip="Exclude files with specified extensions from scanning"))
	TArray<FString> ExcludedExtensions;

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override
	{
		Super::PostEditChangeProperty(PropertyChangedEvent);

		SaveConfig();
	}
#endif
};

struct FPjcStatItem
{
	FText Name;
	FText Num;
	FText Size;
	FText TooltipName;
	FText ToolTipNum;
	FText ToolTipSize;
	FLinearColor TextColor{FLinearColor::White};
	FMargin NamePadding{FMargin{0.0f}};
};

struct FPjcFileExternalItem
{
	int64 FileSize = 0;
	bool bExcluded = false;
	FString FileName;
	FString FileExt;
	FString FilePath;
};

struct FPjcCorruptedAssetItem
{
	int64 FileSize = 0;
	FString FileName;
	FString FileExt;
	FString FilePath;
};

struct FPjcTreeItem
{
	FString FolderPath;
	FString FolderName;
	bool bIsDev = false;
	bool bIsRoot = false;
	bool bIsEmpty = false;
	bool bIsExcluded = false;
	bool bIsExpanded = false;
	bool bIsVisible = false;
	int32 NumAssetsTotal = 0;
	int32 NumAssetsUsed = 0;
	int32 NumAssetsUnused = 0;
	float SizeAssetsUnused = 0;
	float PercentageUnused = 0;
	float PercentageUnusedNormalized = 0;

	TSharedPtr<FPjcTreeItem> Parent;
	TArray<TSharedPtr<FPjcTreeItem>> SubItems;

	bool operator==(const FPjcTreeItem& Other) const
	{
		return FolderPath.Equals(Other.FolderPath);
	}

	bool operator!=(const FPjcTreeItem& Other) const
	{
		return !FolderPath.Equals(Other.FolderPath);
	}
};

USTRUCT(BlueprintType)
struct FPjcFileInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="FileInfo")
	int32 FileNum = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="FileInfo")
	FString FilePath;

	bool operator==(const FPjcFileInfo& Other) const
	{
		return FilePath.Equals(Other.FilePath) && FileNum == Other.FileNum;
	}

	bool operator!=(const FPjcFileInfo& Other) const
	{
		return !(FilePath.Equals(Other.FilePath) && FileNum == Other.FileNum);
	}
};

USTRUCT(BlueprintType)
struct FPjcAssetIndirectInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="AssetIndirectInfo")
	FAssetData Asset;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="AssetIndirectInfo")
	FString FilePath;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="AssetIndirectInfo")
	int32 FileNum = 0;

	bool operator==(const FPjcAssetIndirectInfo& Other) const
	{
		return Asset == Other.Asset && FilePath.Equals(Other.FilePath) && FileNum == Other.FileNum;
	}

	bool operator!=(const FPjcAssetIndirectInfo& Other) const
	{
		return !(Asset == Other.Asset && FilePath.Equals(Other.FilePath) && FileNum == Other.FileNum);
	}
};
