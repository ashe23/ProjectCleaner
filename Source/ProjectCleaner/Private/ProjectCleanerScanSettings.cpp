// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerScanSettings.h"
// Engine Headers
#include "EditorUtilityBlueprint.h"
#include "EditorUtilityWidgetBlueprint.h"

UProjectCleanerScanSettings::UProjectCleanerScanSettings()
{
	UsedAssetClasses.Add(UWorld::StaticClass()); // todo::ashe23 change to primary assets classes later
	UsedAssetClasses.Add(UEditorUtilityBlueprint::StaticClass());
	UsedAssetClasses.Add(UEditorUtilityWidgetBlueprint::StaticClass());
}

#if WITH_EDITOR
void UProjectCleanerScanSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();

	if (OnChangeDelegate.IsBound())
	{
		OnChangeDelegate.Execute();
	}
}
#endif

FProjectCleanerScanSettingsChangeDelegate& UProjectCleanerScanSettings::OnChange()
{
	return OnChangeDelegate;
}
