// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerAsyncActions.h"
#include "ProjectCleanerSubsystem.h"

void UProjectCleanerScanAction::Activate()
{
	Super::Activate();

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
	if (!GEditor) return;
	UProjectCleanerSubsystem* Subsystem = GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>();
	if (!Subsystem) return;

	ScanData = Subsystem->ProjectScan(ScanSettings);
	ScanData.ScanResult == EProjectCleanerScanResult::Success ? OnScanFinished.Broadcast(ScanData) : OnScanFailed.Broadcast(ScanData);
}
