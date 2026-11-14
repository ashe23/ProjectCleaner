// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcOverviewPanel.h"
#include "Slate/PjcTabs.h"
#include "Slate/PjcWidgets.h"
#include "Slate/SPjcTabIssues.h"
#include "PjcCookInfo.h"
#include "PjcFormat.h"
#include "PjcShim.h"
#include "PjcStyles.h"
#include "PjcSubsystem.h"
#include "PjcUnused.h"
#include "Runtime/Launch/Resources/Version.h"
// Engine Headers
#include "Framework/Docking/TabManager.h"
#include "HAL/PlatformProperties.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	// unity builds join files: names unique to this file
	const TCHAR* PjcOverviewReasonEditor = TEXT("Editor assets");
	const TCHAR* PjcOverviewReasonCooked = TEXT("Packaged by last cook");

	FLinearColor PjcOverviewMarked() {
		return FLinearColor {FColor::FromHex(TEXT("#1D2A3A"))};
	}
}	// namespace

void SPjcOverviewPanel::Construct(const FArguments& InArgs) {
	TabManager = InArgs._TabManager;
	OnShowSet = InArgs._OnShowSet;
	IsSetShown = InArgs._IsSetShown;
	OnShowDeleteFailed = InArgs._OnShowDeleteFailed;
	ScanFinishedHandle = UPjcSubsystem::OnScanFinished().AddSP(this, &SPjcOverviewPanel::OnScanFinished);

	// clang-format off
	ChildSlot
	[
		SAssignNew(Body, SBox)
	];
	// clang-format on

	OnScanFinished();
}

SPjcOverviewPanel::~SPjcOverviewPanel() {
	UPjcSubsystem::OnScanFinished().Remove(ScanFinishedHandle);
}

void SPjcOverviewPanel::OnScanFinished() {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	Overview = FPjcOverview::Make(Result);

	// what each platform's last cook shipped; only the numbers are kept, not the package lists
	Cooks.Reset();
	if (Result.IsValid()) {
		for (const FPjcCookInfo* Info : FPjcCookInfo::GetAll()) {
			FCook& Cook = Cooks.AddDefaulted_GetRef();
			Cook.Platform = Info->Platform;
			Cook.File = Info->File;
			Cook.Time = Info->Time;
			if (Info->IsValid()) {
				Cook.Shipped = FPjcOverview::MakeShipped(Result, *Info);
			}
		}
	}
	SelectedCook = FMath::Clamp(SelectedCook, 0, FMath::Max(0, Cooks.Num() - 1));

	Rebuild();
}

void SPjcOverviewPanel::Rebuild() {
	if (!UPjcSubsystem::GetScanResult().IsValid()) {
		Body->SetContent(SNullWidget::NullWidget);
		return;
	}

	const TSharedRef<SVerticalBox> Cards = SNew(SVerticalBox);
	for (const TSharedRef<SWidget>& Card : {MakeStatus(), MakeIssues(), MakeShipped()}) {
		if (Card == SNullWidget::NullWidget) continue;

		// clang-format off
		Cards->AddSlot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 10.0f})
		[
			Card
		];
		// clang-format on
	}
	Body->SetContent(Cards);
}

