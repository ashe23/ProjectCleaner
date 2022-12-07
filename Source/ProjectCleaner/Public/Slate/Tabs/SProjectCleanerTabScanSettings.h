// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UProjectCleanerScanSettings;
struct FProjectCleanerScanner;

class SProjectCleanerTabScanSettings final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerTabScanSettings)
		{
		}

		SLATE_ARGUMENT(TSharedPtr<FProjectCleanerScanner>, Scanner)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
private:
	bool BtnScanProjectEnabled() const;
	bool BtnCleanProjectEnabled() const;
	bool BtnDeleteEmptyFoldersEnabled() const;

	FReply OnBtnScanProjectClick() const;
	FReply OnBtnCleanProjectClick() const;
	FReply OnBtnDeleteEmptyFoldersClick() const;

	EVisibility GetBtnScanProjectStatusVisibility() const;
	FText GetBtnScanProjectToolTipText() const;
	FText GetStatsTextAssetsTotal() const;
	FText GetStatsTextAssetsIndirect() const;
	FText GetStatsTextAssetsExcluded() const;
	FText GetStatsTextAssetsUnused() const;
	FText GetStatsTextFilesNonEngine() const;
	FText GetStatsTextFilesCorrupted() const;
	FText GetStatsTextFoldersEmpty() const;


	TSharedPtr<FProjectCleanerScanner> Scanner;
	TSharedPtr<IDetailsView> ScanSettingsProperty;
	TWeakObjectPtr<UProjectCleanerScanSettings> ScanSettings;
};
