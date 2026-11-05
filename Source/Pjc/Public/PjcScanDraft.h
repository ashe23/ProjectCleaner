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

	/** Exclusions (UPjcAssetExcludeSettings): folder paths, class paths, asset object paths */
	TArray<FString> ExcludedFolders;
	TArray<FString> ExcludedClasses;
	TArray<FString> ExcludedAssets;

	/** What the project settings hold now */
	static FPjcScanDraft FromSettings();

	/** Writes the draft into the project settings and exclusions, and DefaultEditor.ini */
	void ApplyToSettings() const;

	bool operator==(const FPjcScanDraft& Other) const;
	bool operator!=(const FPjcScanDraft& Other) const {
		return !(*this == Other);
	}

	/** The draft every tool edits. Follows the project settings until it is edited (NotifyEdited). */
	static FPjcScanDraft& Get();

	/** True when the draft differs from the project settings: results are from the last scan */
	static bool IsChanged();

	/** Call after editing the draft: it stops following the settings and every panel shows the edit */
	static void NotifyEdited();

	/** Back to the project settings (Undo) */
	static void Revert();

	/** Broadcast after the draft was edited or reverted, so every Scan settings panel shows it */
	static FSimpleMulticastDelegate& OnEdited();
};