TSharedRef<SWidget> SPjcOverviewPanel::MakeStatus() {
	const FPjcOverview::FTotal& All = Overview.All;
	const FLinearColor ColorUsed = PjcWidgets::GetColor("ProjectCleaner.Color.UsedFill");
	const FLinearColor ColorEditor = PjcWidgets::GetColor("ProjectCleaner.Color.EditorFill");
	const FLinearColor ColorReview = PjcWidgets::GetColor("ProjectCleaner.Color.ReviewFill");
	const FLinearColor ColorUnused = PjcWidgets::GetColor("ProjectCleaner.Color.UnusedFill");
	const FSlateColor Plain = FSlateColor::UseForeground();
	const FSlateColor Muted = FSlateColor::UseSubduedForeground();

	// clang-format off
	const TCHAR* UsedTip = TEXT("The game uses them: maps, primary assets and other roots, and what they reference.");
	const TCHAR* EditorTip = TEXT("Kept, but only the editor uses them: editor assets, and what only editor-only references or labels reach. ")
		TEXT("They do not ship.");
	const TCHAR* ReviewTip = TEXT("The scan is not sure about them: unused assets a cook packaged, GameplayCue notifies ")
		TEXT("nothing fires. Clean project keeps them, and the unused assets they use.");
	const TCHAR* UnusedTip = TEXT("Nothing uses them. Clean project deletes them.");
	// clang-format on
	const FString ClickTip = TEXT("Click: list them on the right");

	const TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	const auto AddRow = [this, &Rows, &All, &ClickTip](
							const FString& Name,
							const FLinearColor& Swatch,
							const FPjcOverview::FTotal& Total,
							const FSlateColor& NumberColor,
							const TCHAR* ToolTip,
							const EPjcAssetSet Set,
							const FString& Reason
						) {
		const FString Size = Total.Num > 0 ? PjcFormat::Size(Total.Size) : FString {};
		const FString Share = Total.Num > 0 ? PjcFormat::Percent(Total.Size, All.Size) : FString {};
		const FSimpleDelegate OnClick = FSimpleDelegate::CreateLambda([this, Set, Reason]() {
			OnShowSet.ExecuteIfBound(Set, Reason);
		});
		const TAttribute<bool> IsOn = TAttribute<bool>::Create([this, Set, Reason]() {
			return IsSetShown.IsBound() && IsSetShown.Execute(Set, Reason);
		});
		const TSharedRef<SToolTip> Tip = PjcWidgets::MakeToolTip(Name, ToolTip, ClickTip);
		// clang-format off
		Rows->AddSlot().AutoHeight()
		[
			MakeRow(Swatch, Name, PjcFormat::Count(Total.Num), NumberColor, Size, Share, Tip, OnClick, IsOn)
		];
		// clang-format on
	};
	const FSlateColor ReviewNumber = Overview.Review.Num > 0 ? FSlateColor {PjcWidgets::GetColor("ProjectCleaner.Color.Review")} : Muted;
	const FSlateColor UnusedNumber = Overview.Unused.Num > 0 ? FSlateColor {PjcWidgets::GetColor("ProjectCleaner.Color.Red")} : Muted;
	AddRow(TEXT("Used"), ColorUsed, Overview.Used, Plain, UsedTip, EPjcAssetSet::Used, FString {});
	AddRow(TEXT("Editor only"), ColorEditor, Overview.EditorOnly, Plain, EditorTip, EPjcAssetSet::Used, PjcOverviewReasonEditor);
	AddRow(TEXT("Review"), ColorReview, Overview.Review, ReviewNumber, ReviewTip, EPjcAssetSet::Review, FString {});
	AddRow(TEXT("Unused"), ColorUnused, Overview.Unused, UnusedNumber, UnusedTip, EPjcAssetSet::Unused, FString {});

	// storage by size
	TArray<PjcWidgets::FBarSegment> Segments;
	const auto AddSegment = [&Segments, &All](const FString& Label, const FPjcOverview::FTotal& Total, const FLinearColor& Color) {
		PjcWidgets::FBarSegment& Segment = Segments.AddDefaulted_GetRef();
		Segment.Value = static_cast<double>(Total.Size);
		Segment.Color = Color;
		const FString Sizes = FString::Printf(TEXT("%s · %s"), *PjcFormat::Size(Total.Size), *PjcFormat::Percent(Total.Size, All.Size));
		Segment.ToolTip = PjcWidgets::MakeToolTip(Label, Sizes + TEXT(" · ") + PjcFormat::Count(Total.Num) + TEXT(" assets"));
	};
	AddSegment(TEXT("Used"), Overview.Used, ColorUsed);
	AddSegment(TEXT("Editor only"), Overview.EditorOnly, ColorEditor);
	AddSegment(TEXT("Review"), Overview.Review, ColorReview);
	AddSegment(TEXT("Unused"), Overview.Unused, ColorUnused);

	const FString Totals = FString::Printf(TEXT("%s · %s"), *PjcFormat::Count(All.Num), *PjcFormat::Size(All.Size));
	// clang-format off
	return MakeCard(
		TEXT("Assets"),
		SNew(STextBlock)
		.Text(FText::FromString(Totals))
		.Font(PjcWidgets::GetFontMono(9))
		.ToolTip(PjcWidgets::MakeToolTip(TEXT("Assets"), TEXT("Every scanned asset and their size on disk. Redirectors are not counted."))),
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 6.0f})
		[
			PjcWidgets::MakeBar(Segments, 6.0f)
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			Rows
		]
	);
	// clang-format on
}

