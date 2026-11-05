// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/CurveSequence.h"
#include "Widgets/SCompoundWidget.h"

/**
 * Frame of every Project Cleaner tool: scan bar on top, a strip on the left edge whose Scan settings tab slides the
 * settings panel out next to it (pushing the tool content), then the tool content.
 * Each tool remembers whether its panel is open (per user).
 */
class SPjcToolFrame final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcToolFrame) : _PanelOpen(false) {}
	/** Name the open state is saved under */
	SLATE_ARGUMENT(FName, PageName)
	/** Open state the first time the tool is shown */
	SLATE_ARGUMENT(bool, PanelOpen)
	SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void TogglePanel();
	bool IsPanelOpen() const {
		return bPanelOpen;
	}

	static constexpr float PanelWidth = 290.0f;

  private:

	FOptionalSize GetPanelWidth() const;
	EVisibility GetPanelVisibility() const;

	FName PageName;
	bool bPanelOpen = false;
	FCurveSequence Slide;
	FCurveHandle SlideCurve;
};
