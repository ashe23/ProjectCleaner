// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerScanSettings.h"

UProjectCleanerScanSettings::UProjectCleanerScanSettings()
{
}

#if WITH_EDITOR
void UProjectCleanerScanSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// todo:ashe23 save only if settings are valid
	bool bIsValidSettings = true;
	for (const auto& ExcludedFolder : ExcludedFolders)
	{
		if (!FPaths::DirectoryExists(ExcludedFolder.Path))
		{
			bIsValidSettings = false;
			break;
		}
	}

	for (const auto& ExcludedClass : ExcludedClasses)
	{
		if (!ExcludedClass.IsValid())
		{
			bIsValidSettings = false;
			break;
		}
	}

	if (bIsValidSettings)
	{
		SaveConfig();
		
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
