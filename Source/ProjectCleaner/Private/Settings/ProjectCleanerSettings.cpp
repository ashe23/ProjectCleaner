// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Settings/ProjectCleanerSettings.h"

FName UProjectCleanerSettings::GetContainerName() const
{
	return FName{TEXT("Project")};
}

FName UProjectCleanerSettings::GetCategoryName() const
{
	return FName{TEXT("Project Cleaner")};
}

FText UProjectCleanerSettings::GetSectionText() const
{
	return FText::FromString(TEXT("Settings"));
}

FText UProjectCleanerSettings::GetSectionDescription() const
{
	return FText::FromString(TEXT("Project Cleaner Global Settings"));
}
