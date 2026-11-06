// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabOverview.h"
#include "Slate/PjcWidgets.h"
#include "Slate/SPjcScanBar.h"
#include "Slate/SPjcToolFrame.h"
#include "PjcConstants.h"
#include "PjcFormat.h"
#include "PjcSettings.h"
#include "PjcStyles.h"
#include "PjcSubsystem.h"
// Engine Headers
#include "Framework/Docking/TabManager.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Text/STextBlock.h"

void SPjcTabOverview::Construct(const FArguments& InArgs) {
	TabManager = InArgs._TabManager;
	ScanFinishedHandle = UPjcSubsystem::OnScanFinished().AddSP(this, &SPjcTabOverview::OnScanFinished);

	// clang-format off
	ChildSlot
	[
		SNew(SPjcToolFrame)
		.PageName(TEXT("Overview"))
		.PanelOpen(true)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SAssignNew(Body, SBox).Padding(FMargin {14.0f})
			]
		]
	];
	// clang-format on

	OnScanFinished();

	if (!UPjcSubsystem::GetScanResult().IsValid() && GetDefault<UPjcEditorSettings>()->bScanOnOpen) {
		SPjcScanBar::ScanProject();
	}
}

SPjcTabOverview::~SPjcTabOverview() {
	UPjcSubsystem::OnScanFinished().Remove(ScanFinishedHandle);
}

void SPjcTabOverview::OnScanFinished() {
	Overview = FPjcOverview::Make(UPjcSubsystem::GetScanResult());
	Rebuild();
}

void SPjcTabOverview::Rebuild() {
	if (!UPjcSubsystem::GetScanResult().IsValid()) {
		Body->SetContent(SNew(STextBlock)
							 .Text(FText::FromString(TEXT("Not scanned yet. Press Scan to see the project overview.")))
							 .ColorAndOpacity(FSlateColor::UseSubduedForeground()));
		return;
	}

	// clang-format off
	Body->SetContent(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			MakeSummary()
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 14.0f, 0.0f, 0.0f})
		[
			MakeIssues()
		]
	);
	// clang-format on
}

void SPjcTabOverview::OpenTab(const FName TabId) const {
	if (const TSharedPtr<FTabManager> Tabs = TabManager.Pin()) {
		Tabs->TryInvokeTab(FTabId {TabId});
	}
}

