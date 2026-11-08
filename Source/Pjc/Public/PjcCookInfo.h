// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * What the last cook of this project folder packaged, read from its development asset registry
 * (Saved/Cooked/<Platform>/<Project>/Metadata/DevelopmentAssetRegistry.bin). Every engine version writes it on a cook.
 * Cooks made on another machine or into another folder are not seen.
 */
struct FPjcCookInfo
{
	// Platform folder of the cook, e.g. Windows
	FString Platform;

	// The development asset registry of the cook, absolute path
	FString File;

	// When the cook finished (UTC): time of its registry file. 0 ticks when there is no cook (4.27 does not zero FDateTime)
	FDateTime Time {0};

	// Every content package the cook saved, with its cooked size (before pak compression). Script packages left out
	TMap<FName, int64> Cooked;

	// Digest of the editor package each content package of the cook was made from, cooked or not (PjcShim::GetPackageSavedDigest)
	TMap<FName, uint32> SavedDigests;

	// A registry was found but could not be read, e.g. written by another engine version
	bool bReadFailed = false;

	bool IsFound() const {
		return Time.GetTicks() > 0;
	}

	bool IsValid() const {
		return IsFound() && !bReadFailed;
	}

	// The package was saved since this cook: what it was saved with differs, or its file is newer when that is not
	// known (packages the cook did not see too)
	bool IsSavedSince(const FName Package) const;

	// Newest cook of this project folder for any platform. Read again only when its file changes.
	static const FPjcCookInfo& GetLast();

	// Last cook of this project folder for every platform, newest first. Each is read again only when its file changes.
	static TArray<const FPjcCookInfo*> GetAll();

	// Folder the cooks of this project are in: Saved/Cooked
	static FString GetCookedDir();

	// Reads the packages a development asset registry lists, and which of them are cooked
	static bool Read(const FString& InFile, TMap<FName, int64>& OutCooked, TMap<FName, uint32>& OutSavedDigests);
};
