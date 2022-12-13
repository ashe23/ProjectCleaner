// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Settings/ProjectCleanerExcludeSettings.h"

void UProjectCleanerExcludeSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();
}
