// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "PjcTypes.h"

/** What a scanned asset is for, in the shared scan result */
enum class EPjcAssetStatus : uint8
{
	/** nothing used references it, cleanup deletes it */
	Unused,
	/** the game uses it: reached from a root that is not an editor asset, through references the game loads */
	Used,
	/** only the editor uses it: editor assets, or reached only through editor-only references. Cleanup keeps it. */
	EditorOnly,
};

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

	/**
	 * Scanned assets, one per package (GetAssetsAll without redirectors).
	 * Redirectors are not content: they are left out of Assets, Used, Unused, FolderLabelOnly and Sizes.
	 */
	TArray<FAssetData> Assets;

	/** Used assets, folder label assets included while folder labels are used (GetAssetsUsed) */
	TArray<FAssetData> Used;

	/** Unused assets (GetAssetsUnused) */
	TArray<FAssetData> Unused;

	/** Assets only folder labels keep (GetAssetsFolderLabelOnly) */
	TArray<FAssetData> FolderLabelOnly;

	/** Redirectors in scanned content, one per package */
	TArray<FAssetData> Redirectors;

	/**
	 * Why each scanned package is used or unused (GetAssetsUsage).
	 * Redirector packages stay here, chains go through them (referencer, redirector, target).
	 */
	TMap<FName, FPjcAssetUsage> Usage;

	/** Size on disk of each scanned asset */
	TMap<FName, int64> Sizes;

	/** Status of each scanned asset */
	TMap<FName, EPjcAssetStatus> Status;

	/** Files in content folders that are not assets, absolute paths: to review (cleanup deletes them) */
	TArray<FString> FilesExternal;

	/** Files in content folders that are not assets but kept: used by the project or engine, or excluded in settings */
	TArray<FString> FilesExternalKept;

	/** Asset files the asset registry does not know, absolute paths */
	TArray<FString> FilesCorrupted;

	/** Empty folders cleanup deletes, absolute paths */
	TArray<FString> FoldersEmpty;

	EPjcAssetStatus GetStatus(const FName Package) const {
		const EPjcAssetStatus* Found = Status.Find(Package);
		return Found ? *Found : EPjcAssetStatus::Unused;
	}

	/**
	 * Reason of the asset that starts the usage chain of a package, following Parent through Dependency steps:
	 * the own reason of a root, FolderLabel for what a folder label keeps, None for unused packages.
	 */
	EPjcUsageReason GetChainReason(const FName Package) const {
		const FPjcAssetUsage* Step = Usage.Find(Package);
		for (int32 NumSteps = 0; Step && Step->Reason == EPjcUsageReason::Dependency && NumSteps < 1024; ++NumSteps) {
			Step = Usage.Find(Step->Parent);
		}
		return Step ? Step->Reason : EPjcUsageReason::None;
	}

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
