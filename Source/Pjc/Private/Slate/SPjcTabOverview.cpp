// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabOverview.h"
#include "Slate/PjcTabs.h"
#include "Slate/PjcWidgets.h"
#include "Slate/SPjcScanBar.h"

#include "Slate/SPjcToolFrame.h"
#include "PjcConstants.h"
#include "PjcFormat.h"
#include "PjcSettings.h"
#include "PjcStyles.h"
#include "PjcSubsystem.h"
#include "Runtime/Launch/Resources/Version.h"
// Engine Headers
#include "Framework/Docking/TabManager.h"
#include "HAL/PlatformProperties.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Docking/SDockTab.h"
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
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	Overview = FPjcOverview::Make(Result);

	Cook = FPjcCookInfo::GetLast();
	Shipped.Reset();
	if (Result.IsValid() && Cook.IsValid()) {
		Shipped = FPjcOverview::MakeShipped(Result, Cook);
	}
	// the card needs only the summary of the cook
	Cook.Cooked.Empty();
	Cook.SavedDigests.Empty();

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
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 14.0f, 0.0f, 0.0f})
		[
			MakeShippedCard()
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 14.0f, 0.0f, 0.0f})
		[
			MakeColumns(MakeReasons(), MakeClasses())
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 14.0f, 0.0f, 0.0f})
		[
			MakeColumns(MakeUnusedFolders(), MakeLargestUnused())
		]
	);
	// clang-format on
}

void SPjcTabOverview::OpenTab(const FName TabId) const {
	PjcTabs::Open(TabManager, TabId);
}

void SPjcTabOverview::Inspect(const FName Package) const {
	PjcTabs::Inspect(TabManager, Package);
}

void SPjcTabOverview::ShowFolder(const FString Path) const {
	PjcTabs::ShowFolder(TabManager, Path);
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
	   NAME_None},
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
						// Not deleted (no tab of its own): the kept assets in Assets Unused
						if (TabId.IsNone()) {
							PjcTabs::ShowNotDeleted(TabManager);
						}
						else {
							OpenTab(TabId);
						}
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

TSharedRef<SWidget> SPjcTabOverview::MakeReasons() const {
	const TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	const int64 MaxSize = Overview.Reasons.Num() > 0 ? Overview.Reasons[0].Total.Size : 0;

	for (const auto& Row : Overview.Reasons) {
		FString Name;
		FString ToolTip;
		FLinearColor Color;
		GetReasonInfo(Row.Reason, Name, ToolTip, Color);

		PjcWidgets::FBarSegment Segment;
		Segment.Value = static_cast<double>(Row.Total.Size);
		Segment.Color = Color;

		// clang-format off
		const TSharedRef<SWidget> NameWidget = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 7.0f, 0.0f})
			[
				PjcWidgets::MakeSwatch(Color, 8.0f)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(Name))
			];
		// clang-format on
		const FString ValueText = FString::Printf(TEXT("%s · %s"), *PjcFormat::Size(Row.Total.Size), *PjcFormat::Count(Row.Total.Num));
		const TSharedRef<SWidget> Value = SNew(STextBlock)
											  .Text(FText::FromString(ValueText))
											  .Font(PjcWidgets::GetFontMono(9))
											  .ColorAndOpacity(FSlateColor::UseSubduedForeground());
		const FString Share = FString::Printf(TEXT("%s of the project size"), *PjcFormat::Percent(Row.Total.Size, Overview.All.Size));

		Rows->AddSlot()
			.AutoHeight()[MakeRow(NameWidget, Value, {Segment}, static_cast<double>(MaxSize), PjcWidgets::MakeToolTip(Name, ToolTip, Share))];
	}

	return MakeSection(TEXT("Usage reasons"), TEXT("by chain start"), Rows);
}

