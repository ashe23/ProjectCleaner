// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcOverview.h"

// What the Assets tool lists
enum class EPjcAssetSet : uint8
{
	// nothing used references them: cleanup deletes them
	Unused,
	// an exclusion rule matches them, whatever else keeps them
	Excluded,
	// used or editor only
	Used,
	// the scan is not sure: cleanup keeps them, the user decides
	Review,
};

// Folder of the scanned content with what is inside it, subfolders included
struct FPjcUnusedFolder
{
	// Content path, e.g. /Game/Characters
	FString Path;
	FString Name;

	TWeakPtr<FPjcUnusedFolder> Parent;
	TArray<TSharedPtr<FPjcUnusedFolder>> Children;

	// Assets directly in this folder, as indices into FPjcScanResult::Assets
	TArray<int32> Assets;

	FPjcOverview::FTotal All;
	// Assets of the tree's set
	FPjcOverview::FTotal Listed;

	// Empty folder cleanup deletes
	bool bEmpty = false;

	bool IsRoot() const {
		return !Parent.IsValid();
	}

	bool IsAllListed() const {
		return Listed.Num > 0 && Listed.Num == All.Num;
	}
};

// Folders of a scan with the totals of one set, for the Assets tool
struct FPjcUnusedTree
{
	// Every folder by path
	TMap<FString, TSharedPtr<FPjcUnusedFolder>> Folders;

	// Content roots: /Game first, then the most listed
	TArray<TSharedPtr<FPjcUnusedFolder>> Roots;

	// The set the totals count
	EPjcAssetSet Set = EPjcAssetSet::Unused;

	static FPjcUnusedTree Make(const FPjcScanResult& Result, const EPjcAssetSet InSet = EPjcAssetSet::Unused);

	static bool IsInSet(const FPjcScanResult& Result, const FName Package, const EPjcAssetSet InSet);

	TSharedPtr<FPjcUnusedFolder> Find(const FString& Path) const {
		const TSharedPtr<FPjcUnusedFolder>* Found = Folders.Find(Path);
		return Found ? *Found : nullptr;
	}

	// Assets of the set in a folder, and in its subfolders when asked, as indices into FPjcScanResult::Assets
	void GetListed(const FPjcScanResult& Result, const FString& Path, const bool bSubfolders, TArray<int32>& OutAssets) const;

	// What goes with deleting the given packages: the unused and review assets and the redirectors that reference
	// them, transitively. The engine does not delete an asset something still references. Given packages are not in
	// the group.
	static void GetDeleteGroup(const FPjcScanResult& Result, const TArray<FName>& Packages, TArray<FName>& OutGroup);

	// Used assets that reference the packages or their delete group: deleting them would break those. Folder labels
	// are left out, their folder references are not usage (cleanup resaves them).
	static void GetUsedHolders(const FPjcScanResult& Result, const TArray<FName>& Packages, TArray<FName>& OutHolders);
};
