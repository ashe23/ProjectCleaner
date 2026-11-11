// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabInspector.h"
#include "Slate/PjcActions.h"
#include "Slate/PjcWidgets.h"
#include "Slate/SPjcClassBar.h"
#include "Slate/SPjcInspectorGraph.h"
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
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

namespace
{
	// unity builds join files: names unique to this file
	constexpr int32 PjcInspectorMaxChips = 8;
	constexpr int32 PjcInspectorMaxGroupRoots = 100;

	FLinearColor PjcInspectorHex(const TCHAR* Hex) {
		return FLinearColor {FColor::FromHex(Hex)};
	}

	FLinearColor PjcInspectorStatusColor(const EPjcAssetStatus Status, const bool bScanned) {
		if (!bScanned) return PjcInspectorHex(TEXT("#8A9099"));
		switch (Status) {
			case EPjcAssetStatus::Unused :	   return PjcInspectorHex(TEXT("#FE6D73"));
			case EPjcAssetStatus::EditorOnly : return PjcInspectorHex(TEXT("#7FB2FF"));
			case EPjcAssetStatus::Review :	   return PjcInspectorHex(TEXT("#E0B050"));
			default :						   return PjcInspectorHex(TEXT("#17C3B2"));
		}
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

	const TCHAR* PjcInspectorKindText(const EPjcLinkKind Kind) {
		switch (Kind) {
			case EPjcLinkKind::Soft :		return TEXT("soft");
			case EPjcLinkKind::EditorOnly : return TEXT("editor");
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

	FLinearColor PjcInspectorKindColor(const EPjcLinkKind Kind) {
		switch (Kind) {
			case EPjcLinkKind::Soft :		return PjcInspectorHex(TEXT("#C4A7FF"));
			case EPjcLinkKind::EditorOnly : return PjcInspectorHex(TEXT("#7FB2FF"));
			default :						return PjcInspectorHex(TEXT("#9FB3C8"));
		}
	}

	FAssetData PjcInspectorAssetOf(const FName Package) {
		TArray<FAssetData> Assets;
		UPjcSubsystem::GetModuleAssetRegistry().Get().GetAssetsByPackageName(Package, Assets);
		return Assets.Num() > 0 ? Assets[0] : FAssetData {};
	}

	// Toggle of a row of filters, on while IsOn
	TSharedRef<SWidget> PjcInspectorToggle(const FString& Label, const FString& ToolTip, TFunction<bool()> IsOn, TFunction<void()> OnClick) {
		// clang-format off
		return SNew(SCheckBox)
			.Style(&PjcShim::GetToggleStyle())
			.ToolTipText(FText::FromString(ToolTip))
			.IsChecked_Lambda([IsOn]() { return IsOn() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([OnClick](ECheckBoxState) { OnClick(); })
			[
				SNew(SBox).Padding(FMargin {4.0f, 1.0f})
				[
					SNew(STextBlock).Text(FText::FromString(Label)).Font(FPjcStyles::GetFont("Regular", 8))
				]
			];
		// clang-format on
	}

	TSharedRef<SWidget> PjcInspectorButton(const FString& Label, const FString& ToolTip, const FName Color, TFunction<void()> OnClick) {
		// clang-format off
		return SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
			.ButtonStyle(&PjcShim::GetButtonStyle427(!Color.IsNone()))
			.ForegroundColor(FSlateColor::UseForeground())
#endif
			.ButtonColorAndOpacity(Color.IsNone() ? FLinearColor::White : PjcWidgets::GetColor(Color))
			.ContentPadding(FMargin {10.0f, 2.0f})
			.ToolTipText(FText::FromString(ToolTip))
			.OnClicked_Lambda([OnClick]() {
				OnClick();
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
				.ColorAndOpacity(Color.IsNone() ? FSlateColor::UseForeground() : FSlateColor {FLinearColor::White})
			];
		// clang-format on
	}

	// Fact of the details: muted label, then the value
	TSharedRef<SWidget> PjcInspectorFact(const FString& Label, const TSharedRef<SWidget>& Value) {
		// clang-format off
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin {0.0f, 2.0f, 0.0f, 0.0f})
			[
				SNew(SBox).WidthOverride(84.0f)
				[
					SNew(STextBlock).Text(FText::FromString(Label)).ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				Value
			];
		// clang-format on
	}

	TSharedRef<SWidget> PjcInspectorText(const FString& Text, const FLinearColor& Color = FLinearColor::White) {
		return SNew(STextBlock).Text(FText::FromString(Text)).ColorAndOpacity(Color).AutoWrapText(true);
	}
}	// namespace

void SPjcTabInspector::Construct(const FArguments& InArgs) {
	TabManager = InArgs._TabManager;
	ScanFinishedHandle = UPjcSubsystem::OnScanFinished().AddSP(this, &SPjcTabInspector::OnScanFinished);

	// clang-format off
	ChildSlot
	[
		SNew(SPjcToolFrame).PageName(TEXT("Inspector")).PanelOpen(false)
		[
			SNew(SSplitter).PhysicalSplitterHandleSize(3.0f)
			+ SSplitter::Slot().Value(0.26f)
			[
				MakeFinder()
			]
			+ SSplitter::Slot().Value(0.44f)
			[
				MakeGraphPane()
			]
			+ SSplitter::Slot().Value(0.30f)
			[
				MakeDetails()
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
	if (Focus.IsNone()) {
		RebuildGraph();
		return;
	}

	// the asset may be gone (deleted, renamed)
	if (!Result.Usage.Contains(Focus) && !PjcInspectorAssetOf(Focus).IsValid()) {
		HistoryBack.Remove(Focus);
		HistoryForward.Remove(Focus);
		Focus = NAME_None;
		RebuildGraph();
		return;
	}
	const FName SelectedPackage = GetSelectedPackage();
	RebuildGraph();
	const int32 Node = Graph.FindNode(SelectedPackage);
	Select(Node != INDEX_NONE ? Node : Graph.FindNode(Focus));
}

void SPjcTabInspector::SetFocus(const FName Package, const bool bRecord) {
	if (Package.IsNone()) return;

	if (bRecord && !Focus.IsNone() && Package != Focus) {
		HistoryBack.Add(Focus);
		HistoryForward.Reset();
	}
	Focus = Package;
	OpenGroups.Reset();
	RebuildGraph();
	Select(Graph.FindNode(Focus));

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

void SPjcTabInspector::RebuildGraph() {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	if (Result.Time != CacheTime) {
		Cache.Reset();
		CacheTime = Result.Time;
	}
	// a map with thousands of actors takes seconds the first time: a progress window once it takes long
	FScopedSlowTask Reading {1.0f, FText::FromString(TEXT("Reading references..."))};
	Reading.MakeDialogDelayed(0.4f);
	Reading.EnterProgressFrame(1.0f);
	Graph = Focus.IsNone() || !Result.IsValid() ? FPjcInspectorGraph {} : FPjcInspectorGraph::Make(Result, Focus, OpenGroups, bEditorLinks, &Cache);
	Selected = INDEX_NONE;
	if (GraphView.IsValid()) {
		GraphView->SetGraph(&Result, Graph);
	}
	RebuildDetails();
}

void SPjcTabInspector::Select(const int32 Node) {
	Selected = Graph.Nodes.IsValidIndex(Node) && Graph.Nodes[Node].Kind != EPjcInspectorNodeKind::Source ? Node : INDEX_NONE;
	if (GraphView.IsValid()) {
		GraphView->SetSelected(Selected);
	}
	RebuildDetails();
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

FName SPjcTabInspector::GetSelectedPackage() const {
	if (!Graph.Nodes.IsValidIndex(Selected)) return NAME_None;

	const FPjcInspectorNode& Node = Graph.Nodes[Selected];
	return Node.Kind == EPjcInspectorNodeKind::Asset ? Node.Package : NAME_None;
}

SPjcTabInspector::FRowPtr SPjcTabInspector::MakeRow(const FName Package) const {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	const FAssetData Asset = PjcInspectorAssetOf(Package);
	const PjcWidgets::FAssetType& Type = PjcWidgets::GetAssetType(Asset);

	const FRowPtr Row = MakeShared<FPjcInspectorRow>();
	Row->Package = Package;
	Row->Name = FPackageName::GetShortName(Package);
	Row->Path = FPackageName::GetLongPackagePath(Package.ToString());
	Row->ClassLabel = Type.Label;
	Row->Icon = Type.Icon;
	Row->IconColor = Type.Color;
	Row->bScanned = Result.Usage.Contains(Package);
	Row->Status = Result.GetStatus(Package);
	return Row;
}

// finder

TSharedRef<SWidget> SPjcTabInspector::MakeFinder() {
	const auto Status = [this](const EStatusFilter Filter, const FString& Label, const FString& ToolTip) {
		return PjcInspectorToggle(
			Label,
			ToolTip,
			[this, Filter]() {
				return FinderStatus == Filter;
			},
			[this, Filter]() {
				FinderStatus = Filter;
				ApplyFinderFilter();
			}
		);
	};

	// clang-format off
	return SNew(SBorder).BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.GroupBorder"))).Padding(0.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {8.0f, 8.0f, 8.0f, 4.0f})
			[
				SAssignNew(FinderSearch, SSearchBox)
#if ENGINE_MAJOR_VERSION < 5
				.Style(&PjcShim::GetSearchBoxStyle427())
#endif
				.HintText(FText::FromString(TEXT("Search assets")))
				.OnTextChanged_Lambda([this](const FText& Text) {
					FinderText = Text.ToString();
					ApplyFinderFilter();
				})
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {8.0f, 0.0f, 8.0f, 4.0f})
			[
				SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D {2.0f, 2.0f})
				+ SWrapBox::Slot()
				[
					Status(EStatusFilter::All, TEXT("All"), TEXT("Every scanned asset"))
				]
				+ SWrapBox::Slot()
				[
					Status(EStatusFilter::Used, TEXT("Used"), TEXT("Used by the game"))
				]
				+ SWrapBox::Slot()
				[
					Status(EStatusFilter::EditorOnly, TEXT("Editor only"), TEXT("Used, but only the editor needs it. Cleanup keeps it."))
				]
				+ SWrapBox::Slot()
				[
					Status(EStatusFilter::Review, TEXT("Review"), TEXT("The scan is not sure about it. Cleanup keeps it."))
				]
				+ SWrapBox::Slot()
				[
					Status(EStatusFilter::Unused, TEXT("Unused"), TEXT("Nothing used references it. Cleanup deletes it."))
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {10.0f, 0.0f, 8.0f, 4.0f})
			[
				SNew(STextBlock)
				.Text_Lambda([this]() {
					return FText::FromString(PjcFormat::Count(FinderRows.Num()) + (FinderRows.Num() == 1 ? TEXT(" asset") : TEXT(" assets")));
				})
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.Font(FPjcStyles::GetFont("Regular", 8))
			]
			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
				SNew(SHorizontalBox)
				// class bar, as in the Assets tab: a class in its color is listed
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(184.0f)
					[
						SNew(SBorder).BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.DarkGroupBorder"))).Padding(FMargin {0.0f, 4.0f})
						[
							SAssignNew(FinderClasses, SPjcClassBar).OnChanged(FSimpleDelegate::CreateSP(this, &SPjcTabInspector::FilterFinderRows))
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f)
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
		];
	// clang-format on
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
	for (const FRowPtr& Row : FinderAll) {
		if (FinderStatus == EStatusFilter::Used && Row->Status != EPjcAssetStatus::Used) continue;
		if (FinderStatus == EStatusFilter::EditorOnly && Row->Status != EPjcAssetStatus::EditorOnly) continue;
		if (FinderStatus == EStatusFilter::Review && Row->Status != EPjcAssetStatus::Review) continue;
		if (FinderStatus == EStatusFilter::Unused && Row->Status != EPjcAssetStatus::Unused) continue;
		if (!FinderText.IsEmpty() && !Row->Name.Contains(FinderText) && !Row->Path.Contains(FinderText) && !Row->ClassLabel.Contains(FinderText))
			continue;

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
	// clang-format off
	return SNew(STableRow<FRowPtr>, Table)
#if ENGINE_MAJOR_VERSION < 5
		.Style(&PjcShim::GetRowStyle427())
#endif
		.Padding(FMargin {6.0f, 2.0f})
		.ToolTipText(
			FText::FromString(Row->Package.ToString() + TEXT("\n") + Row->ClassLabel + TEXT(" \u00B7 ") + PjcInspectorStatusText(Row->Status, true))
		)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 6.0f, 0.0f})
			[
				PjcWidgets::MakeSwatch(PjcInspectorStatusColor(Row->Status, true), 7.0f)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 6.0f, 0.0f})
			[
				SNew(SBox).WidthOverride(16.0f).HeightOverride(16.0f)
				[
					SNew(SImage).Image(Row->Icon).ColorAndOpacity(Row->IconColor)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(FText::FromString(Row->Name)).HighlightText_Lambda([this]() { return FText::FromString(FinderText); })
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(Row->Path))
					.Font(FPjcStyles::GetFont("Regular", 7))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					.HighlightText_Lambda([this]() { return FText::FromString(FinderText); })
				]
			]
		];
	// clang-format on
}

// graph

TSharedRef<SWidget> SPjcTabInspector::MakeGraphPane() {
	const auto Legend = [](const FString& Label, const FString& Color, const FString& ToolTip) {
		// clang-format off
		return SNew(SHorizontalBox).ToolTipText(FText::FromString(ToolTip))
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 5.0f, 0.0f})
			[
				SNew(SBox).WidthOverride(16.0f).HeightOverride(2.0f)
				[
					SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(PjcInspectorHex(*Color))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
				.Font(FPjcStyles::GetFont("Regular", 8))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			];
		// clang-format on
	};
	const auto Small = [](const FString& Label, const FString& ToolTip, TFunction<void()> OnClick) {
		// clang-format off
		return SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
			.ButtonStyle(&PjcShim::GetButtonStyle427(false))
			.ForegroundColor(FSlateColor::UseForeground())
#endif
			.ContentPadding(FMargin {6.0f, 1.0f})
			.ToolTipText(FText::FromString(ToolTip))
			.OnClicked_Lambda([OnClick]() {
				OnClick();
				return FReply::Handled();
			})
			[
				SNew(STextBlock).Text(FText::FromString(Label)).Font(FPjcStyles::GetFont("Regular", 8))
			];
		// clang-format on
	};

	// clang-format off
	return SNew(SBorder)
		.BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.GroupBorder")))
		.Padding(0.0f)
		[
			SNew(SVerticalBox)
			// head: history and what the graph shows
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.DarkGroupBorder")))
				.Padding(FMargin {8.0f, 6.0f})
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
						.ForegroundColor(FSlateColor::UseForeground())
#endif
						.ButtonStyle(&FPjcStyles::Get().GetWidgetStyle<FButtonStyle>("ProjectCleaner.Button.Row"))
						.ToolTipText(FText::FromString(TEXT("Back")))
						.IsEnabled_Lambda([this]() { return HistoryBack.Num() > 0; })
						.OnClicked_Lambda([this]() {
							Back();
							return FReply::Handled();
						})
						[
							SNew(SImage).Image(PjcShim::GetIconBack()).ColorAndOpacity(FSlateColor::UseForeground())
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {2.0f, 0.0f, 8.0f, 0.0f})
					[
						SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
						.ForegroundColor(FSlateColor::UseForeground())
#endif
						.ButtonStyle(&FPjcStyles::Get().GetWidgetStyle<FButtonStyle>("ProjectCleaner.Button.Row"))
						.ToolTipText(FText::FromString(TEXT("Forward")))
						.IsEnabled_Lambda([this]() { return HistoryForward.Num() > 0; })
						.OnClicked_Lambda([this]() {
							Forward();
							return FReply::Handled();
						})
						[
							SNew(SImage).Image(PjcShim::GetIconForward()).ColorAndOpacity(FSlateColor::UseForeground())
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(this, &SPjcTabInspector::GetGraphTitle).Font(FPjcStyles::GetFont("Bold", 10))
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text(this, &SPjcTabInspector::GetGraphSubtitle)
							.Font(FPjcStyles::GetFont("Regular", 8))
							.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						]
					]
				]
			]
			// the graph, view controls over it
			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SAssignNew(GraphView, SPjcInspectorGraph)
					.OnNodeClicked(this, &SPjcTabInspector::OnNodeClicked)
					.OnNodeDoubleClicked(this, &SPjcTabInspector::OnNodeDoubleClicked)
				]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Visibility_Lambda([this]() { return Focus.IsNone() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
					.Text_Lambda([]() {
						return FText::FromString(UPjcSubsystem::GetScanResult().IsValid()
							? TEXT("Pick an asset on the left, or Inspect one from another tool")
							: TEXT("Scan the project to inspect its assets"));
					})
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
				+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(10.0f)
				[
					SNew(SBorder)
					.BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.DarkGroupBorder")))
					.Padding(4.0f)
					.Visibility_Lambda([this]() { return Focus.IsNone() ? EVisibility::Collapsed : EVisibility::Visible; })
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()
						[
							Small(TEXT("\u2212"), TEXT("Zoom out (mouse wheel)"), [this]() { GraphView->ZoomBy(0.8f); })
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {4.0f, 0.0f})
						[
							SNew(SBox).WidthOverride(40.0f).HAlign(HAlign_Center)
							[
								SNew(STextBlock)
								.Text_Lambda([this]() {
									return FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(GraphView->GetZoom() * 100.0f)));
								})
								.Font(PjcWidgets::GetFontMono(8))
								.ColorAndOpacity(FSlateColor::UseSubduedForeground())
							]
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							Small(TEXT("+"), TEXT("Zoom in (mouse wheel)"), [this]() { GraphView->ZoomBy(1.25f); })
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})
						[
							Small(TEXT("Fit"), TEXT("Fits the whole graph (Home)"), [this]() { GraphView->FitAll(); })
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin {4.0f, 0.0f, 0.0f, 0.0f})
						[
							Small(TEXT("Frame selected"), TEXT("Frames the selected asset (F)"), [this]() { GraphView->FrameSelected(); })
						]
					]
				]
			]
			// legend
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.DarkGroupBorder")))
				.Padding(FMargin {10.0f, 5.0f})
				[
					SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D {14.0f, 2.0f})
					+ SWrapBox::Slot()
					[
						// unused group: its editor-only links are left out, they often close circles and do not ship
						SNew(SCheckBox)
						.ToolTipText(FText::FromString(TEXT("Draws the editor-only links of the group too. They do not ship and move nothing in the picture.")))
						.Visibility_Lambda([this]() { return Graph.bUnusedGroup ? EVisibility::Visible : EVisibility::Collapsed; })
						.IsChecked_Lambda([this]() { return bEditorLinks ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](const ECheckBoxState State) {
							bEditorLinks = State == ECheckBoxState::Checked;
							RebuildGraph();
							Select(Graph.FindNode(Focus));
						})
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("Editor-only links"))).Font(FPjcStyles::GetFont("Regular", 8))
						]
					]
					+ SWrapBox::Slot()
					[
						Legend(TEXT("Root source"), TEXT("#8A7440"), TEXT("Why the root is used: Asset Manager, config, code, exclusions"))
					]
					+ SWrapBox::Slot()
					[
						Legend(TEXT("Hard"), TEXT("#2C9A8F"), TEXT("Loads with the asset that references it"))
					]
					+ SWrapBox::Slot()
					[
						Legend(TEXT("Soft (dashed)"), TEXT("#2C9A8F"), TEXT("Loaded on demand by path"))
					]
					+ SWrapBox::Slot()
					[
						Legend(TEXT("Editor only (dotted)"), TEXT("#7FB2FF"), TEXT("Kept in the editor, not cooked"))
					]
					+ SWrapBox::Slot()
					[
						Legend(TEXT("Circular"), TEXT("#D2A640"), TEXT("Assets that reference each other in a circle. Arrows on the line: two assets use each other. A ring: the reference leads back to an asset that reaches it. Point at the mark to see which"))
					]
					+ SWrapBox::Slot()
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Hover lights a box and its links \u00B7 Click keeps them lit \u00B7 Double-click inspect \u00B7 Wheel zoom \u00B7 Drag pan \u00B7 F frame \u00B7 Home fit")))
						.Font(FPjcStyles::GetFont("Regular", 8))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
				]
			]
		];
	// clang-format on
}

