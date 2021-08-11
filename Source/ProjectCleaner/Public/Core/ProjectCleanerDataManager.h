// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "StructsContainer.h"
#include "CoreMinimal.h"

struct FAssetData;
class FAssetToolsModule;
class FAssetRegistryModule;
class IPlatformFile;

class FProjectCleanerDataManager : public ICleanerUIActions
{
public:

	// ctor/dtor
	FProjectCleanerDataManager();
	virtual ~FProjectCleanerDataManager() override;

	void AnalyzeProject();
	void SetSilentMode(const bool SilentMode);
	void SetScanDeveloperContents(const bool bScan);
	void PrintInfo();
	
	void CleanProject();

	// UI Actions
	virtual void ExcludeSelectedAssets(const TArray<FAssetData>& Assets) override;
	virtual void ExcludeSelectedAssetsByType(const TArray<FAssetData>& Assets) override;
	virtual void IncludeSelectedAssets(const TArray<FAssetData>& Assets) override;
	virtual void IncludeAllAssets() override;
	virtual void ExcludePath(const FString& InPath) override;
	virtual void IncludePath(const FString& InPath) override;
	virtual int32 DeleteSelectedAssets(const TArray<FAssetData>& Assets) override;
	virtual void DeleteAllUnusedAssets() override;
	virtual void DeleteEmptyFolders() override;

	// getters
	const FAssetRegistryModule* GetAssetRegistry() const;
	const TArray<FAssetData>& GetAllAssets() const;
	const TArray<FAssetData>& GetUnusedAssets() const;
	const TSet<FName>& GetExcludedAssets() const;
	const TSet<FName>& GetCorruptedAssets() const;
	const TSet<FName>& GetNonEngineFiles() const;
	const TMap<FAssetData, FIndirectAsset>& GetIndirectAssets() const;
	const TSet<FName>& GetEmptyFolders() const;
	const TSet<FName>& GetPrimaryAssetClasses() const;
	
	// setters
	void SetCleanerConfigs(const UCleanerConfigs* CleanerConfigs);
	
	
private:
	
	void FixupRedirectors() const;
	void FindAllAssets();
	void FindInvalidFilesAndAssets();
	void FindIndirectAssets();
	void FindEmptyFolders(const bool bScanDevelopersContent);
	void FindPrimaryAssetClasses();
	void FindAssetsWithExternalReferencers();
	void FindUnusedAssets();
	void FindUsedAssets(TSet<FName>& UsedAssets);
	void FindUsedAssetsDependencies(const TSet<FName>& UsedAssets, TSet<FName>& UsedAssetsDeps) const;
	void FindExcludedAssets(TSet<FName>& UsedAssets);
	void AnalyzeUnusedAssets();
	void GetDeletionAssetsBucket(TArray<FAssetData>& Bucket, const int32 BucketSize);

	/* Check Functions */
	bool IsExcludedByClass(const FAssetData& AssetData) const;
	bool IsExcludedByPath(const FAssetData& AssetData) const;
	
	/* Data Containers */
	TArray<FAssetData> AllAssets;
	TArray<FAssetData> UnusedAssets;
	TArray<FAssetData> PrimaryAssets;
	TArray<FAssetData> UserExcludedAssets;
	TArray<FAssetData> AssetsWithExternalRefs;
	TSet<FName> CorruptedAssets;
	TSet<FName> NonEngineFiles;
	TSet<FName> EmptyFolders;
	TSet<FName> PrimaryAssetClasses;
	TSet<FName> ExcludedAssets;
	TMap<FAssetData, FIndirectAsset> IndirectAssets;
	TMap<FAssetData, FUnusedAssetInfo> UnusedAssetsInfos;

	/* Configs */
	bool bSilentMode;
	bool bScanDeveloperContents;
	bool bAutomaticallyDeleteEmptyFolders;
	TSet<FName> ExcludedPaths;
	TSet<FName> ExcludedClasses;

	/* Engine Modules */
	FAssetRegistryModule* AssetRegistry;
	FAssetToolsModule* AssetTools;
	IPlatformFile* PlatformFile;

	/* Constants */
	const FName RelativeRoot;
};