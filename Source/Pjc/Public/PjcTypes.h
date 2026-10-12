// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "Engine/EngineTypes.h"
#include "Layout/Margin.h"
#include "PjcTypes.generated.h"

UCLASS(Config = EditorPerProjectUserSettings)
class UPjcAssetExcludeSettings : public UObject
{
	GENERATED_BODY()

  public:

	UPROPERTY(
		BlueprintReadWrite,
		EditAnywhere,
		Config,
		Category = "AssetExcludeSettings",
		meta = (ContentDir, ToolTip = "Consider assets in specified folders as used")
	)
	TArray<FDirectoryPath> ExcludedFolders;

	UPROPERTY(
		BlueprintReadWrite,
		EditAnywhere,
		Config,
		Category = "AssetExcludeSettings",
		meta = (ToolTip = "Consider assets of specified classes as used")
	)
	TArray<TSoftClassPtr<UObject>> ExcludedClasses;

	UPROPERTY(Config)
	TArray<TSoftObjectPtr<UObject>> ExcludedAssets;

  protected:

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override {
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

	UPROPERTY(
		BlueprintReadWrite,
		EditAnywhere,
		Config,
		Category = "FileExcludeSettings",
		meta = (RelativeToGameDir, ToolTip = "Exclude specified files from scanning")
	)
	TArray<FFilePath> ExcludedFiles;

	UPROPERTY(
		BlueprintReadWrite,
		EditAnywhere,
		Config,
		Category = "FileExcludeSettings",
		meta = (ToolTip = "Exclude files with specified extensions from scanning")
	)
	TArray<FString> ExcludedExtensions;

  protected:

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override {
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
	FLinearColor TextColor {FLinearColor::White};
	FMargin NamePadding {FMargin {0.0f}};
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

	bool operator==(const FPjcTreeItem& Other) const {
		return FolderPath.Equals(Other.FolderPath);
	}

	bool operator!=(const FPjcTreeItem& Other) const {
		return !FolderPath.Equals(Other.FolderPath);
	}
};

USTRUCT(BlueprintType)
struct FPjcFileInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "FileInfo")
	int32 FileNum = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "FileInfo")
	FString FilePath;

	bool operator==(const FPjcFileInfo& Other) const {
		return FilePath.Equals(Other.FilePath) && FileNum == Other.FileNum;
	}

	bool operator!=(const FPjcFileInfo& Other) const {
		return !(FilePath.Equals(Other.FilePath) && FileNum == Other.FileNum);
	}
};

USTRUCT(BlueprintType)
struct FPjcAssetIndirectInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "AssetIndirectInfo")
	FAssetData Asset;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "AssetIndirectInfo")
	FString FilePath;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "AssetIndirectInfo")
	int32 FileNum = 0;

	bool operator==(const FPjcAssetIndirectInfo& Other) const {
		return Asset == Other.Asset && FilePath.Equals(Other.FilePath) && FileNum == Other.FileNum;
	}

	bool operator!=(const FPjcAssetIndirectInfo& Other) const {
		return !(Asset == Other.Asset && FilePath.Equals(Other.FilePath) && FileNum == Other.FileNum);
	}
};

/** Why an asset counts as used, see UPjcSubsystem::GetAssetsUsage */
UENUM(BlueprintType)
enum class EPjcUsageReason : uint8
{
	/** nothing used references it */
	None,
	/** Asset Manager primary asset (maps, primary asset labels, types from project settings or game features) */
	Primary,
	/** referenced from outside scanned content: engine, not scanned plugins, actors saved in their own files */
	EngineReferenced,
	/** named in project or plugin Source/Config files */
	Indirect,
	/** exclude settings, always used classes or cinematic assets */
	Excluded,
	/** editor utility blueprint or widget */
	Editor,
	/** GameplayCue notify, loaded by its tag */
	GameplayCue,
	/** Megascans preset */
	Megascans,
	/** listed explicitly by a Primary Asset Label */
	FolderLabelAsset,
	/** referenced by another used asset (Parent) */
	Dependency,
	/** in folder of a Primary Asset Label with "Label Assets In My Directory" (Parent) */
	FolderLabel,
};

USTRUCT(BlueprintType)
struct FPjcAssetUsage
{
	GENERATED_BODY()

	/** False for unused assets and for assets only folder labels keep while "Folder Labels Used" is off */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "AssetUsage")
	bool bUsed = false;

	/** Only a folder label keeps it, directly or through assets that only a folder label keeps */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "AssetUsage")
	bool bFolderLabelOnly = false;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "AssetUsage")
	EPjcUsageReason Reason = EPjcUsageReason::None;

	/** Package that pulled it in (Dependency, FolderLabel, FolderLabelAsset). Following parents leads to the asset that is used for its own reason */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "AssetUsage")
	FName Parent;

	/** Reason specifics: primary asset id, file and line, exclude rule, referencing package outside scanned content */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "AssetUsage")
	FString Detail;
};

/** Package shown in Asset Usage tab */
struct FPjcUsageItem
{
	FName PackageName;
	/** main asset of package, invalid when registry has no asset for it */
	FAssetData Asset;
	FPjcAssetUsage Usage;
	/** why last cleanup kept it (unused assets only), empty if it was not tried */
	FString Kept;
	FString ClassName;
	/** short reason for lists */
	FString ReasonText;
	int64 Size = 0;
	/** inside scanned content roots, so it has a usage result */
	bool bScanned = false;
};
