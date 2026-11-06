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
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 14.0f, 0.0f, 0.0f})
		[
			MakeColumns(MakeReasons(), MakeClasses())
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
		// UE5 class names are paths (/Script/Engine.Texture2D): the name is after the dot
		const FString ClassPath = Class.Name.ToString();
		int32 Dot = INDEX_NONE;
		const FString ClassName = ClassPath.FindLastChar(TEXT('.'), Dot) ? ClassPath.Mid(Dot + 1) : ClassPath;
		Rows->AddSlot().AutoHeight()[MakeRow(
			SNew(STextBlock).Text(FText::FromString(ClassName)),
			Value,
			Segments,
			static_cast<double>(MaxSize),
			PjcWidgets::MakeToolTip(ClassPath, Text, Lines)
		)];
	}

	return MakeSection(TEXT("Classes"), FString::Printf(TEXT("top %d by size"), MaxRows), Rows);
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
