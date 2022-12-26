// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Settings/ProjectCleanerExcludeSettings.h"

FName UProjectCleanerExcludeSettings::GetContainerName() const
{
	return FName{TEXT("Project")};
}

FName UProjectCleanerExcludeSettings::GetCategoryName() const
{
	return FName{TEXT("Plugins")};
}

FName UProjectCleanerExcludeSettings::GetSectionName() const
{
	return FName{TEXT("Project Cleaner Exclude Settings")};
}

FText UProjectCleanerExcludeSettings::GetSectionText() const
{
	return FText::FromString(TEXT("Project Cleaner Exclude Settings"));
}

FText UProjectCleanerExcludeSettings::GetSectionDescription() const
{
	return FText::FromString(TEXT("Project Cleaner Exclude Settings"));
}

#if WITH_EDITOR
void UProjectCleanerExcludeSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();
}
#endif