// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Settings/ProjectCleanerExcludeSettings.h"


#if WITH_EDITOR
void UProjectCleanerExcludeSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();

	if (DelegateExcludeSettingChanged.IsBound())
	{
		DelegateExcludeSettingChanged.Broadcast(PropertyChangedEvent.GetPropertyName());
	}
}
#endif

FProjectCleanerDelegateExcludeSettingsChanged& UProjectCleanerExcludeSettings::OnChange()
{
	return DelegateExcludeSettingChanged;
}