TSharedRef<SWidget> SPjcTabOverview::MakeSummary() {
	const FPjcOverview::FTotal& All = Overview.All;
	const FLinearColor ColorUsed = PjcWidgets::GetColor("ProjectCleaner.Color.UsedFill");
	const FLinearColor ColorEditor = PjcWidgets::GetColor("ProjectCleaner.Color.EditorFill");
	const FLinearColor ColorUnused = PjcWidgets::GetColor("ProjectCleaner.Color.UnusedFill");

	const auto Share = [&All](const FPjcOverview::FTotal& Total) {
		return FString::Printf(TEXT("%s · %s"), *PjcFormat::Size(Total.Size), *PjcFormat::Percent(Total.Size, All.Size));
	};

	// one column per number: label (with its bar color), count, size
	const auto Column = [](const FString& Label,
						   const TOptional<FLinearColor>& Swatch,
						   const int32 Num,
						   const FString& Sub,
						   const FSlateColor& NumColor,
						   const FString& ToolTip,
						   const TSharedRef<SWidget>& Extra) {
		const TSharedRef<SHorizontalBox> Head = SNew(SHorizontalBox);
		if (Swatch.IsSet()) {
			Head->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 6.0f, 0.0f})[PjcWidgets::MakeSwatch(Swatch.GetValue())];
		}
		Head->AddSlot().FillWidth(1.0f).VAlign(
			VAlign_Center
		)[SNew(STextBlock).Text(FText::FromString(Label)).ColorAndOpacity(FSlateColor::UseSubduedForeground())];
		Head->AddSlot().AutoWidth().VAlign(VAlign_Center)[Extra];

		// clang-format off
		return SNew(SBox)
			.Padding(FMargin {16.0f, 11.0f, 16.0f, 10.0f})
			.ToolTip(PjcWidgets::MakeToolTip(Label, ToolTip))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					Head
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 2.0f, 0.0f, 1.0f})
				[
					SNew(STextBlock).Text(FText::FromString(PjcFormat::Count(Num))).Font(PjcWidgets::GetFontMono(18)).ColorAndOpacity(NumColor)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(FText::FromString(Sub)).ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			];
		// clang-format on
	};

	const TSharedRef<SWidget> Review = SNew(SHyperlink)
										   .Text(FText::FromString(TEXT("Review ›")))
										   .ToolTipText(FText::FromString(TEXT("Opens Assets Unused")))
										   .OnNavigate_Lambda([this]() {
											   OpenTab(PjcConstants::TabAssetsUnused);
										   });

	const TSharedRef<SHorizontalBox> Columns = SNew(SHorizontalBox);
	const auto AddColumn = [&Columns](const TSharedRef<SWidget>& Widget) {
		if (Columns->NumSlots() > 0) {
			Columns->AddSlot().AutoWidth()[SNew(SBox).WidthOverride(
				1.0f
			)[SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.Line"))]];
		}
		Columns->AddSlot().FillWidth(1.0f)[Widget];
	};
	AddColumn(Column(
		TEXT("Assets"),
		{},
		All.Num,
		PjcFormat::Size(All.Size),
		FSlateColor::UseForeground(),
		TEXT("Every scanned asset. Redirectors are not counted."),
		SNullWidget::NullWidget
	));
	AddColumn(Column(
		TEXT("Used"),
		ColorUsed,
		Overview.Used.Num,
		Share(Overview.Used),
		FSlateColor::UseForeground(),
		TEXT("The game uses them: maps, primary assets and other roots, and what they reference."),
		SNullWidget::NullWidget
	));
	AddColumn(Column(
		TEXT("Editor only"),
		ColorEditor,
		Overview.EditorOnly.Num,
		Share(Overview.EditorOnly),
		FSlateColor::UseForeground(),
		TEXT("Kept, but only the editor uses them: editor assets, and what only editor-only references or labels reach. They do not ship."),
		SNullWidget::NullWidget
	));
	AddColumn(Column(
		TEXT("Unused"),
		ColorUnused,
		Overview.Unused.Num,
		Share(Overview.Unused),
		Overview.Unused.Num > 0 ? FSlateColor {PjcWidgets::GetColor("ProjectCleaner.Color.Red")} : FSlateColor::UseForeground(),
		TEXT("Nothing used references them. Cleanup deletes them."),
		Review
	));

	// storage along the bottom, by size
	TArray<PjcWidgets::FBarSegment> Segments;
	const auto AddSegment = [&Segments, &Share](const FString& Label, const FPjcOverview::FTotal& Total, const FLinearColor& Color) {
		PjcWidgets::FBarSegment& Segment = Segments.AddDefaulted_GetRef();
		Segment.Value = static_cast<double>(Total.Size);
		Segment.Color = Color;
		Segment.ToolTip = PjcWidgets::MakeToolTip(Label, Share(Total) + TEXT(" · ") + PjcFormat::Count(Total.Num) + TEXT(" assets"));
	};
	AddSegment(TEXT("Used"), Overview.Used, ColorUsed);
	AddSegment(TEXT("Editor only"), Overview.EditorOnly, ColorEditor);
	AddSegment(TEXT("Unused"), Overview.Unused, ColorUnused);

	// clang-format off
	return PjcWidgets::MakeCard(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			Columns
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			PjcWidgets::MakeBar(Segments, 6.0f)
		],
		FMargin {1.0f}
	);
	// clang-format on
}