TSharedRef<SWidget> SPjcOverviewPanel::MakeIssues() {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();

	struct FIssue
	{
		FString Title;
		int32 Num;
		// what to do with them, after the number
		FString What;
		FString ToolTip;
		// Issues category, -1 for the assets the last delete could not remove
		int32 Category;
	};
	const FString ExternalExample = Result.FilesExternal.Num() > 0 ? TEXT(", e.g. ") + FPaths::GetCleanFilename(Result.FilesExternal[0]) : FString {};
	const FString KeptText = PjcFormat::Count(Result.FilesExternalKept.Num());
	const FString ExternalKept = FString::Printf(TEXT(" %s more are kept: used by the project, or excluded in settings."), *KeptText);
	const FString ExternalTip = TEXT("Files in content folders the engine does not use") + ExternalExample + TEXT(".") + ExternalKept;
	const FString ExternalWhat = TEXT("to review · ") + KeptText + TEXT(" kept");
	// clang-format off
	const TCHAR* RedirectorsTip = TEXT("Left by renamed or moved assets. ")
		TEXT("Scanning follows them, fixing resaves the assets that still use the old path.");
	const TCHAR* CorruptedTip = TEXT("Asset files the engine cannot read.");
	const TCHAR* EmptyTip = TEXT("Folders without assets or files.");
	const TCHAR* DeleteFailedTip = TEXT("Unused assets the last cleanup could not delete: a live object or a saved asset still holds them.");
	const auto ToIndex = [](const EPjcIssueCategory Value) { return static_cast<int32>(Value); };
	const TArray<FIssue> Issues {
		{TEXT("Redirectors"), Result.Redirectors.Num(), TEXT("to fix"), RedirectorsTip, ToIndex(EPjcIssueCategory::Redirectors)},
		{TEXT("External files"), Result.FilesExternal.Num(), ExternalWhat, ExternalTip, ToIndex(EPjcIssueCategory::External)},
		{TEXT("Corrupted files"), Result.FilesCorrupted.Num(), TEXT("to delete"), CorruptedTip, ToIndex(EPjcIssueCategory::Corrupted)},
		{TEXT("Empty folders"), Result.FoldersEmpty.Num(), TEXT("to delete"), EmptyTip, ToIndex(EPjcIssueCategory::Empty)},
		{TEXT("Delete failed"), UPjcSubsystem::GetLastCleanupKept().Num(), TEXT("held in memory"), DeleteFailedTip, -1},
	};
	// clang-format on

	// only what was found is listed, nothing found: no card
	const FSlateColor Warn {PjcWidgets::GetColor("ProjectCleaner.Color.Yellow")};
	const TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	int32 NumFound = 0;
	for (const auto& Issue : Issues) {
		if (Issue.Num == 0) continue;

		NumFound += Issue.Num;
		const int32 Category = Issue.Category;
		const FSimpleDelegate OnClick = FSimpleDelegate::CreateLambda([this, Category]() {
			if (Category < 0) {
				OnShowDeleteFailed.ExecuteIfBound();
			}
			else {
				PjcTabs::ShowIssues(TabManager, static_cast<uint8>(Category));
			}
		});
		const FString ClickTip = Category < 0 ? TEXT("Click: list them on the right") : TEXT("Click: open them in Issues");
		const TSharedRef<SToolTip> Tip = PjcWidgets::MakeToolTip(Issue.Title, Issue.ToolTip, ClickTip);
		// clang-format off
		Rows->AddSlot().AutoHeight()
		[
			MakeRow({}, Issue.Title, PjcFormat::Count(Issue.Num), Warn, Issue.What, TEXT("›"), Tip, OnClick, false, 0.0f)
		];
		// clang-format on
	}
	if (NumFound == 0) return SNullWidget::NullWidget;

	// clang-format off
	return MakeCard(
		TEXT("Issues"),
		SNew(STextBlock)
		.Text(FText::FromString(PjcFormat::Count(NumFound) + TEXT(" to look at")))
		.Font(PjcWidgets::GetFontMono(9)),
		Rows,
		FPjcStyles::Get().GetBrush("ProjectCleaner.Icon.Warning32"),
		PjcWidgets::GetColor("ProjectCleaner.Color.Yellow")
	);
	// clang-format on
}