void SPjcTabInspector::OnNodeClicked(const int32 Node) {
	if (!Graph.Nodes.IsValidIndex(Node)) return;

	// a source box stands for the root below it
	if (Graph.Nodes[Node].Kind == EPjcInspectorNodeKind::Source) {
		const FPjcInspectorEdge* Edge = Graph.Edges.FindByPredicate([Node](const FPjcInspectorEdge& Existing) {
			return Existing.From == Node;
		});
		if (Edge) Select(Edge->To);
		return;
	}
	Select(Node);
}

void SPjcTabInspector::OnNodeDoubleClicked(const int32 Node) {
	if (!Graph.Nodes.IsValidIndex(Node)) return;

	const FPjcInspectorNode& Shown = Graph.Nodes[Node];
	if (Shown.Kind == EPjcInspectorNodeKind::Group) {
		OpenGroups.Add(Shown.GroupClass);
		RebuildGraph();
		Select(Graph.FindNode(Focus));
		return;
	}
	if (Shown.Kind == EPjcInspectorNodeKind::Asset && Shown.Package != Focus) {
		SetFocus(Shown.Package, true);
	}
}

FText SPjcTabInspector::GetGraphTitle() const {
	return FText::FromString(Focus.IsNone() ? TEXT("Usage graph") : FPackageName::GetShortName(Focus));
}

