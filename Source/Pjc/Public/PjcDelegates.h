// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcTypes.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FPjcDelegateOnProjectScan, const EPjcScanResult ScanResult, const FString& ErrMsg)
// DECLARE_MULTICAST_DELEGATE_OneParam(FPjcDelegateProjectCleaned, const FPjcScanData& ScanData)
// DECLARE_MULTICAST_DELEGATE_OneParam(FPjcDelegateRequestedFilesDelete, const TArray<FString>& Files)
// DECLARE_MULTICAST_DELEGATE_OneParam(FPjcDelegateFilterChanged, const bool bActive)
// DECLARE_MULTICAST_DELEGATE_OneParam(FPjcDelegatePathsExclude, const TArray<FString>& Paths)
// DECLARE_MULTICAST_DELEGATE_OneParam(FPjcDelegatePathsInclude, const TArray<FString>& Paths)
// DECLARE_MULTICAST_DELEGATE_OneParam(FPjcDelegatePathsDelete, const TArray<FString>& Paths)
// DECLARE_MULTICAST_DELEGATE_OneParam(FPjcDelegateAssetsExclude, const TArray<FAssetData>& Assets)
// DECLARE_MULTICAST_DELEGATE_OneParam(FPjcDelegateAssetsInclude, const TArray<FAssetData>& Assets)
// DECLARE_MULTICAST_DELEGATE_OneParam(FPjcDelegateAssetsExcludeByClass, const TArray<FAssetData>& Assets)
// DECLARE_MULTICAST_DELEGATE_OneParam(FPjcDelegateAssetsIncludeByClass, const TArray<FAssetData>& Assets)
// DECLARE_MULTICAST_DELEGATE_OneParam(FPjcDelegateAssetsDelete, const TArray<FAssetData>& Assets)
