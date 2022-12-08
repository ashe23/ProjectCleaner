// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerDelegates.h"
#include "ProjectCleanerScanSettings.generated.h"

UCLASS(Transient, Config=EditorPerProjectUserSettings)
class UProjectCleanerScanSettings final : public UObject
{
	GENERATED_BODY()

public:
	UProjectCleanerScanSettings();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(EditAnywhere, Config, Category="ScanSettings",
		meta=(ToolTip="Automatically scan the project when scan settings change. On large projects, this can be unfavorable. By default, it is disabled."))
	bool bAutoScan = false;

	UPROPERTY(DisplayName="Auto Delete Empty Folders", EditAnywhere, Config, Category="ScanSettings",
		meta=(ToolTip="Automatically delete empty folders after cleaning a project of unused assets. By default, it is enabled."))
	bool bAutoDeleteEmptyFolders = true;

	UPROPERTY(DisplayName="Scan Developers Content", EditAnywhere, Config, Category="ScanSettings", meta=(ToolTip="Scan the 'Developers' folder for unused assets. By default, it is disabled."))
	bool bScanDeveloperContents = false;

	FProjectCleanerDelegateScanSettingsChanged& OnChange();

private:
	FProjectCleanerDelegateScanSettingsChanged DelegateScanSettingsChanged;
};
