// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcScanWindow.h"
#include "Slate/PjcTabs.h"
#include "Slate/PjcWidgets.h"
#include "Slate/SPjcTable.h"
#include "PjcFormat.h"
#include "PjcShim.h"
#include "PjcStyles.h"
// Engine Headers
#include "Framework/Application/SlateApplication.h"
#include "Misc/App.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
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
		{TEXT("Asset Registry"), TEXT("Waits until Asset Registry finds all assets"), 0.5f},
		{TEXT("Assets"), TEXT("Lists assets, folders and redirectors"), 0.5f},
		{TEXT("Source & config"), TEXT("Searches C++, C#, Python and .ini files for asset paths"), 1.5f},
		{TEXT("Rules"), TEXT("Primary assets, labels, editor assets, cinematics and exclusions"), 0.8f},
		{TEXT("References"), TEXT("Follows references from every root"), 2.2f},
		{TEXT("Files"), TEXT("External files, corrupted files and empty folders"), 0.7f},
		{TEXT("Cook check"), TEXT("Compares unused assets with last cook"), 0.4f},
	};
	// clang-format on
	constexpr int32 PjcScanWindowNumSteps = UE_ARRAY_COUNT(PjcScanWindowSteps);
	constexpr float PjcScanWindowWidth = 560.0f;
}	// namespace

