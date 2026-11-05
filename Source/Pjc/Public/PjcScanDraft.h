// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Scan settings while they are edited in the Scan settings panel. One draft for every tool: Rescan applies it to the
 * project settings (DefaultEditor.ini) and scans, so results always match the applied settings.
 * Member defaults are the plugin defaults, same as UPjcScanSettings.
 */
struct FPjcScanDraft
{
	bool bScanProjectPlugins = false;
	TArray<FString> IgnoredPlugins;
	bool bUnregisteredPrimaryUsed = true;
	bool bFolderLabelsUsed = true;
	bool bCinematicAssetsUsed = true;

	/** Always used classes, as class paths */
	TArray<FString> UsedClasses;

	/** What the project settings hold now */
	static FPjcScanDraft FromSettings();

	/** Writes the draft into the project settings and DefaultEditor.ini */
	void ApplyToSettings() const;

	bool operator==(const FPjcScanDraft& Other) const;
	bool operator!=(const FPjcScanDraft& Other) const {
		return !(*this == Other);
	}

	/** The draft every tool edits, starts as the project settings */
	static FPjcScanDraft& Get();

	/** True when the draft differs from the project settings: results are from the last scan */
	static bool IsChanged();

	/** Back to the project settings (Undo) */
	static void Revert();

	/** Broadcast after the draft was edited or reverted, so every Scan settings panel shows it */
	static FSimpleMulticastDelegate& OnEdited();
};