TSharedRef<SWidget> SPjcOverviewPanel::MakeShipped() {
	const FString Where = TEXT("Project Cleaner reads the last cook of each platform: Saved/Cooked/[Platform]/[Project]/Metadata. ")
		TEXT("For a cook made on a build machine, set its Metadata folder in Project Settings > Plugins > Project Cleaner > Cook Check.");
	if (Cooks.Num() == 0) {
#if ENGINE_MAJOR_VERSION >= 5
		const FString HowTo = FString::Printf(TEXT("Platforms › %s › Cook Content"), ANSI_TO_TCHAR(FPlatformProperties::IniPlatformName()));
#else
		const FString HowTo = FString::Printf(TEXT("File › Cook Content for %s"), ANSI_TO_TCHAR(FPlatformProperties::IniPlatformName()));
#endif
		const FString Optional = TEXT("Optional. Shows what a cook packages, for projects that ship a game: cook the project (") + HowTo + TEXT(").");
		// clang-format off
		return MakeCard(
			TEXT("Shipped"),
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("No cook found · optional")))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			.ToolTip(PjcWidgets::MakeToolTip(TEXT("No cook found"), Optional, Where)),
			SNullWidget::NullWidget
		);
		// clang-format on
	}

	const TSharedRef<SVerticalBox> Platforms = SNew(SVerticalBox);
	for (int32 Index = 0; Index < Cooks.Num(); ++Index) {
		// clang-format off
		Platforms->AddSlot().AutoHeight()
		[
			MakeCookRow(Index)
		];
		// clang-format on
	}
	const FString NumPlatforms = Cooks.Num() > 1 ? PjcFormat::Count(Cooks.Num()) + TEXT(" platforms") : FString {};

	// clang-format off
	return MakeCard(
		TEXT("Shipped"),
		SNew(STextBlock).Text(FText::FromString(NumPlatforms)).Font(PjcWidgets::GetFontMono(9)),
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			Platforms
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 4.0f, 0.0f, 8.0f})
		[
			SNew(SBox).HeightOverride(1.0f)
			[
				SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.Line"))
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			MakeCookNumbers(Cooks[SelectedCook])
		]
	);
	// clang-format on
}

TSharedRef<SWidget> SPjcOverviewPanel::MakeCookRow(const int32 Index) {
	const FCook& Cook = Cooks[Index];
	const bool bRead = Cook.Shipped.IsSet();
	const int32 NumChanged = bRead ? Cook.Shipped->NumChanged : 0;
	const int32 NumDeleted = bRead ? Cook.Shipped->NumDeleted : 0;
	const bool bUpToDate = bRead && NumChanged == 0 && NumDeleted == 0;

	FString State = TEXT("not readable");
	FString StateTip = TEXT("Its asset registry could not be read. It may come from another engine version.");
	FLinearColor StateColor = PjcWidgets::GetColor("ProjectCleaner.Color.Yellow");
	if (bUpToDate) {
		State = TEXT("up to date");
		StateTip = TEXT("No scanned asset was saved, added or deleted since this cook.");
		StateColor = PjcWidgets::GetColor("ProjectCleaner.Color.Ok");
	}
	else if (bRead) {
		TArray<FString> Since;
		if (NumChanged > 0) Since.Add(FString::Printf(TEXT("%s saved or added"), *PjcFormat::Count(NumChanged)));
		if (NumDeleted > 0) Since.Add(FString::Printf(TEXT("%s deleted"), *PjcFormat::Count(NumDeleted)));
		State = TEXT("out of date");
		StateTip = FString::Join(Since, TEXT(", ")) + TEXT(" since this cook, so its numbers can be out of date. Cook again for current numbers.");
	}
	const bool bMany = Cooks.Num() > 1;
	const FString Title = Cook.Platform + TEXT(" · last cook ") + FormatCookTime(Cook.Time, true);
	const FString More = Cook.File + (bMany ? TEXT("\nClick: show its numbers") : TEXT(""));
	const FString When = FormatCookTime(Cook.Time, false) + TEXT(" · ") + FormatAgo(Cook.Time);

	// clang-format off
	const TSharedRef<SWidget> Line = SNew(SBox).Padding(FMargin {6.0f, 2.0f}).MinDesiredHeight(22.0f).VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(Cook.Platform)).Font(FPjcStyles::GetFont("Bold", 9))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {8.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock).Text(FText::FromString(State)).ColorAndOpacity(StateColor)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {8.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock).Text(FText::FromString(When)).Font(PjcWidgets::GetFontMono(8)).ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		];
	// clang-format on
	const TSharedRef<SToolTip> Tip = PjcWidgets::MakeToolTip(Title, StateTip, More);
	if (!bMany) {
		// clang-format off
		return SNew(SBox).ToolTip(Tip)
			[
				Line
			];
		// clang-format on
	}

	// clang-format off
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(Index == SelectedCook ? PjcOverviewMarked() : FLinearColor::Transparent)
		.Padding(0.0f)
		[
			SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
			.ForegroundColor(FSlateColor::UseForeground())
#endif
			.ButtonStyle(&FPjcStyles::Get().GetWidgetStyle<FButtonStyle>("ProjectCleaner.Button.Row"))
			.ContentPadding(FMargin {0.0f})
			.ToolTip(Tip)
			.OnClicked_Lambda([this, Index]() {
				SelectedCook = Index;
				Rebuild();
				return FReply::Handled();
			})
			[
				Line
			]
		];
	// clang-format on
}

