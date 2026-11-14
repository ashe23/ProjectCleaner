// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabInspector.h"
#include "Slate/PjcActions.h"
#include "Slate/PjcWidgets.h"
#include "Slate/SPjcClassBar.h"
#include "Slate/SPjcDialog.h"
#include "Slate/SPjcScanBar.h"
#include "Slate/SPjcToolFrame.h"
#include "PjcFormat.h"
#include "PjcSettings.h"
#include "PjcShim.h"
#include "PjcStyles.h"
#include "PjcSubsystem.h"
#include "PjcUnused.h"
// Engine Headers
#include "ContentBrowserModule.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "HAL/PlatformApplicationMisc.h"
#include "IContentBrowserSingleton.h"
#include "Misc/PackageName.h"
#include "Misc/ScopedSlowTask.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SExpanderArrow.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"

namespace
{
	// unity builds join files: names unique to this file

	// a level with more rows is grouped by class; a group shows this many at first, Show more adds MorePage
	constexpr int32 PjcInspectorGroupAt = 12;
	constexpr int32 PjcInspectorPage = 50;
	constexpr int32 PjcInspectorMorePage = 100;
	constexpr int32 PjcInspectorMaxChain = 6;
	constexpr int32 PjcInspectorMaxList = 8;
	constexpr int64 PjcInspectorAskAbove = 200ll * 1024 * 1024;

	const FName PjcInspectorColAsset {TEXT("Asset")};
	const FName PjcInspectorColKind {TEXT("Kind")};
	const FName PjcInspectorColWhere {TEXT("Where")};
	const FName PjcInspectorColMeta {TEXT("Meta")};
	const FName PjcInspectorColGo {TEXT("Go")};

	FLinearColor PjcInspectorColor(const FName Name) {
		return PjcWidgets::GetColor(Name);
	}

	// status as text shows it: the light shade of each status color
	FName PjcInspectorStatusColorName(const EPjcAssetStatus Status, const bool bScanned) {
		if (!bScanned) return TEXT("ProjectCleaner.Color.Muted");
		switch (Status) {
			case EPjcAssetStatus::Unused :	   return TEXT("ProjectCleaner.Color.Red");
			case EPjcAssetStatus::EditorOnly : return TEXT("ProjectCleaner.Color.Editor");
			case EPjcAssetStatus::Review :	   return TEXT("ProjectCleaner.Color.Review");
			default :						   return TEXT("ProjectCleaner.Color.Ok");
		}
	}

	FLinearColor PjcInspectorStatusColor(const EPjcAssetStatus Status, const bool bScanned) {
		return PjcInspectorColor(PjcInspectorStatusColorName(Status, bScanned));
	}

	const TCHAR* PjcInspectorStatusText(const EPjcAssetStatus Status, const bool bScanned) {
		if (!bScanned) return TEXT("Not scanned");
		switch (Status) {
			case EPjcAssetStatus::Unused :	   return TEXT("Unused");
			case EPjcAssetStatus::EditorOnly : return TEXT("Editor only");
			case EPjcAssetStatus::Review :	   return TEXT("Review");
			default :						   return TEXT("Used");
		}
	}

	// sort order of what uses an asset: used first, then editor only and review, not scanned, unused last
	int32 PjcInspectorStatusRank(const EPjcAssetStatus Status, const bool bScanned) {
		if (!bScanned) return 2;
		switch (Status) {
			case EPjcAssetStatus::Unused :	   return 3;
			case EPjcAssetStatus::EditorOnly :
			case EPjcAssetStatus::Review :	   return 1;
			default :						   return 0;
		}
	}

	const TCHAR* PjcInspectorKindText(const EPjcLinkKind Kind) {
		switch (Kind) {
			case EPjcLinkKind::Soft :		return TEXT("soft");
			case EPjcLinkKind::EditorOnly : return TEXT("editor only");
			default :						return TEXT("hard");
		}
	}

	const TCHAR* PjcInspectorKindToolTip(const EPjcLinkKind Kind) {
		switch (Kind) {
			case EPjcLinkKind::Soft :		return TEXT("Soft reference: loaded on demand by path");
			case EPjcLinkKind::EditorOnly : return TEXT("Editor-only reference: kept in the editor, not cooked");
			default :						return TEXT("Hard reference: loads with the asset");
		}
	}

	FName PjcInspectorKindColor(const EPjcLinkKind Kind) {
		switch (Kind) {
			case EPjcLinkKind::Soft :		return TEXT("ProjectCleaner.Color.SoftLink");
			case EPjcLinkKind::EditorOnly : return TEXT("ProjectCleaner.Color.Editor");
			default :						return TEXT("ProjectCleaner.Color.Muted");
		}
	}

	// Who keeps a root used, in a few words
	const TCHAR* PjcInspectorSourceTitle(const EPjcUsageReason Reason) {
		switch (Reason) {
			case EPjcUsageReason::Primary :
			case EPjcUsageReason::ConfigFolder :		return TEXT("Asset Manager");
			case EPjcUsageReason::PrimaryClass :		return TEXT("Primary class");
			case EPjcUsageReason::EngineReferenced :	return TEXT("Engine or plugins");
			case EPjcUsageReason::Indirect :			return TEXT("Source & config");
			case EPjcUsageReason::Excluded :			return TEXT("Exclusion");
			case EPjcUsageReason::Cinematic :			return TEXT("Cinematics");
			case EPjcUsageReason::Editor :				return TEXT("Editor asset");
			case EPjcUsageReason::Splash :				return TEXT("Splash folder");
			case EPjcUsageReason::GameplayCue :			return TEXT("GameplayCue");
			case EPjcUsageReason::Megascans :			return TEXT("Megascans");
			case EPjcUsageReason::Localized :			return TEXT("Localization");
			case EPjcUsageReason::FolderLabelAsset :
			case EPjcUsageReason::FolderLabel :			return TEXT("Folder label");
			case EPjcUsageReason::CookPackaged :		return TEXT("Last cook");
			case EPjcUsageReason::GameplayCueNotFired : return TEXT("Cue nothing fires");
			default :									return TEXT("Root");
		}
	}

	FString PjcInspectorPlural(const int32 Num, const FString& One) {
		return PjcFormat::Count(Num) + TEXT(" ") + One + (Num == 1 ? TEXT("") : TEXT("s"));
	}

	FString PjcInspectorSteps(const int32 Steps) {
		return Steps == 0 ? FString {TEXT("itself a root")} : PjcInspectorPlural(Steps, TEXT("step"));
	}

	// Plural of a class label: "Level Sequences", "Blueprint Classes"
	FString PjcInspectorClassPlural(const int32 Num, const FString& Label) {
		if (Num == 1) return Label;
		return Label.EndsWith(TEXT("s")) ? Label + TEXT("es") : Label + TEXT("s");
	}

	FString PjcInspectorShortName(const FName Package) {
		return FPackageName::GetShortName(Package);
	}

	TSharedRef<SWidget> PjcInspectorText(const FString& Text, const FName Color = NAME_None, const bool bMono = false) {
		// clang-format off
		return SNew(STextBlock)
			.Text(FText::FromString(Text))
			.Font(bMono ? PjcWidgets::GetFontMono(9) : FPjcStyles::GetFont("Regular", 9))
			.ColorAndOpacity(Color.IsNone() ? FSlateColor::UseForeground() : FSlateColor {PjcInspectorColor(Color)});
		// clang-format on
	}

	// A number of the answer: mono, bright
	TSharedRef<SWidget> PjcInspectorNumber(const FString& Text) {
		return SNew(STextBlock)
			.Text(FText::FromString(Text))
			.Font(PjcWidgets::GetFontMono(9))
			.ColorAndOpacity(PjcInspectorColor("ProjectCleaner.Color.Strong"));
	}

	// Small caption: bold, upper case, muted
	TSharedRef<SWidget> PjcInspectorCaption(const FString& Text, const FString& ToolTip = FString {}) {
		// clang-format off
		return SNew(STextBlock)
			.Text(FText::FromString(Text))
			.ToolTipText(FText::FromString(ToolTip))
			.Font(FPjcStyles::GetFont("Bold", 8))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground());
		// clang-format on
	}

	TSharedRef<SWidget> PjcInspectorLine(const bool bVertical) {
		// clang-format off
		return SNew(SBox).WidthOverride(bVertical ? FOptionalSize {1.0f} : FOptionalSize {}).HeightOverride(bVertical ? FOptionalSize {} : FOptionalSize {1.0f})
			[
				SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(PjcInspectorColor("ProjectCleaner.Color.Line"))
			];
		// clang-format on
	}

	// Button of the toolbar: an icon of the plugin set, then the label
	TSharedRef<SWidget> PjcInspectorToolButton(
		const FName Brush,
		const FString& Label,
		const FString& ToolTip,
		const bool bDanger,
		TFunction<bool()> Enabled,
		TFunction<void()> OnClick,
		const FSlateBrush* Icon = nullptr
	) {
		const FSlateColor TextColor = bDanger ? FSlateColor {PjcInspectorColor("ProjectCleaner.Color.DangerText")} : FSlateColor::UseForeground();
		// clang-format off
		const TSharedRef<SWidget> Content = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 6.0f, 0.0f})
			[
				SNew(SBox).WidthOverride(14.0f).HeightOverride(14.0f)
				[
					SNew(SImage).Image(Icon ? Icon : FPjcStyles::Get().GetBrush(Brush))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(Label)).ColorAndOpacity(TextColor)
			];
		if (bDanger) {
			return SNew(SButton)
				.ButtonStyle(&FPjcStyles::Get().GetWidgetStyle<FButtonStyle>("ProjectCleaner.Button.DangerQuiet"))
#if ENGINE_MAJOR_VERSION < 5
				.ForegroundColor(FSlateColor::UseForeground())
#endif
				.ContentPadding(FMargin {8.0f, 1.0f, 10.0f, 1.0f})
				.ToolTipText(FText::FromString(ToolTip))
				.IsEnabled_Lambda([Enabled]() { return Enabled(); })
				.OnClicked_Lambda([OnClick]() { OnClick(); return FReply::Handled(); })
				[
					Content
				];
		}
		return SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
			.ButtonStyle(&PjcShim::GetButtonStyle427(false))
			.ForegroundColor(FSlateColor::UseForeground())
#endif
			.ContentPadding(FMargin {8.0f, 1.0f, 10.0f, 1.0f})
			.ToolTipText(FText::FromString(ToolTip))
			.IsEnabled_Lambda([Enabled]() { return Enabled(); })
			.OnClicked_Lambda([OnClick]() { OnClick(); return FReply::Handled(); })
			[
				Content
			];
		// clang-format on
	}

	// A toggle chip: a color square, the label, a count; picked: on the marked color
	TSharedRef<SWidget> PjcInspectorChipToggle(
		const FString& Label,
		const FString& ToolTip,
		const FLinearColor& Color,
		const int32 Num,
		TFunction<bool()> IsOn,
		TFunction<void()> OnClick
	) {
		// clang-format off
		const TSharedRef<SHorizontalBox> Box = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 6.0f, 0.0f})
			[
				SNew(SBox).WidthOverride(8.0f).HeightOverride(8.0f)
				[
					SNew(SImage)
					.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.ColorAndOpacity_Lambda([IsOn, Color]() { return FSlateColor {IsOn() ? Color : Color.CopyWithNewOpacity(0.35f)}; })
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
				.Font(FPjcStyles::GetFont("Regular", 8))
				.ColorAndOpacity_Lambda([IsOn]() { return IsOn() ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground(); })
			];
		if (Num >= 0) {
			Box->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock).Text(FText::FromString(PjcFormat::Count(Num))).Font(PjcWidgets::GetFontMono(7)).ColorAndOpacity(FSlateColor::UseSubduedForeground())
			];
		}
		return SNew(SButton)
			.ButtonStyle(&PjcShim::GetStyle().GetWidgetStyle<FButtonStyle>(TEXT("NoBorder")))
#if ENGINE_MAJOR_VERSION < 5
			.ForegroundColor(FSlateColor::UseForeground())
#endif
			.ContentPadding(FMargin {0.0f})
			.ToolTipText(FText::FromString(ToolTip))
			.OnClicked_Lambda([OnClick]() { OnClick(); return FReply::Handled(); })
			[
				SNew(SBorder)
				.BorderImage(FPjcStyles::Get().GetBrush("ProjectCleaner.Pill"))
				.BorderBackgroundColor_Lambda([IsOn]() { return FSlateColor {PjcInspectorColor(IsOn() ? "ProjectCleaner.Color.Marked" : "ProjectCleaner.Color.Hover")}; })
				.Padding(FMargin {8.0f, 2.0f})
				[
					Box
				]
			];
		// clang-format on
	}

	bool PjcInspectorExists(const FName Package) {
		TArray<FAssetData> Assets;
		UPjcSubsystem::GetModuleAssetRegistry().Get().GetAssetsByPackageName(Package, Assets);
		return Assets.Num() > 0;
	}

	void PjcInspectorCopy(const FString& Text, const FString& What) {
		FPlatformApplicationMisc::ClipboardCopy(*Text);
		UPjcSubsystem::ShowNotification(TEXT("Copied ") + What, SNotificationItem::CS_None, 2.0f);
	}

	void PjcInspectorBrowse(const TArray<FAssetData>& Assets) {
		TArray<FAssetData> Valid;
		for (const FAssetData& Asset : Assets) {
			if (Asset.IsValid()) Valid.Add(Asset);
		}
		if (Valid.Num() > 0) UPjcSubsystem::GetModuleContentBrowser().Get().SyncBrowserToAssets(Valid);
	}
}	// namespace

// One row of the tree: the asset with its expander, the reference kind, where it is set, how far to a root or its size
class SPjcInspectorTreeRow final : public SMultiColumnTableRow<SPjcTabInspector::FItemPtr>
{
  public:

	SLATE_BEGIN_ARGS(SPjcInspectorTreeRow) {}
	SLATE_ARGUMENT(SPjcTabInspector::FItemPtr, Item)
	SLATE_ARGUMENT(TFunction<TSharedRef<SWidget>(const SPjcTabInspector::FItemPtr&, const FName, const TSharedRef<ITableRow>&)>, MakeCell)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& Table) {
		Item = InArgs._Item;
		MakeCell = InArgs._MakeCell;
		// clang-format off
		SMultiColumnTableRow<SPjcTabInspector::FItemPtr>::Construct(
			FSuperRowType::FArguments()
#if ENGINE_MAJOR_VERSION < 5
			.Style(&PjcShim::GetRowStyle427())
#endif
			.Padding(FMargin {0.0f, 1.0f}),
			Table
		);
		// clang-format on
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Column) override {
		return MakeCell(Item, Column, SharedThis(this));
	}

  private:

	SPjcTabInspector::FItemPtr Item;
	TFunction<TSharedRef<SWidget>(const SPjcTabInspector::FItemPtr&, const FName, const TSharedRef<ITableRow>&)> MakeCell;
};

void SPjcTabInspector::Construct(const FArguments& InArgs) {
	TabManager = InArgs._TabManager;
	ScanFinishedHandle = UPjcSubsystem::OnScanFinished().AddSP(this, &SPjcTabInspector::OnScanFinished);

	// clang-format off
	ChildSlot
	[
		SNew(SPjcToolFrame).PageName(TEXT("Inspector")).PanelOpen(false)
		[
			SNew(SSplitter).PhysicalSplitterHandleSize(3.0f)
			+ SSplitter::Slot().Value(0.36f)
			[
				MakeFinder()
			]
			+ SSplitter::Slot().Value(0.64f)
			[
				MakeMain()
			]
		]
	];
	// clang-format on

	OnScanFinished();
}

SPjcTabInspector::~SPjcTabInspector() {
	UPjcSubsystem::OnScanFinished().Remove(ScanFinishedHandle);
}

void SPjcTabInspector::Inspect(const FName Package) {
	if (Package.IsNone()) return;

	if (!UPjcSubsystem::GetScanResult().IsValid()) {
		Pending = Package;
		return;
	}
	SetFocus(Package, true);
}