FText SPjcTabInspector::GetGraphSubtitle() const {
	if (Focus.IsNone()) return FText::FromString(TEXT("Why an asset is used, from every root down to it"));

	if (Graph.bUnusedGroup) {
		const int32 Num = Graph.NumGroup;
		const TCHAR* Plural = Num == 2 ? TEXT("") : TEXT("s");
		// the group is what references each other; what a delete of this one takes with it is in the details
		const FString Together = FString::Printf(TEXT("Unused \u00B7 %d unused asset%s around it reference each other"), Num - 1, Plural);
		return FText::FromString(Num > 1 ? Together : FString {TEXT("Unused \u00B7 nothing references it")});
	}
	if (Graph.NumRoots == 0) return FText::FromString(TEXT("Not scanned \u00B7 outside the scanned content"));

	const FString RootsText = PjcFormat::Count(Graph.NumRoots);
	FString Text = Graph.NumRoots == 1 ? TEXT("1 root keeps it used") : FString::Printf(TEXT("%s roots keep it used"), *RootsText);
	if (Graph.NumHidden > 0) {
		Text += FString::Printf(TEXT(" \u00B7 %d longest paths drawn"), FPjcInspectorGraph::MaxPaths);
	}
	return FText::FromString(Text);
}

// details

TSharedRef<SWidget> SPjcTabInspector::MakeDetails() {
	// clang-format off
	return SNew(SBorder).BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.GroupBorder"))).Padding(0.0f)
		[
			SAssignNew(DetailsBox, SBox)
		];
	// clang-format on
}

