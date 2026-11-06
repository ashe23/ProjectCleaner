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

	/** Every scanned asset (redirectors are not content) */
	FTotal All;
	FTotal Used;
	FTotal EditorOnly;
	FTotal Unused;

	static FPjcOverview Make(const FPjcScanResult& Result);
};
