// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "PjcDelegates.h"
#include "PjcTypes.h"
#include "PjcSubsystem.generated.h"

UCLASS(Config=EditorPerProjectUserSettings, meta=(ToolTip="ProjectCleanerSubsystem"))
class UPjcSubsystem final : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner")
	void ProjectScan(const FPjcAssetExcludeSettings& InAssetExcludeSetting, FPjcScanResult& ScanResult) const;

	void ScanAssets(const FPjcAssetExcludeSettings& InAssetExcludeSetting, FPjcScanDataAssets& ScanDataAssets) const;
	void ScanFiles(FPjcScanDataFiles& ScanDataFiles) const;

	FPjcDelegateOnScanAssets& OnScanAssets();
	FPjcDelegateOnScanFiles& OnScanFiles();

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	bool CanScanProject(FString& ErrMsg) const;

	bool bScanningInProgress = false;
	bool bCleaningInProgress = false;

	FPjcDelegateOnScanAssets DelegateOnScanAssets;
	FPjcDelegateOnScanFiles DelegateOnScanFiles;
};