void SPjcTabInspector::RebuildDetails() {
	if (!DetailsBox.IsValid()) return;

	UsedByRows.Reset();
	UsesRows.Reset();
	const bool bSeveral
		= Graph.Nodes.IsValidIndex(Selected) && Graph.Nodes[Selected].Kind != EPjcInspectorNodeKind::Asset && Graph.Nodes[Selected].Roots.Num() > 0;
	if (bSeveral) {
		// a group of roots, or a stack of unused assets with the same links
		DetailsBox->SetContent(MakeGroupDetails(Graph.Nodes[Selected]));
		return;
	}

	const FName Package = GetSelectedPackage();
	if (Package.IsNone()) {
		// clang-format off
		DetailsBox->SetContent(
			SNew(SBox).Padding(16.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Select an asset in the graph"))).ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		);
		// clang-format on
		return;
	}

	// what uses it and what it uses, through the actors a map saves in their own files
	const auto InGraph = [this](const FName From, const FName To) {
		const int32 FromNode = Graph.FindNode(From);
		const int32 ToNode = Graph.FindNode(To);
		return FromNode != INDEX_NONE && ToNode != INDEX_NONE && Graph.Edges.ContainsByPredicate([FromNode, ToNode](const FPjcInspectorEdge& Edge) {
			return Edge.From == FromNode && Edge.To == ToNode;
		});
	};
	TArray<FPjcInspectorLink> Links;
	FScopedSlowTask Reading {1.0f, FText::FromString(TEXT("Reading references..."))};
	Reading.MakeDialogDelayed(0.4f);
	Reading.EnterProgressFrame(1.0f);
	FPjcInspectorGraph::GetLinks(Package, true, Links, UsedByThrough, &Cache);
	for (const FPjcInspectorLink& Link : Links) {
		const FRowPtr Row = MakeRow(Link.Package);
		Row->Kind = Link.Kind;
		Row->bInGraph = InGraph(Link.Package, Package);
		UsedByRows.Add(Row);
	}
	FPjcInspectorGraph::GetLinks(Package, false, Links, UsesThrough, &Cache);
	for (const FPjcInspectorLink& Link : Links) {
		const FRowPtr Row = MakeRow(Link.Package);
		Row->Kind = Link.Kind;
		Row->bInGraph = InGraph(Package, Link.Package);
		UsesRows.Add(Row);
	}
	const auto ByName = [](const FRowPtr& A, const FRowPtr& B) {
		if (A->bInGraph != B->bInGraph) return A->bInGraph;
		return A->Name.Compare(B->Name, ESearchCase::IgnoreCase) < 0;
	};
	UsedByRows.Sort(ByName);
	UsesRows.Sort(ByName);

	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	const FAssetData Asset = PjcInspectorAssetOf(Package);
	const PjcWidgets::FAssetType& Type = PjcWidgets::GetAssetType(Asset);
	const bool bScanned = Result.Usage.Contains(Package);
	const EPjcAssetStatus Status = Result.GetStatus(Package);
	const FLinearColor StatusColor = PjcInspectorStatusColor(Status, bScanned);

	// clang-format off
	DetailsBox->SetContent(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {12.0f, 10.0f, 12.0f, 6.0f})
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 8.0f, 0.0f})
				[
					SNew(SBox).WidthOverride(20.0f).HeightOverride(20.0f)
					[
						SNew(SImage).Image(Type.Icon).ColorAndOpacity(Type.Color)
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(FText::FromString(FPackageName::GetShortName(Package))).Font(FPjcStyles::GetFont("Bold", 11))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {8.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.BorderBackgroundColor(StatusColor * FLinearColor {0.35f, 0.35f, 0.35f, 1.0f})
					.Padding(FMargin {7.0f, 1.0f})
					[
						SNew(STextBlock)
						.Text(FText::FromString(PjcInspectorStatusText(Status, bScanned)))
						.ColorAndOpacity(StatusColor)
						.Font(FPjcStyles::GetFont("Bold", 8))
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 4.0f, 0.0f, 8.0f})
			[
				SNew(STextBlock)
				.Text(
					FText::FromString(
						FString::Printf(TEXT("%s \u00B7 %s \u00B7 %s"), *Type.Label, *PjcFormat::Size(Result.GetSize(Package)), *Package.ToString())
					)
				)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.AutoWrapText(true)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				MakeFacts()
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 10.0f, 0.0f, 0.0f})
			[
				MakeActions()
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			SNew(SSplitter).Orientation(Orient_Horizontal).PhysicalSplitterHandleSize(3.0f)
			+ SSplitter::Slot().Value(0.5f)
			[
				MakeLinkList(true)
			]
			+ SSplitter::Slot().Value(0.5f)
			[
				MakeLinkList(false)
			]
		]
	);
	// clang-format on
}

