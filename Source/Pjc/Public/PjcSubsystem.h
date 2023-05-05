// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "PjcTypes.h"
#include "PjcSubsystem.generated.h"

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

	void ScanProjectAssets();

	const TSet<FAssetData>& GetAssetsAll() const;

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
	TSet<FAssetData> AssetsAll;
	TSet<FAssetData> AssetsUsed;
	TSet<FAssetData> AssetsUnused;
	TSet<FAssetData> AssetsPrimary;
	TSet<FAssetData> AssetsIndirect;
	TSet<FAssetData> AssetsEditor;
	TSet<FAssetData> AssetsExcluded;
	TSet<FAssetData> AssetsExtReferenced;
	TMap<FAssetData, FPjcAssetIndirectUsageInfo> AssetsIndirectInfoMap;
};
