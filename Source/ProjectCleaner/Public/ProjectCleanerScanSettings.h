// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerScanSettings.generated.h"

UCLASS(Transient, Config=EditorPerProjectUserSettings)
class UProjectCleanerScanSettings final : public UObject
{
	GENERATED_BODY()
public:
	UProjectCleanerScanSettings();
	
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	
	UPROPERTY(DisplayName="Scan Developer Content", EditAnywhere, Config, Category="ScanSettings", meta=(ToolTip="Scan assets in 'Developers' folder. By Default false"))
	bool bScanDeveloperContents = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Config, Category = "ScanSettings", meta=(ToolTip="Automatically delete empty folders after deleting unused assets"))
	bool bAutomaticallyDeleteEmptyFolders = true;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Config, Category="ScanSettings", DisplayName="Used Assets Classes",
		meta=(ToolTip="List of asset classes that are considered used in project. This type of assets and their dependencies will never be scanned and wont be deleted."))
	TArray<TSoftClassPtr<UObject>> UsedAssetClasses;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Config, Category="ExcludeOptions", meta=(ContentDir, ToolTip="Exclude assets and their dependencies inside specified paths"))
	TArray<FDirectoryPath> ExcludedDirectories;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Config, Category="ExcludeOptions", meta=(ToolTip="Exclude assets and their dependencies of specified classes"))
	TArray<TSoftClassPtr<UObject>> ExcludedClasses;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Config, Category="ExcludeOptions", meta=(ToolTip="Exclude specified assets and their dependencies from scanning"))
	TArray<TSoftObjectPtr<UObject>> ExcludedAssets;
};