void SPjcTabInspector::OnScanFinished() {
	RebuildFinder();

	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	if (!Pending.IsNone() && Result.IsValid()) {
		const FName Package = Pending;
		Pending = NAME_None;
		SetFocus(Package, true);
		return;
	}
	// the asset may be gone (deleted, renamed)
	if (!Focus.IsNone() && !Result.Usage.Contains(Focus) && !PjcInspectorExists(Focus)) {
		HistoryBack.Remove(Focus);
		HistoryForward.Remove(Focus);
		Focus = NAME_None;
	}
	RebuildAll();
}

bool SPjcTabInspector::EnsureModel() {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	if (!Result.IsValid()) return false;
	if (Model.IsBuiltFor(Result)) return true;

	// what was read belongs to the scan it was read in; what was loaded for it stays to be unloaded
	Cache.Reset();
	Locations.Reset();
	OpenNodes.Reset();
	Model.Build(Result, &Cache);
	// the model holds every reference now: the registry answers are not needed again
	Cache.Reset();
	return true;
}

void SPjcTabInspector::SetFocus(const FName Package, const bool bRecord) {
	if (Package.IsNone()) return;

	if (bRecord && !Focus.IsNone() && Package != Focus) {
		HistoryBack.Add(Focus);
		HistoryForward.Reset();
	}
	Focus = Package;
	RouteRoot = INDEX_NONE;
	bDeleteList = false;
	WhatIfUser = INDEX_NONE;
	Keepers.Reset();
	TreeSearch.Reset();
	ClassChips.Reset();
	Expanded.Reset();
	Closed.Reset();
	Pages.Reset();
	RebuildAll();

	// the finder shows it
	if (FinderList.IsValid()) {
		const FRowPtr* Row = FinderRows.FindByPredicate([Package](const FRowPtr& Existing) {
			return Existing->Package == Package;
		});
		if (Row) {
			FinderList->SetSelection(*Row, ESelectInfo::Direct);
			FinderList->RequestScrollIntoView(*Row);
		}
		else {
			FinderList->ClearSelection();
		}
	}
}

void SPjcTabInspector::Back() {
	if (HistoryBack.Num() == 0) return;

	HistoryForward.Add(Focus);
	SetFocus(PjcShim::PopArray(HistoryBack), false);
}

void SPjcTabInspector::Forward() {
	if (HistoryForward.Num() == 0) return;

	HistoryBack.Add(Focus);
	SetFocus(PjcShim::PopArray(HistoryForward), false);
}

void SPjcTabInspector::RebuildAll() {
	FocusNode = !Focus.IsNone() && EnsureModel() ? Model.Find(Focus) : INDEX_NONE;
	ComputeAnswer();
	RebuildHead();
	RebuildTree();
}

void SPjcTabInspector::ShowUses(const bool bInUses) {
	if (bUses == bInUses) return;

	bUses = bInUses;
	TreeSearch.Reset();
	ClassChips.Reset();
	RebuildAll();
}

// finder

TSharedRef<SWidget> SPjcTabInspector::MakeFinder() {
	// clang-format off
	return SNew(SBorder).BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.GroupBorder"))).Padding(0.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {8.0f, 8.0f, 8.0f, 6.0f})
			[
				SAssignNew(FinderSearch, SSearchBox)
				.Style(&PjcShim::GetSearchBoxStyle())
				.HintText(FText::FromString(TEXT("Search assets")))
				.OnTextChanged_Lambda([this](const FText& Text) {
					FinderText = Text.ToString();
					ApplyFinderFilter();
				})
			]
			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
				SNew(SHorizontalBox)
				// filters next to the list, as in the Assets tab: status filters one above the other, then the class bar
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(196.0f)
					[
						SNew(SBorder).BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.DarkGroupBorder"))).Padding(FMargin {0.0f, 4.0f})
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {10.0f, 2.0f, 10.0f, 5.0f})
							[
								PjcWidgets::MakeFilterHeading(FText::FromString(TEXT("STATUS FILTERS")), FText::FromString(TEXT("Which assets the list shows, by status")))
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SAssignNew(FinderPills, SVerticalBox)
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {6.0f, 2.0f, 6.0f, 6.0f})
							[
								SNew(SBox).HeightOverride(1.0f)
								[
									SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(PjcInspectorColor("ProjectCleaner.Color.Line"))
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {10.0f, 2.0f, 10.0f, 5.0f})
							[
								PjcWidgets::MakeFilterHeading(FText::FromString(TEXT("ASSET FILTERS")), FText::FromString(TEXT("Filter by asset class: a class in its color is listed")))
							]
							+ SVerticalBox::Slot().FillHeight(1.0f)
							[
								SAssignNew(FinderClasses, SPjcClassBar).OnChanged(FSimpleDelegate::CreateSP(this, &SPjcTabInspector::FilterFinderRows))
							]
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
						.BorderBackgroundColor(PjcInspectorColor("ProjectCleaner.Color.Header"))
						.Padding(FMargin {10.0f, 5.0f})
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("NAME"))).Font(FPjcStyles::GetFont("Bold", 8)).ColorAndOpacity(FSlateColor::UseSubduedForeground())
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text_Lambda([this]() { return FText::FromString(PjcFormat::Count(FinderRows.Num())); })
								.ToolTipText(FText::FromString(TEXT("Assets listed")))
								.Font(PjcWidgets::GetFontMono(8))
								.ColorAndOpacity(FSlateColor::UseSubduedForeground())
							]
						]
					]
					+ SVerticalBox::Slot().FillHeight(1.0f)
					[
						SAssignNew(FinderList, SListView<FRowPtr>)
						.ListItemsSource(&FinderRows)
						.SelectionMode(ESelectionMode::Single)
						.OnGenerateRow(this, &SPjcTabInspector::OnGenerateFinderRow)
						.OnSelectionChanged_Lambda([this](FRowPtr Row, ESelectInfo::Type SelectInfo) {
							if (Row.IsValid() && SelectInfo != ESelectInfo::Direct && Row->Package != Focus) {
								SetFocus(Row->Package, true);
							}
						})
					]
				]
			]
		];
	// clang-format on
}

void SPjcTabInspector::RebuildFinderPills(const TMap<EStatusFilter, int32>& Counts) {
	if (!FinderPills.IsValid()) return;

	struct FPill
	{
		EStatusFilter Filter;
		const TCHAR* Label;
		const TCHAR* ToolTip;
		const TCHAR* Color;
	};
	// clang-format off
	const FPill Pills[] = {
		{EStatusFilter::All, TEXT("All"), TEXT("Every scanned asset"), TEXT("ProjectCleaner.Color.Accent")},
		{EStatusFilter::Used, TEXT("Used"), TEXT("Used by the game"), TEXT("ProjectCleaner.Color.UsedFill")},
		{EStatusFilter::EditorOnly, TEXT("Editor only"), TEXT("Used, but only the editor needs it. Clean project keeps it."), TEXT("ProjectCleaner.Color.EditorFill")},
		{EStatusFilter::Unused, TEXT("Unused"), TEXT("Nothing uses it. Clean project deletes it."), TEXT("ProjectCleaner.Color.UnusedFill")},
		{EStatusFilter::Review, TEXT("Review"), TEXT("The scan is not sure about it. Clean project keeps it."), TEXT("ProjectCleaner.Color.ReviewFill")},
	};
	// clang-format on
	FinderPills->ClearChildren();
	for (const FPill& Pill : Pills) {
		const EStatusFilter Filter = Pill.Filter;
		// clang-format off
		FinderPills->AddSlot().AutoHeight().Padding(FMargin {6.0f, 0.0f, 6.0f, 4.0f})
		[
			PjcWidgets::MakeFilterPill(
				Pill.Label,
				Pill.ToolTip,
				FString {},
				PjcInspectorColor(Pill.Color),
				Counts.FindRef(Filter),
				Filter != EStatusFilter::All,
				[this, Filter]() { return FinderStatus == Filter; },
				[this, Filter]() { return Filter != EStatusFilter::All && FinderStatus != EStatusFilter::All && FinderStatus != Filter; },
				[this, Filter]() {
					FinderStatus = Filter;
					ApplyFinderFilter();
				}
			)
		];
		// clang-format on
	}
}

void SPjcTabInspector::RebuildFinder() {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	FinderAll.Reset(Result.Assets.Num());
	for (const FAssetData& Asset : Result.Assets) {
		const PjcWidgets::FAssetType& Type = PjcWidgets::GetAssetType(Asset);
		const FRowPtr Row = MakeShared<FPjcInspectorRow>();
		Row->Package = Asset.PackageName;
		Row->Name = Asset.AssetName.ToString();
		Row->Path = Asset.PackagePath.ToString();
		Row->ClassName = PjcShim::GetAssetClassName(Asset);
		Row->ClassLabel = Type.Label;
		Row->Icon = Type.Icon;
		Row->IconColor = Type.Color;
		Row->Status = Result.GetStatus(Asset.PackageName);
		FinderAll.Add(Row);
	}
	FinderAll.Sort([](const FRowPtr& A, const FRowPtr& B) {
		const int32 Compare = A->Name.Compare(B->Name, ESearchCase::IgnoreCase);
		return Compare != 0 ? Compare < 0 : A->Path < B->Path;
	});
	ApplyFinderFilter();
}

void SPjcTabInspector::ApplyFinderFilter() {
	// status and search first: the class bar counts what they leave
	FinderBase.Reset();
	TMap<FName, FPjcClassBarItem> Classes;
	TMap<EStatusFilter, int32> Counts;
	for (const FRowPtr& Row : FinderAll) {
		if (!FinderText.IsEmpty() && !Row->Name.Contains(FinderText) && !Row->Path.Contains(FinderText) && !Row->ClassLabel.Contains(FinderText))
			continue;

		const EStatusFilter RowFilter = Row->Status == EPjcAssetStatus::Used ? EStatusFilter::Used
			: Row->Status == EPjcAssetStatus::EditorOnly					 ? EStatusFilter::EditorOnly
			: Row->Status == EPjcAssetStatus::Review						 ? EStatusFilter::Review
																			 : EStatusFilter::Unused;
		++Counts.FindOrAdd(EStatusFilter::All);
		++Counts.FindOrAdd(RowFilter);
		if (FinderStatus != EStatusFilter::All && FinderStatus != RowFilter) continue;

		FinderBase.Add(Row);
		FPjcClassBarItem& Item = Classes.FindOrAdd(Row->ClassName);
		Item.ClassName = Row->ClassName;
		Item.Label = Row->ClassLabel;
		Item.Color = Row->IconColor;
		Item.ToolTip = Row->ClassName.ToString();
		++Item.Num;
	}
	if (FinderClasses.IsValid()) {
		TArray<FPjcClassBarItem> Items;
		Classes.GenerateValueArray(Items);
		Items.Sort([](const FPjcClassBarItem& A, const FPjcClassBarItem& B) {
			return A.Num != B.Num ? A.Num > B.Num : A.Label < B.Label;
		});
		FinderClasses->SetItems(Items);
	}
	RebuildFinderPills(Counts);
	FilterFinderRows();
}

void SPjcTabInspector::FilterFinderRows() {
	FinderRows.Reset();
	for (const FRowPtr& Row : FinderBase) {
		if (FinderClasses.IsValid() && !FinderClasses->IsShown(Row->ClassName)) continue;

		FinderRows.Add(Row);
	}
	if (FinderList.IsValid()) {
		FinderList->RequestListRefresh();
		const FRowPtr* Row = FinderRows.FindByPredicate([this](const FRowPtr& Existing) {
			return Existing->Package == Focus;
		});
		if (Row) {
			FinderList->SetSelection(*Row, ESelectInfo::Direct);
		}
	}
}

TSharedRef<ITableRow> SPjcTabInspector::OnGenerateFinderRow(FRowPtr Row, const TSharedRef<STableViewBase>& Table) {
	// one line as in the Assets list: class stripe, class icon, name; the status only when it is not Used
	const FString Size = PjcFormat::Size(UPjcSubsystem::GetScanResult().GetSize(Row->Package));
	const FString More = Row->ClassLabel + TEXT(" \u00B7 ") + Size + TEXT(" \u00B7 ") + PjcInspectorStatusText(Row->Status, true);
	// clang-format off
	return SNew(STableRow<FRowPtr>, Table)
#if ENGINE_MAJOR_VERSION < 5
		.Style(&PjcShim::GetRowStyle427())
#endif
		.Padding(FMargin {4.0f, 3.0f, 8.0f, 3.0f})
		.ToolTip(PjcWidgets::MakeToolTip(Row->Name, Row->Package.ToString(), More))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 6.0f, 0.0f})
			[
				SNew(SBox).WidthOverride(3.0f).HeightOverride(16.0f)
				[
					SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(Row->IconColor)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 6.0f, 0.0f})
			[
				SNew(SBox).WidthOverride(16.0f).HeightOverride(16.0f)
				[
					SNew(SImage).Image(Row->Icon)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Row->Name))
				.HighlightText_Lambda([this]() { return FText::FromString(FinderText); })
				.Clipping(EWidgetClipping::ClipToBounds)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock)
				.Visibility(Row->Status == EPjcAssetStatus::Used ? EVisibility::Collapsed : EVisibility::Visible)
				.Text(FText::FromString(PjcInspectorStatusText(Row->Status, true)))
				.Font(FPjcStyles::GetFont("Regular", 8))
				.ColorAndOpacity(PjcInspectorStatusColor(Row->Status, true))
			]
		];
	// clang-format on
}

// the asset and the answer

TSharedRef<SWidget> SPjcTabInspector::MakeMain() {
	// clang-format off
	TreeView = SNew(STreeView<FItemPtr>)
		.TreeItemsSource(&TreeItems)
		.SelectionMode(ESelectionMode::Single)
		.OnGenerateRow(this, &SPjcTabInspector::OnGenerateTreeRow)
		.OnGetChildren_Lambda([this](FItemPtr Item, TArray<FItemPtr>& OutChildren) {
			BuildChildren(Item);
			OutChildren = Item->Children;
		})
		.OnExpansionChanged(this, &SPjcTabInspector::OnExpansionChanged)
		.OnContextMenuOpening(this, &SPjcTabInspector::OnTreeContextMenu)
		.OnMouseButtonDoubleClick(this, &SPjcTabInspector::OnTreeDoubleClick)
		.HeaderRow(
			SNew(SHeaderRow)
			+ SHeaderRow::Column(PjcInspectorColAsset).FillWidth(0.42f)
			.HeaderContent()
			[
				SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(bUses ? TEXT("Uses") : TEXT("Used by")); })
			]
			+ SHeaderRow::Column(PjcInspectorColKind).FixedWidth(86.0f).DefaultLabel(FText::FromString(TEXT("Kind")))
			.DefaultTooltip(FText::FromString(TEXT("Hard: loads with the asset. Soft: loaded on demand by path. Editor only: not cooked")))
			+ SHeaderRow::Column(PjcInspectorColWhere).FillWidth(0.4f)
			.HeaderContent()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Where")))
					.ToolTipText_Lambda([this]() {
						return FText::FromString(bUses ? TEXT("Where the asset one level up holds each one") : TEXT("Where each asset holds the one it opens from"));
					})
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {8.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
					.ButtonStyle(&PjcShim::GetButtonStyle427(true))
					.ForegroundColor(FSlateColor::UseForeground())
#endif
					.ButtonColorAndOpacity(PjcInspectorColor("ProjectCleaner.Color.Accent"))
					.ContentPadding(FMargin {8.0f, 0.0f})
					.Visibility_Lambda([this]() { return NeedRead.Num() > 0 ? EVisibility::Visible : EVisibility::Collapsed; })
					.ToolTip(PjcWidgets::MakeToolTip(
						TEXT("Show where"),
						TEXT("The asset registry knows which assets use which, not where. Loads the assets of the rows on screen to read where they hold their references. Levels are not loaded."),
						TEXT("What is read stays until the next scan")))
					.OnClicked_Lambda([this]() {
						ShowWhere(true);
						return FReply::Handled();
					})
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("Show"))).Font(FPjcStyles::GetFont("Regular", 8)).ColorAndOpacity(FLinearColor::White)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {8.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(STextBlock)
					.Visibility_Lambda([this]() { return NeedRead.Num() == 0 && LoadedHere.Num() > 0 ? EVisibility::Visible : EVisibility::Collapsed; })
					.Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("● %d loaded"), LoadedHere.Num())); })
					.ToolTipText(FText::FromString(TEXT("The Inspector loaded these assets to read where")))
					.Font(FPjcStyles::GetFont("Regular", 8))
					.ColorAndOpacity(PjcInspectorColor("ProjectCleaner.Color.Yellow"))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
					.ButtonStyle(&PjcShim::GetButtonStyle427(false))
					.ForegroundColor(FSlateColor::UseForeground())
