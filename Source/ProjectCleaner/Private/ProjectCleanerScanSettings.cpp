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

	if (DelegateScanSettingsChanged.IsBound())
	{
		DelegateScanSettingsChanged.Broadcast();
	}
}
#endif

FProjectCleanerDelegateScanSettingsChanged& UProjectCleanerScanSettings::OnChange()
{
	return DelegateScanSettingsChanged;
}
