// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * Bar under the tabs of every Project Cleaner tool: Scan (Rescan when settings or the project changed), when the last
 * scan was, Clean project. Every tool shows the same shared scan result (UPjcSubsystem::GetScanResult).
 */
class SPjcScanBar final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcScanBar) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SPjcScanBar() override;

	// Scans the project for every tool. Asks to save unsaved assets first, the scan reads them from disk. The Scan window
	// closes when the scan ends; bByUser: a scan that failed stays open for the user to read
	static void ScanProject(const bool bByUser = false);

	// Opens the Clean project window: what gets deleted and the checks, the progress, the result
	static void CleanProject();

	// Results are from before a change: scan settings edited, or assets changed since the scan
	static bool NeedsRescan();

  private:

	// Icon and text of a scan bar button
	static TSharedRef<SWidget> MakeButtonLabel(const FString& Text);

	FReply OnScanClicked() const;
	FReply OnRescanClicked() const;
	EVisibility GetScanVisibility() const;
	EVisibility GetRescanVisibility() const;
	FSlateColor GetStatusColor() const;
	FReply OnCleanClicked() const;
	FText GetStatusText() const;
	FText GetStatusToolTip() const;
	bool IsCleanEnabled() const;
	FText GetCleanToolTip() const;
};
