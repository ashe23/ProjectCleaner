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
		if (OnChangeDelegate.IsBound())
		{
			OnChangeDelegate.Broadcast();
		}
	}
}
#endif

FProjectCleanerScanSettingsChangeDelegate& UProjectCleanerScanSettings::OnChange()
{
	return OnChangeDelegate;
}
