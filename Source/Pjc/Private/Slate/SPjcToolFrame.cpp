// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcToolFrame.h"
#include "Slate/SPjcScanBar.h"
#include "Slate/PjcTabs.h"
#include "Slate/SPjcScanSettings.h"
#include "Slate/PjcWidgets.h"
#include "PjcScanDraft.h"
#include "PjcShim.h"
#include "PjcStyles.h"
// Engine Headers
#include "Misc/ConfigCacheIni.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCanvas.h"
#include "Widgets/SOverlay.h"
#include "PjcSubsystem.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	const TCHAR* PanelSection = TEXT("ProjectCleaner.ScanSettingsPanel");
	constexpr float StripWidth = 30.0f;
	constexpr float LabelLength = 96.0f;
}	// namespace

void SPjcToolFrame::Construct(const FArguments& InArgs) {
	PageName = InArgs._PageName;
	bPanelOpen = InArgs._PanelOpen;
	if (GConfig) {
		GConfig->GetBool(PanelSection, *PageName.ToString(), bPanelOpen, GEditorPerProjectIni);
	}

	SlideCurve = Slide.AddCurve(0.0f, 0.18f, ECurveEaseFunction::QuadOut);
	if (bPanelOpen) {
		Slide.JumpToEnd();
	}
	else {
		Slide.JumpToStart();
	}

	const TSharedRef<SWidget> Side = InArgs._Side.Widget;
	const TSharedRef<SWidget> Content = MakeContent(InArgs._Content.Widget);
	// clang-format off
	const TSharedRef<SWidget> Work = Side != SNullWidget::NullWidget ? MakeSideColumn(Side, Content) : StaticCastSharedRef<SWidget>(
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			MakeStrip()
		]
		// panel slides out by its width, pushing the tool content
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(this, &SPjcToolFrame::GetPanelWidth)
			.Visibility(this, &SPjcToolFrame::GetPanelVisibility)
			.Clipping(EWidgetClipping::ClipToBounds)
			[
				SNew(SBorder)
				.BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.GroupBorder")))
				.Padding(FMargin {4.0f, 0.0f})
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SNew(SPjcScanSettings).OnClose(this, &SPjcToolFrame::TogglePanel)
					]
				]
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f)
		[
			Content
		]
	);

	ChildSlot
	[
		SNew(SBorder).BorderImage(PjcShim::GetBrushPage()).Padding(0.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SPjcScanBar)
			]
			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
				Work
			]
		]
	];
	// clang-format on
}

TSharedRef<SWidget> SPjcToolFrame::MakeStrip() {
	// clang-format off
	// strip with the Scan settings tab, label rotated (a canvas keeps its full length, a box would clip it)
	return SNew(SBorder)
		.BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.GroupBorder")))
		.Padding(0.0f)
		[
			SNew(SBox).WidthOverride(StripWidth).VAlign(VAlign_Top)
			[
				SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
				.ForegroundColor(FSlateColor::UseForeground())
#endif
				.ButtonStyle(&PjcShim::GetStyle().GetWidgetStyle<FButtonStyle>(TEXT("NoBorder")))
				.ContentPadding(FMargin {0.0f, 8.0f})
				.ToolTipText_Lambda([this]() {
					const TCHAR* Action = bPanelOpen ? TEXT("Hide") : TEXT("Show");
					const TCHAR* Changed = FPjcScanDraft::IsChanged() ? TEXT(" \u00B7 changed, not scanned yet") : TEXT("");
					return FText::FromString(FString::Printf(TEXT("%s scan settings%s"), Action, Changed));
				})
				.OnClicked_Lambda([this]() {
					TogglePanel();
					return FReply::Handled();
				})
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(SBox).WidthOverride(16.0f).HeightOverride(16.0f)
						[
							SNew(SImage).Image(PjcShim::GetIconSettings())
						]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin {0.0f, 6.0f})
					[
						SNew(SBox).WidthOverride(StripWidth).HeightOverride(LabelLength)
						[
							SNew(SCanvas)
							+ SCanvas::Slot()
							.Position(FVector2D {StripWidth * 0.5f, LabelLength * 0.5f})
							.Size(FVector2D {LabelLength, StripWidth})
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							[
								SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
								.RenderTransform(FSlateRenderTransform {FQuat2D {FMath::DegreesToRadians(-90.0f)}})
								.RenderTransformPivot(FVector2D {0.5f, 0.5f})
								[
									SNew(STextBlock).Text(FText::FromString(TEXT("Scan settings")))
								]
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						// changed, not scanned yet
						SNew(SBox).WidthOverride(6.0f).HeightOverride(6.0f)
						.Visibility_Lambda([]() { return FPjcScanDraft::IsChanged() ? EVisibility::Visible : EVisibility::Hidden; })
						[
							SNew(SImage)
							.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
							.ColorAndOpacity(FSlateColor {FPjcStyles::Get().GetColor("ProjectCleaner.Color.Yellow")})
						]
					]
				]
			]
		];
	// clang-format on
}

