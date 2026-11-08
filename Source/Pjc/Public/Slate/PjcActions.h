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
}	// namespace PjcActions