TSharedRef<SWidget> SPjcTabOverview::MakeClasses() const {
	constexpr int32 MaxRows = 8;
	const TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	const int64 MaxSize = Overview.Classes.Num() > 0 ? Overview.Classes[0].Total.Size : 0;

	for (int32 Index = 0; Index < Overview.Classes.Num() && Index < MaxRows; ++Index) {
		const FPjcOverview::FClass& Class = Overview.Classes[Index];

		TArray<PjcWidgets::FBarSegment> Segments;
		Segments.AddDefaulted(3);
		Segments[0].Value = static_cast<double>(Class.SizeUsed);
		Segments[0].Color = PjcWidgets::GetColor("ProjectCleaner.Color.UsedFill");
		Segments[1].Value = static_cast<double>(Class.SizeEditorOnly);
		Segments[1].Color = PjcWidgets::GetColor("ProjectCleaner.Color.EditorFill");
		Segments[2].Value = static_cast<double>(Class.SizeUnused);
		Segments[2].Color = PjcWidgets::GetColor("ProjectCleaner.Color.UnusedFill");

		const FString ValueText = FString::Printf(TEXT("%s · %s"), *PjcFormat::Size(Class.Total.Size), *PjcFormat::Count(Class.Total.Num));
		const TSharedRef<SHorizontalBox> Value = SNew(SHorizontalBox);
		Value->AddSlot().AutoWidth()[SNew(STextBlock)
										 .Text(FText::FromString(ValueText))
										 .Font(PjcWidgets::GetFontMono(9))
										 .ColorAndOpacity(FSlateColor::UseSubduedForeground())];
		if (Class.SizeUnused > 0) {
			Value->AddSlot().AutoWidth()[SNew(STextBlock)
											 .Text(FText::FromString(FString::Printf(TEXT(" · %s unused"), *PjcFormat::Size(Class.SizeUnused))))
											 .Font(PjcWidgets::GetFontMono(9))
											 .ColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.Red"))];
		}

		const FString Text = FString::Printf(TEXT("%s assets, %s"), *PjcFormat::Count(Class.Total.Num), *PjcFormat::Size(Class.Total.Size));
		const FString Lines = FString::Printf(
			TEXT("Used: %s\nEditor only: %s\nUnused: %s"),
			*PjcFormat::Size(Class.SizeUsed),
			*PjcFormat::Size(Class.SizeEditorOnly),
			*PjcFormat::Size(Class.SizeUnused)
		);
		Rows->AddSlot().AutoHeight()[MakeRow(
			SNew(STextBlock).Text(FText::FromString(GetClassShortName(Class.Name))),
			Value,
			Segments,
			static_cast<double>(MaxSize),
			PjcWidgets::MakeToolTip(Class.Name.ToString(), Text, Lines)
		)];
	}

	return MakeSection(TEXT("Classes"), FString::Printf(TEXT("top %d by size"), MaxRows), Rows);
}

TSharedRef<SWidget> SPjcTabOverview::MakeUnusedFolders() {
	constexpr int32 MaxRows = 6;
	const TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	const int64 MaxSize = Overview.UnusedFolders.Num() > 0 ? Overview.UnusedFolders[0].Unused.Size : 0;
	const FLinearColor ColorUnused = PjcWidgets::GetColor("ProjectCleaner.Color.Red");

	for (int32 Index = 0; Index < Overview.UnusedFolders.Num() && Index < MaxRows; ++Index) {
		const FPjcOverview::FFolder& Folder = Overview.UnusedFolders[Index];
		const bool bAllUnused = Folder.Unused.Num == Folder.All.Num;

		// name, and the parent path without its leading slash
		int32 Slash = INDEX_NONE;
		Folder.Path.FindLastChar(TEXT('/'), Slash);
		const FString Name = Folder.Path.Mid(Slash + 1);
		const FString Parent = Slash > 0 ? Folder.Path.Mid(1, Slash - 1) : FString {};

		// clang-format off
		const TSharedRef<SWidget> NameWidget = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(Name)).Font(FPjcStyles::GetFont("Bold", 9))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(FMargin {7.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock)
				.Text(FText::FromString(Parent))
				.Font(FPjcStyles::GetFont("Regular", 8))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			];

		const TSharedRef<SWidget> Value = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(STextBlock)
				.Text(FText::FromString(PjcFormat::Size(Folder.Unused.Size) + TEXT(" · ")))
				.Font(PjcWidgets::GetFontMono(9))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(STextBlock)
				.Text(FText::FromString(PjcFormat::Percent(Folder.Unused.Size, Folder.All.Size)))
				.Font(PjcWidgets::GetFontMono(9))
				.ColorAndOpacity(bAllUnused ? FSlateColor {ColorUnused} : FSlateColor::UseSubduedForeground())
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT(" · ") + PjcFormat::Count(Folder.Unused.Num)))
				.Font(PjcWidgets::GetFontMono(9))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin {8.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Review ›"))).ColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.Accent"))
			];
		// clang-format on

		PjcWidgets::FBarSegment Segment;
		Segment.Value = static_cast<double>(Folder.Unused.Size);
		Segment.Color = PjcWidgets::GetColor("ProjectCleaner.Color.UnusedFill");

		const FString Text = bAllUnused
			? FString::Printf(TEXT("All %s assets unused"), *PjcFormat::Count(Folder.All.Num))
			: FString::Printf(TEXT("%s of %s assets unused"), *PjcFormat::Count(Folder.Unused.Num), *PjcFormat::Count(Folder.All.Num));
		const FString Sizes
			= FString::Printf(TEXT("%s of %s\nOpens Assets Unused"), *PjcFormat::Size(Folder.Unused.Size), *PjcFormat::Size(Folder.All.Size));
		Rows->AddSlot().AutoHeight()[MakeRow(
			NameWidget,
			Value,
			{Segment},
			static_cast<double>(MaxSize),
			PjcWidgets::MakeToolTip(Folder.Path, Text, Sizes),
			FSimpleDelegate::CreateSP(this, &SPjcTabOverview::ShowFolder, Folder.Path)
		)];
	}

	if (Overview.UnusedFolders.Num() == 0) {
		Rows->AddSlot().AutoHeight().Padding(
			FMargin {6.0f, 4.0f}
		)[SNew(STextBlock).Text(FText::FromString(TEXT("No folder is mostly unused."))).ColorAndOpacity(FSlateColor::UseSubduedForeground())];
	}

	return MakeSection(TEXT("Unused by folder"), TEXT("50% or more unused"), Rows);
}

