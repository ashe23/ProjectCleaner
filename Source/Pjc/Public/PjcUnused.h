// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcOverview.h"

/** Folder of the scanned content with what is inside it, subfolders included */
struct FPjcUnusedFolder
{
	/** Content path, e.g. /Game/Characters */
	FString Path;
	FString Name;

	TWeakPtr<FPjcUnusedFolder> Parent;
	TArray<TSharedPtr<FPjcUnusedFolder>> Children;

	/** Assets directly in this folder, as indices into FPjcScanResult::Assets */
	TArray<int32> Assets;

	FPjcOverview::FTotal All;
	FPjcOverview::FTotal Unused;

	/** Empty folder cleanup deletes */
	bool bEmpty = false;

	bool IsRoot() const {
		return !Parent.IsValid();
	}

	bool IsAllUnused() const {
		return Unused.Num > 0 && Unused.Num == All.Num;
	}
};

/** Folders of a scan with their unused totals, for the Assets Unused tool */
struct FPjcUnusedTree
{
	/** Every folder by path */
	TMap<FString, TSharedPtr<FPjcUnusedFolder>> Folders;

	/** Content roots: /Game first, then the most unused */
	TArray<TSharedPtr<FPjcUnusedFolder>> Roots;

	static FPjcUnusedTree Make(const FPjcScanResult& Result);

	TSharedPtr<FPjcUnusedFolder> Find(const FString& Path) const {
		const TSharedPtr<FPjcUnusedFolder>* Found = Folders.Find(Path);
		return Found ? *Found : nullptr;
	}

	/** Unused assets in a folder, and in its subfolders when asked, as indices into FPjcScanResult::Assets */
	void GetUnused(const FPjcScanResult& Result, const FString& Path, const bool bSubfolders, TArray<int32>& OutAssets) const;
};
