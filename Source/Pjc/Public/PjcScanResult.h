// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "PjcTypes.h"

/**
 * One scan of the project, shared by every Project Cleaner tab.
 * Made by UPjcSubsystem::Scan, read with UPjcSubsystem::GetScanResult. It is not updated when assets change:
 * tabs scan again after creating or deleting assets.
 */
struct FPjcScanResult
{
	/** When the scan finished (UTC), unset before the first scan */
	FDateTime Time;

	/** How long the scan took */
	double Seconds = 0.0;

	/** Scanned assets, one per package (GetAssetsAll) */
	TArray<FAssetData> Assets;

	/** Used assets, folder label assets included while folder labels are used (GetAssetsUsed) */
	TArray<FAssetData> Used;

	/** Unused assets (GetAssetsUnused) */
	TArray<FAssetData> Unused;

	/** Assets only folder labels keep (GetAssetsFolderLabelOnly) */
	TArray<FAssetData> FolderLabelOnly;

	/** Why each scanned package is used or unused (GetAssetsUsage) */
	TMap<FName, FPjcAssetUsage> Usage;

	/** Size on disk of each scanned package */
	TMap<FName, int64> Sizes;

	bool IsValid() const {
		return Time.GetTicks() > 0;
	}

	int64 GetSize(const FName Package) const {
		const int64* Size = Sizes.Find(Package);
		return Size ? *Size : 0;
	}

	bool IsUsed(const FName Package) const {
		const FPjcAssetUsage* AssetUsage = Usage.Find(Package);
		return AssetUsage && AssetUsage->bUsed;
	}
};