TSharedRef<SWidget> SPjcTabOverview::MakeLargestUnused() {
	constexpr int32 MaxRows = 6;
	const TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	const int64 MaxSize = Overview.LargestUnused.Num() > 0 ? Overview.LargestUnused[0].Size : 0;

	for (int32 Index = 0; Index < Overview.LargestUnused.Num() && Index < MaxRows; ++Index) {
		const FPjcOverview::FAsset& Asset = Overview.LargestUnused[Index];
		const FString Name = FPackageName::GetShortName(Asset.Package);
		const FString ClassName = GetClassShortName(Asset.Class);

		// clang-format off
		const TSharedRef<SWidget> NameWidget = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(Name))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(FMargin {7.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock)
				.Text(FText::FromString(ClassName))
				.Font(FPjcStyles::GetFont("Regular", 8))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			];

		const TSharedRef<SWidget> Value = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(STextBlock)
				.Text(FText::FromString(PjcFormat::Size(Asset.Size)))
				.Font(PjcWidgets::GetFontMono(9))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin {8.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Inspect ›"))).ColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.Accent"))
			];
		// clang-format on

		PjcWidgets::FBarSegment Segment;
		Segment.Value = static_cast<double>(Asset.Size);
		Segment.Color = PjcWidgets::GetColor("ProjectCleaner.Color.UnusedFill");

		const FString More = FString::Printf(TEXT("%s, %s\nOpens it in Asset Usage"), *ClassName, *PjcFormat::Size(Asset.Size));
		Rows->AddSlot().AutoHeight()[MakeRow(
			NameWidget,
			Value,
			{Segment},
			static_cast<double>(MaxSize),
			PjcWidgets::MakeToolTip(Name, Asset.Package.ToString(), More),
			FSimpleDelegate::CreateSP(this, &SPjcTabOverview::Inspect, Asset.Package)
		)];
	}

	if (Overview.LargestUnused.Num() == 0) {
		Rows->AddSlot().AutoHeight().Padding(
			FMargin {6.0f, 4.0f}
		)[SNew(STextBlock).Text(FText::FromString(TEXT("Nothing unused."))).ColorAndOpacity(FSlateColor::UseSubduedForeground())];
	}

	return MakeSection(TEXT("Largest unused"), FString::Printf(TEXT("top %d by size"), MaxRows), Rows);
}

