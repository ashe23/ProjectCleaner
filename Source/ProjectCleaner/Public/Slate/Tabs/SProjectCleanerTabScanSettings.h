// // Copyright Ashot Barkhudaryan. All Rights Reserved.
//
// #pragma once
//
// #include "CoreMinimal.h"
// #include "Widgets/SCompoundWidget.h"
//
// class SProjectCleanerTabScanSettings final : public SCompoundWidget
// {
// public:
// 	SLATE_BEGIN_ARGS(SProjectCleanerTabScanSettings)
// 		{
// 		}
//
// 	SLATE_END_ARGS()
//
// 	void Construct(const FArguments& InArgs);
//
// private:
// 	bool BtnCleanProjectEnabled() const;
// 	bool BtnCleanEmptyFoldersEnabled() const;
//
// 	FReply OnBtnScanProjectClick() const;
// 	FReply OnBtnCleanProjectClick() const;
// 	FReply OnBtnDeleteEmptyFoldersClick() const;
// 	FReply OnBtnResetExcludeSettingsClick() const;
//
// 	// EVisibility GetInfoBoxVisibility() const;
// 	// FText GetInfoBoxText() const;
// 	FText GetStatsTextAssetsTotal() const;
// 	FText GetStatsTextAssetsIndirect() const;
// 	FText GetStatsTextAssetsExcluded() const;
// 	FText GetStatsTextAssetsUnused() const;
// 	FText GetStatsTextFilesNonEngine() const;
// 	FText GetStatsTextFilesCorrupted() const;
// 	FText GetStatsTextFoldersEmpty() const;
// };
