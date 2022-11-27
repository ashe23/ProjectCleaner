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

	UPROPERTY(DisplayName="Scan Developer Content", EditAnywhere, Config, Category="ScanSettings", meta=(ToolTip="Scan assets in 'Developers' folder. By Default false"))
	bool bScanDeveloperContents = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Config, Category="ExcludeOptions", DisplayName="Excluded Folders",
		meta=(ContentDir, ToolTip="Exclude from scanning assets inside this folders"))
	TArray<FDirectoryPath> ExcludedDirectories;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Config, Category="ExcludeOptions", meta=(ToolTip="Exclude from scanning assets of specified classes"))
	TArray<TSoftClassPtr<UObject>> ExcludedClasses;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Config, Category="ExcludeOptions", meta=(ToolTip="Exclude from scanning specified assets"))
	TArray<TSoftObjectPtr<UObject>> ExcludedAssets;

	FProjectCleanerScanSettingsChangeDelegate& OnChange();
private:
	FProjectCleanerScanSettingsChangeDelegate OnChangeDelegate;
};
