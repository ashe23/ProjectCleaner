// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// Actions the Project Cleaner tools share
namespace PjcActions
{
	// Deletes unused assets with the unused assets only they reference, after the delete dialog, then scans again.
	// False when nothing was deleted (no packages, or the user cancelled).
	bool DeleteWithGroup(const TArray<FName>& Packages);

	// Adds exclusions to the project settings and the scan draft, tells Message, then scans again
	void Exclude(const TArray<FString>& Folders, const TArray<FString>& Classes, const TArray<FString>& Assets, const FString& Message);
}	// namespace PjcActions