TSharedRef<SWidget> SPjcTabOverview::MakeIssues() {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();

	struct FIssue
	{
		FString Title;
		FString Lower;
		int32 Num;
		FString ToolTip;
		FName TabId;
	};
	const FString ExternalExample
		= Result.FilesExternal.Num() > 0 ? FString::Printf(TEXT(", e.g. %s"), *FPaths::GetCleanFilename(Result.FilesExternal[0])) : FString {};
	const TArray<FIssue> Issues {
	  {TEXT("Redirectors"),
	   TEXT("redirectors"),
	   Result.Redirectors.Num(),
	   TEXT("Left by renamed or moved assets. Scanning follows them, fixing resaves the assets that still use the old path."),
	   PjcConstants::TabAssetsUnused},
	  {TEXT("External files"),
	   TEXT("external files"),
	   Result.FilesExternal.Num(),
	   FString::Printf(
		   TEXT("Files in content folders the engine does not use%s. %s more are kept: used by the project, or excluded in settings."),
	   *ExternalExample,
	   *PjcFormat::Count(Result.FilesExternalKept.Num())
	   ), PjcConstants::TabFilesExternal},
	  {TEXT("Corrupted files"),
	   TEXT("corrupted files"),
	   Result.FilesCorrupted.Num(),
	   TEXT("Asset files the engine cannot read."),
	   PjcConstants::TabAssetsCorrupted},
	  {TEXT("Empty folders"),
	   TEXT("empty folders"),
	   Result.FoldersEmpty.Num(),
	   TEXT("Folders without assets or files."),
	   PjcConstants::TabAssetsUnused},
	  {TEXT("Not deleted"),
	   TEXT("assets left by the last cleanup"),
	   UPjcSubsystem::GetLastCleanupKept().Num(),
	   TEXT("Unused assets the last cleanup could not delete, because something holds them."),
	   PjcConstants::TabAssetsUnused},
	};

	const FLinearColor ColorWarn = PjcWidgets::GetColor("ProjectCleaner.Color.Yellow");
	const TSharedRef<SWrapBox> Row = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D {8.0f, 8.0f});
	TArray<FString> None;
	TArray<FString> NoneLines;
	for (const auto& Issue : Issues) {
		if (Issue.Num == 0) {
			None.Add(Issue.Lower);
			NoneLines.Add(Issue.Title + TEXT(": none"));
			continue;
		}

		const FName TabId = Issue.TabId;
		// clang-format off
		Row->AddSlot()
		[
			PjcWidgets::MakeCard(
				SNew(SHorizontalBox)
				.ToolTip(PjcWidgets::MakeToolTip(Issue.Title, Issue.ToolTip))
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(3.0f)
					[
						SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(ColorWarn)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {10.0f, 4.0f, 8.0f, 4.0f})
				[
					SNew(STextBlock).Text(FText::FromString(PjcFormat::Count(Issue.Num))).Font(PjcWidgets::GetFontMono(11)).ColorAndOpacity(ColorWarn)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(FText::FromString(Issue.Title))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {10.0f, 3.0f, 6.0f, 3.0f})
				[
					SNew(SButton)
					.ContentPadding(FMargin {8.0f, 0.0f})
					.OnClicked_Lambda([this, TabId]() {
						OpenTab(TabId);
						return FReply::Handled();
					})
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("Review")))
					]
				],
				FMargin {1.0f}
			)
		];
		// clang-format on
	}

	if (None.Num() > 0) {
		FString Clear = TEXT("No issues found");
		if (None.Num() < Issues.Num()) {
			const FString Last = None.Pop();
			Clear = TEXT("No ") + (None.Num() > 0 ? FString::Join(None, TEXT(", ")) + TEXT(" or ") : FString {}) + Last;
		}

		// clang-format off
		Row->AddSlot().VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			.ToolTip(PjcWidgets::MakeToolTip(TEXT("No issues"), FString::Join(NoneLines, TEXT("\n"))))
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {2.0f, 0.0f, 6.0f, 0.0f})
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("✓"))).ColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.Ok"))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(Clear)).ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		];
		// clang-format on
	}

	return Row;
}
