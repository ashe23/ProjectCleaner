// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerExcludeSettings.h"

UProjectCleanerExcludeSettings::UProjectCleanerExcludeSettings()
{
}

#if WITH_EDITOR
void UProjectCleanerExcludeSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();

	if (DelegateExcludeSettingChanged.IsBound())
	{
		DelegateExcludeSettingChanged.Broadcast();
	}
}
#endif

FProjectCleanerDelegateExcludeSettingsChanged& UProjectCleanerExcludeSettings::OnChange()
{
	return DelegateExcludeSettingChanged;
}
