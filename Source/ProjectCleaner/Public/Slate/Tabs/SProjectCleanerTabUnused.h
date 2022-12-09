// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FProjectCleanerScanner;
class SProjectCleanerTreeView;
class SProjectCleanerAssetBrowser;
class UProjectCleanerScanSettings;
class UProjectCleanerExcludeSettings;

class SProjectCleanerTabUnused final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerTabUnused)
		{
		}

		SLATE_ARGUMENT(TSharedPtr<FProjectCleanerScanner>, Scanner)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	void UpdateView();
	void UpdateFilter();

	TSet<FString> SelectedPaths;
	TSharedPtr<FProjectCleanerScanner> Scanner;
	TSharedPtr<SProjectCleanerTreeView> ProjectCleanerTreeView;
	TSharedPtr<SProjectCleanerAssetBrowser> ProjectCleanerAssetBrowser;
	UProjectCleanerScanSettings* ScanSettings = nullptr;
	UProjectCleanerExcludeSettings* ExcludeSettings = nullptr;
};