#endif
					.ContentPadding(FMargin {8.0f, 0.0f})
					.Visibility_Lambda([this]() { return NeedRead.Num() == 0 && LoadedHere.Num() > 0 ? EVisibility::Visible : EVisibility::Collapsed; })
					.ToolTipText(FText::FromString(TEXT("Lets go of the assets the Inspector loaded. What was read stays until the next scan")))
					.OnClicked_Lambda([this]() {
						UnloadRead();
						return FReply::Handled();
					})
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("Unload"))).Font(FPjcStyles::GetFont("Regular", 8))
					]
				]
			]
			+ SHeaderRow::Column(PjcInspectorColMeta).FixedWidth(150.0f).HAlignHeader(HAlign_Right).HAlignCell(HAlign_Right)
			.HeaderContent()
			[
				SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(bUses ? TEXT("Size") : TEXT("To a root")); })
			]
			+ SHeaderRow::Column(PjcInspectorColGo).FixedWidth(30.0f).DefaultLabel(FText::GetEmpty())
		);

	return SNew(SBorder).BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.GroupBorder"))).Padding(0.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SAssignNew(HeadBox, SBox)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {12.0f, 8.0f, 12.0f, 4.0f})
			[
				SNew(SHorizontalBox)
				.Visibility_Lambda([this]() { return FocusNode != INDEX_NONE ? EVisibility::Visible : EVisibility::Collapsed; })
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(280.0f)
					[
						SAssignNew(TreeSearchBox, SSearchBox)
						.Style(&PjcShim::GetSearchBoxStyle())
						.HintText(FText::FromString(TEXT("Search all levels: name or property")))
						.OnTextChanged_Lambda([this](const FText& Text) {
							TreeSearch = Text.ToString().TrimStartAndEnd();
							RebuildTree();
						})
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(FMargin {12.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { return FText::FromString(GetStatsText()); })
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					.Font(FPjcStyles::GetFont("Regular", 8))
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {12.0f, 0.0f, 12.0f, 4.0f})
			[
				SAssignNew(ChipsBox, SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D {4.0f, 4.0f})
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(FMargin {12.0f, 0.0f, 12.0f, 10.0f})
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SBorder).BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.DarkGroupBorder"))).Padding(0.0f)
					.Visibility_Lambda([this]() { return TreeItems.Num() > 0 ? EVisibility::Visible : EVisibility::Collapsed; })
					[
						TreeView.ToSharedRef()
					]
				]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(FMargin {0.0f, 24.0f})
				[
					SNew(STextBlock)
					.Visibility_Lambda([this]() { return TreeItems.Num() == 0 && FocusNode != INDEX_NONE ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
					.Text_Lambda([this]() { return FText::FromString(GetEmptyText()); })
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			]
		];
	// clang-format on
}

void SPjcTabInspector::RebuildHead() {
	if (!HeadBox.IsValid()) return;

	HeadBox->SetContent(MakeHead());
}

void SPjcTabInspector::ComputeAnswer() {
	Routes.Reset();
	KeyRoutes.Reset();
	Retained.Reset();
	RetainedSet.Reset();
	Choke.Reset();
	ReachUp = {};
	ReachDown = {};
	if (FocusNode == INDEX_NONE) return;

	if (Model.bUsed[FocusNode]) {
		Routes = Model.GetRoutes(FocusNode, Kinds, Prefer);
		KeyRoutes = Model.GetKeyRoutes(Routes);
		Retained = Model.GetRetained(FocusNode, Kinds);
		RetainedSet.Append(Retained);
		Choke = Model.GetChoke(FocusNode, Kinds);
	}
	ReachUp = Model.GetReach(FocusNode, true, Kinds);
	ReachDown = Model.GetReach(FocusNode, false, Kinds);
}

const FPjcRoute* SPjcTabInspector::GetPickedRoute() const {
	const FPjcRoute* Picked = Routes.FindByPredicate([this](const FPjcRoute& Route) {
		return Route.Root == RouteRoot;
	});
	return Picked ? Picked : (Routes.Num() > 0 ? &Routes[0] : nullptr);
}

FString SPjcTabInspector::GetEmptyText() const {
	if (FocusNode == INDEX_NONE) return FString {};
	if (IsTreeSearch()) return TEXT("Nothing matches over all levels");
	if (bUses) return TEXT("It uses no other asset");
	return Model.bRoot[FocusNode] ? TEXT("Nothing uses it: it is a root, kept for its own reason") : TEXT("Nothing uses it");
}

TSharedRef<SWidget> SPjcTabInspector::MakeHead() {
	if (Focus.IsNone()) {
		// clang-format off
		return SNew(SBox).Padding(FMargin {16.0f, 40.0f}).HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Pick an asset on the left: why it is used, what uses it and what it uses")))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			];
		// clang-format on
	}

	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	const FAssetData Asset = FocusNode != INDEX_NONE ? Model.Assets[FocusNode] : FAssetData {};
	const PjcWidgets::FAssetType& Type = PjcWidgets::GetAssetType(Asset);
	const bool bScanned = Result.Usage.Contains(Focus);
	const EPjcAssetStatus Status = Result.GetStatus(Focus);
	const FName StatusColor = PjcInspectorStatusColorName(Status, bScanned);
	const TCHAR* Quiet = !bScanned				? TEXT("ProjectCleaner.Color.Hover")
		: Status == EPjcAssetStatus::Unused		? TEXT("ProjectCleaner.Color.DangerQuiet")
		: Status == EPjcAssetStatus::EditorOnly ? TEXT("ProjectCleaner.Color.Marked")
		: Status == EPjcAssetStatus::Review		? TEXT("ProjectCleaner.Color.ReviewQuiet")
												: TEXT("ProjectCleaner.Color.UsedQuiet");
	const FString Path = Focus.ToString();

	// back, forward, and the assets inspected before
	const TSharedRef<SHorizontalBox> Trail = SNew(SHorizontalBox);
	const auto NavButton = [this](const FName Brush, const TCHAR* ToolTip, TFunction<bool()> Enabled, TFunction<void()> OnClick) {
		// clang-format off
		return SNew(SButton)
			.ButtonStyle(&FPjcStyles::Get().GetWidgetStyle<FButtonStyle>("ProjectCleaner.Button.Row"))
#if ENGINE_MAJOR_VERSION < 5
			.ForegroundColor(FSlateColor::UseForeground())
#endif
			.ContentPadding(FMargin {4.0f, 3.0f})
			.ToolTipText(FText::FromString(ToolTip))
			.IsEnabled_Lambda([Enabled]() { return Enabled(); })
			.OnClicked_Lambda([OnClick]() { OnClick(); return FReply::Handled(); })
			[
				SNew(SBox).WidthOverride(14.0f).HeightOverride(14.0f)
				[
					SNew(SImage).Image(FPjcStyles::Get().GetBrush(Brush))
				]
			];
		// clang-format on
	};
	Trail->AddSlot().AutoWidth().VAlign(VAlign_Center)[NavButton(
		"ProjectCleaner.Icon.Back",
		TEXT("Back (Alt+Left, Backspace)"),
		[this]() {
			return HistoryBack.Num() > 0;
		},
		[this]() {
			Back();
		}
	)];
	Trail->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(FMargin {2.0f, 0.0f, 8.0f, 0.0f})[NavButton(
			"ProjectCleaner.Icon.Forward",
			TEXT("Forward (Alt+Right)"),
			[this]() {
				return HistoryForward.Num() > 0;
			},
			[this]() {
				Forward();
			}
		)];
	for (int32 Index = FMath::Max(0, HistoryBack.Num() - 4); Index < HistoryBack.Num(); ++Index) {
		const FName Earlier = HistoryBack[Index];
		// clang-format off
		Trail->AddSlot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SButton)
			.ButtonStyle(&FPjcStyles::Get().GetWidgetStyle<FButtonStyle>("ProjectCleaner.Button.Row"))
#if ENGINE_MAJOR_VERSION < 5
			.ForegroundColor(FSlateColor::UseForeground())
#endif
			.ContentPadding(FMargin {2.0f, 0.0f})
			.ToolTipText(FText::FromString(TEXT("Inspect ") + Earlier.ToString()))
			.OnClicked_Lambda([this, Earlier]() { SetFocus(Earlier, true); return FReply::Handled(); })
			[
				SNew(STextBlock).Text(FText::FromString(PjcInspectorShortName(Earlier))).Font(FPjcStyles::GetFont("Regular", 8)).ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		];
		Trail->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {3.0f, 0.0f})
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("›"))).Font(FPjcStyles::GetFont("Regular", 8)).ColorAndOpacity(FSlateColor::UseSubduedForeground())
		];
		// clang-format on
	}
	if (HistoryBack.Num() > 0) {
		Trail->AddSlot().AutoWidth().VAlign(
			VAlign_Center
		)[SNew(STextBlock).Text(FText::FromString(PjcInspectorShortName(Focus))).Font(FPjcStyles::GetFont("Regular", 8))];
	}

	// clang-format off
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {10.0f, 6.0f, 12.0f, 0.0f})
		[
			Trail
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {14.0f, 4.0f, 14.0f, 0.0f})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 8.0f, 0.0f})
			[
				SNew(SBox).WidthOverride(20.0f).HeightOverride(20.0f)
				[
					SNew(SImage).Image(Type.Icon)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(PjcInspectorShortName(Focus))).Font(FPjcStyles::GetFont("Bold", 12)).ColorAndOpacity(PjcInspectorColor("ProjectCleaner.Color.Strong"))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {10.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(PjcInspectorColor(Quiet))
				.Padding(FMargin {7.0f, 2.0f})
				.ToolTip(PjcWidgets::MakeToolTip(PjcInspectorStatusText(Status, bScanned), GetBasisText()))
				[
					SNew(STextBlock)
					.Text(FText::FromString(PjcInspectorStatusText(Status, bScanned)))
					.Font(FPjcStyles::GetFont("Bold", 8))
					.ColorAndOpacity(PjcInspectorColor(StatusColor))
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(PjcFormat::Size(Result.GetSize(Focus))))
				.ToolTipText(FText::FromString(TEXT("Size on disk")))
				.Font(PjcWidgets::GetFontMono(9))
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {42.0f, 2.0f, 14.0f, 6.0f})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(Type.Label + TEXT("  ·  "))).Font(FPjcStyles::GetFont("Regular", 8)).ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonStyle(&FPjcStyles::Get().GetWidgetStyle<FButtonStyle>("ProjectCleaner.Button.Row"))
#if ENGINE_MAJOR_VERSION < 5
				.ForegroundColor(FSlateColor::UseForeground())
#endif
				.ContentPadding(FMargin {0.0f})
				.ToolTipText(FText::FromString(TEXT("Click to copy the path (Ctrl+C)")))
				.OnClicked_Lambda([Path]() { PjcInspectorCopy(Path, TEXT("the path")); return FReply::Handled(); })
				[
					SNew(STextBlock).Text(FText::FromString(Path)).Font(FPjcStyles::GetFont("Regular", 8)).ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {12.0f, 2.0f, 12.0f, 8.0f})
		[
			MakeToolbar()
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			PjcInspectorLine(false)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {14.0f, 8.0f, 14.0f, 10.0f})
		[
			MakeAnswer()
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			PjcInspectorLine(false)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {8.0f, 0.0f, 12.0f, 0.0f})
		[
			MakeTabs()
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			PjcInspectorLine(false)
		];
	// clang-format on
}

FString SPjcTabInspector::GetBasisText() const {
	TArray<FString> KindNames;
	if (Kinds & PjcLinkKinds::Hard) KindNames.Add(TEXT("hard"));
	if (Kinds & PjcLinkKinds::Soft) KindNames.Add(TEXT("soft"));
	if (Kinds & PjcLinkKinds::EditorOnly) KindNames.Add(TEXT("editor only"));
	int32 Named = 0;
	for (const FPjcRoute& Route : Routes) {
		if (Model.Reasons[Route.Root] == EPjcUsageReason::Indirect) ++Named;
	}
	FString Text = TEXT("Based on saved asset data · ") + FString::Join(KindNames, TEXT(" + ")) + TEXT(" references");
	if (Named > 0) Text += FString::Printf(TEXT(" · %s named in config or source, counted as used"), *PjcInspectorPlural(Named, TEXT("root")));
	return Text;
}

TSharedRef<SWidget> SPjcTabInspector::MakeToolbar() {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	const bool bScanned = Result.Usage.Contains(Focus);
	const bool bDeletable = bScanned && (!Result.IsUsed(Focus) || Result.GetStatus(Focus) == EPjcAssetStatus::Review);
	const FAssetData Asset = FocusNode != INDEX_NONE ? Model.Assets[FocusNode] : FAssetData {};
	const FSlateBrush* ClassIcon = PjcWidgets::GetAssetType(Asset).Icon;

	// clang-format off
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 4.0f, 0.0f})
		[
			PjcInspectorToolButton("ProjectCleaner.Menu.Delete", TEXT("Delete…"),
				bDeletable ? TEXT("Deletes it with the unused assets only it keeps, after asking") : TEXT("Only unused assets and assets to review are deleted here"),
				true, [bDeletable]() { return bDeletable; }, [this]() { DeleteFocus(); })
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 4.0f, 0.0f})
		[
			SNew(SComboButton)
			.ComboButtonStyle(&PjcShim::GetComboButtonStyle())
#if ENGINE_MAJOR_VERSION < 5
			.ForegroundColor(FSlateColor::UseForeground())
#endif
			.IsEnabled(bScanned)
			.ContentPadding(FMargin {6.0f, 1.0f})
			.ToolTipText(FText::FromString(TEXT("Counts it as used from now on, saved to the project settings")))
			.OnGetMenuContent(this, &SPjcTabInspector::MakeExcludeMenu)
			.ButtonContent()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 6.0f, 0.0f})
				[
					SNew(SBox).WidthOverride(14.0f).HeightOverride(14.0f)
					[
						SNew(SImage).Image(FPjcStyles::Get().GetBrush("ProjectCleaner.Menu.Exclude"))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("Exclude")))
				]
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin {4.0f, 3.0f})
		[
			PjcInspectorLine(true)
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {4.0f, 0.0f, 4.0f, 0.0f})
		[
			PjcInspectorToolButton(NAME_None, TEXT("Open"), TEXT("Opens it in its editor"), false,
				[Asset]() { return Asset.IsValid(); }, [Asset]() { UPjcSubsystem::OpenAssetEditor(Asset); }, ClassIcon)
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 4.0f, 0.0f})
		[
			PjcInspectorToolButton("ProjectCleaner.Menu.Browse", TEXT("Browse"), TEXT("Shows it in the Content Browser (Ctrl+B)"), false,
				[Asset]() { return Asset.IsValid(); }, [Asset]() { PjcInspectorBrowse({Asset}); })
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 4.0f, 0.0f})
		[
			PjcInspectorToolButton("ProjectCleaner.Menu.SizeMap", TEXT("Size map"), TEXT("Shows it and what it pulls in, by size"), false,
				[Asset]() { return Asset.IsValid(); }, [Asset]() { UPjcSubsystem::OpenSizeMapViewer({Asset}); })
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Saved asset data")))
			.ToolTip(PjcWidgets::MakeToolTip(TEXT("Where the answer comes from"), GetBasisText()))
			.Font(FPjcStyles::GetFont("Regular", 8))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		];
	// clang-format on
}