TSharedRef<SWidget> SPjcToolFrame::MakeSideColumn(const TSharedRef<SWidget>& Side, const TSharedRef<SWidget>& Content) {
	if (GConfig) {
		GConfig->GetFloat(PanelSection, *(PageName.ToString() + TEXT("Side")), SideShare, GEditorPerProjectIni);
	}
	SideShare = FMath::Clamp(SideShare, 0.15f, 0.5f);

	// clang-format off
	return SNew(SSplitter).PhysicalSplitterHandleSize(3.0f)
		+ SSplitter::Slot()
		.Value(SideShare)
		.OnSlotResized(SSplitter::FOnSlotResized::CreateLambda([this](const float Value) {
			SideShare = Value;
			if (GConfig) {
				GConfig->SetFloat(PanelSection, *(PageName.ToString() + TEXT("Side")), SideShare, GEditorPerProjectIni);
			}
		}))
		[
			SNew(SScrollBox)
			// the numbers of the scan: none before the first scan, greyed while the results are out of date
			+ SScrollBox::Slot().Padding(FMargin {8.0f, 8.0f, 8.0f, 0.0f})
			[
				SNew(SBox)
				.Visibility_Lambda([]() { return UPjcSubsystem::GetScanResult().IsValid() ? EVisibility::Visible : EVisibility::Collapsed; })
				.IsEnabled_Lambda([]() { return !SPjcScanBar::NeedsRescan() && !PjcTabs::IsWindowOpen(); })
				[
					Side
				]
			]
			+ SScrollBox::Slot().Padding(FMargin {8.0f, 0.0f, 8.0f, 8.0f})
			[
				PjcWidgets::MakeCard(SNew(SPjcScanSettings), FMargin {4.0f, 6.0f})
			]
		]
		+ SSplitter::Slot()
		.Value(1.0f - SideShare)
		[
			// on a card with a margin: the gap shows where the column can be resized
			SNew(SBox).Padding(FMargin {4.0f, 8.0f, 8.0f, 8.0f})
			[
				PjcWidgets::MakeCard(Content, FMargin {1.0f})
			]
		];
	// clang-format on
}

TSharedRef<SWidget> SPjcToolFrame::MakeContent(const TSharedRef<SWidget>& Content) const {
	// clang-format off
	return SNew(SOverlay)
		// results from before a change are greyed and take no clicks: only Rescan and Scan settings work
		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([]() { return UPjcSubsystem::GetScanResult().IsValid() ? EVisibility::Visible : EVisibility::Collapsed; })
			.IsEnabled_Lambda([]() { return !SPjcScanBar::NeedsRescan() && !PjcTabs::IsWindowOpen(); })
			[
				Content
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			.Visibility_Lambda([]() { return UPjcSubsystem::GetScanResult().IsValid() ? EVisibility::Collapsed : EVisibility::Visible; })
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Not scanned yet"))).Font(FPjcStyles::GetFont("Bold", 12))
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin {0.0f, 6.0f, 0.0f, 12.0f})
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Scan the project to see which assets are used, unused or to review.")))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
				.ForegroundColor(FSlateColor::UseForeground())
#endif
				.ButtonStyle(&PjcShim::GetPrimaryButtonStyle())
				.ContentPadding(FMargin {16.0f, 4.0f})
				.OnClicked_Lambda([]() {
					if (FPjcScanDraft::IsChanged()) {
						FPjcScanDraft::Get().ApplyToSettings();
					}
					SPjcScanBar::ScanProject(true);
					return FReply::Handled();
				})
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("Scan project")))
				]
			]
		];
	// clang-format on
}

void SPjcToolFrame::TogglePanel() {
	bPanelOpen = !bPanelOpen;
	if (GConfig) {
		GConfig->SetBool(PanelSection, *PageName.ToString(), bPanelOpen, GEditorPerProjectIni);
	}

	if (bPanelOpen) {
		Slide.Play(AsShared());
	}
	else {
		Slide.PlayReverse(AsShared());
	}
}

FOptionalSize SPjcToolFrame::GetPanelWidth() const {
	return FOptionalSize {PanelWidth * SlideCurve.GetLerp()};
}

EVisibility SPjcToolFrame::GetPanelVisibility() const {
	return SlideCurve.GetLerp() > 0.0f ? EVisibility::Visible : EVisibility::Collapsed;
}
