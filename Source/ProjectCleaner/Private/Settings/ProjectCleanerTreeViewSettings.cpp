// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Settings/ProjectCleanerTreeViewSettings.h"

#if WITH_EDITOR
void UProjectCleanerTreeViewSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();
}
#endif
