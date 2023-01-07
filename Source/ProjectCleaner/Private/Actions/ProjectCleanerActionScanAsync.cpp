// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Actions/ProjectCleanerActionScanAsync.h"
#include "ProjectCleanerSubsystem.h"

void UProjectCleanerActionScanAsync::Activate()
{
	Super::Activate();

	ExecuteScanProject();
}

UProjectCleanerActionScanAsync* UProjectCleanerActionScanAsync::ScanProject(const FProjectCleanerScanSettings& ScanSettings)
{
	UProjectCleanerActionScanAsync* BlueprintNode = NewObject<UProjectCleanerActionScanAsync>();
	if (!BlueprintNode) return nullptr;

	BlueprintNode->ScanSettings = ScanSettings;

	return BlueprintNode;
}

void UProjectCleanerActionScanAsync::ExecuteScanProject()
{
	if (!GEditor) return;
	UProjectCleanerSubsystem* Subsystem = GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>();
	if (!Subsystem) return;

	// Subsystem->ProjectScan(ScanSettings);
	// ScanData = Subsystem->GetScanData();
	// ScanData.ScanResult == EProjectCleanerScanResult::Success ? OnScanFinished.Broadcast(ScanData) : OnScanFailed.Broadcast(ScanData);
}
