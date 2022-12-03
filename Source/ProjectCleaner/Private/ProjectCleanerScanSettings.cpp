// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerScanSettings.h"

UProjectCleanerScanSettings::UProjectCleanerScanSettings()
{
}

#if WITH_EDITOR
void UProjectCleanerScanSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();

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
