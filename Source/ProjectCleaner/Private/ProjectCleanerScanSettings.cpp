// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerScanSettings.h"
#include "ProjectCleanerLibrary.h"
// Engine Headers
#include "EditorUtilityBlueprint.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "ProjectCleaner/Public/ProjectCleanerLibrary.h"

UProjectCleanerScanSettings::UProjectCleanerScanSettings()
{
	TArray<UClass*> PrimaryAssetClasses;
	UProjectCleanerLibrary::GetPrimaryAssetClasses(PrimaryAssetClasses);

	UsedAssetClasses.Append(PrimaryAssetClasses);
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