TSharedRef<SWidget> SPjcTabInspector::MakeFacts() {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	const FName Package = GetSelectedPackage();
	const FPjcAssetUsage* Usage = Result.Usage.Find(Package);
	const TSharedRef<SVerticalBox> Facts = SNew(SVerticalBox);
	const auto Add = [&Facts](const FString& Label, const TSharedRef<SWidget>& Value) {
		// clang-format off
		Facts->AddSlot().AutoHeight().Padding(FMargin {0.0f, 2.0f})
		[
			PjcInspectorFact(Label, Value)
		];
		// clang-format on
	};

	if (!Usage) {
		Add(TEXT("Scan"), PjcInspectorText(TEXT("Not scanned: outside the scanned content, never deleted. What it uses stays used.")));
		return Facts;
	}

	Add(TEXT("Why"), PjcInspectorText(UPjcSubsystem::GetUsageReasonText(*Usage)));

	if (Usage->bUsed) {
		// roots of the asset: of the focus when it is the selection, else only the selection's own chain
		const FPjcInspectorGraph Own = Package == Focus ? Graph : FPjcInspectorGraph::Make(Result, Package, {}, false, &Cache);
		const TSharedRef<SWrapBox> Chips = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D {4.0f, 4.0f});
		for (int32 Index = 0; Index < Own.Roots.Num() && Index < PjcInspectorMaxChips; ++Index) {
			const FPjcInspectorRoot& Root = Own.Roots[Index];
			const FPjcAssetUsage* RootUsage = Result.Usage.Find(Root.Package);
			int32 Node = Graph.FindNode(Root.Package);
			if (Node == INDEX_NONE) {
				Node = Graph.Nodes.IndexOfByPredicate([&Root](const FPjcInspectorNode& Existing) {
					return Existing.Kind == EPjcInspectorNodeKind::Group && Existing.Roots.Contains(Root.Package);
				});
			}
			const TCHAR* StepPlural = Root.Steps == 1 ? TEXT("") : TEXT("s");
			const FString Steps = Root.Steps == 0 ? TEXT("itself") : FString::Printf(TEXT("%d step%s"), Root.Steps, StepPlural);
			const FString ReasonText = RootUsage ? UPjcSubsystem::GetUsageReasonText(*RootUsage) : FString {};
			const TCHAR* NotDrawn = TEXT("\nNot drawn: the graph shows the longest paths");
			const FString ToolTip = ReasonText + (Node == INDEX_NONE ? NotDrawn : TEXT("\nClick to select it in the graph"));
			// clang-format off
			Chips->AddSlot()
			[
				SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
				.ButtonStyle(&PjcShim::GetButtonStyle427(false))
				.ForegroundColor(FSlateColor::UseForeground())
#endif
				.ContentPadding(FMargin {6.0f, 1.0f})
				.ToolTipText(FText::FromString(ToolTip))
				.OnClicked_Lambda([this, Node]() {
					Select(Node);
					return FReply::Handled();
				})
				.OnHovered_Lambda([this, Node]() { GraphView->SetHighlighted(Node); })
				.OnUnhovered_Lambda([this]() { GraphView->SetHighlighted(INDEX_NONE); })
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 5.0f, 0.0f})
					[
						PjcWidgets::MakeSwatch(PjcInspectorHex(TEXT("#FFCB77")), 7.0f)
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(FPackageName::GetShortName(Root.Package))).Font(FPjcStyles::GetFont("Regular", 8))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {5.0f, 0.0f, 0.0f, 0.0f})
					[
						SNew(STextBlock)
						.Text(FText::FromString(Steps))
						.Font(FPjcStyles::GetFont("Regular", 7))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
				]
			];
			// clang-format on
		}
		if (Own.Roots.Num() > PjcInspectorMaxChips) {
			// clang-format off
			Chips->AddSlot()
			[
				PjcInspectorText(FString::Printf(TEXT("+%s more"), *PjcFormat::Count(Own.Roots.Num() - PjcInspectorMaxChips)), FLinearColor::Gray)
			];
			// clang-format on
		}
		if (Own.Roots.Num() > 0) {
			Add(Own.Roots.Num() == 1 ? TEXT("Root") : FString::Printf(TEXT("Roots (%s)"), *PjcFormat::Count(Own.Roots.Num())), Chips);
		}
		if (Result.GetStatus(Package) == EPjcAssetStatus::EditorOnly) {
			Add(TEXT("Shipping"),
				PjcInspectorText(TEXT("Not shipped: only the editor uses it. Cleanup keeps it."), PjcInspectorHex(TEXT("#9FC3FF"))));
		}
		if (Result.GetStatus(Package) == EPjcAssetStatus::Review) {
			const TCHAR* ReviewText = TEXT("The scan is not sure about it. Cleanup keeps it: delete or keep it.");
			Add(TEXT("Review"), PjcInspectorText(ReviewText, PjcInspectorHex(TEXT("#E0B050"))));
		}
		return Facts;
	}

	// unused: what cleanup deletes with it
	TArray<FName> Group;
	FPjcUnusedTree::GetDeleteGroup(Result, {Package}, Group);
	int64 Size = Result.GetSize(Package);
	for (const FName Other : Group) {
		Size += Result.GetSize(Other);
	}
	const TCHAR* GroupPlural = Group.Num() == 1 ? TEXT("") : TEXT("s");
	const FString SizeText = PjcFormat::Size(Size);
	const FString GroupText = PjcFormat::Count(Group.Num());
	const FString WithGroup = FString::Printf(TEXT("Deleted with %s unused asset%s \u00B7 %s"), *GroupText, GroupPlural, *SizeText);
	const FString Cleanup = Group.Num() > 0 ? WithGroup : FString::Printf(TEXT("Deleted \u00B7 %s"), *SizeText);
	Add(TEXT("Cleanup"), PjcInspectorText(Cleanup));
	if (const FString* Holder = UPjcSubsystem::GetLastCleanupKept().Find(Package)) {
		Add(TEXT("Last cleanup"), PjcInspectorText(TEXT("Not deleted \u00B7 ") + *Holder, PjcInspectorHex(TEXT("#FFCB77"))));
	}
	return Facts;
}

