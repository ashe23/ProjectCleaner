// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/ProjectCleanerDataManager.h"

struct FIndirectAsset;
class UCleanerConfigs;
class FAssetRegistryModule;

DECLARE_DELEGATE(FOnCleanerManagerUpdated);

class FProjectCleanerManager : public ICleanerUIActions
{
public:
	FProjectCleanerManager();
	virtual ~FProjectCleanerManager() override;

	// UI actions
	void Update();
	virtual void ExcludeSelectedAssets(const TArray<FAssetData>& Assets) override;
	virtual void ExcludeSelectedAssetsByType(const TArray<FAssetData>& Assets) override;
	virtual bool ExcludePath(const FString& InPath) override;
	virtual bool IncludePath(const FString& InPath) override;
	virtual bool IncludeSelectedAssets(const TArray<FAssetData>& Assets) override;
	virtual void IncludeAllAssets() override;
	virtual int32 DeleteSelectedAssets(const TArray<FAssetData>& Assets) override;
	virtual int32 DeleteAllUnusedAssets() override;
	virtual int32 DeleteEmptyFolders() override;

	// getters
	const FProjectCleanerDataManager& GetDataManager() const;
	const TArray<FAssetData>& GetAllAssets() const;
	const TArray<FAssetData>& GetUnusedAssets() const;
	const TSet<FName>& GetExcludedAssets() const;
	const TSet<FName>& GetCorruptedAssets() const;
	const TSet<FName>& GetNonEngineFiles() const;
	const TMap<FAssetData, FIndirectAsset>& GetIndirectAssets() const;
	const TSet<FName>& GetEmptyFolders() const;
	const TSet<FName>& GetPrimaryAssetClasses() const;
	UCleanerConfigs* GetCleanerConfigs() const;
	float GetUnusedAssetsPercent() const;

	/**
	 * @brief Delegate, called when data updates
	 */
	FOnCleanerManagerUpdated OnCleanerManagerUpdated;
private:
	class UCleanerConfigs* CleanerConfigs;
	FProjectCleanerDataManager DataManager;
};