TSharedRef<SWidget> SPjcTabInspector::MakeChip(const int32 Node) {
	if (!Model.Packages.IsValidIndex(Node)) return SNullWidget::NullWidget;

	const bool bMe = Node == FocusNode;
	const FAssetData& Asset = Model.Assets[Node];
	const PjcWidgets::FAssetType& Type = PjcWidgets::GetAssetType(Asset);
	const FName Package = Model.Packages[Node];
	const FString Name = bMe ? FString {TEXT("this")} : PjcInspectorShortName(Package);
	const FString More = Type.Label + TEXT(" · ") + PjcFormat::Size(Model.Sizes[Node]) + (bMe ? TEXT("") : TEXT("\nClick: inspect it"));
	// clang-format off
	return SNew(SButton)
		.ButtonStyle(&PjcShim::GetStyle().GetWidgetStyle<FButtonStyle>(TEXT("NoBorder")))
#if ENGINE_MAJOR_VERSION < 5
		.ForegroundColor(FSlateColor::UseForeground())
#endif
		.ContentPadding(FMargin {0.0f})
		.ToolTip(PjcWidgets::MakeToolTip(PjcInspectorShortName(Package), Package.ToString(), More))
		.IsEnabled(!bMe)
		.OnClicked_Lambda([this, Package]() { SetFocus(Package, true); return FReply::Handled(); })
		[
			SNew(SBorder)
			.BorderImage(FPjcStyles::Get().GetBrush("ProjectCleaner.Pill"))
			.BorderBackgroundColor(PjcInspectorColor(bMe ? "ProjectCleaner.Color.Marked" : "ProjectCleaner.Color.Hover"))
			.Padding(FMargin {5.0f, 1.0f, 7.0f, 1.0f})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 5.0f, 0.0f})
				[
					SNew(SBox).WidthOverride(14.0f).HeightOverride(14.0f)
					[
						SNew(SImage).Image(Type.Icon)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Name))
					.Font(FPjcStyles::GetFont("Regular", 9))
					.ColorAndOpacity(PjcInspectorColor(bMe ? "ProjectCleaner.Color.Strong" : "ProjectCleaner.Color.Text"))
				]
			]
		];
	// clang-format on
}

TSharedRef<SWidget> SPjcTabInspector::MakeAnswer() {
	const TSharedRef<SVerticalBox> Facts = SNew(SVerticalBox);
	const auto Fact = [&Facts](const FString& Caption, const TSharedRef<SWrapBox>& Value, const FString& ToolTip = FString {}) {
		// clang-format off
		Facts->AddSlot().AutoHeight().Padding(FMargin {0.0f, 2.0f})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(FMargin {0.0f, 4.0f, 0.0f, 0.0f})
			[
				SNew(SBox).WidthOverride(96.0f)
				[
					PjcInspectorCaption(Caption, ToolTip)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				Value
			]
		];
		// clang-format on
	};
	const auto Line = []() {
		return SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D {4.0f, 4.0f});
	};
	const auto Add = [](const TSharedRef<SWrapBox>& Box, const TSharedRef<SWidget>& Widget) {
		Box->AddSlot().VAlign(VAlign_Center)[Widget];
	};
	const auto Separator = []() {
		return PjcInspectorText(TEXT("›"), TEXT("ProjectCleaner.Color.Muted"));
	};
	const auto ShowButton = [this](const int32 Num) -> TSharedRef<SWidget> {
		if (Num <= 0) return SNullWidget::NullWidget;
		// clang-format off
		return SNew(SButton)
			.ButtonStyle(&FPjcStyles::Get().GetWidgetStyle<FButtonStyle>("ProjectCleaner.Button.Row"))
#if ENGINE_MAJOR_VERSION < 5
			.ForegroundColor(FSlateColor::UseForeground())
#endif
			.ContentPadding(FMargin {4.0f, 0.0f})
			.OnClicked_Lambda([this]() { bDeleteList = !bDeleteList; RebuildHead(); return FReply::Handled(); })
			[
				SNew(STextBlock).Text(FText::FromString(bDeleteList ? TEXT("Hide") : TEXT("Show"))).ColorAndOpacity(PjcInspectorColor("ProjectCleaner.Color.Link"))
			];
		// clang-format on
	};

	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	TArray<int32> GoesWith;
	if (FocusNode == INDEX_NONE || !Model.bScanned[FocusNode]) {
		const TSharedRef<SWrapBox> Why = Line();
		Add(
			Why,
			PjcInspectorText(TEXT("Not scanned content: the scan does not look inside it, and never deletes it"), TEXT("ProjectCleaner.Color.Muted"))
		);
		Fact(TEXT("WHY USED"), Why);
	}
	else if (!Model.bUsed[FocusNode]) {
		TArray<int32> Users;
		for (const FPjcModelLink& Link : Model.UsedBy[FocusNode]) {
			Users.AddUnique(Link.Other);
		}
		const TSharedRef<SWrapBox> Why = Line();
		if (Users.Num() == 0) {
			Add(Why, PjcInspectorText(TEXT("Nothing in the project uses it")));
		}
		else {
			Add(Why, PjcInspectorText(TEXT("Only unused assets use it")));
			if (Users.Num() <= 3) {
				for (const int32 User : Users) Add(Why, MakeChip(User));
			}
			else {
				Add(Why, PjcInspectorText(TEXT("·")));
				Add(Why, PjcInspectorNumber(PjcFormat::Count(Users.Num())));
				Add(Why, PjcInspectorText(TEXT("of them")));
			}
		}
		Fact(TEXT("WHY UNUSED"), Why);

		// what Delete takes with it
		TArray<FName> Group;
		FPjcUnusedTree::GetDeleteGroup(Result, {Focus}, Group);
		int64 Size = Result.GetSize(Focus);
		for (const FName Other : Group) {
			Size += Result.GetSize(Other);
			const int32 Node = Model.Find(Other);
			if (Node != INDEX_NONE) GoesWith.Add(Node);
		}
		const TSharedRef<SWrapBox> Deleted = Line();
		if (Group.Num() > 0) {
			Add(Deleted, PjcInspectorText(TEXT("Goes together with")));
			Add(Deleted, PjcInspectorNumber(PjcFormat::Count(Group.Num())));
			Add(Deleted, PjcInspectorText(Group.Num() == 1 ? TEXT("other unused asset · frees") : TEXT("other unused assets · frees")));
		}
		else {
			Add(Deleted, PjcInspectorText(TEXT("Goes alone · frees")));
		}
		Add(Deleted, PjcInspectorNumber(PjcFormat::Size(Size)));
		Add(Deleted, ShowButton(Group.Num()));
		Fact(TEXT("IF DELETED"), Deleted, TEXT("What Delete takes with it: the unused assets only it keeps"));
		if (const FString* Holder = UPjcSubsystem::GetLastCleanupKept().Find(Focus)) {
			const TSharedRef<SWrapBox> Failed = Line();
			Add(Failed, PjcInspectorText(TEXT("The last delete could not remove it: ") + *Holder, TEXT("ProjectCleaner.Color.Yellow")));
			Fact(TEXT("LAST DELETE"), Failed);
		}
	}
	else {
		const FPjcRoute* Route = GetPickedRoute();
		const int32 NumRoutes = Routes.Num();
		const TSharedRef<SWrapBox> Why = Line();
		if (!Route) {
			Add(Why, PjcInspectorText(TEXT("No root reaches it through the reference kinds in scope"), TEXT("ProjectCleaner.Color.Muted")));
		}
		else if (Route->Steps == 0) {
			const FPjcAssetUsage* Usage = Result.Usage.Find(Focus);
			Add(Why, PjcInspectorText(TEXT("It is a root · ") + (Usage ? UPjcSubsystem::GetUsageReasonText(*Usage) : FString {})));
		}
		else {
			const TArray<int32>& Path = Route->Path;
			const bool bLong = Path.Num() > PjcInspectorMaxChain;
			for (int32 Index = 0; Index < Path.Num(); ++Index) {
				if (bLong && Index >= 2 && Index < Path.Num() - 3) {
					if (Index == 2) {
						TArray<FString> Hidden;
						for (int32 Step = 2; Step < Path.Num() - 3; ++Step) Hidden.Add(PjcInspectorShortName(Model.Packages[Path[Step]]));
						// clang-format off
						Add(Why, SNew(SBorder)
							.BorderImage(FPjcStyles::Get().GetBrush("ProjectCleaner.Pill"))
							.BorderBackgroundColor(PjcInspectorColor("ProjectCleaner.Color.Hover"))
							.Padding(FMargin {7.0f, 1.0f})
							.ToolTip(PjcWidgets::MakeToolTip(TEXT("Steps not shown"), FString::Join(Hidden, TEXT("\n")), TEXT("Used by shows them all")))
							[
								SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("+%d"), Path.Num() - 5))).ColorAndOpacity(FSlateColor::UseSubduedForeground())
							]);
						// clang-format on
						Add(Why, Separator());
					}
					continue;
				}
				Add(Why, MakeChip(Path[Index]));
				if (Index < Path.Num() - 1) Add(Why, Separator());
			}
			if (Route->bEditor) Add(Why, PjcInspectorText(TEXT("editor only"), TEXT("ProjectCleaner.Color.Editor"), true));
		}
		if (NumRoutes > 1) {
			const int32 Picked = Routes.IndexOfByPredicate([Route](const FPjcRoute& Existing) {
				return Route && Existing.Root == Route->Root;
			});
			// clang-format off
			Add(Why, SNew(SComboButton)
				.ComboButtonStyle(&PjcShim::GetComboButtonStyle())
#if ENGINE_MAJOR_VERSION < 5
				.ForegroundColor(FSlateColor::UseForeground())
#endif
				.ContentPadding(FMargin {6.0f, 0.0f})
				.ToolTipText(FText::FromString(TEXT("The other roots that keep it, and the routes that differ most")))
				.OnGetMenuContent(this, &SPjcTabInspector::MakeRouteMenu)
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("Route %s of %s"), *PjcFormat::Count(Picked + 1), *PjcFormat::Count(NumRoutes))))
					.Font(FPjcStyles::GetFont("Regular", 8))
				]);
			// clang-format on
		}
		Fact(TEXT("WHY USED"), Why);

		if (NumRoutes > 0 && !(NumRoutes == 1 && Routes[0].Steps == 0)) {
			const TSharedRef<SWrapBox> Kept = Line();
			if (NumRoutes == 1) {
				Add(Kept, PjcInspectorText(TEXT("1 root · ") + PjcWidgets::GetAssetType(Model.Assets[Routes[0].Root]).Label));
			}
			else {
				TMap<FString, int32> ByClass;
				for (const FPjcRoute& Each : Routes) {
					++ByClass.FindOrAdd(PjcWidgets::GetAssetType(Model.Assets[Each.Root]).Label);
				}
				ByClass.ValueSort([](const int32 A, const int32 B) {
					return A > B;
				});
				TArray<FString> Parts;
				for (const auto& Pair : ByClass) {
					if (Parts.Num() == 3) {
						Parts.Add(FString::Printf(TEXT("+%d kinds"), ByClass.Num() - 3));
						break;
					}
					Parts.Add(PjcFormat::Count(Pair.Value) + TEXT(" ") + PjcInspectorClassPlural(Pair.Value, Pair.Key));
				}
				Add(Kept, PjcInspectorNumber(PjcFormat::Count(NumRoutes)));
				Add(Kept, PjcInspectorText(TEXT("roots · ") + FString::Join(Parts, TEXT(" · "))));
			}
			if (!Model.bRoot[FocusNode]) {
				if (Choke.Num() > 0) {
					Add(Kept, PjcInspectorText(TEXT("· every route passes")));
					Add(Kept, MakeChip(Choke[0]));
				}
				else if (NumRoutes > 1) {
					Add(Kept, PjcInspectorText(TEXT("· no single asset on every route"), TEXT("ProjectCleaner.Color.Muted")));
				}
			}
			Fact(TEXT("KEPT BY"), Kept, TEXT("The roots that keep it used, and the one asset every route passes"));
		}

		// the used assets that use it, when that says more than the line above
		if (!Model.bRoot[FocusNode]) {
			TArray<int32> Users;
			for (const FPjcModelLink& Link : Model.UsedBy[FocusNode]) {
				if (FPjcInspectorModel::Counts(Kinds, Link) && Model.GetDepth(Link.Other, Kinds) != INDEX_NONE) Users.AddUnique(Link.Other);
			}
			if (Users.Num() > 0 && Users.Num() <= 3 && !(Users.Num() == 1 && Choke.Num() > 0 && Choke[0] == Users[0])) {
				const TSharedRef<SWrapBox> Free = Line();
				Add(Free, PjcInspectorText(TEXT("Remove the references in")));
				for (const int32 User : Users) Add(Free, MakeChip(User));
				Fact(TEXT("TO FREE IT"), Free, TEXT("What to change so nothing keeps it"));
			}
		}

		if (Retained.Num() > 0) {
			const int32 Others = Retained.Num() - 1;
			int64 Size = 0;
			for (const int32 Node : Retained) Size += Model.Sizes[Node];
			const TSharedRef<SWrapBox> Deleted = Line();
			if (Others > 0) {
				Add(Deleted, PjcInspectorNumber(PjcFormat::Count(Others)));
				Add(Deleted, PjcInspectorText(Others == 1 ? TEXT("other asset becomes unused · frees") : TEXT("other assets become unused · frees")));
			}
			else {
				Add(Deleted, PjcInspectorText(TEXT("Nothing else becomes unused · frees")));
			}
			Add(Deleted, PjcInspectorNumber(PjcFormat::Size(Size)));
			Add(Deleted, ShowButton(Others));
			Fact(TEXT("IF DELETED"), Deleted, TEXT("What goes with it: the assets only it keeps used"));
			for (int32 Index = 1; Index < Retained.Num(); ++Index) GoesWith.Add(Retained[Index]);
		}
	}

	if (FocusNode != INDEX_NONE && Result.GetStatus(Focus) == EPjcAssetStatus::Review) {
		const FPjcAssetUsage* Usage = Result.Usage.Find(Focus);
		const TSharedRef<SWrapBox> Review = Line();
		const FString Text = Usage && FPjcScanResult::IsReviewReason(Usage->Reason) ? UPjcSubsystem::GetUsageReasonText(*Usage)
																					: TEXT("Used only through assets to review");
		Add(Review, PjcInspectorText(Text + TEXT(" · Clean project keeps it: delete or keep it"), TEXT("ProjectCleaner.Color.Review")));
		Fact(TEXT("REVIEW"), Review);
	}

	// what if a user did not use it
	if (WhatIfUser != INDEX_NONE && FocusNode != INDEX_NONE) {
		const FPjcInspectorModel::FWhatIf What = Model.GetWhatIf(FocusNode, WhatIfUser, Kinds);
		const TSharedRef<SWrapBox> If = Line();
		Add(If, PjcInspectorText(TEXT("Without")));
		Add(If, MakeChip(WhatIfUser));
		if (What.bStill) {
			Add(If, PjcInspectorText(TEXT("it is still used via")));
			for (int32 Index = 0; Index < What.Path.Num(); ++Index) {
				Add(If, MakeChip(What.Path[Index]));
				if (Index < What.Path.Num() - 1) Add(If, Separator());
			}
			if (What.bEditor) Add(If, PjcInspectorText(TEXT("editor only"), TEXT("ProjectCleaner.Color.Editor"), true));
		}
		else {
			const int32 Others = FMath::Max(0, What.Num - 1);
			Add(If,
				PjcInspectorText(
					Others > 0 ? FString::Printf(TEXT("it becomes unused, with %s · frees"), *PjcInspectorPlural(Others, TEXT("other asset")))
							   : TEXT("it becomes unused · frees"),
					TEXT("ProjectCleaner.Color.Red")
				));
			Add(If, PjcInspectorNumber(PjcFormat::Size(What.Size)));
		}
		// clang-format off
		Add(If, SNew(SButton)
			.ButtonStyle(&FPjcStyles::Get().GetWidgetStyle<FButtonStyle>("ProjectCleaner.Button.Row"))
#if ENGINE_MAJOR_VERSION < 5
			.ForegroundColor(FSlateColor::UseForeground())
#endif
			.ContentPadding(FMargin {4.0f, 0.0f})
			.OnClicked_Lambda([this]() { WhatIfUser = INDEX_NONE; RebuildHead(); return FReply::Handled(); })
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Close"))).ColorAndOpacity(PjcInspectorColor("ProjectCleaner.Color.Link"))
			]);
		// clang-format on
		Fact(TEXT("WHAT IF"), If, TEXT("What keeps it, or what becomes unused, if that asset did not use it"));
	}

	// who keeps the assets of a group
	if (Keepers.Num() > 0) {
		const TSharedRef<SWrapBox> Keep = Line();
		Add(Keep, PjcInspectorText(KeepersLabel));
		for (int32 Index = 0; Index < Keepers.Num(); Index += 2) {
			const int32 Keeper = Keepers[Index];
			const int32 Num = Keepers[Index + 1];
			if (Keeper >= 0) {
				Add(Keep, MakeChip(Keeper));
				Add(Keep, PjcInspectorText(TEXT("keeps ") + PjcFormat::Count(Num)));
			}
			else {
				const TCHAR* What = Keeper == FPjcInspectorModel::SeveralRoots ? TEXT("several roots, no single keeper")
					: Keeper == -10											   ? TEXT("roots themselves")
																			   : TEXT("nothing: unused");
				Add(Keep, PjcInspectorText(FString::Printf(TEXT("%s %s"), What, *PjcFormat::Count(Num)), TEXT("ProjectCleaner.Color.Muted")));
			}
			if (Index + 2 < Keepers.Num()) Add(Keep, PjcInspectorText(TEXT("·"), TEXT("ProjectCleaner.Color.Muted")));
		}
		// clang-format off
		Add(Keep, SNew(SButton)
			.ButtonStyle(&FPjcStyles::Get().GetWidgetStyle<FButtonStyle>("ProjectCleaner.Button.Row"))
#if ENGINE_MAJOR_VERSION < 5
			.ForegroundColor(FSlateColor::UseForeground())
#endif
			.ContentPadding(FMargin {4.0f, 0.0f})
			.OnClicked_Lambda([this]() { Keepers.Reset(); RebuildHead(); return FReply::Handled(); })
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Close"))).ColorAndOpacity(PjcInspectorColor("ProjectCleaner.Color.Link"))
			]);
		// clang-format on
		Fact(TEXT("KEPT BY"), Keep, TEXT("Which assets keep the assets of the group"));
	}

	if (bDeleteList && GoesWith.Num() > 0) {
		const TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
		for (int32 Index = 0; Index < GoesWith.Num() && Index < PjcInspectorMaxList; ++Index) {
			const int32 Node = GoesWith[Index];
			const FName Package = Model.Packages[Node];
			const PjcWidgets::FAssetType& Type = PjcWidgets::GetAssetType(Model.Assets[Node]);
			// clang-format off
			List->AddSlot().AutoHeight()
			[
				SNew(SButton)
				.ButtonStyle(&FPjcStyles::Get().GetWidgetStyle<FButtonStyle>("ProjectCleaner.Button.Row"))
#if ENGINE_MAJOR_VERSION < 5
				.ForegroundColor(FSlateColor::UseForeground())
#endif
				.ContentPadding(FMargin {8.0f, 2.0f})
				.ToolTipText(FText::FromString(Package.ToString() + TEXT("\nClick: inspect it")))
				.OnClicked_Lambda([this, Package]() { SetFocus(Package, true); return FReply::Handled(); })
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 6.0f, 0.0f})
					[
						SNew(SBox).WidthOverride(16.0f).HeightOverride(16.0f)
						[
							SNew(SImage).Image(Type.Icon)
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(PjcInspectorShortName(Package)))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(PjcFormat::Size(Model.Sizes[Node]))).Font(PjcWidgets::GetFontMono(8)).ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
				]
			];
			// clang-format on
		}
		if (GoesWith.Num() > PjcInspectorMaxList) {
			TArray<FAssetData> All;
			for (const int32 Node : GoesWith) All.Add(Model.Assets[Node]);
			// clang-format off
			List->AddSlot().AutoHeight()
			[
				SNew(SButton)
				.ButtonStyle(&FPjcStyles::Get().GetWidgetStyle<FButtonStyle>("ProjectCleaner.Button.Row"))
#if ENGINE_MAJOR_VERSION < 5
				.ForegroundColor(FSlateColor::UseForeground())
#endif
				.ContentPadding(FMargin {8.0f, 2.0f})
				.ToolTipText(FText::FromString(TEXT("Selects them all in the Content Browser")))
				.OnClicked_Lambda([All]() { PjcInspectorBrowse(All); return FReply::Handled(); })
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("+%s more · Browse to them all"), *PjcFormat::Count(GoesWith.Num() - PjcInspectorMaxList))))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			];
			// clang-format on
		}
		// clang-format off
		Facts->AddSlot().AutoHeight().Padding(FMargin {96.0f, 4.0f, 0.0f, 0.0f})
		[
			SNew(SBox).MaxDesiredWidth(560.0f)
			[
				SNew(SBorder).BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.DarkGroupBorder"))).Padding(FMargin {2.0f})
				[
					List
				]
			]
		];
		// clang-format on
	}
	return Facts;
}

