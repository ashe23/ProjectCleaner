// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Settings/ProjectCleanerExcludeSettings.h"

FProjectCleanerDelegateExcludeSettingsChanged& UProjectCleanerExcludeSettings::OnChange()
{
	return DelegateExcludeSettingsChanged;
}

#if WITH_EDITOR
void UProjectCleanerExcludeSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();

	if (DelegateExcludeSettingsChanged.IsBound())
	{
		DelegateExcludeSettingsChanged.Broadcast(PropertyChangedEvent.GetPropertyName());
	}
}
#endif