TSharedRef<SWidget> SPjcTabOverview::MakeShippedCard() const {
	const FSlateColor Muted = FSlateColor::UseSubduedForeground();
	const auto Text = [&Muted](const FString& String, const bool bMuted = true) {
		return SNew(STextBlock).Text(FText::FromString(String)).ColorAndOpacity(bMuted ? Muted : FSlateColor::UseForeground());
	};
	const auto Card = [](const TSharedRef<SWidget>& Status, const TSharedRef<SWidget>& Content) {
		// clang-format off
		return PjcWidgets::MakeCard(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 10.0f})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					PjcWidgets::MakeCardTitle(TEXT("Shipped"))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					Status
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				Content
			],
			FMargin {14.0f, 12.0f}
		);
		// clang-format on
	};

	const FString Where = TEXT("The card reads the last cook of this project folder: Saved/Cooked/[Platform]/[Project]/Metadata. ")
		TEXT("Cooks made on another machine are not seen.");
	if (!Cook.IsFound()) {
#if ENGINE_MAJOR_VERSION >= 5
		const FString HowTo = FString::Printf(TEXT("Platforms › %s › Cook Content"), ANSI_TO_TCHAR(FPlatformProperties::IniPlatformName()));
#else
		const FString HowTo = FString::Printf(TEXT("File › Cook Content for %s"), ANSI_TO_TCHAR(FPlatformProperties::IniPlatformName()));
#endif
		const FString Message = FString::Printf(TEXT("No cook found for this project. Cook it for a platform (%s) to see what ships."), *HowTo);
		return Card(SNullWidget::NullWidget, SNew(SBox).ToolTip(PjcWidgets::MakeToolTip(TEXT("No cook"), Where))[Text(Message)]);
	}
	if (!Shipped.IsSet()) {
		const FString Message
			= FString::Printf(TEXT("The last cook (%s) could not be read. It may come from another engine version."), *Cook.Platform);
		return Card(SNullWidget::NullWidget, SNew(SBox).ToolTip(PjcWidgets::MakeToolTip(TEXT("Last cook"), Cook.File, Where))[Text(Message)]);
	}

	const FPjcOverview::FShipped& Ship = Shipped.GetValue();

	// cook time and whether assets changed since
	const FTimespan Age = FDateTime::UtcNow() - Cook.Time;
	const FString Ago = Age.GetTotalMinutes() < 1.0 ? TEXT("just now")
		: Age.GetTotalHours() < 1.0					? FString::Printf(TEXT("%d min ago"), FMath::FloorToInt(Age.GetTotalMinutes()))
		: Age.GetTotalDays() < 1.0					? FString::Printf(TEXT("%d h ago"), FMath::FloorToInt(Age.GetTotalHours()))
													: FString::Printf(TEXT("%d days ago"), FMath::FloorToInt(Age.GetTotalDays()));
	const FDateTime LocalTime = Cook.Time + (FDateTime::Now() - FDateTime::UtcNow());
	const FString When = TEXT("Last cook: ") + LocalTime.ToString(TEXT("%Y-%m-%d %H:%M"));
	const bool bUpToDate = Ship.NumChanged == 0 && Ship.NumDeleted == 0;
	TArray<FString> Since;
	if (Ship.NumChanged > 0) Since.Add(FString::Printf(TEXT("%s changed"), *PjcFormat::Count(Ship.NumChanged)));
	if (Ship.NumDeleted > 0) Since.Add(FString::Printf(TEXT("%s deleted"), *PjcFormat::Count(Ship.NumDeleted)));
	const FString Fresh = bUpToDate ? TEXT("up to date") : FString::Join(Since, TEXT(", ")) + TEXT(" since");
	const FString FreshTip = bUpToDate
		? TEXT("No scanned asset was saved, added or deleted since the cook.")
		: TEXT("Assets were saved, added or deleted after the cook, so these numbers can be out of date. Cook again for current numbers.");

	// clang-format off
	const TSharedRef<SWidget> Status = SNew(SHorizontalBox)
		.ToolTip(PjcWidgets::MakeToolTip(When, FreshTip, Cook.File))
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(STextBlock).Text(FText::FromString(Cook.Platform)).Font(FPjcStyles::GetFont("Bold", 9))
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			Text(TEXT(" · cooked ") + Ago + TEXT(" · "))
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(STextBlock)
			.Text(FText::FromString(Fresh))
			.ColorAndOpacity(PjcWidgets::GetColor(bUpToDate ? "ProjectCleaner.Color.Ok" : "ProjectCleaner.Color.Yellow"))
		];
	// clang-format on

	// bar by editor size
	const int64 AllSize = Ship.Shipped.Size + Ship.UnusedShipped.Size + Ship.UsedNotShipped.Size + Ship.Unused.Size;
	TArray<PjcWidgets::FBarSegment> Segments;
	const auto AddSegment = [&Segments, AllSize](const FString& Label, const FPjcOverview::FTotal& Total, const FLinearColor& Color) {
		PjcWidgets::FBarSegment& Segment = Segments.AddDefaulted_GetRef();
		Segment.Value = static_cast<double>(Total.Size);
		Segment.Color = Color;
		Segment.ToolTip = PjcWidgets::MakeToolTip(
			Label,
			FString::Printf(
				TEXT("%s assets · %s in the editor (%s)"),
				*PjcFormat::Count(Total.Num),
				*PjcFormat::Size(Total.Size),
				*PjcFormat::Percent(Total.Size, AllSize)
			)
		);
	};
	AddSegment(TEXT("Shipped"), Ship.Shipped, PjcWidgets::GetColor("ProjectCleaner.Color.ShipFill"));
	AddSegment(TEXT("Unused, shipped"), Ship.UnusedShipped, PjcWidgets::GetColor("ProjectCleaner.Color.Yellow"));
	AddSegment(TEXT("Used, not shipped"), Ship.UsedNotShipped, PjcWidgets::GetColor("ProjectCleaner.Color.KeepFill"));
	AddSegment(TEXT("Unused"), Ship.Unused, PjcWidgets::GetColor("ProjectCleaner.Color.UnusedFill"));

	// one column per group
	const auto Column = [&Muted](
							const FString& Label,
							const TOptional<FLinearColor>& Swatch,
							const FString& Number,
							const FSlateColor& NumberColor,
							const FString& Sub,
							const FString& ToolTip
						) {
		const TSharedRef<SHorizontalBox> Head = SNew(SHorizontalBox);
		if (Swatch.IsSet()) {
			Head->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 6.0f, 0.0f})[PjcWidgets::MakeSwatch(Swatch.GetValue())];
		}
		Head->AddSlot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(Label)).ColorAndOpacity(Muted)];

		// clang-format off
		return SNew(SBox)
			.ToolTip(PjcWidgets::MakeToolTip(Label, ToolTip))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					Head
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 1.0f})
				[
					SNew(STextBlock).Text(FText::FromString(Number)).Font(PjcWidgets::GetFontMono(13)).ColorAndOpacity(NumberColor)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(FText::FromString(Sub)).ColorAndOpacity(Muted)
				]
			];
		// clang-format on
	};

	TArray<FString> ReasonLines;
	for (const auto& Row : Ship.UsedNotShippedReasons) {
		FString Name;
		FString ReasonTip;
		FLinearColor Color;
		GetReasonInfo(Row.Reason, Name, ReasonTip, Color);
		ReasonLines.Add(FString::Printf(TEXT("%s: %s"), *Name, *PjcFormat::Count(Row.Total.Num)));
	}
	const auto Share = [AllSize](const FPjcOverview::FTotal& Total) {
		return FString::Printf(TEXT("%s · %s"), *PjcFormat::Size(Total.Size), *PjcFormat::Percent(Total.Size, AllSize));
	};
	const int32 NumShipped = Ship.Shipped.Num + Ship.UnusedShipped.Num;
	const int64 SizeShippedCooked = Ship.ShippedCooked + Ship.UnusedShippedCooked;
	const int64 SizeShippedEditor = Ship.Shipped.Size + Ship.UnusedShipped.Size;

	const TSharedRef<SHorizontalBox> Columns = SNew(SHorizontalBox);
	const auto AddColumn = [&Columns](const TSharedRef<SWidget>& Widget) {
		Columns->AddSlot().FillWidth(1.0f).Padding(FMargin {0.0f, 0.0f, 10.0f, 0.0f})[Widget];
	};
	AddColumn(Column(
		TEXT("Shipped"),
		PjcWidgets::GetColor("ProjectCleaner.Color.ShipFill"),
		PjcFormat::Count(NumShipped),
		FSlateColor::UseForeground(),
		PjcFormat::Size(SizeShippedCooked) + TEXT(" cooked"),
		FString::Printf(
			TEXT("Packaged in the game for %s: %s cooked, %s in the editor. Cooked size is before pak compression."),
			*Cook.Platform,
			*PjcFormat::Size(SizeShippedCooked),
			*PjcFormat::Size(SizeShippedEditor)
		)
	));
	AddColumn(Column(
		TEXT("Used, not shipped"),
		PjcWidgets::GetColor("ProjectCleaner.Color.KeepFill"),
		PjcFormat::Count(Ship.UsedNotShipped.Num),
		FSlateColor::UseForeground(),
		Share(Ship.UsedNotShipped),
		TEXT("Used in the editor, or only by content that is not cooked: editor-only references, maps and tools left out of the cook. ")
				TEXT("By chain start:\n")
			+ FString::Join(ReasonLines, TEXT("\n"))
	));
	AddColumn(Column(
		TEXT("Unused"),
		PjcWidgets::GetColor("ProjectCleaner.Color.UnusedFill"),
		PjcFormat::Count(Ship.Unused.Num),
		FSlateColor::UseForeground(),
		Share(Ship.Unused),
		TEXT("Not shipped and not used. Cleanup deletes them.")
	));
	if (Ship.UnusedShipped.Num > 0) {
		AddColumn(Column(
			TEXT("Unused, shipped"),
			{},
			PjcFormat::Count(Ship.UnusedShipped.Num),
			PjcWidgets::GetColor("ProjectCleaner.Color.Yellow"),
			PjcFormat::Size(Ship.UnusedShippedCooked) + TEXT(" cooked"),
			TEXT("The cook packages them but the scan marks them unused. Check them before deleting.")
		));
	}
	else {
		AddColumn(Column(
			TEXT("Unused, shipped"),
			{},
			TEXT("✓ 0"),
			PjcWidgets::GetColor("ProjectCleaner.Color.Ok"),
			TEXT("scan and cook agree"),
			TEXT("No asset the scan marks unused is in the cook.")
		));
	}

	// what else the cook packaged
	TArray<FString> RootLines;
	for (int32 Index = 0; Index < Ship.OutsideRoots.Num() && Index < 8; ++Index) {
		RootLines.Add(FString::Printf(TEXT("%s: %s"), *Ship.OutsideRoots[Index].Key, *PjcFormat::Count(Ship.OutsideRoots[Index].Value)));
	}
	const TSharedRef<SHorizontalBox> Footer = SNew(SHorizontalBox);
	Footer->AddSlot()
		.AutoWidth()[SNew(SBox).ToolTip(PjcWidgets::MakeToolTip(TEXT("Shipped outside the scan"), FString::Join(RootLines, TEXT("\n"))))[Text(
			FString::Printf(
				TEXT("Also shipped: %s engine and plugin packages, %s cooked"), *PjcFormat::Count(Ship.NumOutside), *PjcFormat::Size(Ship.SizeOutside)
			)
		)]];
	if (Ship.NumGenerated > 0) {
		Footer->AddSlot().AutoWidth().Padding(FMargin {18.0f, 0.0f, 0.0f, 0.0f})[SNew(SBox).ToolTip(
			PjcWidgets::MakeToolTip(TEXT("Generated packages"), TEXT("World Partition cells and other packages the cook generates."))
		)[Text(FString::Printf(TEXT("%s generated packages"), *PjcFormat::Count(Ship.NumGenerated)))]];
	}

	// clang-format off
	return Card(
		Status,
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			PjcWidgets::MakeBar(Segments, 8.0f)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 10.0f})
		[
			Columns
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox).HeightOverride(1.0f)
			[
				SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.Line"))
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 8.0f, 0.0f, 0.0f})
		[
			Footer
		]
	);
	// clang-format on
}

