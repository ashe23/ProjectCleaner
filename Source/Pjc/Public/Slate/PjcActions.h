// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// Actions the Project Cleaner tools share
namespace PjcActions
{
	// Deletes unused or review assets with the unused and review assets that reference them, after the delete dialog,
	// then scans again. Refused while a used asset references them (it would break). False when nothing was deleted.
	bool DeleteWithGroup(const TArray<FName>& Packages);

	// Adds exclusions to the project settings and the scan draft, tells Message, then scans again
	void Exclude(const TArray<FString>& Folders, const TArray<FString>& Classes, const TArray<FString>& Assets, const FString& Message);

	// Removes exclusion rules (folder paths, class paths, asset object paths) from the project settings and the scan
	// draft, tells Message, then scans again
	void RemoveExclusions(const TArray<FString>& Folders, const TArray<FString>& Classes, const TArray<FString>& Assets, const FString& Message);

	// What Force delete does, for the dialog and the Clean window
	FString GetForceDeleteWarning();

	// Force delete of assets the last delete could not remove: the warning dialog when bAsk, the engine's Force Delete,
	// then a scan when bScan. Returns how many are gone from disk; OutInMemory: of those, how many the editor still holds
	// (string tables before UE 5.7): they stay in the lists until the editor restarts
	int32 ForceDelete(const TArray<FName>& Packages, const bool bAsk, const bool bScan, int32* OutInMemory = nullptr);
}	// namespace PjcActions
