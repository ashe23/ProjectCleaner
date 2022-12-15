// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerAsyncActions.h"

#include "ProjectCleaner.h"
#include "ProjectCleanerSubsystem.h"

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
	ScanResult = GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>()->ScanProject(ScanSettings);

	// ScanResult.bSuccess ? OnScanFailed.Broadcast() : OnScanFinished.Broadcast();

	OutScanResult.Broadcast(ScanResult);
}
