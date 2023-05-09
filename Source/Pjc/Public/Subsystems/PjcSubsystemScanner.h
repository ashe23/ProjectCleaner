// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "PjcTypes.h"
#include "PjcSubsystemScanner.generated.h"

DECLARE_MULTICAST_DELEGATE(FPjcDelegateProjectAssetsScanSuccess)
DECLARE_MULTICAST_DELEGATE(FPjcDelegateProjectFilesScanSuccess)
DECLARE_MULTICAST_DELEGATE_OneParam(FPjcDelegateProjectAssetsScanFail, const FString& ErrMsg)

UCLASS()
class UPjcScannerSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category="PjcScanner")
	void ScanProjectAssets(const FPjcAssetExcludeSettings& InExcludeSettings);
	void ScanProjectAssets();

	UFUNCTION(BlueprintCallable, Category="PjcScanner")
	void ScanProjectPaths(const FPjcFileExcludeSettings& InExcludeSettings);
	void ScanProjectPaths();
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="PjcScanner")
	const TSet<FAssetData>& GetAssetsByCategory(const EPjcAssetCategory AssetCategory);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="PjcScanner")
	const TSet<FString>& GetFilesByCategory(const EPjcFileCategory FileCategory);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="PjcScanner")
	const TSet<FString>& GetFoldersByCategory(const EPjcFolderCategory FolderCategory);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="PjcScanner")
	void GetAssetIndirectInfo(const FAssetData& InAssetData, TArray<FPjcFileInfo>& Infos);

	FPjcDelegateProjectAssetsScanFail& OnProjectAssetsScanFail();
	FPjcDelegateProjectFilesScanSuccess& OnProjectFilesScanSuccess();
	FPjcDelegateProjectAssetsScanSuccess& OnProjectAssetsScanSuccess();

protected:
	FString GetScanPreparationErrMsg() const;
	void FindAssetsIndirect();
	void FindAssetsExcluded(const FPjcAssetExcludeSettings& InExcludeSettings);
	void FixupRedirectorsInProject() const;

private:
	bool bIsIdle = true;

	TMap<EPjcAssetCategory, TSet<FAssetData>> MapAssets;
	TMap<EPjcFileCategory, TSet<FString>> MapFiles;
	TMap<EPjcFolderCategory, TSet<FString>> MapFolders;
	TMap<FAssetData, TArray<FPjcFileInfo>> AssetsIndirectInfos;

	FPjcDelegateProjectAssetsScanFail DelegateProjectAssetsScanFail;
	FPjcDelegateProjectAssetsScanSuccess DelegateProjectAssetsScanSuccess;
	FPjcDelegateProjectFilesScanSuccess DelegateProjectFilesScanSuccess;
};