TSharedRef<SWidget> SPjcTabInspector::MakeRouteMenu() {
	FMenuBuilder Menu {true, nullptr};
	const FPjcRoute* Picked = GetPickedRoute();
	const auto Label = [this, Picked](const FPjcRoute& Route, const bool bVia) {
		FString Text = (Picked && Picked->Root == Route.Root ? TEXT("✓ ") : TEXT("")) + PjcInspectorShortName(Model.Packages[Route.Root]);
		Text += TEXT(" · ") + FString {PjcInspectorSourceTitle(Model.Reasons[Route.Root])};
		if (bVia && Route.Path.Num() > 2) Text += TEXT(" · via ") + PjcInspectorShortName(Model.Packages[Route.Path[Route.Path.Num() - 2]]);
		Text += TEXT(" · ") + PjcInspectorSteps(Route.Steps) + (Route.bEditor ? TEXT(" · editor only") : TEXT(""));
		return Text;
	};
	const auto Pick = [this](const int32 Root) {
		RouteRoot = Root;
		Expanded.Reset();
		Closed.Reset();
		bUses = false;
		RebuildAll();
	};

	Menu.BeginSection(NAME_None, FText::FromString(TEXT("Key routes: the top route and the ones that differ most")));
	for (const int32 Index : KeyRoutes) {
		const FPjcRoute& Route = Routes[Index];
		const int32 Root = Route.Root;
		PjcWidgets::AddMenuEntry(Menu, EPjcMenuIcon::None, Label(Route, true), Model.Packages[Root].ToString(), [Pick, Root]() {
			Pick(Root);
		});
	}
	Menu.EndSection();

	const int32 Rest = Routes.Num() - KeyRoutes.Num();
	if (Rest > 0) {
		// every route by the class of its root, each class in a menu of its own
		TMap<FString, TArray<int32>> ByClass;
		for (int32 Index = 0; Index < Routes.Num(); ++Index) {
			ByClass.FindOrAdd(PjcWidgets::GetAssetType(Model.Assets[Routes[Index].Root]).Label).Add(Index);
		}
		ByClass.ValueSort([](const TArray<int32>& A, const TArray<int32>& B) {
			return A.Num() > B.Num();
		});
		Menu.BeginSection(
			NAME_None, FText::FromString(FString::Printf(TEXT("%s · by kind of root"), *PjcInspectorPlural(Routes.Num(), TEXT("route"))))
		);
		for (const auto& Pair : ByClass) {
			const TArray<int32> Indices = Pair.Value;
			const FString Title = PjcFormat::Count(Indices.Num()) + TEXT(" ") + PjcInspectorClassPlural(Indices.Num(), Pair.Key);
			Menu.AddSubMenu(
				FText::FromString(Title), FText::GetEmpty(), FNewMenuDelegate::CreateLambda([this, Indices, Label, Pick](FMenuBuilder& Sub) {
					for (int32 Each = 0; Each < Indices.Num() && Each < PjcInspectorPage; ++Each) {
						const FPjcRoute& Route = Routes[Indices[Each]];
						const int32 Root = Route.Root;
						PjcWidgets::AddMenuEntry(Sub, EPjcMenuIcon::None, Label(Route, true), Model.Packages[Root].ToString(), [Pick, Root]() {
							Pick(Root);
						});
					}
					if (Indices.Num() > PjcInspectorPage) {
						Sub.AddMenuEntry(
							FText::FromString(FString::Printf(TEXT("+%s more"), *PjcFormat::Count(Indices.Num() - PjcInspectorPage))),
							FText::GetEmpty(),
							FSlateIcon {},
							FUIAction {FExecuteAction {}, FCanExecuteAction::CreateLambda([]() {
										   return false;
									   })}
						);
					}
				})
			);
		}
		Menu.EndSection();
	}
	return Menu.MakeWidget();
}

TSharedRef<SWidget> SPjcTabInspector::MakeTabs() {
	const auto Tab = [this](const bool bForUses, const TCHAR* Label, const int32 Num) {
		// clang-format off
		return SNew(SButton)
			.ButtonStyle(&PjcShim::GetStyle().GetWidgetStyle<FButtonStyle>(TEXT("NoBorder")))
#if ENGINE_MAJOR_VERSION < 5
			.ForegroundColor(FSlateColor::UseForeground())
#endif
			.ContentPadding(FMargin {0.0f})
			.OnClicked_Lambda([this, bForUses]() { ShowUses(bForUses); return FReply::Handled(); })
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {10.0f, 7.0f, 10.0f, 5.0f})
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Label))
						.ColorAndOpacity(bUses == bForUses ? FSlateColor {PjcInspectorColor("ProjectCleaner.Color.Strong")} : FSlateColor::UseSubduedForeground())
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})
					[
						SNew(STextBlock).Text(FText::FromString(PjcFormat::Count(Num))).Font(PjcWidgets::GetFontMono(8)).ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBox).HeightOverride(2.0f)
					[
						SNew(SImage)
						.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
						.ColorAndOpacity(bUses == bForUses ? PjcInspectorColor("ProjectCleaner.Color.Accent") : FLinearColor::Transparent)
					]
				]
			];
		// clang-format on
	};
	int32 NumBy = 0;
	int32 NumUses = 0;
	if (FocusNode != INDEX_NONE) {
		for (const FPjcModelLink& Link : Model.UsedBy[FocusNode]) {
			if (FPjcInspectorModel::Counts(Kinds, Link) || Link.Via != EPjcUsageReason::None) ++NumBy;
		}
		for (const FPjcModelLink& Link : Model.Uses[FocusNode]) {
			if (FPjcInspectorModel::Counts(Kinds, Link) || Link.Via != EPjcUsageReason::None) ++NumUses;
		}
	}
	const auto KindChip = [this](const uint8 Bit, const TCHAR* Label, const FName Color, const TCHAR* ToolTip) {
		return PjcInspectorChipToggle(
			Label,
			ToolTip,
			PjcInspectorColor(Color),
			-1,
			[this, Bit]() {
				return (Kinds & Bit) != 0;
			},
			[this, Bit]() {
				if ((Kinds & Bit) != 0 && (Kinds & ~Bit) == 0) {
					UPjcSubsystem::ShowNotification(TEXT("At least one reference kind stays on"), SNotificationItem::CS_None, 2.0f);
					return;
				}
				Kinds ^= Bit;
				WhatIfUser = INDEX_NONE;
				Expanded.Reset();
				Closed.Reset();
				RebuildAll();
			}
		);
	};

	// clang-format off
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)
		[
			Tab(false, TEXT("Used by"), NumBy)
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)
		[
			Tab(true, TEXT("Uses"), NumUses)
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 8.0f, 0.0f})
		[
			PjcInspectorCaption(TEXT("REFERENCES"), TEXT("Which reference kinds count, for every answer on this page"))
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 4.0f, 0.0f})
		[
			KindChip(PjcLinkKinds::Hard, TEXT("Hard"), "ProjectCleaner.Color.Muted", PjcInspectorKindToolTip(EPjcLinkKind::Hard))
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 4.0f, 0.0f})
		[
			KindChip(PjcLinkKinds::Soft, TEXT("Soft"), "ProjectCleaner.Color.SoftLink", PjcInspectorKindToolTip(EPjcLinkKind::Soft))
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			KindChip(PjcLinkKinds::EditorOnly, TEXT("Editor only"), "ProjectCleaner.Color.Editor", PjcInspectorKindToolTip(EPjcLinkKind::EditorOnly))
		];
	// clang-format on
}

// the tree

void SPjcTabInspector::RebuildTree() {
	TreeItems.Reset();
	OpenNodes.Reset();
	if (TreeView.IsValid()) TreeView->ClearExpandedItems();

	SearchResult = FPjcInspectorModel::FSearch {};
	TMap<FString, TPair<int32, FLinearColor>> Classes;
	if (FocusNode != INDEX_NONE) {
		if (IsTreeSearch()) {
			// a name, or where a reference is set when that was read
			SearchResult = Model.Search(FocusNode, !bUses, Kinds, [this](const int32 From, const FPjcModelLink& Link) {
				if (PjcInspectorShortName(Model.Packages[Link.Other]).Contains(TreeSearch)) return true;
				FPjcInspectorItem Probe;
				Probe.Holder = bUses ? From : Link.Other;
				Probe.Target = bUses ? Link.Other : From;
				Probe.Link = Link;
				FString ToolTip;
				FName Color;
				return GetWhereText(Probe, ToolTip, Color).Contains(TreeSearch) || ToolTip.Contains(TreeSearch);
			});
		}
		TArray<int32> RouteRest;
		const FPjcRoute* Route = bUses ? nullptr : GetPickedRoute();
		if (Route) {
			for (int32 Index = Route->Path.Num() - 2; Index >= 0; --Index) RouteRest.Add(Route->Path[Index]);
		}
		TreeItems = MakeLevel(FocusNode, 0, nullptr, Focus.ToString(), Route ? &RouteRest : nullptr);

		for (const FPjcModelLink& Link : bUses ? Model.Uses[FocusNode] : Model.UsedBy[FocusNode]) {
			if (!FPjcInspectorModel::Counts(Kinds, Link) && Link.Via == EPjcUsageReason::None) continue;

			const PjcWidgets::FAssetType& Type = PjcWidgets::GetAssetType(Model.Assets[Link.Other]);
			TPair<int32, FLinearColor>& Entry = Classes.FindOrAdd(Type.Label, TPair<int32, FLinearColor> {0, Type.Color});
			++Entry.Key;
		}
	}

	// class chips over the first level
	if (ChipsBox.IsValid()) {
		ChipsBox->ClearChildren();
		if (Classes.Num() > 1) {
			Classes.ValueSort([](const TPair<int32, FLinearColor>& A, const TPair<int32, FLinearColor>& B) {
				return A.Key > B.Key;
			});
			for (const auto& Pair : Classes) {
				const FString Label = Pair.Key;
				// clang-format off
				ChipsBox->AddSlot()
				[
					PjcInspectorChipToggle(Label, TEXT("Shows only these on the first level"), Pair.Value.Value, Pair.Value.Key,
						[this, Label]() { return ClassChips.Num() == 0 || ClassChips.Contains(Label); },
						[this, Label]() {
							if (ClassChips.Contains(Label)) ClassChips.Remove(Label);
							else ClassChips.Add(Label);
							RebuildTree();
						})
				];
				// clang-format on
			}
		}
	}

	if (TreeView.IsValid()) {
		TreeView->RequestTreeRefresh();
		RestoreExpansion(TreeItems);
	}
	UpdateNeedRead();
}

