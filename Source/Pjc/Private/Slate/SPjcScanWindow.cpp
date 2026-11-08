// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcScanWindow.h"
#include "Slate/PjcTabs.h"
#include "Slate/PjcWidgets.h"
#include "PjcFormat.h"
#include "PjcShim.h"
#include "PjcStyles.h"
// Engine Headers
#include "Framework/Application/SlateApplication.h"
#include "Misc/App.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	// unity builds join files: names unique to this file
	struct FPjcScanWindowStep
	{
		const TCHAR* Name;
		const TCHAR* ToolTip;
		// share of the scan time
		float Weight;
	};

	// clang-format off
	const FPjcScanWindowStep PjcScanWindowSteps[] = {
		{TEXT("Asset registry"), TEXT("Waits until the asset registry has found every asset"), 0.5f},
		{TEXT("Assets"), TEXT("Lists assets, folders and redirectors"), 0.5f},
		{TEXT("Source & config"), TEXT("Searches C++, Python and .ini files for asset paths"), 1.5f},
		{TEXT("Rules"), TEXT("Primary assets, labels, editor assets, cinematics and exclusions"), 0.8f},
		{TEXT("References"), TEXT("Follows references from every root"), 2.2f},
		{TEXT("Files"), TEXT("External files, corrupted files and empty folders"), 0.7f},
		{TEXT("Cook check"), TEXT("Compares unused assets with the last cook"), 0.4f},
	};
	// clang-format on
	constexpr int32 PjcScanWindowNumSteps = UE_ARRAY_COUNT(PjcScanWindowSteps);
}	// namespace

void SPjcScanWindow::Construct(const FArguments& InArgs) {
	Window = InArgs._Window;
	Found.SetNum(PjcScanWindowNumSteps);

	const TSharedRef<SVerticalBox> Steps = SNew(SVerticalBox);
	for (int32 Index = 0; Index < PjcScanWindowNumSteps; ++Index) {
		const FPjcScanWindowStep& Step = PjcScanWindowSteps[Index];
		// clang-format off
		Steps->AddSlot().AutoHeight().Padding(FMargin {0.0f, 2.0f})
		[
			SNew(SHorizontalBox).ToolTipText(FText::FromString(Step.ToolTip))
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin {0.0f, 0.0f, 8.0f, 0.0f})
			[
				SNew(SBox).WidthOverride(14.0f).HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([this, Index]() {
						const bool bDone = bFinished || Index < StepIndex;
						return FText::FromString(bDone ? TEXT("\u2713") : (Index == StepIndex ? TEXT("\u203A") : TEXT("\u00B7")));
					})
					.ColorAndOpacity_Lambda([this, Index]() {
						const bool bDone = bFinished || Index < StepIndex;
						return bDone ? FSlateColor {PjcWidgets::GetColor("ProjectCleaner.Color.Ok")} : FSlateColor::UseSubduedForeground();
					})
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Step.Name))
				.ColorAndOpacity_Lambda([this, Index]() { return Index <= StepIndex ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground(); })
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(STextBlock)
				.Text_Lambda([this, Index]() { return FText::FromString(Found[Index]); })
				.Font(PjcWidgets::GetFontMono(9))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		];
		// clang-format on
	}

	// clang-format off
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {16.0f, 14.0f, 16.0f, 8.0f})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(STextBlock)
				.Text_Lambda([this]() { return FText::FromString(bFinished ? TEXT("\u2713 Scan finished") : TEXT("Scanning project")); })
				.Font(FPjcStyles::GetFont("Bold", 12))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(FApp::GetProjectName())).ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(FMargin {16.0f, 4.0f})
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				Steps
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 12.0f, 0.0f, 4.0f})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SProgressBar).Percent_Lambda([this]() { return TOptional<float> {GetPercent()}; })
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {10.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(SBox).WidthOverride(40.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("%d%%"), FMath::FloorToInt(GetPercent() * 100.0f))); })
						.Font(PjcWidgets::GetFontMono(9))
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text_Lambda([this]() { return FText::FromString(CurrentText); })
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.Clipping(EWidgetClipping::ClipToBounds)
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBorder).BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.DarkGroupBorder"))).Padding(FMargin {16.0f, 8.0f})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 12.0f, 0.0f})
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("%.1f s"), FPlatformTime::Seconds() - StartTime)); })
					.Font(PjcWidgets::GetFontMono(9))
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("The last result stays if you cancel"))).ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
					.ContentPadding(FMargin {12.0f, 2.0f})
					.IsEnabled_Lambda([this]() { return !Progress.bCancelRequested && !bFinished; })
					.OnClicked_Lambda([this]() {
						Progress.bCancelRequested = true;
						return FReply::Handled();
					})
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("Cancel")))
					]
				]
			]
		]
	];
	// clang-format on

	Progress.OnStep = [this](const EPjcScanStep Step, const FString& Current) {
		StepIndex = static_cast<int32>(Step);
		CurrentText = Current;
		PumpUi();
	};
	Progress.OnFound = [this](const EPjcScanStep Step, const FString& InFound) {
		Found[static_cast<int32>(Step)] = InFound;
		PumpUi();
	};

	StartTime = FPlatformTime::Seconds();
}

