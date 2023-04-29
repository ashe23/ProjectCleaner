// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcSubsystem.h"

void UPjcSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UPjcSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UPjcSubsystem::ToggleShowFoldersDev()
{
	bShowFoldersDev = !bShowFoldersDev;
	PostEditChange();
}

void UPjcSubsystem::ToggleShowFoldersEmpty()
{
	bShowFoldersEmpty = !bShowFoldersEmpty;
	PostEditChange();
}

void UPjcSubsystem::ToggleShowFoldersExcluded()
{
	bShowFoldersExcluded = !bShowFoldersExcluded;
	PostEditChange();
}

bool UPjcSubsystem::CanShowFoldersDev() const
{
	return bShowFoldersDev;
}

bool UPjcSubsystem::CanShowFoldersEmpty() const
{
	return bShowFoldersEmpty;
}

bool UPjcSubsystem::CanShowFoldersExcluded() const
{
	return bShowFoldersExcluded;
}

#if WITH_EDITOR
void UPjcSubsystem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();
}
#endif