TSharedRef<SWidget> SPjcOverviewPanel::MakeCookNumbers(const FCook& Cook) const {
	if (!Cook.Shipped.IsSet()) {
		const FString Message = TEXT("The cook for ") + Cook.Platform + TEXT(" could not be read. It may come from another engine version.");
		// clang-format off
		return SNew(STextBlock)
			.Text(FText::FromString(Message))
			.AutoWrapText(true)
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			.ToolTip(PjcWidgets::MakeToolTip(TEXT("Last cook"), Cook.File));
		// clang-format on
	}

	const FPjcOverview::FShipped& Ship = Cook.Shipped.GetValue();
	const int64 AllSize = Ship.Shipped.Size + Ship.UnusedShipped.Size + Ship.UsedNotShipped.Size + Ship.Unused.Size;
	const FLinearColor ShipColor = PjcWidgets::GetColor("ProjectCleaner.Color.ShipFill");
	const FLinearColor KeepColor = PjcWidgets::GetColor("ProjectCleaner.Color.KeepFill");
	const FLinearColor WarnColor = PjcWidgets::GetColor("ProjectCleaner.Color.Yellow");
	const FLinearColor UnusedColor = PjcWidgets::GetColor("ProjectCleaner.Color.UnusedFill");

	// bar by editor size
	TArray<PjcWidgets::FBarSegment> Segments;
	const auto AddSegment = [&Segments, AllSize](const FString& Label, const FPjcOverview::FTotal& Total, const FLinearColor& Color) {
		PjcWidgets::FBarSegment& Segment = Segments.AddDefaulted_GetRef();
		Segment.Value = static_cast<double>(Total.Size);
		Segment.Color = Color;
		const FString NumText = PjcFormat::Count(Total.Num);
		const FString PercentText = PjcFormat::Percent(Total.Size, AllSize);
		const FString Sizes = FString::Printf(TEXT("%s assets · %s in the editor (%s)"), *NumText, *PjcFormat::Size(Total.Size), *PercentText);
		Segment.ToolTip = PjcWidgets::MakeToolTip(Label, Sizes);
	};
	AddSegment(TEXT("Shipped"), Ship.Shipped, ShipColor);
	AddSegment(TEXT("Unused, shipped"), Ship.UnusedShipped, WarnColor);
	AddSegment(TEXT("Used, not shipped"), Ship.UsedNotShipped, KeepColor);
	AddSegment(TEXT("Unused"), Ship.Unused, UnusedColor);

	const FSlateColor Plain = FSlateColor::UseForeground();
	const TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	const auto AddRow = [&Rows](const TSharedRef<SWidget>& Row) {
		// clang-format off
		Rows->AddSlot().AutoHeight()
		[
			Row
		];
		// clang-format on
	};

	// shipped: cooked size and what else the cook packaged in the tooltip
	TArray<FString> RootLines;
	for (int32 Index = 0; Index < Ship.OutsideRoots.Num() && Index < 8; ++Index) {
		RootLines.Add(FString::Printf(TEXT("%s: %s"), *Ship.OutsideRoots[Index].Key, *PjcFormat::Count(Ship.OutsideRoots[Index].Value)));
	}
	const int32 NumShipped = Ship.Shipped.Num + Ship.UnusedShipped.Num;
	const int64 SizeShippedEditor = Ship.Shipped.Size + Ship.UnusedShipped.Size;
	const FString ShipCooked = PjcFormat::Size(Ship.ShippedCooked + Ship.UnusedShippedCooked);
	const FString ShipWhere = FString::Printf(TEXT("Packaged in the game for %s: %s cooked, "), *Cook.Platform, *ShipCooked);
	const FString ShipTip = ShipWhere + PjcFormat::Size(SizeShippedEditor) + TEXT(" in the editor. Cooked size is before pak compression.");
	const FString OutsideCount = PjcFormat::Count(Ship.NumOutside);
	FString Also = FString::Printf(TEXT("Also shipped: %s engine and plugin packages, %s cooked"), *OutsideCount, *PjcFormat::Size(Ship.SizeOutside));
	if (Ship.NumGenerated > 0) {
		Also += FString::Printf(TEXT("\n%s generated packages (World Partition cells and others)"), *PjcFormat::Count(Ship.NumGenerated));
	}
	if (RootLines.Num() > 0) {
		Also += TEXT("\n") + FString::Join(RootLines, TEXT("\n"));
	}
	const FString ShipShare = PjcFormat::Percent(SizeShippedEditor, AllSize);
	const FString ShipSize = PjcFormat::Size(SizeShippedEditor);
	const TSharedRef<SToolTip> ShipToolTip = PjcWidgets::MakeToolTip(TEXT("Shipped"), ShipTip, Also);
	AddRow(MakeRow(ShipColor, TEXT("Shipped"), PjcFormat::Count(NumShipped), Plain, ShipSize, ShipShare, ShipToolTip));

	// used, not shipped: by chain start in the tooltip
	TArray<FString> ReasonLines;
	for (const auto& Row : Ship.UsedNotShippedReasons) {
		FString Name;
		FString ReasonTip;
		FLinearColor Color;
		GetReasonInfo(Row.Reason, Name, ReasonTip, Color);
		ReasonLines.Add(FString::Printf(TEXT("%s: %s"), *Name, *PjcFormat::Count(Row.Total.Num)));
	}
	const FString NotCooked = TEXT("Used in the editor, or only by content that is not cooked: ");
	const FString LeftOut = TEXT("editor-only references, maps and tools left out of the cook. Not a cleanup list.");
	const TSharedRef<SToolTip> KeepToolTip
		= PjcWidgets::MakeToolTip(TEXT("Used, not shipped"), NotCooked + LeftOut, FString::Join(ReasonLines, TEXT("\n")));
	const FString KeepNum = PjcFormat::Count(Ship.UsedNotShipped.Num);
	const FString KeepSize = PjcFormat::Size(Ship.UsedNotShipped.Size);
	const FString KeepShare = PjcFormat::Percent(Ship.UsedNotShipped.Size, AllSize);
	AddRow(MakeRow(KeepColor, TEXT("Used, not shipped"), KeepNum, Plain, KeepSize, KeepShare, KeepToolTip));

	// unused, shipped: the cook check of this platform
	const FSlateColor Muted = FSlateColor::UseSubduedForeground();
	if (UPjcSubsystem::GetScanResult().CookCheck.PlatformsCookAll.Contains(Cook.Platform)) {
		const TCHAR* CookAllTip = TEXT("This cook took nearly every asset, unused ones too, so it cannot tell what the game needs. ")
			TEXT("A cook of only the maps the game uses makes this check work.");
		const TSharedRef<SToolTip> Tip = PjcWidgets::MakeToolTip(TEXT("Unused, shipped"), CookAllTip);
		AddRow(MakeRow(WarnColor, TEXT("Unused, shipped"), TEXT("–"), Muted, TEXT("cook took nearly all"), FString {}, Tip, {}, false, 0.0f));
	}
	else if (Ship.UnusedShipped.Num > 0) {
		const TCHAR* CheckTip = TEXT("This cook packaged them, but the scan finds nothing that uses them: the game may load them in a way ")
			TEXT("the scan cannot follow. They are in Review and Clean project keeps them.");
		const TSharedRef<SToolTip> Tip = PjcWidgets::MakeToolTip(TEXT("Unused, shipped"), CheckTip, TEXT("Click: list them on the right"));
		const FString CheckNum = TEXT("⚠ ") + PjcFormat::Count(Ship.UnusedShipped.Num);
		const FSlateColor CheckColor {PjcWidgets::GetColor("ProjectCleaner.Color.Review")};
		const FSimpleDelegate OnClick = FSimpleDelegate::CreateLambda([this]() {
			OnShowSet.ExecuteIfBound(EPjcAssetSet::Review, PjcOverviewReasonCooked);
		});
		const TAttribute<bool> IsOn = TAttribute<bool>::Create([this]() {
			return IsSetShown.IsBound() && IsSetShown.Execute(EPjcAssetSet::Review, PjcOverviewReasonCooked);
		});
		const FString CheckSize = PjcFormat::Size(Ship.UnusedShipped.Size);
		AddRow(MakeRow(WarnColor, TEXT("Unused, shipped"), CheckNum, CheckColor, CheckSize, TEXT("›"), Tip, OnClick, IsOn));
	}
	else {
		const int32 NumNotChecked = UPjcSubsystem::GetScanResult().CookCheck.NumNotChecked;
		const FString NotCheckedCount = PjcFormat::Count(NumNotChecked);
		const TCHAR* AgreeText = TEXT("This cook packaged none of the unused assets: nothing it cooked needed them. A cook of ")
			TEXT("only some maps leaves out more, so the check covers what that cook took.");
		const FString NotCheckedTip = NotCheckedCount
			+ TEXT(" unused assets were saved after the cook (or put back by source control, which gives the files a new date), ")
			+ TEXT("so it cannot tell about them. Cook the project again to check them.");
		const FString Agree = NumNotChecked > 0 ? NotCheckedCount + TEXT(" unused changed since the cook") : TEXT("Scan and cook agree");
		// clang-format off
		AddRow(
			SNew(SBox).Padding(FMargin {6.0f, 3.0f, 6.0f, 0.0f}).ToolTip(PjcWidgets::MakeToolTip(TEXT("Unused, shipped"), AgreeText, NumNotChecked > 0 ? NotCheckedTip : FString {}))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 5.0f, 0.0f})
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("✓"))).ColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.Ok"))
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(FText::FromString(Agree)).ColorAndOpacity(Muted)
				]
			]
		);
		// clang-format on
	}

	// clang-format off
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 6.0f})
		[
			PjcWidgets::MakeBar(Segments, 6.0f)
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			Rows
		];
	// clang-format on
}

