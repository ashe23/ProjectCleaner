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

	bool IsLoadingAssets() const;
	void AnalyzeProject();
	void PrintInfo() const;

	// cli
	void SetExcludeClasses(const TArray<FString>& Classes);
	void SetExcludePaths(const TArray<FString>& Paths);
	void SetUserExcludedAssets(const TArray<FString>& Assets);

	// UI Actions
	virtual void ExcludeSelectedAssets(const TArray<FAssetData>& Assets) override;
	virtual void ExcludeSelectedAssetsByType(const TArray<FAssetData>& Assets) override;
	virtual bool IncludeSelectedAssets(const TArray<FAssetData>& Assets) override;
	virtual void IncludeAllAssets() override;
	virtual bool ExcludePath(const FString& InPath) override;
	virtual bool IncludePath(const FString& InPath) override;
	virtual int32 DeleteSelectedAssets(const TArray<FAssetData>& Assets) override;
	virtual int32 DeleteAllUnusedAssets() override;
	virtual int32 DeleteEmptyFolders() override;

	// getters
	const FAssetRegistryModule* GetAssetRegistry() const;
	const TArray<FAssetData>& GetAllAssets() const;
	const TArray<FAssetData>& GetUnusedAssets() const;
	const TSet<FName>& GetExcludedAssets() const;
	const TSet<FName>& GetCorruptedAssets() const;
	const TSet<FName>& GetNonEngineFiles() const;
	const TSet<FName>& GetEmptyFolders() const;
	const TSet<FName>& GetPrimaryAssetClasses() const;
	const TMap<FAssetData, FIndirectAsset>& GetIndirectAssets() const;
	
	// setters
	void SetCleanerConfigs(const UCleanerConfigs* CleanerConfigs);
	void SetSilentMode(const bool SilentMode);
	void SetScanDeveloperContents(const bool bScan);
	
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
	void FillBucketWithAssets(TArray<FAssetData>& Bucket, const int32 BucketSize);
	bool PrepareBucketForDeletion(const TArray<FAssetData>& Bucket, TArray<UObject*>& LoadedAssets);
	int32 DeleteBucket(const TArray<UObject*>& LoadedAssets);
	void CleanupAfterDelete();

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

	/* Configs */
	bool bSilentMode;
	bool bScanDeveloperContents;
	bool bAutomaticallyDeleteEmptyFolders;
	TSet<FName> ExcludedPaths;
	TSet<FName> ExcludedClasses;
	bool bCancelledByUser;

	/* Engine Modules */
	FAssetRegistryModule* AssetRegistry;
	FAssetToolsModule* AssetTools;
	IPlatformFile* PlatformFile;

	/* Constants */
	const FName RelativeRoot;
};