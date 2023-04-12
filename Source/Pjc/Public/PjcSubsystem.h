// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "PjcDelegates.h"
#include "PjcSubsystem.generated.h"

class UPjcExcludeSettings;

UCLASS(Config=EditorPerProjectUserSettings, meta=(ToolTip="ProjectCleanerSubsystem"))
class UPjcSubsystem final : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void ProjectScan();
	void ProjectClean();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(AdvancedDisplay="OutScanResult"))
	void ProjectScanBySettings(const FPjcExcludeSettings& InExcludeSettings, UPARAM(DisplayName="OutScanResult") FPjcScanResult& OutScanResult) const;

	FPjcDelegateOnProjectScan& OnProjectScan();
	const FPjcScanResult& GetLastScanResult() const;

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	void ScanAssets(const FPjcExcludeSettings& InExcludeSettings, FPjcScanResult& OutScanResult) const;
	void ScanFiles(FPjcScanResult& OutScanResult) const;
	void ScanFolders(FPjcScanResult& OutScanResult) const;
	void ScanStatsUpdate(FPjcScanResult& InScanResult) const;
	
	bool bScanningInProgress = false;
	bool bCleaningInProgress = false;
	FPjcScanResult LastScanResult;
	FPjcDelegateOnProjectScan DelegateOnProjectScan;
};