TSharedRef<SWidget> SPjcOverviewPanel::MakeCard(
	const FString& Title,
	const TSharedRef<SWidget>& Value,
	const TSharedRef<SWidget>& Content,
	const FSlateBrush* Icon,
	const TOptional<FLinearColor>& Accent
) {
	const TSharedRef<SHorizontalBox> Head = SNew(SHorizontalBox);
	if (Icon) {
		// clang-format off
		Head->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 6.0f, 0.0f})
		[
			SNew(SBox).WidthOverride(16.0f).HeightOverride(16.0f)
			[
				SNew(SImage).Image(Icon)
			]
		];
		// clang-format on
	}
	// clang-format off
	const TSharedRef<SWidget> TitleWidget = !Accent.IsSet() ? PjcWidgets::MakeCardTitle(Title) : StaticCastSharedRef<SWidget>(
		SNew(STextBlock)
		.Text(FText::FromString(Title.ToUpper()))
		.Font(FPjcStyles::GetFont("Bold", 8))
		.ColorAndOpacity(Accent.GetValue())
	);
	Head->AddSlot().FillWidth(1.0f).VAlign(VAlign_Center)
	[
		TitleWidget
	];
	Head->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {8.0f, 0.0f, 0.0f, 0.0f})
	[
		Value
	];
	// clang-format on

	const bool bContent = Content != SNullWidget::NullWidget;
	// clang-format off
	const TSharedRef<SWidget> Inner = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, bContent ? 6.0f : 0.0f})
		[
			Head
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			Content
		];
	// clang-format on
	if (!Accent.IsSet()) {
		return PjcWidgets::MakeCard(Inner, FMargin {10.0f, 8.0f, 10.0f, 9.0f});
	}

	// a strip of the accent color along the left edge
	// clang-format off
	return PjcWidgets::MakeCard(
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SBox).WidthOverride(3.0f)
			[
				SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(Accent.GetValue())
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(FMargin {9.0f, 7.0f, 9.0f, 8.0f})
		[
			Inner
		],
		FMargin {1.0f}
	);
	// clang-format on
}

