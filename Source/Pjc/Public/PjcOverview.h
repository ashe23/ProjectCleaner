// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcScanResult.h"

struct FPjcCookInfo;

// What the Overview tool shows, made from a scan result
struct FPjcOverview
{
	// Number of assets and their size on disk
	struct FTotal
	{
		int32 Num = 0;
		int64 Size = 0;

		void Add(const int64 AssetSize) {
			++Num;
			Size += AssetSize;
		}
	};

	// Assets grouped by what starts their usage chain
	struct FReason
	{
		// Reason of the asset that starts the chain, None for unused assets
		EPjcUsageReason Reason = EPjcUsageReason::None;
		FTotal Total;
	};

	// Assets of one class
	struct FClass
	{
		FName Name;
		FTotal Total;
		int64 SizeUsed = 0;
		int64 SizeEditorOnly = 0;
		int64 SizeReview = 0;
		int64 SizeUnused = 0;
	};

	// Folder with its assets and the unused ones among them
	struct FFolder
	{
		FString Path;
		FTotal All;
		FTotal Unused;
	};

	// Unused asset
	struct FAsset
	{
		FName Package;
		FName Class;
		int64 Size = 0;
	};

	// Scanned assets against the last cook
	struct FShipped
	{
		// Used and cooked
		FTotal Shipped;
		int64 ShippedCooked = 0;

		// Cooked though nothing the scan sees uses them: in Review (packaged by the last cook)
		FTotal UnusedShipped;
		int64 UnusedShippedCooked = 0;

		// Used in the editor, not cooked; by chain start, biggest first
		FTotal UsedNotShipped;
		TArray<FReason> UsedNotShippedReasons;

		// Unused or to review, and not cooked; unused assets saved since the cook too (not checked)
		FTotal Unused;

		// Cooked packages outside the scan (engine, plugins not scanned), by top folder, biggest first
		int32 NumOutside = 0;
		int64 SizeOutside = 0;
		TArray<TPair<FString, int32>> OutsideRoots;

		// Packages the cook generates, e.g. World Partition cells
		int32 NumGenerated = 0;

		// Scanned assets saved or added after the cook, and cooked ones deleted since
		int32 NumChanged = 0;
		int32 NumDeleted = 0;
	};

	// Every scanned asset (redirectors are not content)
	FTotal All;
	FTotal Used;
	FTotal EditorOnly;
	FTotal Review;
	FTotal Unused;

	// Why used assets are used, biggest first: one row per reason of the assets where a chain starts, Dependency for
	// what used assets reference
	TArray<FReason> Reasons;

	// Dependencies by the reason of the chain they belong to, most first
	TArray<FReason> DependencyChains;

	// Biggest first
	TArray<FClass> Classes;

	// Folders with half of their size or more unused, while their parent has less; most unused bytes first
	TArray<FFolder> UnusedFolders;

	// Biggest unused assets, biggest first
	TArray<FAsset> LargestUnused;

	static constexpr int32 MaxLargestUnused = 50;

	static FPjcOverview Make(const FPjcScanResult& Result);

	// Scan against a cook, reads the file times of the scanned assets
	static FShipped MakeShipped(const FPjcScanResult& Result, const FPjcCookInfo& Cook);

	// Why a used asset counts: its own reason where a chain starts, so Primary assets are exactly the
	// primary assets, or Dependency for what a used asset references. Assets a Primary Asset Label lists are primary
	// assets too, assets in config folders count under source and config
	static EPjcUsageReason GetGroupReason(const FPjcScanResult& Result, const FName Package);

	// The reason of the chain an asset belongs to (where it starts), grouped as GetGroupReason does
	static EPjcUsageReason GetChainGroupReason(const FPjcScanResult& Result, const FName Package);

  private:

	static void MakeUnusedFolders(const FPjcScanResult& Result, FPjcOverview& Overview);
};
