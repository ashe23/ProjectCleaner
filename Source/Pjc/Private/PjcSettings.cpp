// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcSettings.h"

FName UPjcSettings::GetContainerName() const
{
	return FName{TEXT("Project")};
}

FName UPjcSettings::GetCategoryName() const
{
	return FName{TEXT("Plugins")};
}

FName UPjcSettings::GetSectionName() const
{
	return FName{TEXT("Project Cleaner")};
}

FText UPjcSettings::GetSectionText() const
{
	return FText::FromString(TEXT("Project Cleaner"));
}

FText UPjcSettings::GetSectionDescription() const
{
	return FText::FromString(TEXT("Project Cleaner Settings"));
}

#if WITH_EDITOR
void UPjcSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();
}
#endif
