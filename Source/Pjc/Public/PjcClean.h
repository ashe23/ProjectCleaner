// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcOverview.h"

// What Clean project deletes and what it keeps, made from the scan result, for its window
struct FPjcCleanPlan
{
	// Unused assets, biggest first
	TArray<FAssetData> Assets;
	int64 Size = 0;

	// Folders empty now, and folders the delete leaves without assets
	int32 NumFoldersEmpty = 0;

	// Redirectors nothing uses that point to an asset the delete removes: they go with it
	int32 NumRedirectors = 0;
	// Redirectors Clean project leaves: nothing used is deleted through them, Issues fixes them
	int32 NumRedirectorsLeft = 0;

	// Where and what, biggest first: folders two levels deep (/Game/Characters) and classes
	TArray<TPair<FString, FPjcOverview::FTotal>> Folders;
	TArray<TPair<FName, FPjcOverview::FTotal>> Classes;

	// Kept: assets exclusion rules match, assets to review
	int32 NumExcluded = 0;
	int32 NumReview = 0;

	static FPjcCleanPlan Make(const FPjcScanResult& Result);

	// Folders the delete of the packages leaves without assets (content roots never), and the empty ones
	static int32 CountFoldersEmptyAfter(const FPjcScanResult& Result, const TSet<FName>& Deleted);

	// Assets still on disk that reference a deleted package: they lost a reference. Folder labels are left out, their
	// folder references are not usage and cleanup resaves them
	static void GetBrokenReferencers(const TArray<FName>& Deleted, TArray<FName>& OutReferencers);
};
