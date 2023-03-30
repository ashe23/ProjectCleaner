// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "PjcDelegates.h"
#include "PjcTypes.h"
#include "PjcSubsystem.generated.h"

class UPjcExcludeSettings;

UCLASS(Config=EditorPerProjectUserSettings, meta=(ToolTip="ProjectCleanerSubsystem"))
class UPjcSubsystem final : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	UPROPERTY(BlueprintReadOnly, Config, Category="ProjectCleaner")
	bool bShowPathsEmpty = true;

	UPROPERTY(BlueprintReadOnly, Config, Category="ProjectCleaner")
	bool bShowPathsExcluded = true;

	UPROPERTY(BlueprintReadOnly, Config, Category="ProjectCleaner")
	bool bShowPathsEngineGenerated = true;

	UPROPERTY(BlueprintReadOnly, Config, Category="ProjectCleaner")
	bool bShowPathsUnusedOnly = false;
	
	void ProjectScan();
	const FPjcScanResult& GetLastScanResult();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(AdvancedDisplay="OutScanResult"))
	void ProjectScanBySettings(const FPjcScanSettings& InScanSettings, UPARAM(DisplayName="OutScanResult") FPjcScanResult& OutScanResult) const;

	FPjcDelegateOnProjectScan& OnProjectScan();

private:
	bool bScanningInProgress = false;
	bool bCleaningInProgress = false;

	FPjcScanResult LastScanResult;
	FPjcDelegateOnProjectScan DelegateOnProjectScan;
};