void SPjcTabInspector::SortLinks(TArray<FPjcModelLink>& Links) const {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	// Used by: closest to a root first, unused last. Uses: what only this keeps first, then the biggest
	Links.Sort([this, &Result](const FPjcModelLink& A, const FPjcModelLink& B) {
		const FName PackageA = Model.Packages[A.Other];
		const FName PackageB = Model.Packages[B.Other];
		if (!bUses) {
			const int32 RankA = PjcInspectorStatusRank(Result.GetStatus(PackageA), Model.bScanned[A.Other]);
			const int32 RankB = PjcInspectorStatusRank(Result.GetStatus(PackageB), Model.bScanned[B.Other]);
			if (RankA != RankB) return RankA < RankB;
			const int32 DepthA = Model.GetDepth(A.Other, Kinds);
			const int32 DepthB = Model.GetDepth(B.Other, Kinds);
			if (DepthA != DepthB) return DepthA == INDEX_NONE ? false : DepthB == INDEX_NONE ? true : DepthA < DepthB;
		}
		else {
			const bool bOnlyA = RetainedSet.Contains(A.Other);
			const bool bOnlyB = RetainedSet.Contains(B.Other);
			if (bOnlyA != bOnlyB) return bOnlyA;
			if (Model.Sizes[A.Other] != Model.Sizes[B.Other]) return Model.Sizes[A.Other] > Model.Sizes[B.Other];
		}
		return PjcInspectorShortName(PackageA) < PjcInspectorShortName(PackageB);
	});
}

SPjcTabInspector::FItemPtr
SPjcTabInspector::MakeAssetItem(const int32 Owner, const FPjcModelLink& Link, const int32 Depth, const FItemPtr& Parent, const FString& Key) {
	const FItemPtr Item = MakeShared<FPjcInspectorItem>();
	Item->Kind = FPjcInspectorItem::EKind::Asset;
	Item->Node = Link.Other;
	Item->Link = Link;
	Item->Holder = bUses ? Owner : Link.Other;
	Item->Target = bUses ? Link.Other : Owner;
	Item->Owner = Owner;
	Item->Depth = Depth;
	Item->Parent = Parent;
	Item->Key = Key + TEXT(">") + Model.Packages[Link.Other].ToString();
	// a reference back to an asset above it: no further levels
	Item->bCircular = Link.Other == FocusNode;
	for (FItemPtr Up = Parent; Up.IsValid() && !Item->bCircular; Up = Up->Parent.Pin()) {
		if (Up->Kind == FPjcInspectorItem::EKind::Asset && Up->Node == Link.Other) Item->bCircular = true;
	}
	return Item;
}

TArray<SPjcTabInspector::FItemPtr>
SPjcTabInspector::MakeLevel(const int32 Owner, const int32 Depth, const FItemPtr& Parent, const FString& Key, const TArray<int32>* RouteRest) {
	TArray<FItemPtr> Out;
	TArray<FPjcModelLink> Links;
	for (const FPjcModelLink& Link : bUses ? Model.Uses[Owner] : Model.UsedBy[Owner]) {
		if (!FPjcInspectorModel::Counts(Kinds, Link) && Link.Via == EPjcUsageReason::None) continue;
		if (IsTreeSearch() && !SearchResult.Keep.Contains(Link.Other)) continue;
		if (!IsTreeSearch() && Depth == 0 && ClassChips.Num() > 0 && !ClassChips.Contains(PjcWidgets::GetAssetType(Model.Assets[Link.Other]).Label))
			continue;

		Links.Add(Link);
	}
	SortLinks(Links);

	// the route the answer shows first, open: the answer to why inside the tree
	if (!IsTreeSearch() && RouteRest && RouteRest->Num() > 0) {
		const int32 Next = (*RouteRest)[0];
		const int32 Index = Links.IndexOfByPredicate([Next](const FPjcModelLink& Link) {
			return Link.Other == Next;
		});
		if (Index != INDEX_NONE) {
			const FItemPtr Item = MakeAssetItem(Owner, Links[Index], Depth, Parent, Key);
			Item->bRoute = true;
			Item->RouteRest = TArray<int32> {RouteRest->GetData() + 1, RouteRest->Num() - 1};
			Out.Add(Item);
			Links.RemoveAt(Index);
		}
	}

	// what a read asset uses, as its Details panel shows it: categories, then the property and its asset
	const FName OwnerPackage = Model.Packages[Owner];
	const FPjcLocations* Read = bUses && !IsTreeSearch() ? Locations.Find(OwnerPackage) : nullptr;
	if (Read && Links.Num() > 0) {
		struct FCat
		{
			FString Label;
			TArray<TSharedPtr<FCat>> Kids;
			TArray<TPair<FPjcModelLink, FString>> Rows;
		};
		const TSharedPtr<FCat> Root = MakeShared<FCat>();
		const auto Child = [](const TSharedPtr<FCat>& Cat, const FString& Label) {
			for (const TSharedPtr<FCat>& Kid : Cat->Kids) {
				if (Kid->Label == Label) return Kid;
			}
			const TSharedPtr<FCat> Kid = MakeShared<FCat>();
			Kid->Label = Label;
			Cat->Kids.Add(Kid);
			return Kid;
		};
		for (const FPjcModelLink& Link : Links) {
			const FName Target = Model.Packages[Link.Other];
			bool bPlaced = false;
			for (const FPjcLocation& Place : Read->Places) {
				if (Place.Target != Target) continue;

				TSharedPtr<FCat> Cat = Root;
				for (const FString& Part : Place.Parts) Cat = Child(Cat, Part);
				Cat->Rows.Emplace(Link, Place.Property);
				bPlaced = true;
			}
			if (!bPlaced) {
				const TCHAR* Where = Link.Via != EPjcUsageReason::None ? TEXT("Kept for it")
					: Read->NotFound.Contains(Target)				   ? TEXT("Not in a property")
																	   : TEXT("Other");
				Child(Root, Where)->Rows.Emplace(Link, FString {});
			}
		}
		const TFunction<void(const TSharedPtr<FCat>&, int32, const FItemPtr&, const FString&, TArray<FItemPtr>&)> Draw
			= [&](const TSharedPtr<FCat>& Cat, const int32 AtDepth, const FItemPtr& Up, const FString& UpKey, TArray<FItemPtr>& Into) {
				  for (const TPair<FPjcModelLink, FString>& Row : Cat->Rows) {
					  const FItemPtr Item = MakeAssetItem(Owner, Row.Key, AtDepth, Up, UpKey);
					  Item->Property = Row.Value;
					  Item->Key += TEXT("@") + Row.Value;
					  Into.Add(Item);
				  }
				  for (const TSharedPtr<FCat>& Kid : Cat->Kids) {
					  // a category with one sub category and nothing else reads as one line
					  TArray<FString> Label {Kid->Label};
					  TSharedPtr<FCat> Inner = Kid;
					  while (Inner->Rows.Num() == 0 && Inner->Kids.Num() == 1) {
						  Inner = Inner->Kids[0];
						  Label.Add(Inner->Label);
					  }
					  const FItemPtr Item = MakeShared<FPjcInspectorItem>();
					  Item->Kind = FPjcInspectorItem::EKind::Category;
					  Item->Label = FString::Join(Label, TEXT(" › "));
					  Item->Key = UpKey + TEXT("|c:") + FString::Join(Label, TEXT("/"));
					  Item->Depth = AtDepth;
					  Item->Parent = Up;
					  Item->Owner = Owner;
					  Item->bBuilt = true;
					  Draw(Inner, AtDepth + 1, Item, Item->Key, Item->Children);
					  int32 Num = 0;
					  TArray<FItemPtr> Count = Item->Children;
					  for (int32 Head = 0; Head < Count.Num(); ++Head) {
						  if (Count[Head]->Kind == FPjcInspectorItem::EKind::Asset) ++Num;
						  Count.Append(Count[Head]->Children);
					  }
					  Item->Shown = Num;
					  Into.Add(Item);
				  }
			  };
		Draw(Root, Depth, Parent, Key, Out);
		return Out;
	}

	if (IsTreeSearch() || Links.Num() <= PjcInspectorGroupAt) {
		for (const FPjcModelLink& Link : Links) {
			Out.Add(MakeAssetItem(Owner, Link, Depth, Parent, Key));
		}
		return Out;
	}

	// a big level: a group per class
	TMap<FString, TArray<FPjcModelLink>> ByClass;
	for (const FPjcModelLink& Link : Links) {
		ByClass.FindOrAdd(PjcWidgets::GetAssetType(Model.Assets[Link.Other]).Label).Add(Link);
	}
	ByClass.ValueSort([](const TArray<FPjcModelLink>& A, const TArray<FPjcModelLink>& B) {
		return A.Num() > B.Num();
	});
	for (auto& Pair : ByClass) {
		const FItemPtr Item = MakeShared<FPjcInspectorItem>();
		Item->Kind = FPjcInspectorItem::EKind::Group;
		Item->Label = Pair.Key;
		Item->Key = Key + TEXT("|") + Pair.Key;
		Item->Depth = Depth;
		Item->Parent = Parent;
		Item->Owner = Owner;
		Item->IconNode = Pair.Value[0].Other;
		Item->Members = MoveTemp(Pair.Value);
		Out.Add(Item);
	}
	return Out;
}

void SPjcTabInspector::AddGroupChildren(const FItemPtr& Item) {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	const auto StatusOf = [this, &Result](const FPjcModelLink& Link) {
		return Result.GetStatus(Model.Packages[Link.Other]);
	};
	const auto Group = [this, &Item](const FString& Label, const FString& KeyPart, TArray<FPjcModelLink>&& Members) {
		const FItemPtr Sub = MakeShared<FPjcInspectorItem>();
		Sub->Kind = FPjcInspectorItem::EKind::Group;
		Sub->Label = Label;
		Sub->Key = Item->Key + KeyPart;
		Sub->Depth = Item->Depth + 1;
		Sub->Parent = Item;
		Sub->Owner = Item->Owner;
		Sub->IconNode = Members.Num() > 0 ? Members[0].Other : INDEX_NONE;
		Sub->Members = MoveTemp(Members);
		return Sub;
	};

	// a class group splits by status, then by folder, then shows pages
	if (!Item->bStatusGroup && !Item->bFolder) {
		TMap<EPjcAssetStatus, TArray<FPjcModelLink>> ByStatus;
		for (const FPjcModelLink& Link : Item->Members) {
			ByStatus.FindOrAdd(StatusOf(Link)).Add(Link);
		}
		if (ByStatus.Num() > 1) {
			ByStatus.KeySort([](const EPjcAssetStatus A, const EPjcAssetStatus B) {
				return PjcInspectorStatusRank(A, true) < PjcInspectorStatusRank(B, true);
			});
			for (auto& Pair : ByStatus) {
				const FItemPtr Sub = Group(
					PjcInspectorStatusText(Pair.Key, true), FString::Printf(TEXT("|s:%d"), static_cast<int32>(Pair.Key)), MoveTemp(Pair.Value)
				);
				Sub->bStatusGroup = true;
				Sub->GroupStatus = Pair.Key;
				Item->Children.Add(Sub);
			}
			return;
		}
	}
	if (!Item->bFolder && Item->Members.Num() > PjcInspectorPage) {
		TMap<FString, TArray<FPjcModelLink>> ByFolder;
		for (const FPjcModelLink& Link : Item->Members) {
			ByFolder.FindOrAdd(FPackageName::GetLongPackagePath(Model.Packages[Link.Other].ToString())).Add(Link);
		}
		if (ByFolder.Num() > 1) {
			ByFolder.KeySort([](const FString& A, const FString& B) {
				return A < B;
			});
			for (auto& Pair : ByFolder) {
				TArray<FString> Parts;
				Pair.Key.ParseIntoArray(Parts, TEXT("/"));
				const FString Short = Parts.Num() >= 2 ? Parts[Parts.Num() - 2] + TEXT("/") + Parts.Last() : Pair.Key;
				const FItemPtr Sub = Group(Short, TEXT("|f:") + Pair.Key, MoveTemp(Pair.Value));
				Sub->bFolder = true;
				Item->Children.Add(Sub);
			}
			return;
		}
	}
	const int32 Shown = FMath::Min(Item->Members.Num(), Pages.FindRef(Item->Key) > 0 ? Pages.FindRef(Item->Key) : PjcInspectorPage);
	for (int32 Index = 0; Index < Shown; ++Index) {
		Item->Children.Add(MakeAssetItem(Item->Owner, Item->Members[Index], Item->Depth + 1, Item, Item->Key));
	}
	if (Shown < Item->Members.Num()) {
		const FItemPtr More = MakeShared<FPjcInspectorItem>();
		More->Kind = FPjcInspectorItem::EKind::More;
		More->Key = Item->Key + TEXT("|more");
		More->Depth = Item->Depth + 1;
		More->Parent = Item;
		More->Owner = Item->Owner;
		More->Shown = Item->Members.Num() - Shown;
		Item->Children.Add(More);
	}
}

void SPjcTabInspector::BuildChildren(const FItemPtr& Item) {
	if (!Item.IsValid() || Item->bBuilt) return;

	Item->bBuilt = true;
	if (Item->Kind == FPjcInspectorItem::EKind::Group) {
		AddGroupChildren(Item);
		return;
	}
	if (Item->Kind != FPjcInspectorItem::EKind::Asset || Item->bCircular) return;

	Item->Children = MakeLevel(Item->Node, Item->Depth + 1, Item, Item->Key, Item->bRoute ? &Item->RouteRest : nullptr);
}

bool SPjcTabInspector::ShouldExpand(const FItemPtr& Item) const {
	if (IsTreeSearch()) {
		if (Item->Kind != FPjcInspectorItem::EKind::Asset || Item->bCircular) return false;
		for (const FPjcModelLink& Link : bUses ? Model.Uses[Item->Node] : Model.UsedBy[Item->Node]) {
			if (SearchResult.Keep.Contains(Link.Other)) return true;
		}
		return false;
	}
	if (Item->bRoute || Item->Kind == FPjcInspectorItem::EKind::Category) return !Closed.Contains(Item->Key);
	return Expanded.Contains(Item->Key);
}

void SPjcTabInspector::RestoreExpansion(const TArray<FItemPtr>& Items) {
	for (const FItemPtr& Item : Items) {
		if (!ShouldExpand(Item)) continue;

		BuildChildren(Item);
		if (Item->Children.Num() == 0) continue;

		TreeView->SetItemExpansion(Item, true);
		RestoreExpansion(Item->Children);
	}
}

void SPjcTabInspector::OnExpansionChanged(FItemPtr Item, bool bExpanded) {
	if (!Item.IsValid()) return;

	// an asset opens once: a second row of it points at the first
	if (bExpanded && Item->Kind == FPjcInspectorItem::EKind::Asset) {
		const TWeakPtr<FPjcInspectorItem>* Open = OpenNodes.Find(Item->Node);
		const FItemPtr Other = Open ? Open->Pin() : nullptr;
		if (Other.IsValid() && Other != Item && TreeView->IsItemExpanded(Other)) {
			TreeView->SetItemExpansion(Item, false);
			TreeView->SetSelection(Other);
			TreeView->RequestScrollIntoView(Other);
			UPjcSubsystem::ShowNotification(
				PjcInspectorShortName(Model.Packages[Item->Node]) + TEXT(": its levels are open above"), SNotificationItem::CS_None, 2.0f
			);
			return;
		}
		OpenNodes.Add(Item->Node, Item);
	}
	else if (!bExpanded && Item->Kind == FPjcInspectorItem::EKind::Asset) {
		const TWeakPtr<FPjcInspectorItem>* Open = OpenNodes.Find(Item->Node);
		if (Open && Open->Pin() == Item) OpenNodes.Remove(Item->Node);
	}

	if (!IsTreeSearch()) {
		if (Item->bRoute || Item->Kind == FPjcInspectorItem::EKind::Category) {
			if (bExpanded)
				Closed.Remove(Item->Key);
			else
				Closed.Add(Item->Key);
		}
		else if (bExpanded) {
			Expanded.Add(Item->Key);
		}
		else {
			Expanded.Remove(Item->Key);
		}
	}
	UpdateNeedRead();
}

