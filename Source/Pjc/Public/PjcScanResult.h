// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "PjcTypes.h"

// What a scanned asset is for, in the shared scan result
enum class EPjcAssetStatus : uint8
{
	// nothing used references it, cleanup deletes it
	Unused,
	// the game uses it: reached from a root that is not an editor asset, through references the game loads
	Used,
	// only the editor uses it: editor assets, or reached only through editor-only references. Cleanup keeps it.
	EditorOnly,
	// the scan is not sure: kept by cleanup, the user decides. Its chain starts at a review reason (packaged by the
	// last cook, GameplayCue notify nothing fires), or it is what such an asset uses
	Review,
};

// What changed in the project after the last scan. Project Cleaner's own deletes and fixes count too until the scan
// that follows them
struct FPjcProjectChanges
{
	int32 NumAdded = 0;
	int32 NumRemoved = 0;
	int32 NumRenamed = 0;
	int32 NumSaved = 0;

	// Packages that changed last, newest last
	TArray<FName> Recent;

	static constexpr int32 MaxRecent = 8;

	int32 Num() const {
		return NumAdded + NumRemoved + NumRenamed + NumSaved;
	}

	void Add(int32& Counter, const FName Package) {
		++Counter;
		Recent.Remove(Package);
		Recent.Add(Package);
		if (Recent.Num() > MaxRecent) {
			Recent.RemoveAt(0);
		}
	}
};

// Rule in Scan settings > Exclusions that keeps an asset
enum class EPjcExclusionKind : uint8
{
	Asset,
	Folder,
	Class,
};

struct FPjcExclusion
{
	EPjcExclusionKind Kind = EPjcExclusionKind::Asset;

	// Folder path, class path or asset object path, as the rule has it
	FString Value;

	// Used even without the rule: another reason or a used asset outside exclusions keeps it
	bool bUsedAnyway = false;
};

// A place a C++, Python or config file names an asset (Source & config): the file, relative to the project, and its line
struct FPjcSourceRef
{
	FString File;
	int32 Line = 0;
};

// Unused assets compared with the last cook of every platform in Saved/Cooked
struct FPjcCookCheck
{
	// Platforms of the cooks read, newest first
	TArray<FString> Platforms;

	// Time of the newest cook (UTC), 0 ticks when there is none
	FDateTime Time {0};

	// Unused assets the cooks left out, not saved or added since
	int32 NumChecked = 0;

	// Unused assets saved since the cooks, or new since: the cooks cannot tell about them
	int32 NumNotChecked = 0;

	// Assets nothing the scan sees uses, but a cook packaged: Review (EPjcUsageReason::CookPackaged)
	int32 NumPackaged = 0;

	// Platforms of the cooks that took nearly every asset, unused ones too: they cannot tell what the game needs, so
	// they are not compared with the unused assets
	TArray<FString> PlatformsCookAll;

	bool IsFound() const {
		return Platforms.Num() > 0;
	}

	// Every cook read took nearly every asset: nothing was compared
	bool IsCookAll() const {
		return IsFound() && PlatformsCookAll.Num() == Platforms.Num();
	}
};

/**
 * One scan of the project, shared by every Project Cleaner tab.
 * Made by UPjcSubsystem::Scan, read with UPjcSubsystem::GetScanResult. It is not updated when assets change:
 * tabs scan again after creating or deleting assets.
 */
struct FPjcScanResult
{
	// When the scan finished (UTC), 0 ticks before the first scan (4.27 does not zero FDateTime)
	FDateTime Time {0};

	// How long the scan took
	double Seconds = 0.0;

	// Scanned assets, one per package (GetAssetsAll).
	// Redirectors are not content: they are left out of Assets, Used, Unused, FolderLabelOnly and Sizes.
	TArray<FAssetData> Assets;

	// Used assets (GetAssetsUsed)
	TArray<FAssetData> Used;

	// Unused assets (GetAssetsUnused)
	TArray<FAssetData> Unused;

	// Assets only folder labels keep: their cook rule packages the folder (GetAssetsFolderLabelOnly)
	TArray<FAssetData> FolderLabelOnly;

	// Unused assets in the folder of a label whose cook rule does not package it: the label lists them, only
	// references would keep them
	TSet<FName> FolderLabelUnused;

	// Redirectors in scanned content, one per package
	TArray<FAssetData> Redirectors;

	// Why each scanned package is used or unused (GetAssetsUsage).
	// Redirector packages stay here, chains go through them (referencer, redirector, target).
	TMap<FName, FPjcAssetUsage> Usage;

	// Size on disk of each scanned asset
	TMap<FName, int64> Sizes;

	// Status of each scanned asset
	TMap<FName, EPjcAssetStatus> Status;

	// Files in content folders that are not assets, absolute paths: to review (cleanup deletes them)
	TArray<FString> FilesExternal;

	// Files in content folders that are not assets but kept: used by the project or engine, or excluded in settings
	TArray<FString> FilesExternalKept;

	// Why each kept file is kept (UPjcSubsystem::GetFileKeptReason), same order as FilesExternalKept
	TArray<FString> FilesExternalKeptReasons;

	// Asset files the asset registry does not know, absolute paths
	TArray<FString> FilesCorrupted;

	// Empty folders cleanup deletes, absolute paths
	TArray<FString> FoldersEmpty;

	// The scan against the last cooks
	FPjcCookCheck CookCheck;

	// Every place C++, Python or config files name a scanned asset, by package (the Source & config list of Assets)
	TMap<FName, TArray<FPjcSourceRef>> SourceRefs;

	// Assets an exclusion rule matches, whatever else keeps them
	TMap<FName, FPjcExclusion> Excluded;

	// Reasons that put an asset in Review: the scan is not sure, cleanup keeps it and the user decides
	static bool IsReviewReason(const EPjcUsageReason Reason) {
		return Reason == EPjcUsageReason::CookPackaged || Reason == EPjcUsageReason::GameplayCueNotFired;
	}

	EPjcAssetStatus GetStatus(const FName Package) const {
		const EPjcAssetStatus* Found = Status.Find(Package);
		return Found ? *Found : EPjcAssetStatus::Unused;
	}

	// Reason of the asset that starts the usage chain of a package, following Parent through Dependency steps:
	// the own reason of a root, FolderLabel for what a folder label keeps, None for unused packages.
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
