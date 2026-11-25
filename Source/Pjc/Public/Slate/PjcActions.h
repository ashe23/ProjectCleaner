// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// Actions the Project Cleaner tools share
namespace PjcActions
{
	// Deletes unused or review assets with the unused and review assets that reference them, after the delete dialog,
	// then scans again. Refused while a used asset references them (it would break). False when nothing was deleted.
	bool DeleteWithGroup(const TArray<FName>& InPackages);

	// Saves unsaved assets before a delete (the editor asks). A delete reads the saved files, and a save can make an asset
	// used: an actor placed in an unsaved level. False when the user does not save. OutSaved: something was saved
	bool SaveForDelete(bool* OutSaved = nullptr);

	// Notification with the warning icon and a link to the Output Log
	void ShowWarning(const FString& Text);

	// Adds exclusions to the project settings and the scan draft, tells Message, then scans again
	void Exclude(const TArray<FString>& Folders, const TArray<FString>& Classes, const TArray<FString>& Assets, const FString& Message);

	// Removes exclusion rules (folder paths, class paths, asset object paths) from the project settings and the scan
	// draft, tells Message, then scans again
	void RemoveExclusions(const TArray<FString>& Folders, const TArray<FString>& Classes, const TArray<FString>& Assets, const FString& Message);

	// Mark unused: assets to review become unused, with the assets to review only they keep. After a dialog, saved to the
	// project settings and the scan draft, then a scan. Nothing is deleted here: Clean project and the Assets tab do that
	void MarkUnused(const TArray<FName>& Packages);

	// Takes the marks off (package names): after the scan they are to review again
	void RemoveMarks(const TArray<FName>& Packages);

	// What Force delete does, for the dialog and the Clean window
	FString GetForceDeleteWarning();

	// Force delete of assets the last delete could not remove: the warning dialog when bAsk, the engine's Force Delete,
	// then a scan when bScan. Returns how many are gone from disk; OutInMemory: of those, how many the editor still holds
	// (string tables before UE 5.7): they stay in the lists until the editor restarts
	int32 ForceDelete(const TArray<FName>& InPackages, const bool bAsk, const bool bScan, int32* OutInMemory = nullptr);
}	// namespace PjcActions