void SPjcTabInspector::UpdateNeedRead() {
	NeedRead.Reset();
	if (!TreeView.IsValid()) return;

	// the holders of the rows on screen that can be read and were not
	TArray<FItemPtr> Visible = TreeItems;
	for (int32 Head = 0; Head < Visible.Num(); ++Head) {
		const FItemPtr& Item = Visible[Head];
		if (Item->Kind == FPjcInspectorItem::EKind::Asset && Item->Link.Via == EPjcUsageReason::None) {
			const FName Holder = Model.Packages[Item->Holder];
			if (!Locations.Contains(Holder) && !NeedRead.Contains(Holder) && FPjcLocationReader::CanRead(Holder)) NeedRead.Add(Holder);
		}
		if (Item->bBuilt && TreeView->IsItemExpanded(Item)) Visible.Append(Item->Children);
	}
}

void SPjcTabInspector::ShowMore(const FItemPtr& Item) {
	const FItemPtr Group = Item.IsValid() ? Item->Parent.Pin() : nullptr;
	if (!Group.IsValid()) return;

	const int32 Now = Pages.FindRef(Group->Key) > 0 ? Pages.FindRef(Group->Key) : PjcInspectorPage;
	Pages.Add(Group->Key, Now + PjcInspectorMorePage);
	Group->Children.Reset();
	Group->bBuilt = false;
	BuildChildren(Group);
	TreeView->RequestTreeRefresh();
	UpdateNeedRead();
}

SPjcTabInspector::FItemPtr SPjcTabInspector::GetSelectedItem() const {
	if (!TreeView.IsValid()) return nullptr;

	const TArray<FItemPtr> Selected = TreeView->GetSelectedItems();
	return Selected.Num() > 0 ? Selected[0] : nullptr;
}

FString SPjcTabInspector::GetWhereText(const FPjcInspectorItem& Item, FString& OutToolTip, FName& OutColor) const {
	OutColor = TEXT("ProjectCleaner.Color.Muted");
	OutToolTip.Reset();
	if (!Model.Packages.IsValidIndex(Item.Holder) || !Model.Packages.IsValidIndex(Item.Target)) return FString {};

	const FName Holder = Model.Packages[Item.Holder];
	const FName Target = Model.Packages[Item.Target];
	const FString HolderName = PjcInspectorShortName(Holder);
	if (Item.Link.Via != EPjcUsageReason::None) {
		const FPjcAssetUsage* Usage = UPjcSubsystem::GetScanResult().Usage.Find(Model.Packages[Item.Target]);
		OutToolTip = Usage ? UPjcSubsystem::GetUsageReasonText(*Usage) : FString {};
		switch (Item.Link.Via) {
			case EPjcUsageReason::Localized :		 return TEXT("localized copy");
			case EPjcUsageReason::FolderLabelAsset : return TEXT("listed by the label");
			case EPjcUsageReason::FolderLabel :		 return TEXT("in the folder of the label");
			case EPjcUsageReason::GameplayCue :		 return TEXT("fires the cue");
			default :								 return PjcInspectorSourceTitle(Item.Link.Via);
		}
	}
	if (Item.Link.bThrough) {
		OutToolTip = TEXT("Through an actor the level saves in its own file (World Partition)");
		return TEXT("▣ level actor");
	}
	if (!FPjcLocationReader::CanRead(Holder)) {
		OutToolTip = FString::Printf(TEXT("Levels are not loaded, so where inside %s is not known"), *HolderName);
		return TEXT("in the level");
	}
	const FPjcLocations* Read = Locations.Find(Holder);
	if (!Read) {
		OutToolTip = FString::Printf(TEXT("Where: not read yet. Show in the heading loads %s to read it"), *HolderName);
		OutColor = TEXT("ProjectCleaner.Color.Line");
		return TEXT("·");
	}
	TArray<FString> Places;
	for (const FPjcLocation& Place : Read->Places) {
		if (Place.Target != Target) continue;

		Places.Add(FString::Join(Place.Parts, TEXT(" › ")) + (Place.Parts.Num() > 0 ? TEXT(" › ") : TEXT("")) + Place.Property);
	}
	if (Places.Num() == 0) {
		OutColor = TEXT("ProjectCleaner.Color.Yellow");
		OutToolTip = TEXT("The asset registry says it is used, but no property of the loaded asset holds it: saved in the asset's own data");
		return TEXT("not in a property");
	}
	OutToolTip = TEXT("Where in ") + HolderName + TEXT(":\n") + FString::Join(Places, TEXT("\n"));
	OutColor = TEXT("ProjectCleaner.Color.Text");
	if (!Item.Property.IsEmpty()) return Item.Property;

	// the last two parts say most
	const FPjcLocation* First = Read->Places.FindByPredicate([Target](const FPjcLocation& Place) {
		return Place.Target == Target;
	});
	TArray<FString> Short;
	if (First->Parts.Num() > 0) Short.Add(First->Parts.Last());
	Short.Add(First->Property);
	return FString::Join(Short, TEXT(" › ")) + (Places.Num() > 1 ? FString::Printf(TEXT("  +%d"), Places.Num() - 1) : FString {});
}

FString SPjcTabInspector::GetStatsText() const {
	if (FocusNode == INDEX_NONE) return FString {};
	if (IsTreeSearch()) {
		const int32 Hits = SearchResult.Hits.Num();
		return FString::Printf(
			TEXT("%s%s in %s · the branches that lead to them"),
			*PjcInspectorPlural(Hits, TEXT("match")).Replace(TEXT("matchs"), TEXT("matches")),
			Hits >= FPjcInspectorModel::MaxHits ? TEXT(" (the first)") : TEXT(""),
			*PjcInspectorPlural(SearchResult.Searched, TEXT("asset"))
		);
	}
	int32 Direct = 0;
	for (const FPjcModelLink& Link : bUses ? Model.Uses[FocusNode] : Model.UsedBy[FocusNode]) {
		if (FPjcInspectorModel::Counts(Kinds, Link) || Link.Via != EPjcUsageReason::None) ++Direct;
	}
	if (!bUses)
		return FString::Printf(TEXT("%s direct · %s in all · closest to a root first"), *PjcFormat::Count(Direct), *PjcFormat::Count(ReachUp.Num));

	FString Text = FString::Printf(TEXT("%s direct · %s in all"), *PjcFormat::Count(Direct), *PjcFormat::Count(ReachDown.Num));
	if (ReachDown.Num > 0) Text += TEXT(" · ") + PjcFormat::Size(ReachDown.Size) + TEXT(" pulled in");
	if (Retained.Num() > 1) {
		int64 Only = 0;
		for (int32 Index = 1; Index < Retained.Num(); ++Index) Only += Model.Sizes[Retained[Index]];
		Text += TEXT(" · ") + PjcFormat::Size(Only) + TEXT(" only through this");
	}
	return Text;
}

TSharedRef<ITableRow> SPjcTabInspector::OnGenerateTreeRow(FItemPtr Item, const TSharedRef<STableViewBase>& Table) {
	// clang-format off
	return SNew(SPjcInspectorTreeRow, Table)
		.Item(Item)
		.MakeCell([this](const FItemPtr& Row, const FName Column, const TSharedRef<ITableRow>& TableRow) { return MakeCell(Row, Column, TableRow); });
	// clang-format on
}

TSharedRef<SWidget> SPjcTabInspector::MakeCell(const FItemPtr& Item, const FName Column, const TSharedRef<ITableRow>& TableRow) {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	const bool bAsset = Item->Kind == FPjcInspectorItem::EKind::Asset;
	const FName Package = bAsset ? Model.Packages[Item->Node] : NAME_None;
	const auto Small = [](const FString& Text, const FName Color, const bool bMono, const FString& ToolTip = FString {}) -> TSharedRef<SWidget> {
		// clang-format off
		return SNew(STextBlock)
			.Text(FText::FromString(Text))
			.ToolTipText(FText::FromString(ToolTip))
			.Font(bMono ? PjcWidgets::GetFontMono(8) : FPjcStyles::GetFont("Regular", 8))
			.ColorAndOpacity(PjcInspectorColor(Color));
		// clang-format on
	};

	if (Column == PjcInspectorColAsset) {
		const TSharedRef<SHorizontalBox> Box = SNew(SHorizontalBox).Clipping(EWidgetClipping::ClipToBounds);
		// clang-format off
		Box->AddSlot().AutoWidth()
		[
			SNew(SBox).WidthOverride(2.0f)
			[
				SNew(SImage)
				.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.ColorAndOpacity(Item->bRoute ? PjcInspectorColor("ProjectCleaner.Color.Link") : FLinearColor::Transparent)
			]
		];
		Box->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {4.0f, 0.0f, 2.0f, 0.0f})
		[
			SNew(SExpanderArrow, TableRow).StyleSet(&FPjcStyles::Get()).IndentAmount(16.0f).ShouldDrawWires(false)
		];
		// clang-format on
		if (bAsset) {
			const PjcWidgets::FAssetType& Type = PjcWidgets::GetAssetType(Model.Assets[Item->Node]);
			const EPjcAssetStatus Status = Result.GetStatus(Package);
			const FString More = Type.Label + TEXT(" · ") + PjcFormat::Size(Model.Sizes[Item->Node]) + TEXT(" · ")
				+ PjcInspectorStatusText(Status, Model.bScanned[Item->Node]) + TEXT("\nDouble-click or Enter: inspect it");
			// clang-format off
			Box->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 6.0f, 0.0f})
			[
				SNew(SBox).WidthOverride(16.0f).HeightOverride(16.0f)
				[
					SNew(SImage).Image(Type.Icon)
				]
			];
			Box->AddSlot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(PjcInspectorShortName(Package)))
				.ToolTip(PjcWidgets::MakeToolTip(PjcInspectorShortName(Package), Package.ToString(), More))
				.ColorAndOpacity(Item->bRoute ? FSlateColor {PjcInspectorColor("ProjectCleaner.Color.Strong")} : FSlateColor::UseForeground())
			];
			// clang-format on
			return Box;
		}
		if (Item->Kind == FPjcInspectorItem::EKind::Group) {
			const FSlateBrush* Icon = Item->bFolder			? FPjcStyles::Get().GetBrush("ProjectCleaner.Icon.Folder")
				: Item->bStatusGroup						? nullptr
				: Model.Assets.IsValidIndex(Item->IconNode) ? PjcWidgets::GetAssetType(Model.Assets[Item->IconNode]).Icon
															: nullptr;
			// clang-format off
			Box->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 6.0f, 0.0f})
			[
				SNew(SBox).WidthOverride(16.0f).HeightOverride(16.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					Item->bStatusGroup
						? PjcWidgets::MakeSwatch(PjcInspectorStatusColor(Item->GroupStatus, true), 7.0f)
						: StaticCastSharedRef<SWidget>(SNew(SImage).Image(Icon).ColorAndOpacity(Item->bFolder ? PjcInspectorColor("ProjectCleaner.Color.FolderTint") : FLinearColor::White))
				]
			];
			Box->AddSlot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Item->Label))
				.ColorAndOpacity(Item->bStatusGroup ? FSlateColor {PjcInspectorStatusColor(Item->GroupStatus, true)} : FSlateColor::UseForeground())
			];
			Box->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {7.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock).Text(FText::FromString(PjcFormat::Count(Item->Members.Num()))).Font(PjcWidgets::GetFontMono(8)).ColorAndOpacity(FSlateColor::UseSubduedForeground())
			];
			// clang-format on
			return Box;
		}
		if (Item->Kind == FPjcInspectorItem::EKind::Category) {
			// clang-format off
			Box->AddSlot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(Item->Label)).Font(FPjcStyles::GetFont("Bold", 9))
			];
			Box->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {7.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock).Text(FText::FromString(PjcFormat::Count(Item->Shown))).Font(PjcWidgets::GetFontMono(8)).ColorAndOpacity(FSlateColor::UseSubduedForeground())
			];
			// clang-format on
			return Box;
		}
		// more of a group
		const FItemPtr More = Item;
		// clang-format off
		Box->AddSlot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
			.ButtonStyle(&PjcShim::GetButtonStyle427(false))
			.ForegroundColor(FSlateColor::UseForeground())
#endif
			.ContentPadding(FMargin {8.0f, 0.0f})
			.OnClicked_Lambda([this, More]() { ShowMore(More); return FReply::Handled(); })
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("Show %s more"), *PjcFormat::Count(FMath::Min(Item->Shown, PjcInspectorMorePage)))))
				.Font(FPjcStyles::GetFont("Regular", 8))
			]
		];
		Box->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {8.0f, 0.0f, 0.0f, 0.0f})
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("%s not shown · or Browse to the group's assets"), *PjcFormat::Count(Item->Shown))))
			.Font(FPjcStyles::GetFont("Regular", 8))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		];
		// clang-format on
		return Box;
	}

	if (Column == PjcInspectorColKind) {
		if (bAsset) {
			if (Item->Link.Via != EPjcUsageReason::None)
				return Small(
					TEXT("kept for"), TEXT("ProjectCleaner.Color.Muted"), true, TEXT("Not a reference: the scan keeps it for the asset it belongs to")
				);
			return Small(
				PjcInspectorKindText(Item->Link.Kind), PjcInspectorKindColor(Item->Link.Kind), true, PjcInspectorKindToolTip(Item->Link.Kind)
			);
		}
		if (Item->Kind == FPjcInspectorItem::EKind::Group) {
			TArray<FString> KindNames;
			for (const FPjcModelLink& Link : Item->Members) {
				KindNames.AddUnique(PjcInspectorKindText(Link.Kind));
			}
			return Small(FString::Join(KindNames, TEXT(" ")), TEXT("ProjectCleaner.Color.Muted"), true);
		}
		return SNullWidget::NullWidget;
	}

	if (Column == PjcInspectorColWhere) {
		if (bAsset) {
			FString ToolTip;
			FName Color;
			const FString Text = GetWhereText(*Item, ToolTip, Color);
			// clang-format off
			return SNew(STextBlock)
				.Text(FText::FromString(Text))
				.ToolTipText(FText::FromString(ToolTip))
				.Font(PjcWidgets::GetFontMono(8))
				.ColorAndOpacity(PjcInspectorColor(Color))
				.Clipping(EWidgetClipping::ClipToBounds);
			// clang-format on
		}
		if (Item->Kind == FPjcInspectorItem::EKind::Group) {
			// what the group holds by status
			TMap<EPjcAssetStatus, int32> Counts;
			for (const FPjcModelLink& Link : Item->Members) {
				++Counts.FindOrAdd(Result.GetStatus(Model.Packages[Link.Other]));
			}
			Counts.KeySort([](const EPjcAssetStatus A, const EPjcAssetStatus B) {
				return PjcInspectorStatusRank(A, true) < PjcInspectorStatusRank(B, true);
			});
			const TSharedRef<SHorizontalBox> Box = SNew(SHorizontalBox);
			for (const auto& Pair : Counts) {
				// clang-format off
				Box->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 10.0f, 0.0f})
				[
					SNew(STextBlock)
					.Text(FText::FromString(PjcFormat::Count(Pair.Value) + TEXT(" ") + FString {PjcInspectorStatusText(Pair.Key, true)}.ToLower()))
					.Font(FPjcStyles::GetFont("Regular", 8))
					.ColorAndOpacity(PjcInspectorStatusColor(Pair.Key, true))
				];
				// clang-format on
			}
			return Box;
		}
		return SNullWidget::NullWidget;
	}

	if (Column == PjcInspectorColMeta) {
		if (bAsset) {
			if (Item->bCircular)
				return Small(TEXT("circular"), TEXT("ProjectCleaner.Color.Circular"), false, TEXT("It leads back to an asset above it"));

			const EPjcAssetStatus Status = Result.GetStatus(Package);
			const bool bScanned = Model.bScanned[Item->Node];
			if (!bUses) {
				if (Model.bRoot[Item->Node]) {
					const FPjcAssetUsage* Usage = Result.Usage.Find(Package);
					return Small(
						TEXT("Root · ") + FString {PjcInspectorSourceTitle(Model.Reasons[Item->Node])},
						TEXT("ProjectCleaner.Color.Yellow"),
						false,
						Usage ? UPjcSubsystem::GetUsageReasonText(*Usage) : FString {}
					);
				}
				const int32 Steps = Model.GetDepth(Item->Node, Kinds);
				if (Steps != INDEX_NONE) return Small(PjcInspectorSteps(Steps) + TEXT(" to a root"), TEXT("ProjectCleaner.Color.Muted"), false);
				return Small(PjcInspectorStatusText(Status, bScanned), PjcInspectorStatusColorName(Status, bScanned), false);
			}
			const TSharedRef<SHorizontalBox> Box = SNew(SHorizontalBox);
			const EPjcAssetStatus Mine = Result.GetStatus(Focus);
			if (Status != Mine || !bScanned) {
				Box->AddSlot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(
						FMargin {0.0f, 0.0f, 6.0f, 0.0f}
					)[Small(PjcInspectorStatusText(Status, bScanned), PjcInspectorStatusColorName(Status, bScanned), false)];
			}
			if (RetainedSet.Contains(Item->Node) && Item->Depth == 0) {
				Box->AddSlot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(
						FMargin {0.0f, 0.0f, 6.0f, 0.0f}
					)[Small(TEXT("only here"), TEXT("ProjectCleaner.Color.Ok"), false, TEXT("Only this asset keeps it used"))];
			}
			Box->AddSlot().AutoWidth().VAlign(
				VAlign_Center
			)[Small(PjcFormat::Size(Model.Sizes[Item->Node]), TEXT("ProjectCleaner.Color.Text"), true)];
			return Box;
		}
		if (Item->Kind == FPjcInspectorItem::EKind::Group) {
			int64 Size = 0;
			for (const FPjcModelLink& Link : Item->Members) Size += Model.Sizes[Link.Other];
			return Small(PjcFormat::Size(Size), TEXT("ProjectCleaner.Color.Muted"), true);
		}
		return SNullWidget::NullWidget;
	}

	if (Column == PjcInspectorColGo && bAsset) {
		// clang-format off
		return SNew(SButton)
			.ButtonStyle(&FPjcStyles::Get().GetWidgetStyle<FButtonStyle>("ProjectCleaner.Button.Row"))
#if ENGINE_MAJOR_VERSION < 5
			.ForegroundColor(FSlateColor::UseForeground())
#endif
			.ContentPadding(FMargin {4.0f, 2.0f})
			.ToolTipText(FText::FromString(TEXT("Inspect ") + PjcInspectorShortName(Package)))
			.OnClicked_Lambda([this, Package]() { SetFocus(Package, true); return FReply::Handled(); })
			[
				SNew(SBox).WidthOverride(12.0f).HeightOverride(12.0f)
				[
					SNew(SImage).Image(FPjcStyles::Get().GetBrush("ProjectCleaner.Icon.Forward")).ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			];
		// clang-format on
	}
	return SNullWidget::NullWidget;
}

