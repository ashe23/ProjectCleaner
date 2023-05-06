// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "PjcTypes.h"
#include "PjcSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE(FPjcDelegateOnScanAssets);

UCLASS(Config=EditorPerProjectUserSettings, meta=(ToolTip="ProjectCleanerSubsystem"))
class UPjcSubsystem final : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void ToggleShowFoldersEmpty();
	void ToggleShowFoldersExcluded();

	bool CanShowFoldersEmpty() const;
	bool CanShowFoldersExcluded() const;

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner")
	void ScanProjectAssets();

	const TSet<FAssetData>& GetAssetsAll() const;
	const TSet<FAssetData>& GetAssetsUsed() const;
	const TSet<FAssetData>& GetAssetsUnused() const;
	const TSet<FAssetData>& GetAssetsPrimary() const;
	const TSet<FAssetData>& GetAssetsIndirect() const;
	const TSet<FAssetData>& GetAssetsEditor() const;
	const TSet<FAssetData>& GetAssetsExcluded() const;
	const TSet<FAssetData>& GetAssetsExtReferenced() const;
	int32 GetNumAssetsTotalInPath(const FString& InPath) const;
	int32 GetNumAssetsUsedInPath(const FString& InPath) const;
	int32 GetNumAssetsUnusedInPath(const FString& InPath) const;
	int64 GetSizeAssetsTotalInPath(const FString& InPath) const;
	int64 GetSizeAssetsUsedInPath(const FString& InPath) const;
	int64 GetSizeAssetsUnusedInPath(const FString& InPath) const;

	FPjcDelegateOnScanAssets& OnScanAssets();

private:
	UPROPERTY(Config)
	bool bShowFoldersEmpty = true;

	UPROPERTY(Config)
	bool bShowFoldersExcluded = true;

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	void CategorizeAssetsByPath();
	void UpdateMapInfo(TMap<FString, int32>& MapNum, TMap<FString, int64>& MapSize, const FString& AssetPath, int64 AssetSize);

	TSet<FAssetData> AssetsAll;
	TSet<FAssetData> AssetsUsed;
	TSet<FAssetData> AssetsUnused;
	TSet<FAssetData> AssetsPrimary;
	TSet<FAssetData> AssetsIndirect;
	TSet<FAssetData> AssetsEditor;
	TSet<FAssetData> AssetsExcluded;
	TSet<FAssetData> AssetsExtReferenced;
	TMap<FAssetData, FPjcAssetIndirectUsageInfo> AssetsIndirectInfoMap;
	TMap<FString, int32> MapNumAssetsAllByPath;
	TMap<FString, int32> MapNumAssetsUsedByPath;
	TMap<FString, int32> MapNumAssetsUnusedByPath;
	TMap<FString, int64> MapSizeAssetsAllByPath;
	TMap<FString, int64> MapSizeAssetsUsedByPath;
	TMap<FString, int64> MapSizeAssetsUnusedByPath;

	FPjcDelegateOnScanAssets DelegateOnScanAssets;
};
