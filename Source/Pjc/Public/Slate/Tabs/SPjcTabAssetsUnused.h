// // Copyright Ashot Barkhudaryan. All Rights Reserved.
//
// #pragma once
//
// #include "CoreMinimal.h"
// // #include "PjcTypes.h"
// #include "Widgets/SCompoundWidget.h"
//
// class SPjcTreeView;
// class SPjcStatsBasic;
// struct FPjcStatItem;
// class UPjcScannerSubsystem;
//
// class SPjcTabAssetsUnused final : public SCompoundWidget
// {
// public:
// 	SLATE_BEGIN_ARGS(SPjcTabAssetsUnused) {}
// 	SLATE_END_ARGS()
//
// 	void Construct(const FArguments& InArgs);
// 	virtual ~SPjcTabAssetsUnused() override;
//
// private:
// 	void OnProjectScanSuccess();
// 	void OnProjectScanFail(const FString& InScanErrMsg);
// 	void StatItemsUpdate();
//
// 	TSharedRef<SWidget> CreateToolbar() const;
// 	TSharedPtr<SWidget> GetContentBrowserContextMenu(const TArray<FAssetData>& Assets) const;
//
// 	TSharedPtr<FUICommandList> Cmds;
// 	TSharedPtr<SPjcStatsBasic> StatsViewPtr;
// 	TSharedPtr<SPjcTreeView> TreeViewPtr;
// 	TArray<TSharedPtr<FPjcStatItem>> StatItems;
// 	UPjcScannerSubsystem* ScannerSubsystemPtr = nullptr;
// };
