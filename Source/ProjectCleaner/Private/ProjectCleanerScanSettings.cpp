// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerScanSettings.h"

#include "ProjectCleaner.h"

UProjectCleanerScanSettings::UProjectCleanerScanSettings()
{
}

#if WITH_EDITOR
void UProjectCleanerScanSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();

	UE_LOG(LogProjectCleaner, Warning, TEXT("Scan Settings Changed"));
	
	if (bAutoScan)
	{
		if (DelegateScanSettingsChanged.IsBound())
		{
			DelegateScanSettingsChanged.Broadcast();
		}
	}
}
#endif

FProjectCleanerDelegateScanSettingsChanged& UProjectCleanerScanSettings::OnChange()
{
	return DelegateScanSettingsChanged;
}
