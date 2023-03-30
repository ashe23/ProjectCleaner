// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcExcludeSettings.h"

#if WITH_EDITOR
void UPjcExcludeSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();
}
#endif
