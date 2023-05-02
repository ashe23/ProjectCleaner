// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcSubsystem.h"

// #include "Settings/ContentBrowserSettings.h"

void UPjcSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UPjcSubsystem::Deinitialize()
{
	Super::Deinitialize();
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

// void UPjcSubsystem::ToggleScanFoldersDev()
// {
// 	bScanFoldersDev = !bScanFoldersDev;
// 	PostEditChange();
//
// 	UContentBrowserSettings* Settings = GetMutableDefault<UContentBrowserSettings>();
// 	if (Settings)
// 	{
// 		Settings->SetDisplayDevelopersFolder(bScanFoldersDev);
// 		Settings->PostEditChange();
// 	}
// }
//
// void UPjcSubsystem::ToggleCleanAssetsUnused()
// {
// 	bCleanAssetsUnused = !bCleanAssetsUnused;
// 	PostEditChange();
// }
//
// void UPjcSubsystem::ToggleCleanFoldersEmpty()
// {
// 	bCleanFoldersEmpty = !bCleanFoldersEmpty;
// 	PostEditChange();
// }

bool UPjcSubsystem::CanShowFoldersEmpty() const
{
	return bShowFoldersEmpty;
}

bool UPjcSubsystem::CanShowFoldersExcluded() const
{
	return bShowFoldersExcluded;
}

// bool UPjcSubsystem::CanScanFoldersDev() const
// {
// 	return bScanFoldersDev;
// }
//
// bool UPjcSubsystem::CanCleanAssetsUnused() const
// {
// 	return bCleanAssetsUnused;
// }
//
// bool UPjcSubsystem::CanCleanFoldersEmpty() const
// {
// 	return bCleanFoldersEmpty;
// }

#if WITH_EDITOR
void UPjcSubsystem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();
}
#endif