void SPjcScanWindow::Construct(const FArguments& InArgs) {
	Window = InArgs._Window;
	Found.SetNum(PjcScanWindowNumSteps);

	// the steps: a mark (done, running, waiting), the name, what the step found
	FPjcTable Steps;
	FPjcTableColumn Mark;
	Mark.FixedWidth = 34.0f;
	Mark.bLead = true;
	FPjcTableColumn Name;
	Name.Label = TEXT("Step");
	FPjcTableColumn Result;
	Result.Label = TEXT("Found");
	Result.Fill = 2.2f;
	Result.bNumber = true;
	Steps.Columns = {Mark, Name, Result};
	Steps.NumVisible = PjcScanWindowNumSteps;
	Steps.bFixedHeight = true;
	const FSlateColor Strong {PjcWidgets::GetColor("ProjectCleaner.Color.Strong")};
	for (int32 Index = 0; Index < PjcScanWindowNumSteps; ++Index) {
		const auto IsDone = [this, Index]() {
			return bFinished || Index < StepIndex;
		};
		const auto IsRunning = [this, Index]() {
			return !bFinished && Index == StepIndex;
		};
		FPjcTableRow& Row = Steps.Rows.AddDefaulted_GetRef();
		Row.Cells = {
		  FString {},
		  FString {PjcScanWindowSteps[Index].Name},
		  TAttribute<FString>::Create(TAttribute<FString>::FGetter::CreateLambda([this, Index]() {
			  return Found[Index];
		  })),
		};
		Row.ToolTipTitle = PjcScanWindowSteps[Index].Name;
		Row.ToolTip = PjcScanWindowSteps[Index].ToolTip;
		Row.MakeLead = [IsDone, IsRunning]() {
			return PjcWidgets::MakeStepMark(IsDone, IsRunning);
		};
		Row.Marked = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda(IsRunning));
		Row.TextColor = TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateLambda([IsDone, IsRunning, Strong]() {
			return IsRunning() ? Strong : (IsDone() ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground());
		}));
	}

	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	const FLinearColor Accent = PjcWidgets::GetColor("ProjectCleaner.Color.Accent");
	const FLinearColor Ok = PjcWidgets::GetColor("ProjectCleaner.Color.Ok");
	// clang-format off
	ChildSlot
	[
		SNew(SBox).WidthOverride(PjcScanWindowWidth)
		[
			SNew(SBorder).BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.GroupBorder"))).Padding(0.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {16.0f, 14.0f, 16.0f, 10.0f})
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 8.0f, 0.0f})
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("\u2713")))
						.Font(FPjcStyles::GetFont("Bold", 12))
						.ColorAndOpacity(Ok)
						.Visibility_Lambda([this]() { return bFinished && bScanned ? EVisibility::Visible : EVisibility::Collapsed; })
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text_Lambda([this]() {
							const TCHAR* Done = bScanned ? TEXT("Scan finished") : TEXT("Scan canceled");
							return FText::FromString(bFinished ? Done : TEXT("Scanning project"));
						})
						.Font(FPjcStyles::GetFont("Bold", 12))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(FApp::GetProjectName())).ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {16.0f, 0.0f, 16.0f, 14.0f})
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SAssignNew(StatsBox, SBox)
						[
							MakeStats(false)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 10.0f, 0.0f, 0.0f})
					[
						SNew(SBox).HeightOverride(4.0f)
						[
							SNew(SProgressBar)
							.Style(&FPjcStyles::Get().GetWidgetStyle<FProgressBarStyle>("ProjectCleaner.ProgressFlat"))
							.BorderPadding(FVector2D {0.0f, 0.0f})
							.FillColorAndOpacity_Lambda([this, Accent, Ok]() { return FSlateColor {bFinished && bScanned ? Ok : Accent}; })
							.Percent_Lambda([this]() { return TOptional<float> {GetPercent()}; })
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 10.0f, 0.0f, 0.0f})
					[
						SNew(SPjcTable).Table(Steps)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 10.0f, 0.0f, 0.0f})
					[
						SNew(SHorizontalBox)
						.Visibility_Lambda([this]() { return bFinished ? EVisibility::Hidden : EVisibility::Visible; })
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 8.0f, 0.0f})
						[
							PjcWidgets::MakeCardTitle(TEXT("Reading"))
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
						[
							SNew(SPjcFitText)
							.Text_Lambda([this]() { return CurrentText; })
							.TrimStart(true)
							.Font(PjcWidgets::GetFontMono(8))
							.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBox).HeightOverride(1.0f)
					[
						SNew(SImage).Image(White).ColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.Line"))
					]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBorder)
					.BorderImage(White)
					.BorderBackgroundColor(PjcWidgets::GetColor("ProjectCleaner.Color.Recess"))
					.Padding(FMargin {16.0f, 10.0f})
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text_Lambda([this]() {
								const FString Scanned = PjcFormat::Count(UPjcSubsystem::GetScanResult().Assets.Num()) + TEXT(" assets scanned");
								const FString Done = bScanned ? Scanned : FString {TEXT("Last scan result stays")};
								return FText::FromString(bFinished ? Done : FString {TEXT("If you cancel, last scan result stays")});
							})
							.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {10.0f, 0.0f, 0.0f, 0.0f})
						[
							SNew(SBox).MinDesiredWidth(76.0f)
							[
								SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
								.ButtonStyle(&PjcShim::GetButtonStyle427(false))
								.ForegroundColor(FSlateColor::UseForeground())
#endif
								.HAlign(HAlign_Center)
								.ContentPadding(FMargin {12.0f, 2.0f})
								.IsEnabled_Lambda([this]() { return bFinished || !Progress.bCancelRequested; })
								.OnClicked_Lambda([this]() {
									if (bFinished) {
										Close();
									}
									else {
										Progress.bCancelRequested = true;
									}
									return FReply::Handled();
								})
								[
									SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(bFinished ? TEXT("Close") : TEXT("Cancel")); })
								]
							]
						]
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