FString SPjcTabOverview::GetClassShortName(const FName ClassName) {
	const FString Path = ClassName.ToString();
	int32 Dot = INDEX_NONE;
	return Path.FindLastChar(TEXT('.'), Dot) ? Path.Mid(Dot + 1) : Path;
}

TSharedRef<SWidget> SPjcTabOverview::MakeSection(const FString& Title, const FString& Hint, const TSharedRef<SWidget>& Content) {
	// clang-format off
	return PjcWidgets::MakeCard(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {6.0f, 0.0f, 6.0f, 8.0f})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				PjcWidgets::MakeCardTitle(Title)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(Hint)).Font(FPjcStyles::GetFont("Regular", 8)).ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			Content
		],
		FMargin {8.0f, 12.0f, 8.0f, 10.0f}
	);
	// clang-format on
}

TSharedRef<SWidget> SPjcTabOverview::MakeColumns(const TSharedRef<SWidget>& Left, const TSharedRef<SWidget>& Right) {
	// clang-format off
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.0f)
		[
			Left
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SBox).WidthOverride(14.0f)
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f)
		[
			Right
		];
	// clang-format on
}

TSharedRef<SWidget> SPjcTabOverview::MakeRow(
	const TSharedRef<SWidget>& Name,
	const TSharedRef<SWidget>& Value,
	const TArray<PjcWidgets::FBarSegment>& Segments,
	const double BarTotal,
	const TSharedPtr<SToolTip>& ToolTip,
	const FSimpleDelegate& OnClick
) {
	// clang-format off
	const TSharedRef<SWidget> Content = SNew(SBox)
		.Padding(FMargin {6.0f, 5.0f})
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					Name
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {12.0f, 0.0f, 0.0f, 0.0f})
				[
					Value
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 4.0f, 0.0f, 0.0f})
			[
				PjcWidgets::MakeBar(Segments, 5.0f, BarTotal)
			]
		];

	if (!OnClick.IsBound()) {
		return SNew(SBox).ToolTip(ToolTip)[Content];
	}

	return SNew(SButton)
		.ButtonStyle(&FPjcStyles::Get().GetWidgetStyle<FButtonStyle>("ProjectCleaner.Button.Row"))
		.ContentPadding(FMargin {0.0f})
		.ToolTip(ToolTip)
		.OnClicked_Lambda([OnClick]() {
			OnClick.ExecuteIfBound();
			return FReply::Handled();
		})
		[
			Content
		];
	// clang-format on
}