TSharedRef<SWidget> SPjcOverviewPanel::MakeRow(
	const TOptional<FLinearColor>& Swatch,
	const FString& Name,
	const FString& Number,
	const FSlateColor& NumberColor,
	const FString& Size,
	const FString& Share,
	const TSharedPtr<SToolTip>& ToolTip,
	const FSimpleDelegate& OnClick,
	const TAttribute<bool>& IsOn,
	const float SizeWidth
) {
	const FSlateColor Muted = FSlateColor::UseSubduedForeground();
	const TSharedRef<SHorizontalBox> Line = SNew(SHorizontalBox);
	if (Swatch.IsSet()) {
		// clang-format off
		Line->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 7.0f, 0.0f})
		[
			PjcWidgets::MakeSwatch(Swatch.GetValue())
		];
		// clang-format on
	}
	// a share of only "›" opens something: in the accent color
	const bool bLink = Share.Equals(TEXT("›"));
	const FSlateColor ShareColor = bLink ? FSlateColor {PjcWidgets::GetColor("ProjectCleaner.Color.Accent")} : Muted;
	// clang-format off
	Line->AddSlot().FillWidth(1.0f).VAlign(VAlign_Center)
	[
		SNew(STextBlock).Text(FText::FromString(Name))
	];
	Line->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {8.0f, 0.0f, 0.0f, 0.0f})
	[
		SNew(STextBlock).Text(FText::FromString(Number)).Font(PjcWidgets::GetFontMono(10)).ColorAndOpacity(NumberColor)
	];
	Line->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {8.0f, 0.0f, 0.0f, 0.0f})
	[
		SNew(SBox).WidthOverride(SizeWidth > 0.0f ? FOptionalSize {SizeWidth} : FOptionalSize {}).HAlign(HAlign_Right)
		[
			SNew(STextBlock).Text(FText::FromString(Size)).Font(PjcWidgets::GetFontMono(8)).ColorAndOpacity(Muted)
		]
	];
	Line->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})
	[
		SNew(SBox).WidthOverride(bLink ? 10.0f : 34.0f).HAlign(HAlign_Right)
		[
			SNew(STextBlock).Text(FText::FromString(Share)).Font(PjcWidgets::GetFontMono(8)).ColorAndOpacity(ShareColor)
		]
	];

	const TSharedRef<SWidget> Content = SNew(SBox).Padding(FMargin {6.0f, 2.0f}).MinDesiredHeight(22.0f).VAlign(VAlign_Center)
		[
			Line
		];
	// clang-format on

	if (!OnClick.IsBound()) {
		// clang-format off
		return SNew(SBox).ToolTip(ToolTip)
			[
				Content
			];
		// clang-format on
	}

	// clang-format off
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor_Lambda([IsOn]() { return IsOn.Get() ? PjcOverviewMarked() : FLinearColor::Transparent; })
		.Padding(0.0f)
		[
			SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
			.ForegroundColor(FSlateColor::UseForeground())
#endif
			.ButtonStyle(&FPjcStyles::Get().GetWidgetStyle<FButtonStyle>("ProjectCleaner.Button.Row"))
			.ContentPadding(FMargin {0.0f})
			.ToolTip(ToolTip)
			.OnClicked_Lambda([OnClick]() {
				OnClick.ExecuteIfBound();
				return FReply::Handled();
			})
			[
				Content
			]
		];
	// clang-format on
}

