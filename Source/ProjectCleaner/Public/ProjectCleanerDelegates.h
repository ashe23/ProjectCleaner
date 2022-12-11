// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// Settings
// DECLARE_MULTICAST_DELEGATE_OneParam(FProjectCleanerDelegateExcludeSettingsChanged, const FName& PropertyName);

// // Scanner
DECLARE_MULTICAST_DELEGATE(FProjectCleanerDelegateScanFinished);
// DECLARE_MULTICAST_DELEGATE(FProjectCleanerDelegateCleanFinished);
// DECLARE_MULTICAST_DELEGATE(FProjectCleanerDelegateEmptyFoldersDeleted);
//
// // STreeView
// DECLARE_MULTICAST_DELEGATE_OneParam(FProjectCleanerDelegatePathSelected, const TSet<FString>& SelectedPaths);
// DECLARE_MULTICAST_DELEGATE_OneParam(FProjectCleanerDelegatePathExcluded, const TSet<FString>& ExcludedPaths);
// DECLARE_MULTICAST_DELEGATE_OneParam(FProjectCleanerDelegatePathIncluded, const TSet<FString>& IncludedPaths);
// DECLARE_MULTICAST_DELEGATE_OneParam(FProjectCleanerDelegatePathCleaned, const TSet<FString>& CleanedPaths);
//
// // ContentBrowser
// DECLARE_MULTICAST_DELEGATE_OneParam(FProjectCleanerDelegateAssetsDeleted, const TArray<FAssetData>& Assets);
// DECLARE_MULTICAST_DELEGATE_OneParam(FProjectCleanerDelegateAssetsExcluded, const TArray<FAssetData>& Assets);
// DECLARE_MULTICAST_DELEGATE_OneParam(FProjectCleanerDelegateAssetsExcludedByType, const TArray<FAssetData>& Assets);
//
// // STabUnused
// DECLARE_MULTICAST_DELEGATE(FProjectCleanerDelegateIncludeAllExcluded)
