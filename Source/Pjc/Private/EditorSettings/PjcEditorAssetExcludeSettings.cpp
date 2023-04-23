// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "EditorSettings/PjcEditorAssetExcludeSettings.h"

#if WITH_EDITOR
void UPjcEditorAssetExcludeSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();
}
#endif