TSharedRef<SWidget> SPjcTabInspector::MakeActions() {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	const FName Package = GetSelectedPackage();
	const bool bScanned = Result.Usage.Contains(Package);
	const TSharedRef<SWrapBox> Actions = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D {4.0f, 4.0f});

	if (Package != Focus) {
		// clang-format off
		Actions->AddSlot()
		[
			PjcInspectorButton(
				TEXT("Inspect"),
				TEXT("Shows why this asset is used"),
				TEXT("ProjectCleaner.Color.Accent"),
				[this, Package]() { SetFocus(Package, true); }
			)
		];
		// clang-format on
	}
	const bool bDeletable = !Result.IsUsed(Package) || Result.GetStatus(Package) == EPjcAssetStatus::Review;
	if (bScanned && bDeletable) {
		// clang-format off
		Actions->AddSlot()
		[
			PjcInspectorButton(
				TEXT("Delete\u2026"),
				TEXT("Deletes it with the unused assets only it keeps, after asking"),
				TEXT("ProjectCleaner.Color.Red"),
				[this]() { DeleteSelected(); }
			)
		];
		// clang-format on
	}
	if (bScanned) {
		// clang-format off
		Actions->AddSlot()
		[
			SNew(SComboButton)
#if ENGINE_MAJOR_VERSION < 5
			.ButtonStyle(&PjcShim::GetButtonStyle427(false))
			.ForegroundColor(FSlateColor::UseForeground())
#endif
			.ContentPadding(FMargin {6.0f, 1.0f})
			.ToolTipText(FText::FromString(TEXT("Counts it as used from now on, saved to the project settings")))
			.OnGetMenuContent(this, &SPjcTabInspector::MakeExcludeMenu)
			.ButtonContent()
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Exclude")))
			]
		];
		// clang-format on
	}
	// clang-format off
	Actions->AddSlot()
	[
		PjcInspectorButton(TEXT("Open"), TEXT("Opens the asset editor"), NAME_None, [this]() { OpenSelected(); })
	];
	Actions->AddSlot()
	[
		PjcInspectorButton(TEXT("Browse"), TEXT("Selects the asset in the Content Browser"), NAME_None, [this]() { BrowseSelected(); })
	];
	// clang-format on
	return Actions;
}

