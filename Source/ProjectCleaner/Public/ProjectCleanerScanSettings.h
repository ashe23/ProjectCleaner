// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerScanSettings.generated.h"

DECLARE_MULTICAST_DELEGATE(FProjectCleanerScanSettingsChangeDelegate);

UCLASS(Transient, Config=EditorPerProjectUserSettings)
class UProjectCleanerScanSettings final : public UObject
{
	GENERATED_BODY()
public:
	UProjectCleanerScanSettings();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(DisplayName="Scan Developers Content", EditAnywhere, Config, Category="ScanSettings", meta=(ToolTip="Scan assets in 'Developers' folder. By Default false"))
	bool bScanDeveloperContents = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Config, Category="ExcludeOptions", DisplayName="Excluded Folders",
		meta=(ContentDir, ToolTip="Exclude from scanning assets inside this folders"))
	TArray<FDirectoryPath> ExcludedFolders;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Config, Category="ExcludeOptions", meta=(ToolTip="Exclude from scanning assets of specified classes"))
	TArray<TSoftClassPtr<UObject>> ExcludedClasses;

	UPROPERTY(Config)
	TArray<FAssetData> ExcludedAssets;

	FProjectCleanerScanSettingsChangeDelegate& OnChange();
private:
	FProjectCleanerScanSettingsChangeDelegate OnChangeDelegate;
};
