// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerAsyncActions.h"

#include "ProjectCleaner.h"

void UProjectCleanerScanAction::Activate()
{
	Super::Activate();

	UE_LOG(LogProjectCleaner, Warning, TEXT("Scanning project"));

	ExecuteScanProject();
}

UProjectCleanerScanAction* UProjectCleanerScanAction::ScanProject(const FProjectCleanerScanSettings& ScanSettings)
{
	UProjectCleanerScanAction* BlueprintNode = NewObject<UProjectCleanerScanAction>();
	if (!BlueprintNode) return nullptr;

	BlueprintNode->ScanSettings = ScanSettings;

	return BlueprintNode;
}

void UProjectCleanerScanAction::ExecuteScanProject()
{
	FPlatformProcess::Sleep(2.0f);

	if (OnScanFinished.IsBound())
	{
		OnScanFinished.Broadcast(ScanResult);
	}
}