TSharedPtr<SWidget> SPjcTabInspector::OnTreeContextMenu() {
	const FItemPtr Item = GetSelectedItem();
	if (!Item.IsValid()) return nullptr;

	FMenuBuilder Menu {true, nullptr};
	const bool bOpen = TreeView->IsItemExpanded(Item);
	const auto AddToggle = [this, &Menu, &Item, bOpen]() {
		BuildChildren(Item);
		if (Item->Children.Num() == 0) return;

		const FItemPtr Row = Item;
		PjcWidgets::AddMenuEntry(
			Menu,
			bOpen ? EPjcMenuIcon::Collapse : EPjcMenuIcon::Expand,
			bOpen ? TEXT("Collapse") : TEXT("Expand"),
			TEXT("Left and Right open and close rows too"),
			[this, Row, bOpen]() {
				TreeView->SetItemExpansion(Row, !bOpen);
			}
		);
	};

	if (Item->Kind == FPjcInspectorItem::EKind::Asset) {
		const FName Package = Model.Packages[Item->Node];
		const FAssetData Asset = Model.Assets[Item->Node];
		const FString Name = PjcInspectorShortName(Package);
		Menu.BeginSection(NAME_None);
		PjcWidgets::AddMenuEntry(Menu, EPjcMenuIcon::Inspect, TEXT("Inspect"), TEXT("Shows it here (Enter, double-click)"), [this, Package]() {
			SetFocus(Package, true);
		});
		PjcWidgets::AddMenuEntry(Menu, EPjcMenuIcon::None, TEXT("Open"), TEXT("Opens it in its editor"), Asset.IsValid(), [Asset]() {
			UPjcSubsystem::OpenAssetEditor(Asset);
		});
		PjcWidgets::AddMenuEntry(
			Menu, EPjcMenuIcon::Browse, TEXT("Browse to asset"), TEXT("Shows it in the Content Browser (Ctrl+B)"), Asset.IsValid(), [Asset]() {
				PjcInspectorBrowse({Asset});
			}
		);
		PjcWidgets::AddMenuEntry(
			Menu, EPjcMenuIcon::SizeMap, TEXT("Size map"), TEXT("Shows it and what it pulls in, by size"), Asset.IsValid(), [Asset]() {
				UPjcSubsystem::OpenSizeMapViewer({Asset});
			}
		);
		Menu.EndSection();
		Menu.BeginSection(NAME_None);
		PjcWidgets::AddMenuEntry(Menu, EPjcMenuIcon::None, TEXT("Copy path"), TEXT("Ctrl+C"), [Package]() {
			PjcInspectorCopy(Package.ToString(), TEXT("the path"));
		});
		PjcWidgets::AddMenuEntry(Menu, EPjcMenuIcon::None, TEXT("Copy name"), FString {}, [Name]() {
			PjcInspectorCopy(Name, Name);
		});
		Menu.EndSection();
		Menu.BeginSection(NAME_None);
		const FName Holder = Model.Packages[Item->Holder];
		if (Item->Link.Via == EPjcUsageReason::None && !Locations.Contains(Holder) && FPjcLocationReader::CanRead(Holder)) {
			PjcWidgets::AddMenuEntry(
				Menu,
				EPjcMenuIcon::None,
				TEXT("Show where (loads ") + PjcInspectorShortName(Holder) + TEXT(")"),
				TEXT("Loads the asset that holds the reference and reads where it is set"),
				[this, Holder]() {
					ReadHolders({Holder}, true);
				}
			);
		}
		if (!bUses && Item->Depth == 0 && FocusNode != INDEX_NONE && Model.bUsed[FocusNode] && Model.GetDepth(Item->Node, Kinds) != INDEX_NONE) {
			const int32 User = Item->Node;
			PjcWidgets::AddMenuEntry(
				Menu,
				EPjcMenuIcon::None,
				TEXT("What if ") + Name + TEXT(" did not use it"),
				TEXT("Shows what keeps it then, or what becomes unused"),
				[this, User]() {
					WhatIfUser = User;
					RebuildHead();
				}
			);
		}
		AddToggle();
		Menu.EndSection();
		return Menu.MakeWidget();
	}

	if (Item->Kind == FPjcInspectorItem::EKind::Group) {
		TArray<FAssetData> Assets;
		TArray<FString> Paths;
		for (const FPjcModelLink& Link : Item->Members) {
			Assets.Add(Model.Assets[Link.Other]);
			Paths.Add(Model.Packages[Link.Other].ToString());
		}
		const FItemPtr Group = Item;
		PjcWidgets::AddMenuEntry(
			Menu,
			EPjcMenuIcon::Browse,
			FString::Printf(TEXT("Browse to these %s"), *PjcFormat::Count(Assets.Num())),
			TEXT("Selects them in the Content Browser"),
			[Assets]() {
				PjcInspectorBrowse(Assets);
			}
		);
		PjcWidgets::AddMenuEntry(Menu, EPjcMenuIcon::None, TEXT("Copy list"), TEXT("Copies their paths"), [Paths]() {
			PjcInspectorCopy(FString::Join(Paths, TEXT("\n")), PjcInspectorPlural(Paths.Num(), TEXT("path")));
		});
		PjcWidgets::AddMenuEntry(Menu, EPjcMenuIcon::None, TEXT("Who keeps these"), TEXT("Which assets keep them used"), [this, Group]() {
			ShowKeepers(Group);
		});
		AddToggle();
		return Menu.MakeWidget();
	}
	if (Item->Kind == FPjcInspectorItem::EKind::More) {
		const FItemPtr More = Item;
		PjcWidgets::AddMenuEntry(Menu, EPjcMenuIcon::Expand, TEXT("Show more"), FString {}, [this, More]() {
			ShowMore(More);
		});
		return Menu.MakeWidget();
	}
	AddToggle();
	return Menu.MakeWidget();
}

void SPjcTabInspector::ShowKeepers(const FItemPtr& Group) {
	// the asset that keeps each one used on every route, counted
	TMap<int32, int32> Counts;
	for (const FPjcModelLink& Link : Group->Members) {
		const int32 Node = Link.Other;
		const int32 Keeper = Model.bRoot[Node] ? -10 : Model.GetDepth(Node, Kinds) == INDEX_NONE ? INDEX_NONE : Model.GetKeeper(Node, Kinds);
		++Counts.FindOrAdd(Keeper);
	}
	Counts.ValueSort([](const int32 A, const int32 B) {
		return A > B;
	});
	Keepers.Reset();
	for (const auto& Pair : Counts) {
		if (Keepers.Num() >= 16) break;
		Keepers.Add(Pair.Key);
		Keepers.Add(Pair.Value);
	}
	KeepersLabel = FString::Printf(TEXT("%s %s:"), *PjcFormat::Count(Group->Members.Num()), *Group->Label);
	RebuildHead();
}

void SPjcTabInspector::OnTreeDoubleClick(FItemPtr Item) {
	if (!Item.IsValid()) return;

	if (Item->Kind == FPjcInspectorItem::EKind::Asset) {
		SetFocus(Model.Packages[Item->Node], true);
		return;
	}
	if (Item->Kind == FPjcInspectorItem::EKind::More) {
		ShowMore(Item);
		return;
	}
	TreeView->SetItemExpansion(Item, !TreeView->IsItemExpanded(Item));
}

// where references are set

void SPjcTabInspector::ShowWhere(const bool bAsk) {
	UpdateNeedRead();
	ReadHolders(NeedRead, bAsk);
}

void SPjcTabInspector::ReadHolders(const TArray<FName>& Holders, const bool bAsk) {
	if (Holders.Num() == 0) return;

	// a big load is asked about first: the editor stops while it loads
	int64 Size = 0;
	for (const FName Holder : Holders) {
		Size += FPjcLocationReader::GetLoadSize(Holder);
	}
	if (bAsk && Size > PjcInspectorAskAbove) {
		TArray<FPjcDialogPart> Parts;
		FPjcDialogPart& What = Parts.AddDefaulted_GetRef();
		What.Note = TEXT("To show where they hold their references, the editor loads the assets with everything they load with them. ")
			TEXT("The editor may stop for a while.");
		What.NoteColor = TEXT("ProjectCleaner.Color.Yellow");
		What.Rows.Emplace(PjcInspectorPlural(Holders.Num(), TEXT("asset")), PjcFormat::Size(Size) + TEXT(" on disk, not in memory yet"));
		if (!SPjcDialog::Show(TEXT("Show where"), Parts, TEXT("Load"))) return;
	}

	int32 Failed = 0;
	{
		FScopedSlowTask Loading {static_cast<float>(Holders.Num()), FText::FromString(TEXT("Reading where references are set..."))};
		Loading.MakeDialogDelayed(0.3f);
		for (const FName Holder : Holders) {
			Loading.EnterProgressFrame(1.0f, FText::FromString(PjcInspectorShortName(Holder)));
			FPjcLocations Read;
			if (!FPjcLocationReader::Read(Holder, Read)) {
				++Failed;
				continue;
			}
			if (Read.bLoadedHere) LoadedHere.AddUnique(Holder);
			Locations.Add(Holder, MoveTemp(Read));
		}
	}
	if (Failed > 0) {
		UPjcSubsystem::ShowNotification(
			FString::Printf(TEXT("%s could not be loaded"), *PjcInspectorPlural(Failed, TEXT("asset"))), SNotificationItem::CS_Fail, 4.0f
		);
	}
	RebuildTree();
}

void SPjcTabInspector::UnloadRead() {
	const int32 Num = LoadedHere.Num();
	const int32 Gone = FPjcLocationReader::Unload(LoadedHere);
	LoadedHere.Reset();
	if (Num > 0) {
		const FString Text = Gone == Num
			? FString::Printf(TEXT("Unloaded %s · what was read stays until the next scan"), *PjcInspectorPlural(Num, TEXT("asset")))
			: FString::Printf(TEXT("Unloaded %d of %d assets: the editor still uses the rest"), Gone, Num);
		UPjcSubsystem::ShowNotification(Text, SNotificationItem::CS_None, 4.0f);
	}
}

// keys

FReply SPjcTabInspector::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) {
	const FKey Key = InKeyEvent.GetKey();
	if (InKeyEvent.IsAltDown() && Key == EKeys::Left) {
		Back();
		return FReply::Handled();
	}
	if (InKeyEvent.IsAltDown() && Key == EKeys::Right) {
		Forward();
		return FReply::Handled();
	}
	// the text boxes take their own keys: what comes here is not typing
	if (Key == EKeys::BackSpace) {
		Back();
		return FReply::Handled();
	}

	const bool bTree = TreeView.IsValid() && TreeView->HasKeyboardFocus();
	const FItemPtr Item = bTree ? GetSelectedItem() : nullptr;
	if (!Item.IsValid()) return FReply::Unhandled();

	const bool bAsset = Item->Kind == FPjcInspectorItem::EKind::Asset;
	if (Key == EKeys::Enter) {
		OnTreeDoubleClick(Item);
		return FReply::Handled();
	}
	if (InKeyEvent.IsControlDown() && Key == EKeys::C && bAsset) {
		PjcInspectorCopy(Model.Packages[Item->Node].ToString(), TEXT("the path"));
		return FReply::Handled();
	}
	if (InKeyEvent.IsControlDown() && Key == EKeys::B && bAsset) {
		PjcInspectorBrowse({Model.Assets[Item->Node]});
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

// actions

void SPjcTabInspector::DeleteFocus() {
	if (!Focus.IsNone()) {
		PjcActions::DeleteWithGroup({Focus});
	}
}

TSharedRef<SWidget> SPjcTabInspector::MakeExcludeMenu() {
	FMenuBuilder Menu {true, nullptr};
	const FAssetData Asset = (FocusNode != INDEX_NONE ? Model.Assets[FocusNode] : FAssetData {});
	if (!Asset.IsValid()) return Menu.MakeWidget();

	const FString ObjectPath = PjcShim::GetObjectPathString(Asset);
	const FString Folder = Asset.PackagePath.ToString();
	const UClass* AssetClass = Asset.GetClass();
	const FString ClassPath = AssetClass ? AssetClass->GetPathName() : FString {};
	const FString ClassLabel = PjcWidgets::GetAssetType(Asset).Label;

	PjcWidgets::AddMenuEntry(
		Menu,
		EPjcMenuIcon::Exclude,
		TEXT("Exclude asset ") + Asset.AssetName.ToString(),
		TEXT("Counts as used and is never deleted"),
		[ObjectPath, Asset]() {
			PjcActions::Exclude({}, {}, {ObjectPath}, TEXT("Excluded ") + Asset.AssetName.ToString());
		}
	);
	const TCHAR* FolderTip = TEXT("Counts every asset of the folder and its subfolders as used");
	PjcWidgets::AddMenuEntry(Menu, EPjcMenuIcon::Exclude, TEXT("Exclude folder ") + Folder, FolderTip, [Folder]() {
		PjcActions::Exclude({Folder}, {}, {}, TEXT("Excluded folder ") + Folder);
	});
	if (!ClassPath.IsEmpty()) {
		const TCHAR* ClassTip = TEXT("Counts every asset of the class as used");
		PjcWidgets::AddMenuEntry(Menu, EPjcMenuIcon::Exclude, TEXT("Exclude class ") + ClassLabel, ClassTip, [ClassPath, ClassLabel]() {
			PjcActions::Exclude({}, {ClassPath}, {}, TEXT("Excluded ") + ClassLabel);
		});
	}
	return Menu.MakeWidget();
}
