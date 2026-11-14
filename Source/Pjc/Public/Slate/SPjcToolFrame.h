// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/CurveSequence.h"
#include "Widgets/SCompoundWidget.h"

/**
 * Frame of every Project Cleaner tool: scan bar on top, a strip on the left edge whose Scan settings tab slides the
 * settings panel out next to it (pushing the tool content), then the tool content.
 * Each tool remembers whether its panel is open (per user).
 * With a Side widget there is no strip: a resizable column on the left holds the Side widget and the scan settings under it.
 */
class SBorder;

class SPjcToolFrame final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcToolFrame) : _PanelOpen(false), _Settings(true) {}
	// Name the open state is saved under
	SLATE_ARGUMENT(FName, PageName)
	// Open state the first time the tool is shown
	SLATE_ARGUMENT(bool, PanelOpen)
	// false: no scan settings at all, e.g. Issues (user, 2026-11-14: they are set once, in Assets)
	SLATE_ARGUMENT(bool, Settings)
	// Column left of the content, above the scan settings (Assets: the overview)
	SLATE_NAMED_SLOT(FArguments, Side)
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
	// Content with Not scanned yet over it before the first scan
	TSharedRef<SWidget> MakeContent(const TSharedRef<SWidget>& Content) const;
	TSharedRef<SWidget> MakeStrip();
	TSharedRef<SWidget> MakeSideColumn(const TSharedRef<SWidget>& Side, const TSharedRef<SWidget>& Content);
	FOptionalSize GetSideWidth() const;
	EVisibility GetPanelVisibility() const;

	FName PageName;
	bool bPanelOpen = false;
	FCurveSequence Slide;
	FCurveHandle SlideCurve;
	// Share of the side column, saved per user
	float SideShare = 0.3f;
	TSharedPtr<SBorder> SideHandle;
	bool bSideDragging = false;
};
