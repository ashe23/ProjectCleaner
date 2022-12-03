// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// SProjectCleaner
DECLARE_MULTICAST_DELEGATE(FProjectCleanerDelegateScanFinished);
DECLARE_MULTICAST_DELEGATE(FProjectCleanerDelegateScanSettingsChanged);
DECLARE_MULTICAST_DELEGATE(FProjectCleanerDelegateCleanFinished);
DECLARE_MULTICAST_DELEGATE(FProjectCleanerDelegateEmptyFoldersDeleted);

// STreeView
DECLARE_MULTICAST_DELEGATE_OneParam(FProjectCleanerDelegatePathChanged, const FString& PathRel);
DECLARE_MULTICAST_DELEGATE_OneParam(FProjectCleanerDelegatePathExcluded, const FString& PathRel);
DECLARE_MULTICAST_DELEGATE_OneParam(FProjectCleanerDelegatePathIncluded, const FString& PathRel);
DECLARE_MULTICAST_DELEGATE_OneParam(FProjectCleanerDelegatePathCleaned, const FString& PathRel);

// ContentBrowser
DECLARE_MULTICAST_DELEGATE_OneParam(FProjectCleanerDelegateAssetsDeleted, const TArray<FAssetData>& Assets);
DECLARE_MULTICAST_DELEGATE_OneParam(FProjectCleanerDelegateAssetsExcluded, const TArray<FAssetData>& Assets);
DECLARE_MULTICAST_DELEGATE_OneParam(FProjectCleanerDelegateAssetsExcludedByType, const TArray<FAssetData>& Assets);

// STabUnused
DECLARE_MULTICAST_DELEGATE(FProjectCleanerDelegateIncludeAllExcluded)
