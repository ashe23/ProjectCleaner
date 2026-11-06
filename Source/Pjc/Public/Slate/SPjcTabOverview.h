// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcOverview.h"
#include "Widgets/SCompoundWidget.h"

class FTabManager;
class SBox;

/** Overview tool: what the shared scan found for the whole project */
class SPjcTabOverview final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcTabOverview) {}
	/** Tabs the Overview links to (none in tests) */
	SLATE_ARGUMENT(TSharedPtr<FTabManager>, TabManager)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SPjcTabOverview() override;

  private:

	void OnScanFinished();
	void Rebuild();
	void OpenTab(const FName TabId) const;

	TSharedRef<SWidget> MakeSummary();

	FDelegateHandle ScanFinishedHandle;
	TWeakPtr<FTabManager> TabManager;
	TSharedPtr<SBox> Body;
	FPjcOverview Overview;
};