TSharedRef<SWidget> SPjcTabInspector::MakeGroupDetails(const FPjcInspectorNode& Node) {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	const FAssetData First = Node.Roots.Num() > 0 ? PjcInspectorAssetOf(Node.Roots[0]) : FAssetData {};
	const PjcWidgets::FAssetType& Type = PjcWidgets::GetAssetType(First);
	int64 Size = 0;
	for (const FName Root : Node.Roots) {
		Size += Result.GetSize(Root);
	}

	UsedByRows.Reset();
	for (int32 Index = 0; Index < Node.Roots.Num() && Index < PjcInspectorMaxGroupRoots; ++Index) {
		UsedByRows.Add(MakeRow(Node.Roots[Index]));
	}
	const FName GroupClass = Node.GroupClass;
	const bool bStack = Node.Kind == EPjcInspectorNodeKind::Stack;
	const FString Each = FString::Printf(TEXT("%d × %s"), Node.Roots.Num(), *Type.Label);
	const FString Title = bStack ? Each : Each + TEXT(" roots");
	const TCHAR* What = bStack ? TEXT("Unused assets with the same links, drawn as one box") : TEXT("Roots of one class, drawn as one box");

	// clang-format off
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {12.0f, 10.0f, 12.0f, 6.0f})
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 8.0f, 0.0f})
				[
					SNew(SBox).WidthOverride(20.0f).HeightOverride(20.0f)
					[
						SNew(SImage).Image(Type.Icon).ColorAndOpacity(Type.Color)
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Title))
					.Font(FPjcStyles::GetFont("Bold", 11))
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 4.0f, 0.0f, 8.0f})
			[
				PjcInspectorText(FString::Printf(TEXT("%s \u00B7 %s"), What, *PjcFormat::Size(Size)), FLinearColor::Gray)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				.Visibility(bStack ? EVisibility::Collapsed : EVisibility::Visible)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					PjcInspectorButton(
						TEXT("Show each"),
						TEXT("Draws the roots one by one (also: double-click the box)"),
						TEXT("ProjectCleaner.Color.Accent"),
						[this, GroupClass]() { OpenGroups.Add(GroupClass); RebuildGraph(); Select(Graph.FindNode(Focus)); }
					)
				]
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			MakeLinkList(true)
		];
	// clang-format on
}

TSharedRef<SWidget> SPjcTabInspector::MakeLinkList(const bool bUsedBy) {
	const bool bStack = Graph.Nodes.IsValidIndex(Selected) && Graph.Nodes[Selected].Kind == EPjcInspectorNodeKind::Stack;
	const bool bGroup = bStack || (Graph.Nodes.IsValidIndex(Selected) && Graph.Nodes[Selected].Kind == EPjcInspectorNodeKind::Group);
	TArray<FRowPtr>& Rows = bUsedBy ? UsedByRows : UsesRows;
	int32 NumUnused = 0;
	for (const FRowPtr& Row : Rows) {
		if (Row->bScanned && Row->Status == EPjcAssetStatus::Unused) ++NumUnused;
	}
	FString Title = bStack ? TEXT("Assets") : (bGroup ? TEXT("Roots") : (bUsedBy ? TEXT("Used by") : TEXT("Uses")));
	Title += FString::Printf(TEXT("  %s"), *PjcFormat::Count(Rows.Num()));
	if (NumUnused > 0) {
		Title += FString::Printf(TEXT(" \u00B7 %s unused"), *PjcFormat::Count(NumUnused));
	}
	// actors a World Partition map saves in their own files are not listed: the list goes on through them
	const int32 NumThrough = bGroup ? 0 : (bUsedBy ? UsedByThrough : UsesThrough);
	const FString Through = NumThrough == 0
		? FString {}
		: FString::Printf(TEXT("through %s level actor%s"), *PjcFormat::Count(NumThrough), NumThrough == 1 ? TEXT("") : TEXT("s"));
	const TCHAR* ThroughTip = TEXT("World Partition saves each actor of a level in its own file. They are part of the level, not assets: ")
		TEXT("the list shows what is on their other side.");
	const TCHAR* NoUsers = Graph.bUnusedGroup ? TEXT("Nothing references it") : TEXT("Nothing references it: it is a root");
	const FString Empty = bUsedBy ? NoUsers : TEXT("Uses no other asset");

	TSharedPtr<SListView<FRowPtr>>& List = bUsedBy ? UsedByList : UsesList;
	// clang-format off
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBorder).BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.DarkGroupBorder"))).Padding(FMargin {12.0f, 4.0f})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(FText::FromString(Title)).Font(FPjcStyles::GetFont("Bold", 9))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {8.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(STextBlock)
					.Text(FText::FromString(Through))
					.ToolTipText(FText::FromString(ThroughTip))
					.Font(FPjcStyles::GetFont("Regular", 8))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					.Visibility(NumThrough > 0 ? EVisibility::Visible : EVisibility::Collapsed)
				]
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SAssignNew(List, SListView<FRowPtr>)
				.ListItemsSource(&Rows)
				.SelectionMode(ESelectionMode::Single)
				.OnGenerateRow(this, &SPjcTabInspector::OnGenerateLinkRow)
				.OnMouseButtonClick(this, &SPjcTabInspector::OnLinkClicked)
				.OnMouseButtonDoubleClick_Lambda([this](FRowPtr Row) {
					if (Row.IsValid()) SetFocus(Row->Package, true);
				})
			]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(FMargin {0.0f, 12.0f})
			[
				SNew(STextBlock)
				.Visibility(Rows.Num() == 0 ? EVisibility::HitTestInvisible : EVisibility::Collapsed)
				.Text(FText::FromString(Empty))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		];
	// clang-format on
}