TSharedRef<SWidget> SPjcScanWindow::MakeStats(const bool bDone) {
	const auto Stat = [](const TAttribute<FString>& Value, const TCHAR* Label, const TCHAR* ToolTip) {
		PjcWidgets::FStat Out;
		Out.Value = Value;
		Out.Label = Label;
		Out.ToolTip = ToolTip;
		return Out;
	};
	const auto Time = TAttribute<FString>::Create(TAttribute<FString>::FGetter::CreateLambda([this]() {
		const double Now = bFinished ? EndTime : FPlatformTime::Seconds();
		return FString::Printf(TEXT("%.1f s"), Now - StartTime);
	}));
	if (bDone) {
		const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
		int32 NumReview = 0;
		for (const auto& Pair : Result.Status) {
			NumReview += Pair.Value == EPjcAssetStatus::Review ? 1 : 0;
		}
		const TArray<PjcWidgets::FStat> Done {
		  Stat(PjcFormat::Count(Result.Assets.Num()), TEXT("Assets"), TEXT("Assets the scan read")),
		  Stat(Time, TEXT("Time"), TEXT("How long the scan took")),
		  Stat(PjcFormat::Count(Result.Unused.Num()), TEXT("Unused"), TEXT("No used asset references them")),
		  Stat(PjcFormat::Count(NumReview), TEXT("To review"), TEXT("The scan is not sure about them. Issues lists them")),
		};
		return PjcWidgets::MakeStats(Done, 1.25f);
	}

	const auto Percent = TAttribute<FString>::Create(TAttribute<FString>::FGetter::CreateLambda([this]() {
		return FString::Printf(TEXT("%d %%"), FMath::FloorToInt(GetPercent() * 100.0f));
	}));
	const auto Step = TAttribute<FString>::Create(TAttribute<FString>::FGetter::CreateLambda([this]() {
		return FString::Printf(TEXT("%d of %d"), FMath::Min(StepIndex + 1, PjcScanWindowNumSteps), PjcScanWindowNumSteps);
	}));
	const auto Assets = TAttribute<FString>::Create(TAttribute<FString>::FGetter::CreateLambda([this]() {
		return Progress.NumAssets >= 0 ? PjcFormat::Count(Progress.NumAssets) : FString {TEXT("\u2013")};
	}));
	const TArray<PjcWidgets::FStat> Running {
	  Stat(Percent, TEXT("Progress"), TEXT("Share of the scan that is done")),
	  Stat(Time, TEXT("Time"), TEXT("Time since the scan started")),
	  Stat(Step, TEXT("Step"), TEXT("The step that runs now")),
	  Stat(Assets, TEXT("Assets"), TEXT("Assets the scan reads, known after the Assets step")),
	};
	return PjcWidgets::MakeStats(Running, 1.25f);
}

bool SPjcScanWindow::Run(const bool bStayOpen) {
	// the content first: the window opens with its size, centered
	const TSharedRef<SPjcScanWindow> Content = SNew(SPjcScanWindow);
	// clang-format off
	const TSharedRef<SWindow> ScanWindow = SNew(SWindow)
		.Title(FText::FromString(TEXT("Scan")))
		.ClientSize(PjcWidgets::GetContentSize(Content))
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.SizingRule(ESizingRule::FixedSize);
	// clang-format on
	Content->Window = ScanWindow;
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
	// the tools behind stay dimmed until the window is closed
	PjcTabs::SetWindowOpen(true);
	ScanWindow->SetOnWindowClosed(FOnWindowClosed::CreateLambda([](const TSharedRef<SWindow>&) {
		PjcTabs::SetWindowOpen(false);
	}));
	FSlateApplication::Get().AddModalWindow(ScanWindow, FSlateApplication::Get().GetActiveTopLevelWindow(), true);
	ScanWindow->ShowWindow();
	Content->StartScan();
	// closes by itself when the scan ends: every step ticked for a moment, then the window closes and
	// a notification says how it went. Only a scan that failed stays open, when asked, for the user to read
	const bool bCanceled = Content->Progress.bCancelRequested;
	if (Content->bScanned || bCanceled || !bStayOpen) {
		FPlatformProcess::Sleep(Content->bScanned ? 0.35f : 0.0f);
		Content->Close();
	}
	return Content->bScanned;
}

void SPjcScanWindow::Close() const {
	if (const TSharedPtr<SWindow> Pinned = Window.Pin()) {
		Pinned->RequestDestroyWindow();
	}
}

void SPjcScanWindow::StartScan() {
	PumpUi();
	bScanned = UPjcSubsystem::Scan(false, &Progress);
	bFinished = true;
	EndTime = FPlatformTime::Seconds();
	CurrentText.Reset();
	if (bScanned) StatsBox->SetContent(MakeStats(true));
	PumpUi();
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
