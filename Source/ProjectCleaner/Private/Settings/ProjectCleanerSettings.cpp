// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Settings/ProjectCleanerSettings.h"

FName UProjectCleanerSettings::GetContainerName() const
{
	return FName{TEXT("Project")};
}

FName UProjectCleanerSettings::GetCategoryName() const
{
	return FName{TEXT("Plugins")};
}

FName UProjectCleanerSettings::GetSectionName() const
{
	return FName{TEXT("Project Cleaner Settings")};
}

FText UProjectCleanerSettings::GetSectionText() const
{
	return FText::FromString(TEXT("Project Cleaner Settings"));
}

FText UProjectCleanerSettings::GetSectionDescription() const
{
	return FText::FromString(TEXT("Project Cleaner Global Settings"));
}

#if WITH_EDITOR
void UProjectCleanerSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();
}
#endif
