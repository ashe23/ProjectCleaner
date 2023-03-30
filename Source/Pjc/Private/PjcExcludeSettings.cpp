// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcExcludeSettings.h"

FName UPjcExcludeSettings::GetContainerName() const
{
	return FName{TEXT("Project")};
}

FName UPjcExcludeSettings::GetCategoryName() const
{
	return FName{TEXT("Plugins")};
}

FName UPjcExcludeSettings::GetSectionName() const
{
	return FName{TEXT("PjcExcludeSettings")};
}

FText UPjcExcludeSettings::GetSectionText() const
{
	return FText::FromString(TEXT("Project Cleaner"));
}

FText UPjcExcludeSettings::GetSectionDescription() const
{
	return FText::FromString(TEXT("Project Cleaner Exclude Settings"));
}

#if WITH_EDITOR
void UPjcExcludeSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();
}
#endif
