// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcTypes.generated.h"

UENUM(BlueprintType)
enum class EPjcAssetCategory : uint8
{
	None UMETA(Hidden),
	Any,
	Used,
	Unused,
	Primary,
	Indirect,
	Circular,
	Editor,
	Excluded,
	ExtReferenced
};

UENUM(BlueprintType)
enum class EPjcFileCategory : uint8
{
	None UMETA(Hidden),
	Any,
	External,
	Excluded,
	Corrupted
};

UENUM(BlueprintType)
enum class EPjcFolderCategory : uint8
{
	None UMETA(Hidden),
	Any,
	Empty
};

UCLASS(Config = EditorPerProjectUserSettings)
class UPjcEditorAssetExcludeSettings : public UObject
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

USTRUCT(BlueprintType)
struct FPjcAssetExcludeSettings
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="AssetExcludeSettings", meta=(ContentDir, ToolTip="Consider assets in specified folders as used"))
	TArray<FString> ExcludedFolders;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="AssetExcludeSettings", meta=(ToolTip="Consider assets of specified classes as used"))
	TArray<FString> ExcludedClasses;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="AssetExcludeSettings", meta=(ToolTip="Consider specified assets as used"))
	TArray<FString> ExcludedAssets;

	void Reset()
	{
		ExcludedFolders.Reset();
		ExcludedClasses.Reset();
		ExcludedAssets.Reset();
	}
};

UCLASS(Config = EditorPerProjectUserSettings)
class UPjcEditorFileExcludeSettings : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Config, Category="FileExcludeSettings", meta=(ContentDir, ToolTip="Consider files in specified folders as used"))
	TArray<FDirectoryPath> ExcludedFolders;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Config, Category="FileExcludeSettings", meta=(ToolTip="Consider specified files as used"))
	TArray<FFilePath> ExcludedFiles;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Config, Category="FileExcludeSettings", meta=(ToolTip="Consider files with specified extensions as used"))
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

USTRUCT(BlueprintType)
struct FPjcFileExcludeSettings
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FileExcludeSettings", meta=(ToolTip="Consider files in specified folders as used"))
	TArray<FString> ExcludedFolders;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FileExcludeSettings", meta=(ToolTip="Consider specified files as used"))
	TArray<FString> ExcludedFiles;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FileExcludeSettings", meta=(ToolTip="Consider files with specified extensions as used"))
	TArray<FString> ExcludedExtensions;

	void Reset()
	{
		ExcludedFolders.Reset();
		ExcludedFiles.Reset();
		ExcludedExtensions.Reset();
	}
};

USTRUCT(BlueprintType)
struct FPjcScanResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ScanResult")
	bool bScanSuccess = true;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="ScanResult")
	FString ScanErrMsg;
};

USTRUCT(BlueprintType)
struct FPjcAssetSearchFilter
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="SearchFilter")
	bool bRecursivePaths = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="SearchFilter")
	bool bRecursiveClasses = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="SearchFilter")
	TArray<FName> PackagePaths;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="SearchFilter")
	TArray<FName> ClassNames;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="SearchFilter")
	TArray<FName> ObjectPaths;

	bool IsEmpty() const
	{
		return PackagePaths.Num() == 0 && ClassNames.Num() == 0 && ObjectPaths.Num() == 0;
	}

	bool IsRecursive() const
	{
		return bRecursivePaths || bRecursiveClasses;
	}

	void Clear()
	{
		bRecursivePaths = false;
		bRecursiveClasses = false;
		PackagePaths.Reset();
		ClassNames.Reset();
		ObjectPaths.Reset();
	}
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
	int32 FileNum;

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
