// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/** Overview tool: what the shared scan found for the whole project */
class SPjcTabOverview final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcTabOverview) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SPjcTabOverview() override;

  private:

	void OnScanFinished();
	FText GetSummaryText() const;

	FDelegateHandle ScanFinishedHandle;
	int32 NumUsed = 0;
	int32 NumEditorOnly = 0;
	int32 NumUnused = 0;
};
