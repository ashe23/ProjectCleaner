// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcScanResult.h"

/** What the Overview tool shows, made from a scan result */
struct FPjcOverview
{
	/** Number of assets and their size on disk */
	struct FTotal
	{
		int32 Num = 0;
		int64 Size = 0;

		void Add(const int64 AssetSize) {
			++Num;
			Size += AssetSize;
		}
	};

	/** Assets grouped by what starts their usage chain */
	struct FReason
	{
		/** Reason of the asset that starts the chain, None for unused assets */
		EPjcUsageReason Reason = EPjcUsageReason::None;
		FTotal Total;
	};

	/** Assets of one class */
	struct FClass
	{
		FName Name;
		FTotal Total;
		int64 SizeUsed = 0;
		int64 SizeEditorOnly = 0;
		int64 SizeUnused = 0;
	};

	/** Folder with its assets and the unused ones among them */
	struct FFolder
	{
		FString Path;
		FTotal All;
		FTotal Unused;
	};

	/** Unused asset */
	struct FAsset
	{
		FName Package;
		FName Class;
		int64 Size = 0;
	};

	/** Every scanned asset (redirectors are not content) */
	FTotal All;
	FTotal Used;
	FTotal EditorOnly;
	FTotal Unused;

	/** Biggest first */
	TArray<FReason> Reasons;

	/** Biggest first */
	TArray<FClass> Classes;

	/** Folders with half of their size or more unused, while their parent has less; most unused bytes first */
	TArray<FFolder> UnusedFolders;

	/** Biggest unused assets, biggest first */
	TArray<FAsset> LargestUnused;

	static constexpr int32 MaxLargestUnused = 50;

	static FPjcOverview Make(const FPjcScanResult& Result);

	/** Reason a chain is grouped under: assets a Primary Asset Label lists are primary assets too */
	static EPjcUsageReason GetGroupReason(const FPjcScanResult& Result, const FName Package);

  private:

	static void MakeUnusedFolders(const FPjcScanResult& Result, FPjcOverview& Overview);
};
