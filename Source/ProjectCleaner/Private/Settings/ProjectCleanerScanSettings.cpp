// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Settings/ProjectCleanerScanSettings.h"
#include "ProjectCleanerDelegates.h"

#if WITH_EDITOR
void UProjectCleanerScanSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();

	if (DelegateScanSettingsChanged.IsBound())
	{
		DelegateScanSettingsChanged.Broadcast(PropertyChangedEvent.GetPropertyName());
	}
}
#endif

FProjectCleanerDelegateScanSettingsChanged& UProjectCleanerScanSettings::OnChange()
{
	return DelegateScanSettingsChanged;
}