FString SPjcOverviewPanel::FormatCookTime(const FDateTime& UtcTime, const bool bWithYear) {
	// FDateTime::ToString has no month names in UE 4.27
	// clang-format off
	static const TCHAR* Months[] = {TEXT("Jan"), TEXT("Feb"), TEXT("Mar"), TEXT("Apr"), TEXT("May"), TEXT("Jun"), TEXT("Jul"), TEXT("Aug"), TEXT("Sep"), TEXT("Oct"), TEXT("Nov"), TEXT("Dec")};
	// clang-format on
	const FDateTime Local = UtcTime + (FDateTime::Now() - FDateTime::UtcNow());
	const FString Year = bWithYear ? FString::Printf(TEXT(", %d"), Local.GetYear()) : FString {};
	return FString::Printf(TEXT("%s %d%s, %02d:%02d"), Months[Local.GetMonth() - 1], Local.GetDay(), *Year, Local.GetHour(), Local.GetMinute());
}

FString SPjcOverviewPanel::FormatAgo(const FDateTime& UtcTime) {
	const int32 Minutes = FMath::FloorToInt((FDateTime::UtcNow() - UtcTime).GetTotalMinutes());
	if (Minutes < 1) return TEXT("just now");
	if (Minutes < 60) return FString::Printf(TEXT("%d min ago"), Minutes);
	if (Minutes < 60 * 24) return FString::Printf(TEXT("%d h ago"), Minutes / 60);
	const int32 Days = Minutes / (60 * 24);
	return FString::Printf(TEXT("%d day%s ago"), Days, Days == 1 ? TEXT("") : TEXT("s"));
}

void SPjcOverviewPanel::GetReasonInfo(const EPjcUsageReason Reason, FString& OutName, FString& OutToolTip, FLinearColor& OutColor) {
	const TCHAR* Color = TEXT("#6FA8A2");
	switch (Reason) {
		case EPjcUsageReason::None :
			OutName = TEXT("Nothing uses it");
			OutToolTip = TEXT("Nothing uses them. Clean project deletes them.");
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
			OutName = TEXT("Outside scanned content");
			OutToolTip = TEXT("Referenced by engine content, plugins left out of the scan, or level actors saved in their own files.");
			Color = TEXT("#9B8CF0");
			break;
		case EPjcUsageReason::Indirect :
			OutName = TEXT("Source & config");
			OutToolTip = TEXT("C++, C#, Python and .ini files name them by path.");
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
			OutName = PjcOverviewReasonEditor;
			OutToolTip = TEXT("Editor utilities, validators, scriptable tools and editor tool data.");
			Color = TEXT("#7FB2FF");
			break;
		case EPjcUsageReason::Excluded :
			OutName = TEXT("Excluded");
			OutToolTip = TEXT("Excluded in settings and always used classes, and what they use.");
			Color = TEXT("#8A9099");
			break;
		case EPjcUsageReason::Cinematic :
			OutName = TEXT("Cinematics");
			OutToolTip = TEXT("Level Sequences, Template Sequences and Movie Render configs, and what they use.");
			Color = TEXT("#C08457");
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
		case EPjcUsageReason::Splash :
			OutName = TEXT("Splash content");
			OutToolTip = TEXT("Texture copies of the splash images in Content/Splash: the editor imports them again when deleted.");
			Color = TEXT("#9FC3FF");
			break;
		default :
			OutName = TEXT("Other");
			OutToolTip = FString {};
			break;
	}
	OutColor = FLinearColor {FColor::FromHex(Color)};
}
