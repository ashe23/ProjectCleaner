// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

DECLARE_MULTICAST_DELEGATE(FPjcDelegateOnScanAssets)
DECLARE_MULTICAST_DELEGATE(FPjcDelegateOnScanFiles)
DECLARE_MULTICAST_DELEGATE_OneParam(FPjcDelegateFilterChanged, const bool bActive)
DECLARE_DELEGATE_OneParam(FPjcDelegatePathSelectionChanged, const TArray<FName>& PathsSelected)
// DECLARE_MULTICAST_DELEGATE_OneParam(FPjcDelegateProjectCleaned, const FPjcScanData& ScanData)
// DECLARE_MULTICAST_DELEGATE_OneParam(FPjcDelegateRequestedFilesDelete, const TArray<FString>& Files)
// DECLARE_MULTICAST_DELEGATE_OneParam(FPjcDelegatePathsExclude, const TArray<FString>& Paths)
// DECLARE_MULTICAST_DELEGATE_OneParam(FPjcDelegatePathsInclude, const TArray<FString>& Paths)
// DECLARE_MULTICAST_DELEGATE_OneParam(FPjcDelegatePathsDelete, const TArray<FString>& Paths)
// DECLARE_MULTICAST_DELEGATE_OneParam(FPjcDelegateAssetsExclude, const TArray<FAssetData>& Assets)
// DECLARE_MULTICAST_DELEGATE_OneParam(FPjcDelegateAssetsInclude, const TArray<FAssetData>& Assets)
// DECLARE_MULTICAST_DELEGATE_OneParam(FPjcDelegateAssetsExcludeByClass, const TArray<FAssetData>& Assets)
// DECLARE_MULTICAST_DELEGATE_OneParam(FPjcDelegateAssetsIncludeByClass, const TArray<FAssetData>& Assets)
// DECLARE_MULTICAST_DELEGATE_OneParam(FPjcDelegateAssetsDelete, const TArray<FAssetData>& Assets)