TSharedRef<ITableRow> SPjcTabInspector::OnGenerateLinkRow(FRowPtr Row, const TSharedRef<STableViewBase>& Table) {
	const bool bStack = Graph.Nodes.IsValidIndex(Selected) && Graph.Nodes[Selected].Kind == EPjcInspectorNodeKind::Stack;
	const bool bGroup = bStack || (Graph.Nodes.IsValidIndex(Selected) && Graph.Nodes[Selected].Kind == EPjcInspectorNodeKind::Group);
	const FString Where = Row->Package.ToString() + TEXT("\n") + Row->ClassLabel;
	const TCHAR* Drawn = Row->bInGraph ? TEXT(" \u00B7 drawn in the graph") : TEXT("");
	const TCHAR* Status = PjcInspectorStatusText(Row->Status, Row->bScanned);
	const FString ToolTip = FString::Printf(TEXT("%s \u00B7 %s%s\nDouble-click to inspect it"), *Where, Status, Drawn);
	const FName Package = Row->Package;

	// clang-format off
	return SNew(STableRow<FRowPtr>, Table)
#if ENGINE_MAJOR_VERSION < 5
		.Style(&PjcShim::GetRowStyle427())
#endif
		.Padding(FMargin {8.0f, 2.0f})
		.ToolTipText(FText::FromString(ToolTip))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 6.0f, 0.0f})
			[
				PjcWidgets::MakeSwatch(PjcInspectorStatusColor(Row->Status, Row->bScanned), 7.0f)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 6.0f, 0.0f})
			[
				SNew(SBox).WidthOverride(16.0f).HeightOverride(16.0f)
				[
					SNew(SImage).Image(Row->Icon).ColorAndOpacity(Row->IconColor)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(Row->Name)).Font(FPjcStyles::GetFont(Row->bInGraph ? "Bold" : "Regular", 9))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock)
				.Visibility(bGroup ? EVisibility::Collapsed : EVisibility::Visible)
				.Text(FText::FromString(PjcInspectorKindText(Row->Kind)))
				.ToolTipText(FText::FromString(PjcInspectorKindToolTip(Row->Kind)))
				.Font(PjcWidgets::GetFontMono(7))
				.ColorAndOpacity(PjcInspectorKindColor(Row->Kind))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {4.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
				.ForegroundColor(FSlateColor::UseForeground())
#endif
				.ButtonStyle(&FPjcStyles::Get().GetWidgetStyle<FButtonStyle>("ProjectCleaner.Button.Row"))
				.ContentPadding(FMargin {4.0f, 0.0f})
				.ToolTipText(FText::FromString(TEXT("Inspect ") + Row->Name))
				.OnClicked_Lambda([this, Package]() {
					SetFocus(Package, true);
					return FReply::Handled();
				})
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("\u2192"))).Font(FPjcStyles::GetFont("Regular", 8))
				]
			]
		];
	// clang-format on
}

void SPjcTabInspector::OnLinkClicked(FRowPtr Row) {
	if (!Row.IsValid() || !GraphView.IsValid()) return;

	// shown in the graph: mark it there
	const int32 Node = Graph.FindNode(Row->Package);
	GraphView->SetHighlighted(Node);
}

TSharedRef<SWidget> SPjcTabInspector::MakeExcludeMenu() {
	FMenuBuilder Menu {true, nullptr};
	const FAssetData Asset = PjcInspectorAssetOf(GetSelectedPackage());
	if (!Asset.IsValid()) return Menu.MakeWidget();

	const FString ObjectPath = PjcShim::GetObjectPathString(Asset);
	const FString Folder = Asset.PackagePath.ToString();
	const UClass* AssetClass = Asset.GetClass();
	const FString ClassPath = AssetClass ? AssetClass->GetPathName() : FString {};
	const FString ClassLabel = PjcWidgets::GetAssetType(Asset).Label;

	PjcWidgets::AddMenuEntry(
		Menu, EPjcMenuIcon::Exclude, TEXT("Exclude asset ") + Asset.AssetName.ToString(), TEXT("Counts this asset as used"), [ObjectPath, Asset]() {
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

void SPjcTabInspector::DeleteSelected() {
	const FName Package = GetSelectedPackage();
	if (!Package.IsNone()) {
		PjcActions::DeleteWithGroup({Package});
	}
}

void SPjcTabInspector::OpenSelected() const {
	const FAssetData Asset = PjcInspectorAssetOf(GetSelectedPackage());
	if (Asset.IsValid()) {
		UPjcSubsystem::OpenAssetEditor(Asset);
	}
}

void SPjcTabInspector::BrowseSelected() const {
	const FAssetData Asset = PjcInspectorAssetOf(GetSelectedPackage());
	if (Asset.IsValid()) {
		UPjcSubsystem::GetModuleContentBrowser().Get().SyncBrowserToAssets(TArray<FAssetData> {Asset});
	}
}
