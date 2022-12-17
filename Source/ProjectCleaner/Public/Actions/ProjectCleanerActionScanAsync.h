// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintAsyncActionBase.h"
#include "ProjectCleanerTypes.h"
#include "ProjectCleanerActionScanAsync.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FProjectCleanerScanDelegate, const FProjectCleanerScanData&, ScanData);

UCLASS()
class UProjectCleanerActionScanAsync final : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	virtual void Activate() override;

	UFUNCTION(BlueprintCallable, Category="ProjectCleanerScanner", meta=(ToolTip="Scans project for unused assets, empty folders and other scan data info.", BlueprintInternalUseOnly="true"))
	static UProjectCleanerActionScanAsync* ScanProject(const FProjectCleanerScanSettings& ScanSettings);

private:
	UFUNCTION()
	void ExecuteScanProject();

public:
	UPROPERTY(BlueprintAssignable)
	FProjectCleanerScanDelegate OnScanFailed;
	UPROPERTY(BlueprintAssignable)
	FProjectCleanerScanDelegate OnScanFinished;

private:
	FProjectCleanerScanSettings ScanSettings;
	FProjectCleanerScanData ScanData;
};
