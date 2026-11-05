// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * Bar under the tabs of every Project Cleaner tool: Scan, what the last scan found, Clean project.
 * Every tool shows the same shared scan result (UPjcSubsystem::GetScanResult).
 */
class SPjcScanBar final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcScanBar) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SPjcScanBar() override;

	/** Scans the project for every tool. Asks to save unsaved assets first, the scan reads them from disk. */
	static void ScanProject();

	/** Asks for confirmation, deletes every unused asset and empty folder, then scans again */
	static void CleanProject();

  private:

	FReply OnScanClicked() const;
	FReply OnRescanClicked() const;
	EVisibility GetScanVisibility() const;
	EVisibility GetRescanVisibility() const;
	FSlateColor GetStatusColor() const;
	FReply OnCleanClicked() const;
	FText GetStatusText() const;
	FText GetStatusToolTip() const;
	bool IsCleanEnabled() const;
	void OnScanFinished();

	FDelegateHandle ScanFinishedHandle;
	int32 NumAssets = 0;
	int32 NumUnused = 0;
	int64 SizeUnused = 0;
};
