// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerScanner.h"
#include "Widgets/SCompoundWidget.h"

class UProjectCleanerScanSettings;

class SProjectCleanerWindowMain final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerWindowMain)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SProjectCleanerWindowMain() override;
private:
	static bool IsWidgetEnabled();
	static int32 GetWidgetIndex();

	FReply OnBtnScanProjectClicked() const;
	FReply OnBtnCleanProjectClicked() const;
	FReply OnBtnDeleteEmptyFoldersClicked() const;

	TWeakObjectPtr<UProjectCleanerScanSettings> ScanSettings;
	TSharedPtr<FProjectCleanerScanner> ProjectCleanerScanner;
};