bool SPjcScanWindow::Run() {
	// clang-format off
	const TSharedRef<SWindow> ScanWindow = SNew(SWindow)
		.Title(FText::FromString(TEXT("Scan")))
		.ClientSize(FVector2D {520.0f, 330.0f})
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.SizingRule(ESizingRule::FixedSize);
	// clang-format on
	const TSharedRef<SPjcScanWindow> Content = SNew(SPjcScanWindow).Window(ScanWindow);
	ScanWindow->SetContent(Content);

	// closing the window cancels the scan
	const TWeakPtr<SPjcScanWindow> WeakContent = Content;
	ScanWindow->SetRequestDestroyWindowOverride(FRequestDestroyWindowOverride::CreateLambda([WeakContent](const TSharedRef<SWindow>& Closing) {
		const TSharedPtr<SPjcScanWindow> Pinned = WeakContent.Pin();
		if (Pinned.IsValid() && !Pinned->bFinished) {
			Pinned->Progress.bCancelRequested = true;
			return;
		}
		FSlateApplication::Get().RequestDestroyWindow(Closing);
	}));

	// modal without a loop of its own, like the engine's progress windows: the scan runs right here and draws the
	// window between its steps. A scan started from a widget timer would start again in every Tick it calls
	PjcTabs::SetWindowOpen(true);
	FSlateApplication::Get().AddModalWindow(ScanWindow, FSlateApplication::Get().GetActiveTopLevelWindow(), true);
	ScanWindow->ShowWindow();
	Content->StartScan();
	PjcTabs::SetWindowOpen(false);
	return Content->bScanned;
}

void SPjcScanWindow::StartScan() {
	PumpUi();
	bScanned = UPjcSubsystem::Scan(false, &Progress);
	bFinished = true;
	CurrentText.Reset();
	PumpUi();

	// every step ticked for a moment, then the window closes
	FPlatformProcess::Sleep(bScanned ? 0.35f : 0.0f);
	if (const TSharedPtr<SWindow> Pinned = Window.Pin()) {
		Pinned->RequestDestroyWindow();
	}
}

float SPjcScanWindow::GetPercent() const {
	if (bFinished) return 1.0f;

	float Total = 0.0f;
	float Done = 0.0f;
	for (int32 Index = 0; Index < PjcScanWindowNumSteps; ++Index) {
		Total += PjcScanWindowSteps[Index].Weight;
		Done += Index < StepIndex ? PjcScanWindowSteps[Index].Weight : 0.0f;
	}
	return Total > 0.0f ? Done / Total : 0.0f;
}

void SPjcScanWindow::PumpUi() {
	// the scan runs here: the window is drawn and Cancel is read between steps
	if (FSlateApplication::IsInitialized()) {
		FSlateApplication::Get().PumpMessages();
		FSlateApplication::Get().Tick();
	}
}