void SPjcTabOverview::GetReasonInfo(const EPjcUsageReason Reason, FString& OutName, FString& OutToolTip, FLinearColor& OutColor) {
	const TCHAR* Color = TEXT("#6FA8A2");
	switch (Reason) {
		case EPjcUsageReason::None :
			OutName = TEXT("Unreferenced");
			OutToolTip = TEXT("Nothing used references them. Cleanup deletes them.");
			Color = TEXT("#FE6D73");
			break;
		case EPjcUsageReason::Primary :
			OutName = TEXT("Primary assets");
			OutToolTip = TEXT("Maps, and the assets the Asset Manager registers or Primary Asset Labels list.");
			Color = TEXT("#4F9DF0");
			break;
		case EPjcUsageReason::PrimaryClass :
			OutName = TEXT("Unregistered primary classes");
			OutToolTip = TEXT("Assets of a primary asset class the Asset Manager did not register.");
			Color = TEXT("#7A9FD0");
			break;
		case EPjcUsageReason::EngineReferenced :
			OutName = TEXT("External references");
			OutToolTip = TEXT("Referenced by engine content, plugins left out of the scan, or level actors saved in their own files.");
			Color = TEXT("#9B8CF0");
			break;
		case EPjcUsageReason::Indirect :
			OutName = TEXT("Source & config");
			OutToolTip = TEXT("C++, Python and .ini files name them by path.");
			Color = TEXT("#E0A85A");
			break;
		case EPjcUsageReason::ConfigFolder :
			OutName = TEXT("Config folders");
			OutToolTip = TEXT("Inside folders set to always cook or scanned for Data Registries.");
			Color = TEXT("#C9A04A");
			break;
		case EPjcUsageReason::Localized :
			OutName = TEXT("Localized assets");
			OutToolTip = TEXT("Language copies of used assets in L10N folders.");
			Color = TEXT("#5FC4A8");
			break;
		case EPjcUsageReason::Editor :
			OutName = TEXT("Editor assets");
			OutToolTip = TEXT("Editor utilities, validators, scriptable tools and editor tool data.");
			Color = TEXT("#7FB2FF");
			break;
		case EPjcUsageReason::Excluded :
			OutName = TEXT("Excluded");
			OutToolTip = TEXT("Excluded in settings, always used classes and cinematics.");
			Color = TEXT("#8A9099");
			break;
		case EPjcUsageReason::GameplayCue :
			OutName = TEXT("GameplayCue notifies");
			OutToolTip = TEXT("Loaded by their tag by the GameplayCue manager.");
			Color = TEXT("#D679C8");
			break;
		case EPjcUsageReason::FolderLabel :
			OutName = TEXT("Folder labels");
			OutToolTip = TEXT("In the folder of a Primary Asset Label with Label Assets In My Directory.");
			Color = TEXT("#FFCB77");
			break;
		case EPjcUsageReason::Megascans :
			OutName = TEXT("Megascans presets");
			OutToolTip = TEXT("Presets the Megascans plugin uses.");
			break;
		default :
			OutName = TEXT("Other");
			OutToolTip = FString {};
			break;
	}
	OutColor = FLinearColor {FColor::FromHex(Color)};
}
