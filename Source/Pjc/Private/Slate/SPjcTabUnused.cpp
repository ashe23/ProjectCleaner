// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabUnused.h"
#include "Slate/PjcActions.h"
#include "Slate/PjcTabs.h"
#include "Slate/PjcWidgets.h"
#include "Slate/SPjcClassBar.h"
#include "Slate/SPjcScanBar.h"
#include "Slate/SPjcSizeMap.h"
#include "Slate/SPjcOverviewPanel.h"
#include "Slate/SPjcToolFrame.h"
#include "PjcFormat.h"
#include "PjcSettings.h"
#include "PjcShim.h"
#include "PjcStyles.h"
#include "PjcSubsystem.h"
// Engine Headers
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Input/DragAndDrop.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SExpanderArrow.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"

namespace
{
	// unity builds join files: names unique to this file
	const TCHAR* PjcUnusedSection = TEXT("ProjectCleaner.AssetsUnused");
	const FName PjcUnusedColFolder {TEXT("Folder")};
	const FName PjcUnusedColShare {TEXT("Unused")};
	const FName PjcUnusedColSize {TEXT("Size")};
	const FName PjcUnusedColAssets {TEXT("Assets")};

	// tree indent and the middle of the arrow: where a folder's guide runs
	constexpr float PjcUnusedIndent = 14.0f;
	constexpr float PjcUnusedArrowMiddle = 5.0f;

	// Folder guides (View): a thin line under the arrow of every folder above, down through its subfolders. Drawn by the
	// plugin: the engine's tree wires look different in UE 4.27 and UE5
	class SPjcTreeGuides final : public SLeafWidget
	{
	  public:

		SLATE_BEGIN_ARGS(SPjcTreeGuides) {}
		SLATE_ATTRIBUTE(int32, Depth)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs) {
			Depth = InArgs._Depth;
		}

		virtual FVector2D ComputeDesiredSize(float) const override {
			return FVector2D {0.0f, 0.0f};
		}

		virtual int32 OnPaint(
			const FPaintArgs&,
			const FGeometry& AllottedGeometry,
			const FSlateRect&,
			FSlateWindowElementList& OutDrawElements,
			int32 LayerId,
			const FWidgetStyle&,
			bool
		) const override {
			const FSlateBrush* Brush = FCoreStyle::Get().GetBrush("WhiteBrush");
			const FLinearColor Color = PjcWidgets::GetColor("ProjectCleaner.Color.Line");
			const FVector2D Size {1.0f, AllottedGeometry.GetLocalSize().Y};
			const int32 Levels = Depth.Get(0);
			for (int32 Level = 0; Level < Levels; ++Level) {
				const FVector2D Position {PjcUnusedIndent * Level + PjcUnusedArrowMiddle, 0.0f};
				const FPaintGeometry Line = AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform {Position});
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId, Line, Brush, ESlateDrawEffect::None, Color);
			}
			return LayerId;
		}

	  private:

		TAttribute<int32> Depth;
	};

	// Folder tree row: name, share bar of the list, its size, listed / all assets
	class SPjcUnusedFolderRow final : public SMultiColumnTableRow<TSharedPtr<FPjcUnusedFolder>>
	{
	  public:

		SLATE_BEGIN_ARGS(SPjcUnusedFolderRow) {}
		SLATE_ARGUMENT(TSharedPtr<FPjcUnusedFolder>, Folder)
		SLATE_ARGUMENT(EPjcAssetSet, Set)
		SLATE_ATTRIBUTE(FText, HighlightText)
		SLATE_ATTRIBUTE(bool, ShowGuides)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable) {
			Folder = InArgs._Folder;
			Set = InArgs._Set;
			HighlightText = InArgs._HighlightText;
			ShowGuides = InArgs._ShowGuides;
			SMultiColumnTableRow::FArguments RowArgs;
#if ENGINE_MAJOR_VERSION < 5
			RowArgs.Style(&PjcShim::GetRowStyle427());
#endif
			// no gap between rows: the folder guides run through (the name column keeps the row height)
			SMultiColumnTableRow::Construct(RowArgs.Padding(FMargin {0.0f}), InTable);
		}

		virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Column) override {
			const FSlateColor Muted = FSlateColor::UseSubduedForeground();
			const bool bAnyListed = Folder->Listed.Num > 0;

			if (Column == PjcUnusedColFolder) {
				// the folder icon says its state, no tags (user, 2026-11-14): gold, red when all of it is unused, gray when empty
				const bool bAllUnused = Set == EPjcAssetSet::Unused && Folder->IsAllListed();
				const FName Tint
					= Folder->bEmpty ? "ProjectCleaner.Color.Muted" : (bAllUnused ? "ProjectCleaner.Color.Red" : "ProjectCleaner.Color.FolderTint");
				FString ToolTip = Folder->Path;
				if (Folder->bEmpty) {
					ToolTip += TEXT("\nEmpty: no assets or files. Clean project deletes it.");
				}
				else if (bAllUnused) {
					ToolTip += TEXT("\nAll unused: Clean project deletes the whole folder.");
				}
				// clang-format off
				const TSharedRef<SHorizontalBox> NameBox = SNew(SHorizontalBox)
					.Clipping(EWidgetClipping::ClipToBounds)
					.ToolTipText(FText::FromString(ToolTip));
				NameBox->AddSlot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SExpanderArrow, SharedThis(this)).StyleSet(&FPjcStyles::Get()).IndentAmount(PjcUnusedIndent).ShouldDrawWires(false)
				];
				NameBox->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 5.0f, 0.0f})
				[
					SNew(SImage)
					.Image_Lambda([this]() {
						return FPjcStyles::Get().GetBrush(IsItemExpanded() ? "ProjectCleaner.Icon.FolderOpen" : "ProjectCleaner.Icon.Folder");
					})
					.ColorAndOpacity(PjcWidgets::GetColor(Tint))
				];
				NameBox->AddSlot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(FText::FromString(Folder->IsRoot() ? Folder->Path.Mid(1) : Folder->Name)).HighlightText(HighlightText)
				];
				return SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SPjcTreeGuides)
						.Visibility_Lambda([this]() { return ShowGuides.Get(true) ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
						.Depth_Lambda([this]() { return GetIndentLevel(); })
					]
					+ SOverlay::Slot()
					[
						SNew(SBox).Padding(FMargin {0.0f, 1.0f})
						[
							NameBox
						]
					];
				// clang-format on
			}

			if (Column == PjcUnusedColShare) {
				PjcWidgets::FBarSegment Segment;
				Segment.Value = static_cast<double>(Folder->Listed.Size);
				Segment.Color = PjcWidgets::GetSetColors(Set).Bar;

				// clang-format off
				return SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(FMargin {6.0f, 0.0f, 4.0f, 0.0f})
					[
						PjcWidgets::MakeBar({Segment}, 6.0f, static_cast<double>(Folder->All.Size))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox).WidthOverride(34.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(bAnyListed ? PjcFormat::Percent(Folder->Listed.Size, Folder->All.Size) : FString {}))
							.Font(PjcWidgets::GetFontMono(8))
							.ColorAndOpacity(Muted)
							.Justification(ETextJustify::Right)
						]
					];
				// clang-format on
			}

			if (Column == PjcUnusedColSize) {
				// clang-format off
				return SNew(SBox).HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 8.0f, 0.0f})
					[
						SNew(STextBlock)
						.Text(FText::FromString(bAnyListed ? PjcFormat::Size(Folder->Listed.Size) : TEXT("\u2014")))
						.Font(PjcWidgets::GetFontMono(9))
						.ColorAndOpacity(bAnyListed ? FSlateColor::UseForeground() : Muted)
					];
				// clang-format on
			}

			if (Column == PjcUnusedColAssets) {
				const FString AllText = PjcFormat::Count(Folder->All.Num);
				const FString Text = bAnyListed ? PjcFormat::Count(Folder->Listed.Num) + TEXT(" / ") + AllText : AllText;
				// clang-format off
				return SNew(SBox).HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 8.0f, 0.0f})
					[
						SNew(STextBlock).Text(FText::FromString(Text)).Font(PjcWidgets::GetFontMono(9)).ColorAndOpacity(Muted)
					];
				// clang-format on
			}

			return SNullWidget::NullWidget;
		}

	  private:

		TSharedPtr<FPjcUnusedFolder> Folder;
		EPjcAssetSet Set = EPjcAssetSet::Unused;
		TAttribute<FText> HighlightText;
		TAttribute<bool> ShowGuides;
	};

	const FName PjcUnusedColName {TEXT("Name")};
	const FName PjcUnusedColReason {TEXT("Reason")};
	const FName PjcUnusedColClass {TEXT("Class")};
	const FName PjcUnusedColAssetSize {TEXT("AssetSize")};
	const FName PjcUnusedColPath {TEXT("Path")};

	// A column header being dragged to another place
	class FPjcColumnDrag final : public FDragDropOperation
	{
	  public:

		DRAG_DROP_OPERATOR_TYPE(FPjcColumnDrag, FDragDropOperation)

		FName Column;
		FString Label;

		static TSharedRef<FPjcColumnDrag> New(const FName InColumn, const FString& InLabel) {
			const TSharedRef<FPjcColumnDrag> Operation = MakeShared<FPjcColumnDrag>();
			Operation->Column = InColumn;
			Operation->Label = InLabel;
			Operation->Construct();
			return Operation;
		}

		virtual TSharedPtr<SWidget> GetDefaultDecorator() const override {
			// clang-format off
			return SNew(SBorder).BorderImage(PjcShim::GetBrush(TEXT("Menu.Background"))).Padding(FMargin {8.0f, 3.0f})
				[
					SNew(STextBlock).Text(FText::FromString(Label))
				];
			// clang-format on
		}
	};

	DECLARE_DELEGATE_TwoParams(FPjcOnColumnMoved, const FName /* Column */, const FName /* Before */);

	// Header label: a click sorts, a drag onto another header moves the column there
	class SPjcColumnLabel final : public SCompoundWidget
	{
	  public:

		SLATE_BEGIN_ARGS(SPjcColumnLabel) {}
		SLATE_ARGUMENT(FName, Column)
		SLATE_ARGUMENT(FString, Label)
		SLATE_ARGUMENT(EHorizontalAlignment, HAlign)
		SLATE_EVENT(FPjcOnColumnMoved, OnMoved)
		SLATE_EVENT(FSimpleDelegate, OnClicked)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs) {
			Column = InArgs._Column;
			Label = InArgs._Label;
			OnMoved = InArgs._OnMoved;
			OnClicked = InArgs._OnClicked;
			// clang-format off
			ChildSlot
			.HAlign(InArgs._HAlign)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor_Lambda([this]() { return bDropTarget ? FLinearColor {1.0f, 1.0f, 1.0f, 0.12f} : FLinearColor::Transparent; })
				.Padding(FMargin {0.0f})
				[
					SNew(STextBlock).Text(FText::FromString(Label))
				]
			];
			// clang-format on
		}

		virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override {
			if (Event.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();

			return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
		}

		virtual FReply OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override {
			if (Event.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();

			OnClicked.ExecuteIfBound();
			return FReply::Handled();
		}

		virtual FReply OnDragDetected(const FGeometry& Geometry, const FPointerEvent& Event) override {
			return FReply::Handled().BeginDragDrop(FPjcColumnDrag::New(Column, Label));
		}

		virtual void OnDragEnter(const FGeometry& Geometry, const FDragDropEvent& Event) override {
			const TSharedPtr<FPjcColumnDrag> Drag = Event.GetOperationAs<FPjcColumnDrag>();
			bDropTarget = Drag.IsValid() && Drag->Column != Column;
		}

		virtual void OnDragLeave(const FDragDropEvent& Event) override {
			bDropTarget = false;
		}

		virtual FReply OnDrop(const FGeometry& Geometry, const FDragDropEvent& Event) override {
			bDropTarget = false;
			const TSharedPtr<FPjcColumnDrag> Drag = Event.GetOperationAs<FPjcColumnDrag>();
			if (!Drag.IsValid() || Drag->Column == Column) return FReply::Unhandled();

			OnMoved.ExecuteIfBound(Drag->Column, Column);
			return FReply::Handled();
		}

	  private:

		FName Column;
		FString Label;
		FPjcOnColumnMoved OnMoved;
		FSimpleDelegate OnClicked;
		bool bDropTarget = false;
	};

	// Asset row: type color and icon, name with tags, reason, class, size, folder
	class SPjcUnusedAssetRow final : public SMultiColumnTableRow<TSharedPtr<FPjcUnusedAssetRow>>
	{
	  public:

		SLATE_BEGIN_ARGS(SPjcUnusedAssetRow) {}
		SLATE_ARGUMENT(TSharedPtr<FPjcUnusedAssetRow>, Row)
		SLATE_ATTRIBUTE(FText, HighlightText)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable) {
			Row = InArgs._Row;
			HighlightText = InArgs._HighlightText;
			SMultiColumnTableRow::FArguments RowArgs;
#if ENGINE_MAJOR_VERSION < 5
			RowArgs.Style(&PjcShim::GetRowStyle427());
#endif
			SMultiColumnTableRow::Construct(RowArgs.Padding(FMargin {0.0f, 1.0f}), InTable);
		}

		virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Column) override {
			const FSlateColor Muted = FSlateColor::UseSubduedForeground();

			if (Column == PjcUnusedColName) {
				const TSharedRef<SHorizontalBox> NameBox = SNew(SHorizontalBox).Clipping(EWidgetClipping::ClipToBounds);
				// clang-format off
				NameBox->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {4.0f, 0.0f, 6.0f, 0.0f})
				[
					SNew(SBox).WidthOverride(3.0f).HeightOverride(16.0f)
					[
						SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(Row->Color)
					]
				];
				NameBox->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 6.0f, 0.0f})
				[
					SNew(SBox).WidthOverride(16.0f).HeightOverride(16.0f)
					[
						SNew(SImage).Image(Row->Icon)
					]
				];
				NameBox->AddSlot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(FText::FromString(Row->Name)).HighlightText(HighlightText)
				];
				// clang-format on
				if (!Row->Holder.IsEmpty()) {
					// clang-format off
					NameBox->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {6.0f, 0.0f, 4.0f, 0.0f})
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
						.BorderBackgroundColor(PjcWidgets::GetColor("ProjectCleaner.Color.DangerQuiet"))
						.Padding(FMargin {4.0f, 0.0f})
						.ToolTipText(
							FText::FromString(
								FString::Printf(TEXT("Delete failed last time: %s"), *Row->Holder)
							)
						)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("DELETE FAILED")))
							.Font(PjcWidgets::GetFontMono(7))
							.ColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.DangerText"))
						]
					];
					// clang-format on
				}
				if (Row->bLabel) {
					// clang-format off
					NameBox->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {6.0f, 0.0f, 4.0f, 0.0f})
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
						.BorderBackgroundColor(PjcWidgets::GetColor("ProjectCleaner.Color.ReviewQuiet"))
						.Padding(FMargin {4.0f, 0.0f})
						.ToolTipText(FText::FromString(TEXT("In the folder of a Primary Asset Label whose cook rule does not package it: only references keep it")))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("LABEL")))
							.Font(PjcWidgets::GetFontMono(7))
							.ColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.Yellow"))
						]
					];
					// clang-format on
				}
				return NameBox;
			}

			if (Column == PjcUnusedColClass) {
				// clang-format off
				return SNew(SBox).VAlign(VAlign_Center).ToolTipText(FText::FromName(Row->ClassName))
					[
						SNew(STextBlock).Text(FText::FromString(Row->ClassLabel)).ColorAndOpacity(Muted)
					];
				// clang-format on
			}

			if (Column == PjcUnusedColAssetSize) {
				// clang-format off
				return SNew(SBox).HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 8.0f, 0.0f})
					[
						SNew(STextBlock).Text(FText::FromString(PjcFormat::Size(Row->Size))).Font(PjcWidgets::GetFontMono(9))
					];
				// clang-format on
			}

			if (Column == PjcUnusedColPath) {
				// clang-format off
				return SNew(SBox).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(Row->Folder.IsEmpty() ? TEXT("\u00B7") : Row->Folder)).ColorAndOpacity(Muted)
					];
				// clang-format on
			}

			if (Column == PjcUnusedColReason) {
				const FSlateColor ReasonColor = Row->bReasonWarn ? FSlateColor {PjcWidgets::GetColor("ProjectCleaner.Color.Yellow")} : Muted;
				// clang-format off
				return SNew(SBox)
					.VAlign(VAlign_Center)
					.Padding(FMargin {0.0f, 0.0f, 8.0f, 0.0f})
					.ToolTipText(FText::FromString(Row->ReasonTip))
					[
						SNew(STextBlock).Text(FText::FromString(Row->Reason)).ColorAndOpacity(ReasonColor)
					];
				// clang-format on
			}

			return SNullWidget::NullWidget;
		}

	  private:

		TSharedPtr<FPjcUnusedAssetRow> Row;
		TAttribute<FText> HighlightText;
	};
}	// namespace

void SPjcTabUnused::Construct(const FArguments& InArgs) {
	TabManager = InArgs._TabManager;
	ScanFinishedHandle = UPjcSubsystem::OnScanFinished().AddSP(this, &SPjcTabUnused::OnScanFinished);

	if (GConfig) {
		FString SortColumn;
		bool bSortDescending = true;
		GConfig->GetBool(PjcUnusedSection, TEXT("OnlyFoldersWithUnused"), bOnlyUnused, GEditorPerProjectIni);
		GConfig->GetBool(PjcUnusedSection, TEXT("FolderGuides"), bFolderGuides, GEditorPerProjectIni);
		if (GConfig->GetString(PjcUnusedSection, TEXT("FolderSort"), SortColumn, GEditorPerProjectIni) && !SortColumn.IsEmpty()) {
			FolderSortColumn = FName {*SortColumn};
		}
		if (GConfig->GetBool(PjcUnusedSection, TEXT("FolderSortDescending"), bSortDescending, GEditorPerProjectIni)) {
			FolderSortMode = bSortDescending ? EColumnSortMode::Descending : EColumnSortMode::Ascending;
		}
		GConfig->GetBool(PjcUnusedSection, TEXT("Subfolders"), bSubfolders, GEditorPerProjectIni);
		GConfig->GetBool(PjcUnusedSection, TEXT("SizeMapAll"), bMapAll, GEditorPerProjectIni);
		if (GConfig->GetString(PjcUnusedSection, TEXT("AssetSort"), SortColumn, GEditorPerProjectIni) && !SortColumn.IsEmpty()) {
			AssetSortColumn = FName {*SortColumn};
		}
		if (GConfig->GetBool(PjcUnusedSection, TEXT("AssetSortDescending"), bSortDescending, GEditorPerProjectIni)) {
			AssetSortMode = bSortDescending ? EColumnSortMode::Descending : EColumnSortMode::Ascending;
		}
	}

	// the saved column order, when it still has every column once
	ColumnOrder = GetDefaultColumns();
	FString Columns;
	if (GConfig && GConfig->GetString(PjcUnusedSection, TEXT("Columns"), Columns, GEditorPerProjectIni)) {
		TArray<FString> Parts;
		Columns.ParseIntoArray(Parts, TEXT(","));
		TArray<FName> Saved;
		for (const auto& Part : Parts) {
			Saved.AddUnique(FName {*Part});
		}
		const bool bComplete = Saved.Num() == ColumnOrder.Num() && !ColumnOrder.ContainsByPredicate([&Saved](const FName Column) {
			return !Saved.Contains(Column);
		});
		if (bComplete) {
			ColumnOrder = Saved;
		}
	}

	// clang-format off
	ChildSlot
	[
		SNew(SPjcToolFrame).PageName(TEXT("AssetsUnused"))
		.Side()
		[
			SNew(SPjcOverviewPanel)
			.TabManager(TabManager.Pin())
			.OnShowSet_Lambda([this](const EPjcAssetSet InSet, const FString& Reason) {
				ShowSet(InSet, Reason);
			})
			.IsSetShown_Lambda([this](const EPjcAssetSet InSet, const FString& Reason) {
				return Set == InSet && ReasonFilter.Equals(Reason) && !bHeldOnly;
			})
			.OnShowDeleteFailed_Lambda([this]() {
				ShowNotDeleted();
			})
		]
		[
			SNew(SVerticalBox)
			// the lists as tabs over both panes (user, 2026-11-14): the tree and the list show the list they pick, and the two
			// toolbars under them line up
			+ SVerticalBox::Slot().AutoHeight()
			[
				MakeTabsRow()
			]
			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
				SNew(SSplitter)
				.Style(&FPjcStyles::Get().GetWidgetStyle<FSplitterStyle>("ProjectCleaner.Splitter"))
				.PhysicalSplitterHandleSize(12.0f)
				.HitDetectionSplitterHandleSize(12.0f)
				+ SSplitter::Slot().Value(0.46f)
				[
					MakeFolderPane()
				]
				+ SSplitter::Slot().Value(0.54f)
				[
					MakeAssetPane()
				]
			]
		]
	];
	// clang-format on

	OnScanFinished();
}

SPjcTabUnused::~SPjcTabUnused() {
	UPjcSubsystem::OnScanFinished().Remove(ScanFinishedHandle);
}

void SPjcTabUnused::SelectFolder(const FString& Path) {
	const FFolderPtr Folder = Tree.Find(Path);
	if (!Folder.IsValid()) return;

	for (FFolderPtr Parent = Folder->Parent.Pin(); Parent.IsValid(); Parent = Parent->Parent.Pin()) {
		ExpandedFolders.Add(Parent->Path);
		TreeView->SetItemExpansion(Parent, true);
	}
	if (!Path.Equals(SelectedFolder)) {
		SelectedFolder = Path;
		ClassBar->Reset();
		RefreshTree();
		RefreshAssets();
	}
	TreeView->SetSelection(Folder, ESelectInfo::Direct);
	TreeView->RequestScrollIntoView(Folder);
}

void SPjcTabUnused::ShowSet(const EPjcAssetSet InSet, const FString& Reason) {
	if (InSet == Set) {
		ReasonFilter = Reason;
		ApplyAssetFilter();
		return;
	}

	Set = InSet;
	ClassBar->Reset();
	ReasonFilter.Reset();
	bHeldOnly = false;
	RebuildTree();
	OnFolderSearchChanged(FText::FromString(FolderSearch));
	RefreshAssets();
	ReasonFilter = Reason;
	ApplyAssetFilter();
}

void SPjcTabUnused::OnScanFinished() {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	SetCounts.Reset();
	for (const auto& Asset : Result.Assets) {
		for (const EPjcAssetSet Each : {EPjcAssetSet::Unused, EPjcAssetSet::Excluded, EPjcAssetSet::Used, EPjcAssetSet::Review}) {
			SetCounts.FindOrAdd(Each) += FPjcUnusedTree::IsInSet(Result, Asset.PackageName, Each) ? 1 : 0;
		}
	}

	RebuildTree();
	GroupSizes.Reset();

	RefreshTree();
	RefreshAssets();
}

void SPjcTabUnused::RebuildTree() {
	Tree = FPjcUnusedTree::Make(UPjcSubsystem::GetScanResult(), Set);

	// first scan: roots open
	if (!bExpansionSynced) {
		for (const auto& Root : Tree.Roots) {
			ExpandedFolders.Add(Root->Path);
		}
		bExpansionSynced = true;
	}
	if (!Tree.Find(SelectedFolder).IsValid() && Tree.Roots.Num() > 0) {
		SelectedFolder = Tree.Roots[0]->Path;
	}
}

TSharedRef<SWidget> SPjcTabUnused::MakeTabsRow() {
	// clang-format off
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox).HeightOverride(36.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					MakeSetSwitch()
				]
				// the folder and what its list holds
				+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(FMargin {16.0f, 0.0f, 6.0f, 0.0f})
				[
					SNew(SBox).Clipping(EWidgetClipping::ClipToBounds)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text(this, &SPjcTabUnused::GetWhereText).Font(FPjcStyles::GetFont("Bold", 9))
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})
						[
							SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(ListSummary); }).ColorAndOpacity(FSlateColor::UseSubduedForeground())
						]
					]
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox).HeightOverride(1.0f)
			[
				SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.Line"))
			]
		];
	// clang-format on
}

TSharedRef<SWidget> SPjcTabUnused::MakeSetSwitch() {
	// tabs (user, 2026-11-14): the name and its count in a small pill, a line under the open one, both in the list's color
	const TSharedRef<SHorizontalBox> Switch = SNew(SHorizontalBox);
	const auto AddSet = [this, &Switch](const EPjcAssetSet Each, const FName ColorName, const FName QuietName, const FString& ToolTip) {
		const FLinearColor Color = PjcWidgets::GetColor(ColorName);
		const FLinearColor Quiet = PjcWidgets::GetColor(QuietName);
		// clang-format off
		Switch->AddSlot().AutoWidth().Padding(FMargin {0.0f, 0.0f, 2.0f, 0.0f})
		[
			SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
			.ForegroundColor(FSlateColor::UseForeground())
#endif
			.ButtonStyle(&PjcShim::GetStyle().GetWidgetStyle<FButtonStyle>(TEXT("NoBorder")))
			.ContentPadding(FMargin {10.0f, 0.0f})
			.VAlign(VAlign_Fill)
			.ToolTip(PjcWidgets::MakeToolTip(PjcWidgets::GetSetLabel(Each), ToolTip))
			.OnClicked_Lambda([this, Each]() {
				ShowSet(Each);
				return FReply::Handled();
			})
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(1.0f).VAlign(VAlign_Center)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(PjcWidgets::GetSetLabel(Each)))
						.Font_Lambda([this, Each]() { return FPjcStyles::GetFont(Set == Each ? TEXT("Bold") : TEXT("Regular"), 9); })
						.ColorAndOpacity_Lambda([this, Each]() { return Set == Each ? FSlateColor {FLinearColor::White} : FSlateColor::UseSubduedForeground(); })
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {7.0f, 0.0f, 0.0f, 0.0f})
					[
						SNew(SBorder)
						.BorderImage(FPjcStyles::Get().GetBrush("ProjectCleaner.Pill"))
						.BorderBackgroundColor_Lambda([this, Each, Quiet]() {
							return FSlateColor {Set == Each ? Quiet : PjcWidgets::GetColor("ProjectCleaner.Color.Track")};
						})
						.Padding(FMargin {6.0f, 1.0f})
						[
							SNew(STextBlock)
							.Text_Lambda([this, Each]() { return FText::FromString(PjcFormat::Count(SetCounts.FindRef(Each))); })
							.Font(PjcWidgets::GetFontMono(8))
							.ColorAndOpacity_Lambda([this, Each, Color]() {
								// a list with assets to review stays yellow when it is not open
								const bool bReviewWaits = Each == EPjcAssetSet::Review && SetCounts.FindRef(Each) > 0;
								return Set == Each || bReviewWaits ? FSlateColor {Color} : FSlateColor::UseSubduedForeground();
							})
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBox).HeightOverride(2.0f)
					[
						SNew(SImage)
						.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
						.ColorAndOpacity_Lambda([this, Each, Color]() { return FSlateColor {Set == Each ? Color : FLinearColor::Transparent}; })
					]
				]
			]
		];
		// clang-format on
	};
	AddSet(
		EPjcAssetSet::Unused, "ProjectCleaner.Color.Red", "ProjectCleaner.Color.DangerQuiet", TEXT("Nothing uses them. Clean project deletes them.")
	);
	AddSet(
		EPjcAssetSet::Excluded,
		"ProjectCleaner.Color.Muted",
		"ProjectCleaner.Color.Track",
		TEXT("Assets your exclusion rules keep: never deleted. Rules are in Scan settings > Exclusions.")
	);
	AddSet(
		EPjcAssetSet::Used,
		"ProjectCleaner.Color.Ok",
		"ProjectCleaner.Color.UsedQuiet",
		TEXT("Used assets, by why they are used: the asset their chain starts at. Editor only assets are in this list too.")
	);
	AddSet(
		EPjcAssetSet::Review,
		"ProjectCleaner.Color.Review",
		"ProjectCleaner.Color.ReviewQuiet",
		TEXT("Assets the scan is not sure about. Clean project keeps them; delete or keep each.")
	);
	return Switch;
}

void SPjcTabUnused::MakeRowReason(const FPjcScanResult& Result, FPjcUnusedAssetRow& Row) {
	const FPjcAssetUsage* Usage = Result.Usage.Find(Row.Package);

	if (Set == EPjcAssetSet::Unused) {
		const FString Count = PjcFormat::Count(Row.Group);
		Row.Reason = Row.Group > 0 ? TEXT("Used only by unused \u00B7 +") + Count : TEXT("Nothing uses it");
		Row.ReasonTip = Row.Group > 0 ? Count + TEXT(" unused assets reference it and are deleted with it") : TEXT("Nothing references it");
		return;
	}

	if (Set == EPjcAssetSet::Excluded) {
		const FPjcExclusion* Exclusion = Result.Excluded.Find(Row.Package);
		if (!Exclusion) return;

		FString Rule;
		if (Exclusion->Kind == EPjcExclusionKind::Folder) {
			Rule = TEXT("Folder ") + FPaths::GetCleanFilename(Exclusion->Value);
		}
		else if (Exclusion->Kind == EPjcExclusionKind::Class) {
			Rule = TEXT("Class ") + FSoftObjectPath {Exclusion->Value}.GetAssetName();
		}
		else {
			Rule = TEXT("Asset rule");
		}
		const TCHAR* Without = Exclusion->bUsedAnyway ? TEXT("used anyway") : TEXT("unused without it");
		const TCHAR* WithoutTip = Exclusion->bUsedAnyway ? TEXT("Used even without the rule: the rule changes nothing for it")
														 : TEXT("Without the rule Clean project would delete it");
		Row.Reason = Rule + TEXT(" \u00B7 ") + Without;
		Row.ReasonTip = Exclusion->Value + TEXT("\n") + WithoutTip;
		Row.ReasonKey = Rule;
		Row.bReasonWarn = !Exclusion->bUsedAnyway;
		return;
	}

	// the asset its chain starts at
	FName Root = Row.Package;
	const FPjcAssetUsage* RootUsage = Usage;
	for (int32 NumSteps = 0; RootUsage && RootUsage->Reason == EPjcUsageReason::Dependency && NumSteps < 1024; ++NumSteps) {
		Root = RootUsage->Parent;
		RootUsage = Result.Usage.Find(Root);
	}
	const EPjcUsageReason ChainReason = RootUsage ? RootUsage->Reason : EPjcUsageReason::None;

	if (Set == EPjcAssetSet::Review) {
		const bool bCue = ChainReason == EPjcUsageReason::GameplayCueNotFired;
		const bool bOutside = bCue && RootUsage->Detail.Contains(TEXT("outside"));
		// clang-format off
		const TCHAR* CookTip = TEXT("Nothing the scan sees uses it, but the last cook packaged it: ")
			TEXT("the game may load it in a way the scan cannot follow.");
		// clang-format on
		const TCHAR* CueTip = TEXT("GameplayCue notify whose tag no used asset, C++ or config file names. Tags can also be built at runtime.");
		const TCHAR* OutsideTip = TEXT("GameplayCue notify outside the cue notify paths: the cue manager never loads it by its tag.");
		const FString RootName = FPackageName::GetShortName(Root);
		Row.ReasonKey = bCue ? TEXT("Cue never fired") : TEXT("Packaged by last cook");
		if (Root != Row.Package) {
			Row.Reason = TEXT("Only used by ") + RootName;
			Row.ReasonTip = RootName + TEXT(" uses it and is in Review, so it stays with it. Clean project deletes neither.");
			return;
		}
		Row.Reason = bOutside ? TEXT("Outside cue paths") : Row.ReasonKey;
		Row.ReasonTip = bOutside ? OutsideTip : (bCue ? CueTip : CookTip);
		return;
	}

	// used: named like the Overview rows. Where a chain starts its reason, else what uses it (a Dependency)
	FString Name;
	FString ToolTip;
	FLinearColor Color;
	SPjcOverviewPanel::GetReasonInfo(FPjcOverview::GetGroupReason(Result, Row.Package), Name, ToolTip, Color);
	Row.ReasonKey = Name;
	if (Usage && Usage->Reason == EPjcUsageReason::Dependency && !Usage->Parent.IsNone()) {
		const FString ParentName = FPackageName::GetShortName(Usage->Parent);
		Row.Reason = TEXT("Used by ") + ParentName;
		Row.ReasonTip = ParentName + TEXT(" uses it");
	}
	else {
		Row.Reason = Name;
		Row.ReasonTip = ToolTip;
	}
	if (Root != Row.Package) {
		Row.ReasonTip += TEXT("\nChain starts at ") + FPackageName::GetShortName(Root);
	}

	// source and config: the file and the line that names it (the asset itself, or the one its chain starts at)
	if (ChainReason == EPjcUsageReason::Indirect && RootUsage && !RootUsage->Detail.IsEmpty()) {
		FString File;
		FString Line;
		if (RootUsage->Detail.Split(TEXT(", line "), &File, &Line, ESearchCase::CaseSensitive, ESearchDir::FromEnd)) {
			if (Root == Row.Package) {
				Row.Reason = FPaths::GetCleanFilename(File) + TEXT(":") + Line;
			}
		}
		Row.ReasonTip += (Root == Row.Package ? TEXT("\nNamed in ") : TEXT("\nThe chain start is named in ")) + RootUsage->Detail;
	}
}

TSharedRef<SWidget> SPjcTabUnused::MakeViewIcon(const FName Brush) {
	// clang-format off
	return SNew(SBox).WidthOverride(16.0f).HeightOverride(16.0f)
		[
			SNew(SImage).Image(FPjcStyles::Get().GetBrush(Brush)).ColorAndOpacity(FSlateColor::UseForeground())
		];
	// clang-format on
}

TSharedRef<SWidget> SPjcTabUnused::MakeFolderPane() {
	// clang-format off
	const TSharedRef<SHeaderRow> Header = SNew(SHeaderRow)
		+ SHeaderRow::Column(PjcUnusedColFolder)
		.DefaultLabel(FText::FromString(TEXT("Folder")))
		.FillWidth(1.0f)
		.SortMode(this, &SPjcTabUnused::GetFolderSortMode, PjcUnusedColFolder)
		.OnSort(this, &SPjcTabUnused::OnFolderSort)
		+ SHeaderRow::Column(PjcUnusedColShare)
		.DefaultLabel_Lambda([this]() { return FText::FromString(PjcWidgets::GetSetLabel(Set)); })
		.DefaultTooltip_Lambda([this]() { return FText::FromString(TEXT("Share of the folder size that is ") + PjcWidgets::GetSetWord(Set)); })
		.FixedWidth(128.0f)
		.SortMode(this, &SPjcTabUnused::GetFolderSortMode, PjcUnusedColShare)
		.OnSort(this, &SPjcTabUnused::OnFolderSort)
		+ SHeaderRow::Column(PjcUnusedColSize)
		.DefaultLabel(FText::FromString(TEXT("Size")))
		.DefaultTooltip_Lambda([this]() { return FText::FromString(TEXT("Size of the assets ") + PjcWidgets::GetSetWord(Set) + TEXT(", subfolders included")); })
		.FixedWidth(80.0f)
		.HAlignHeader(HAlign_Right)
		.SortMode(this, &SPjcTabUnused::GetFolderSortMode, PjcUnusedColSize)
		.OnSort(this, &SPjcTabUnused::OnFolderSort)
		+ SHeaderRow::Column(PjcUnusedColAssets)
		.DefaultLabel(FText::FromString(TEXT("Assets")))
		.DefaultTooltip_Lambda([this]() { return FText::FromString(TEXT("Assets ") + PjcWidgets::GetSetWord(Set) + TEXT(" / all assets, subfolders included")); })
		.FixedWidth(100.0f)
		.HAlignHeader(HAlign_Right)
		.SortMode(this, &SPjcTabUnused::GetFolderSortMode, PjcUnusedColAssets)
		.OnSort(this, &SPjcTabUnused::OnFolderSort);

	SAssignNew(TreeView, STreeView<FFolderPtr>)
	.TreeItemsSource(&TreeRoots)
	.SelectionMode(ESelectionMode::Single)
	.HeaderRow(Header)
	.OnGenerateRow(this, &SPjcTabUnused::OnGenerateFolderRow)
	.OnGetChildren(this, &SPjcTabUnused::OnGetFolderChildren)
	.OnSelectionChanged(this, &SPjcTabUnused::OnFolderSelectionChanged)
	.OnExpansionChanged(this, &SPjcTabUnused::OnFolderExpansionChanged)
	.OnContextMenuOpening(this, &SPjcTabUnused::OnFolderContextMenu)
	.OnMouseButtonDoubleClick_Lambda([this](FFolderPtr Folder) {
		if (Folder.IsValid()) {
			TreeView->SetItemExpansion(Folder, !TreeView->IsItemExpanded(Folder));
		}
	});

	return SNew(SBorder).BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.GroupBorder"))).Padding(0.0f)
		[
			SNew(SVerticalBox)
			// one height in both views: the search box and View are hidden in the size map
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {8.0f, 6.0f})
			[
				SNew(SBox).HeightOverride(28.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 8.0f, 0.0f})
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SCheckBox)
							.Style(&PjcShim::GetToggleStyle())
							.ToolTip(PjcWidgets::MakeToolTip(TEXT("Tree"), TEXT("Folders with the totals of the list")))
							.IsChecked_Lambda([this]() { return bMapView ? ECheckBoxState::Unchecked : ECheckBoxState::Checked; })
							.OnCheckStateChanged_Lambda([this](ECheckBoxState) { SetMapView(false); })
							[
								MakeViewIcon(TEXT("ProjectCleaner.Icon.Tree"))
							]
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin {2.0f, 0.0f, 0.0f, 0.0f})
						[
							SNew(SCheckBox)
							.Style(&PjcShim::GetToggleStyle())
							.ToolTip(PjcWidgets::MakeToolTip(TEXT("Size map"), TEXT("Boxes as big as the listed assets in them, click a folder to open it")))
							.IsChecked_Lambda([this]() { return bMapView ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
							.OnCheckStateChanged_Lambda([this](ECheckBoxState) { SetMapView(true); })
							[
								MakeViewIcon(TEXT("ProjectCleaner.Icon.SizeMap"))
							]
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(SBox).Visibility_Lambda([this]() { return bMapView ? EVisibility::Collapsed : EVisibility::Visible; })
						[
							SNew(SSearchBox)
							.Style(&PjcShim::GetSearchBoxStyle())
							.HintText(FText::FromString(TEXT("Search folders")))
							.OnTextChanged(this, &SPjcTabUnused::OnFolderSearchChanged)
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})
					[
						SNew(SComboButton)
						.ComboButtonStyle(&PjcShim::GetComboButtonStyle())
#if ENGINE_MAJOR_VERSION < 5
						.ForegroundColor(FSlateColor::UseForeground())
#endif
						.Visibility_Lambda([this]() { return bMapView ? EVisibility::Collapsed : EVisibility::Visible; })
						.ToolTipText(FText::FromString(TEXT("View options")))
						.OnGetMenuContent(this, &SPjcTabUnused::MakeViewMenu)
						.ButtonContent()
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("View")))
						]
					]
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
				SNew(SBox).Visibility_Lambda([this]() { return bMapView ? EVisibility::Collapsed : EVisibility::Visible; })
				[
					TreeView.ToSharedRef()
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				MakeMapBar()
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(FMargin {6.0f})
			[
				SNew(SBox).Visibility_Lambda([this]() { return bMapView ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					SAssignNew(SizeMap, SPjcSizeMap)
					.OnFolderClicked_Lambda([this](const FString& Path) { SelectFolder(Path); })
					.OnAssetClicked(this, &SPjcTabUnused::OnMapAssetClicked)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {10.0f, 0.0f, 10.0f, 6.0f})
			[
				MakeMapLegend()
			]
		];
	// clang-format on
}

void SPjcTabUnused::RefreshTree() {
	TreeRoots.Reset();
	for (const auto& Root : Tree.Roots) {
		if (IsFolderShown(*Root)) {
			TreeRoots.Add(Root);
		}
	}
	SortFolders(TreeRoots);

	// folders keep their expansion and selection over scans: items are new objects every scan
	TreeView->ClearExpandedItems();
	for (const auto& Pair : Tree.Folders) {
		if (!FolderSearch.IsEmpty() ? SearchMatches.Contains(Pair.Key) : ExpandedFolders.Contains(Pair.Key)) {
			TreeView->SetItemExpansion(Pair.Value, true);
		}
	}
	TreeView->RequestTreeRefresh();

	if (const FFolderPtr Selected = Tree.Find(SelectedFolder)) {
		TreeView->SetSelection(Selected, ESelectInfo::Direct);
	}
}

bool SPjcTabUnused::IsFolderShown(const FPjcUnusedFolder& Folder) const {
	if (!FolderSearch.IsEmpty() && !SearchMatches.Contains(Folder.Path)) return false;

	return !bOnlyUnused || Folder.Listed.Num > 0 || (bShowEmpty && Folder.bEmpty) || Folder.Path.Equals(SelectedFolder);
}

void SPjcTabUnused::SortFolders(TArray<FFolderPtr>& Folders) const {
	const bool bAscending = FolderSortMode == EColumnSortMode::Ascending;
	const FName Column = FolderSortColumn;
	Folders.Sort([bAscending, Column](const FFolderPtr& A, const FFolderPtr& B) {
		double KeyA = 0.0;
		double KeyB = 0.0;
		if (Column == PjcUnusedColFolder) {
			const int32 Compare = A->Name.Compare(B->Name, ESearchCase::IgnoreCase);
			return bAscending ? Compare < 0 : Compare > 0;
		}
		if (Column == PjcUnusedColShare) {
			KeyA = A->All.Size > 0 ? static_cast<double>(A->Listed.Size) / static_cast<double>(A->All.Size) : 0.0;
			KeyB = B->All.Size > 0 ? static_cast<double>(B->Listed.Size) / static_cast<double>(B->All.Size) : 0.0;
		}
		else if (Column == PjcUnusedColAssets) {
			KeyA = A->Listed.Num;
			KeyB = B->Listed.Num;
		}
		else {
			KeyA = static_cast<double>(A->Listed.Size);
			KeyB = static_cast<double>(B->Listed.Size);
		}
		if (KeyA == KeyB) return A->Name.Compare(B->Name, ESearchCase::IgnoreCase) < 0;
		return bAscending ? KeyA < KeyB : KeyA > KeyB;
	});
}

void SPjcTabUnused::SetExpansionRecursive(const FFolderPtr& Folder, const bool bExpanded) {
	TreeView->SetItemExpansion(Folder, bExpanded);
	for (const auto& Child : Folder->Children) {
		SetExpansionRecursive(Child, bExpanded);
	}
}

TSharedRef<ITableRow> SPjcTabUnused::OnGenerateFolderRow(FFolderPtr Folder, const TSharedRef<STableViewBase>& Table) {
	// clang-format off
	return SNew(SPjcUnusedFolderRow, Table)
		.Folder(Folder)
		.Set(Set)
		.HighlightText_Lambda([this]() {
			return FText::FromString(FolderSearch);
		})
		.ShowGuides_Lambda([this]() {
			return bFolderGuides;
		});
	// clang-format on
}

void SPjcTabUnused::OnGetFolderChildren(FFolderPtr Folder, TArray<FFolderPtr>& OutChildren) {
	for (const auto& Child : Folder->Children) {
		if (IsFolderShown(*Child)) {
			OutChildren.Add(Child);
		}
	}
	SortFolders(OutChildren);
}

void SPjcTabUnused::OnFolderSelectionChanged(FFolderPtr Folder, ESelectInfo::Type SelectInfo) {
	if (!Folder.IsValid() || Folder->Path.Equals(SelectedFolder)) return;

	SelectedFolder = Folder->Path;
	ClassBar->Reset();
	RefreshAssets();
}

void SPjcTabUnused::OnFolderExpansionChanged(FFolderPtr Folder, bool bExpanded) {
	if (!Folder.IsValid() || !FolderSearch.IsEmpty()) return;

	if (bExpanded) {
		ExpandedFolders.Add(Folder->Path);
	}
	else {
		ExpandedFolders.Remove(Folder->Path);
	}
}

void SPjcTabUnused::OnFolderSort(EColumnSortPriority::Type Priority, const FName& Column, EColumnSortMode::Type Mode) {
	// a new column starts with the biggest (names a to z)
	if (Column != FolderSortColumn) {
		Mode = Column == PjcUnusedColFolder ? EColumnSortMode::Ascending : EColumnSortMode::Descending;
	}
	FolderSortColumn = Column;
	FolderSortMode = Mode;
	SaveViewState();
	RefreshTree();
}

EColumnSortMode::Type SPjcTabUnused::GetFolderSortMode(const FName Column) const {
	return Column == FolderSortColumn ? FolderSortMode : EColumnSortMode::None;
}

void SPjcTabUnused::OnFolderSearchChanged(const FText& Text) {
	FolderSearch = Text.ToString().TrimStartAndEnd();
	SearchMatches.Reset();
	if (!FolderSearch.IsEmpty()) {
		for (const auto& Pair : Tree.Folders) {
			const FPjcUnusedFolder& Folder = *Pair.Value;
			const bool bShown = !bOnlyUnused || Folder.Listed.Num > 0 || (bShowEmpty && Folder.bEmpty);
			if (!bShown || !Folder.Path.Contains(FolderSearch)) continue;

			for (FFolderPtr Shown = Pair.Value; Shown.IsValid(); Shown = Shown->Parent.Pin()) {
				SearchMatches.Add(Shown->Path);
			}
		}
	}
	RefreshTree();
}

TSharedPtr<SWidget> SPjcTabUnused::OnFolderContextMenu() {
	const TArray<FFolderPtr> Selection = TreeView->GetSelectedItems();
	if (Selection.Num() == 0) return nullptr;

	const FFolderPtr Folder = Selection[0];
	FMenuBuilder Menu {true, nullptr};
	if (Set == EPjcAssetSet::Unused) {
		const FString DeleteLabel = FString::Printf(TEXT("Delete unused\u2026 (%s)"), *PjcFormat::Count(Folder->Listed.Num));
		const FString DeleteTip = TEXT("Deletes the unused assets of the folder and its subfolders, with the unused assets that reference them");
		PjcWidgets::AddMenuEntry(Menu, EPjcMenuIcon::Delete, DeleteLabel, DeleteTip, Folder->Listed.Num > 0, [this, Folder]() {
			TArray<FName> Packages;
			GetFolderUnused(Folder->Path, Packages);
			PjcActions::DeleteWithGroup(Packages);
		});
	}
	if (Set == EPjcAssetSet::Unused || Set == EPjcAssetSet::Review) {
		const FString ExcludeTip = TEXT("Keeps every asset in the folder, saved to Config/DefaultEditor.ini");
		const TCHAR* ExcludeLabel = Set == EPjcAssetSet::Review ? TEXT("Keep folder") : TEXT("Exclude folder");
		PjcWidgets::AddMenuEntry(Menu, EPjcMenuIcon::Exclude, ExcludeLabel, ExcludeTip, [this, Folder]() {
			ExcludeFolder(Folder->Path);
		});
		Menu.AddSeparator();
	}
	PjcWidgets::AddMenuEntry(Menu, EPjcMenuIcon::SizeMap, TEXT("Size map here"), TEXT("Shows the folder as a size map"), [this]() {
		SetMapView(true);
	});
	PjcWidgets::AddMenuEntry(Menu, EPjcMenuIcon::Expand, TEXT("Expand all"), TEXT("Opens every subfolder"), [this, Folder]() {
		SetExpansionRecursive(Folder, true);
	});
	PjcWidgets::AddMenuEntry(Menu, EPjcMenuIcon::Collapse, TEXT("Collapse all"), TEXT("Closes every subfolder"), [this, Folder]() {
		SetExpansionRecursive(Folder, false);
	});
	Menu.AddSeparator();
	PjcWidgets::AddMenuEntry(Menu, EPjcMenuIcon::Explorer, TEXT("Show in Explorer"), TEXT("Opens the folder in the file explorer"), [Folder]() {
		UPjcSubsystem::OpenPathInFileExplorer(UPjcSubsystem::PathConvertToAbsolute(Folder->Path));
	});
	return Menu.MakeWidget();
}

TSharedRef<SWidget> SPjcTabUnused::MakeViewMenu() {
	int32 NumEmpty = 0;
	for (const auto& Pair : Tree.Folders) {
		NumEmpty += Pair.Value->bEmpty ? 1 : 0;
	}

	FMenuBuilder Menu {true, nullptr};
	const FString OnlyLabel = TEXT("Only folders with ") + PjcWidgets::GetSetWord(Set);
	const FString OnlyTip = TEXT("Hides folders without assets ") + PjcWidgets::GetSetWord(Set);
	PjcWidgets::AddMenuToggle(Menu, OnlyLabel, OnlyTip, &bOnlyUnused, [this]() {
		bOnlyUnused = !bOnlyUnused;
		SaveViewState();
		OnFolderSearchChanged(FText::FromString(FolderSearch));
	});
	const FString EmptyLabel = FString::Printf(TEXT("Empty folders (%s)"), *PjcFormat::Count(NumEmpty));
	PjcWidgets::AddMenuToggle(Menu, EmptyLabel, TEXT("Shows empty folders, Clean project deletes them"), &bShowEmpty, [this]() {
		bShowEmpty = !bShowEmpty;
		OnFolderSearchChanged(FText::FromString(FolderSearch));
	});
	PjcWidgets::AddMenuToggle(Menu, TEXT("Folder guides"), TEXT("A thin line down the tree under each open folder"), &bFolderGuides, [this]() {
		bFolderGuides = !bFolderGuides;
		SaveViewState();
	});
	Menu.AddSeparator();
	// what a folder lists (user, 2026-11-14: moved here from the list)
	PjcWidgets::AddMenuToggle(
		Menu, TEXT("Include subfolders"), TEXT("The list shows the assets of the subfolders of the folder too"), &bSubfolders, [this]() {
			bSubfolders = !bSubfolders;
			SaveViewState();
			RefreshAssets();
		}
	);
	Menu.AddSeparator();
	Menu.AddMenuEntry(
		FText::FromString(TEXT("Expand all")),
		FText::GetEmpty(),
		PjcWidgets::GetMenuIcon(EPjcMenuIcon::Expand),
		FUIAction {FExecuteAction::CreateLambda([this]() {
			for (const auto& Root : TreeRoots) {
				SetExpansionRecursive(Root, true);
			}
		})}
	);
	Menu.AddMenuEntry(
		FText::FromString(TEXT("Collapse all")),
		FText::GetEmpty(),
		PjcWidgets::GetMenuIcon(EPjcMenuIcon::Collapse),
		FUIAction {FExecuteAction::CreateLambda([this]() {
			for (const auto& Root : TreeRoots) {
				SetExpansionRecursive(Root, false);
				TreeView->SetItemExpansion(Root, true);
			}
		})}
	);
	return Menu.MakeWidget();
}

TSharedRef<SWidget> SPjcTabUnused::MakeAssetPane() {
	SAssignNew(AssetHeader, SHeaderRow);
	RebuildHeader();

	// clang-format off
	SAssignNew(ListView, SListView<FAssetRowPtr>)
	.ListItemsSource(&ListRows)
	.SelectionMode(ESelectionMode::Multi)
	.HeaderRow(AssetHeader)
	.OnGenerateRow(this, &SPjcTabUnused::OnGenerateAssetRow)
	.OnContextMenuOpening(this, &SPjcTabUnused::OnAssetContextMenu)
	.OnSelectionChanged(this, &SPjcTabUnused::OnAssetSelectionChanged)
	.OnMouseButtonDoubleClick_Lambda([this](FAssetRowPtr Row) {
		if (Row.IsValid()) {
			PjcTabs::Inspect(TabManager, Row->Package);
		}
	})
	.OnKeyDownHandler(this, &SPjcTabUnused::OnAssetKeyDown);

	return SNew(SBorder)
		.BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.GroupBorder")))
		.Padding(0.0f)
		[
			SNew(SVerticalBox)
			// one height with the folder pane's toolbar, so they line up under the tabs
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {8.0f, 6.0f})
			[
				MakeToolbar()
			]
			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
				SNew(SHorizontalBox)
				// reason filters and the class bar next to the list: a class in its color is listed
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(196.0f)
					[
						SNew(SBorder).BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.DarkGroupBorder"))).Padding(FMargin {0.0f, 4.0f})
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SAssignNew(ChipsBox, SVerticalBox)
							]
							+ SVerticalBox::Slot().FillHeight(1.0f)
							[
								SAssignNew(ClassBar, SPjcClassBar).OnChanged(FSimpleDelegate::CreateSP(this, &SPjcTabUnused::ApplyAssetFilter))
							]
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().FillHeight(1.0f)
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						[
							ListView.ToSharedRef()
						]
						+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(FMargin {16.0f})
						[
							SNew(STextBlock)
							.Text(this, &SPjcTabUnused::GetEmptyText)
							.Visibility_Lambda([this]() { return ListRows.Num() == 0 ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
							.ColorAndOpacity(FSlateColor::UseSubduedForeground())
							.AutoWrapText(true)
							.Justification(ETextJustify::Center)
						]
					]
				]
			]
		];
	// clang-format on
}

void SPjcTabUnused::RefreshAssets() {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	TArray<int32> Indices;
	Tree.GetListed(Result, SelectedFolder, bSubfolders, Indices);
	const bool bGroups = Set == EPjcAssetSet::Unused || Set == EPjcAssetSet::Review;

	FolderRows.Reset(Indices.Num());
	for (const int32 Index : Indices) {
		const FAssetData& Asset = Result.Assets[Index];
		const PjcWidgets::FAssetType& Type = PjcWidgets::GetAssetType(Asset);
		const FString Path = Asset.PackagePath.ToString();

		const FAssetRowPtr Row = MakeShared<FPjcUnusedAssetRow>();
		Row->Index = Index;
		Row->Package = Asset.PackageName;
		Row->Name = Asset.AssetName.ToString();
		Row->ClassName = PjcShim::GetAssetClassName(Asset);
		Row->ClassLabel = Type.Label;
		Row->Color = Type.Color;
		Row->Icon = Type.Icon;
		Row->Size = Result.GetSize(Asset.PackageName);
		Row->Folder = Path.Len() > SelectedFolder.Len() ? Path.Mid(SelectedFolder.Len() + 1) : FString {};
		Row->bLabel = Result.FolderLabelUnused.Contains(Asset.PackageName);
		Row->Holder = UPjcSubsystem::GetLastCleanupKept().FindRef(Asset.PackageName);
		if (!bGroups) {
			Row->Group = 0;
		}
		else if (const int32* Group = GroupSizes.Find(Asset.PackageName)) {
			Row->Group = *Group;
		}
		else {
			TArray<FName> With;
			FPjcUnusedTree::GetDeleteGroup(Result, {Asset.PackageName}, With);
			Row->Group = GroupSizes.Add(Asset.PackageName, With.Num());
		}
		MakeRowReason(Result, *Row);
		FolderRows.Add(Row);
	}

	// filters of classes and reasons no longer here go
	TSet<FString> Reasons;
	for (const auto& Row : FolderRows) {
		Reasons.Add(Row->ReasonKey);
	}
	if (!Reasons.Contains(ReasonFilter)) {
		ReasonFilter.Reset();
	}

	RebuildChips();
	ApplyAssetFilter();
	RefreshMap();
}

void SPjcTabUnused::ApplyAssetFilter() {
	// selection stays over refreshes
	TSet<FName> Selected;
	for (const auto& Row : ListView->GetSelectedItems()) {
		Selected.Add(Row->Package);
	}

	ListRows.Reset();
	for (const auto& Row : FolderRows) {
		if (!ClassBar->IsShown(Row->ClassName)) continue;
		if (!ReasonFilter.IsEmpty() && Row->ReasonKey != ReasonFilter) continue;
		if (bHeldOnly && Row->Holder.IsEmpty()) continue;
		if (!AssetSearch.IsEmpty() && !Row->Name.Contains(AssetSearch) && !Row->ClassLabel.Contains(AssetSearch)) continue;

		ListRows.Add(Row);
	}

	// what the line above the list says about it
	int64 ListSize = 0;
	for (const auto& Row : ListRows) {
		ListSize += Row->Size;
	}
	const FString Shown = PjcFormat::Count(ListRows.Num());
	const FString Count = ListRows.Num() == FolderRows.Num() ? Shown : Shown + TEXT(" of ") + PjcFormat::Count(FolderRows.Num());
	ListSummary = FString::Printf(TEXT("\u00B7 %s assets \u00B7 %s"), *Count, *PjcFormat::Size(ListSize));

	const bool bAscending = AssetSortMode == EColumnSortMode::Ascending;
	const FName Column = AssetSortColumn;
	ListRows.Sort([bAscending, Column](const FAssetRowPtr& A, const FAssetRowPtr& B) {
		int32 Compare = 0;
		if (Column == PjcUnusedColName) {
			Compare = A->Name.Compare(B->Name, ESearchCase::IgnoreCase);
		}
		else if (Column == PjcUnusedColClass) {
			Compare = A->ClassLabel.Compare(B->ClassLabel, ESearchCase::IgnoreCase);
		}
		else if (Column == PjcUnusedColReason) {
			Compare = A->Reason.Compare(B->Reason, ESearchCase::IgnoreCase);
		}
		else if (Column == PjcUnusedColPath) {
			Compare = A->Folder.Compare(B->Folder, ESearchCase::IgnoreCase);
		}
		else {
			Compare = A->Size < B->Size ? -1 : (A->Size > B->Size ? 1 : 0);
		}
		if (Compare == 0) return A->Size != B->Size ? A->Size > B->Size : A->Name < B->Name;
		return bAscending ? Compare < 0 : Compare > 0;
	});

	ListView->ClearSelection();
	for (const auto& Row : ListRows) {
		if (Selected.Contains(Row->Package)) {
			ListView->SetItemSelection(Row, true, ESelectInfo::Direct);
		}
	}
	ListView->RequestListRefresh();
}

void SPjcTabUnused::RebuildChips() {
	struct FChip
	{
		FName ClassName;
		PjcWidgets::FAssetType Type;
		int32 Num = 0;
		int64 Size = 0;
	};
	TMap<FName, FChip> Chips;
	for (const auto& Row : FolderRows) {
		FChip& Chip = Chips.FindOrAdd(Row->ClassName);
		Chip.ClassName = Row->ClassName;
		Chip.Type = PjcWidgets::FAssetType {Row->ClassLabel, Row->Color, Row->Icon};
		++Chip.Num;
		Chip.Size += Row->Size;
	}
	TArray<FChip> Sorted;
	Chips.GenerateValueArray(Sorted);
	Sorted.Sort([](const FChip& A, const FChip& B) {
		return A.Size != B.Size ? A.Size > B.Size : A.Type.Label < B.Type.Label;
	});

	ChipsBox->ClearChildren();
	int32 NumHeld = 0;
	for (const auto& Row : FolderRows) {
		NumHeld += Row->Holder.IsEmpty() ? 0 : 1;
	}
	if (NumHeld == 0) {
		bHeldOnly = false;
	}
	else {
		// clang-format off
		ChipsBox->AddSlot().AutoHeight().Padding(FMargin {6.0f, 0.0f, 6.0f, 3.0f})
		[
			SNew(SCheckBox)
			.Style(&PjcShim::GetToggleStyle())
			.ToolTipText(FText::FromString(TEXT("The last delete could not remove them: something still holds them")))
			.IsChecked_Lambda([this]() { return bHeldOnly ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this](ECheckBoxState) {
				bHeldOnly = !bHeldOnly;
				ApplyAssetFilter();
			})
			[
				SNew(SBox).Padding(FMargin {8.0f, 2.0f})
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("Delete failed"))).ColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.DangerText"))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})
					[
						SNew(STextBlock)
						.Text(FText::FromString(PjcFormat::Count(NumHeld)))
						.Font(PjcWidgets::GetFontMono(8))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
				]
			]
		];
		// clang-format on
	}
	// reasons, biggest first (not for unused assets: nothing uses any of them)
	struct FReasonChip
	{
		FString Key;
		int32 Num = 0;
		int64 Size = 0;
	};
	TMap<FString, FReasonChip> ReasonChips;
	for (const auto& Row : FolderRows) {
		if (Set == EPjcAssetSet::Unused || Row->ReasonKey.IsEmpty()) continue;

		FReasonChip& Chip = ReasonChips.FindOrAdd(Row->ReasonKey);
		Chip.Key = Row->ReasonKey;
		++Chip.Num;
		Chip.Size += Row->Size;
	}
	TArray<FReasonChip> ReasonsSorted;
	ReasonChips.GenerateValueArray(ReasonsSorted);
	ReasonsSorted.Sort([](const FReasonChip& A, const FReasonChip& B) {
		return A.Size != B.Size ? A.Size > B.Size : A.Key < B.Key;
	});
	for (const auto& Chip : ReasonsSorted) {
		const FString Key = Chip.Key;
		const FString Tip = FString::Printf(TEXT("%s \u00B7 %s"), *Chip.Key, *PjcFormat::Size(Chip.Size));
		// clang-format off
		ChipsBox->AddSlot().AutoHeight().Padding(FMargin {6.0f, 0.0f, 6.0f, 3.0f})
		[
			SNew(SCheckBox)
			.Style(&PjcShim::GetToggleStyle())
			.ToolTipText(FText::FromString(Tip))
			.IsChecked_Lambda([this, Key]() { return ReasonFilter == Key ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this, Key](ECheckBoxState) { OnReasonChipClicked(Key); })
			[
				SNew(SBox).Padding(FMargin {8.0f, 2.0f})
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(Chip.Key)).ColorAndOpacity(PjcWidgets::GetSetColors(Set).BoxText)
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})
					[
						SNew(STextBlock)
						.Text(FText::FromString(PjcFormat::Count(Chip.Num)))
						.Font(PjcWidgets::GetFontMono(8))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
				]
			]
		];
		// clang-format on
	}
	TArray<FPjcClassBarItem> Items;
	for (const auto& Chip : Sorted) {
		FPjcClassBarItem& Item = Items.AddDefaulted_GetRef();
		Item.ClassName = Chip.ClassName;
		Item.Label = Chip.Type.Label;
		Item.Color = Chip.Type.Color;
		Item.Num = Chip.Num;
		Item.ToolTip = FString::Printf(TEXT("%s \u00B7 %s"), *Chip.ClassName.ToString(), *PjcFormat::Size(Chip.Size));
	}
	ClassBar->SetItems(Items);
}

TSharedRef<ITableRow> SPjcTabUnused::OnGenerateAssetRow(FAssetRowPtr Row, const TSharedRef<STableViewBase>& Table) {
	// clang-format off
	return SNew(SPjcUnusedAssetRow, Table)
		.Row(Row)
		.HighlightText_Lambda([this]() {
			return FText::FromString(AssetSearch);
		});
	// clang-format on
}

void SPjcTabUnused::OnAssetSort(EColumnSortPriority::Type Priority, const FName& Column, EColumnSortMode::Type Mode) {
	// a new column starts with the biggest (names a to z)
	if (Column != AssetSortColumn) {
		Mode = Column == PjcUnusedColAssetSize ? EColumnSortMode::Descending : EColumnSortMode::Ascending;
	}
	AssetSortColumn = Column;
	AssetSortMode = Mode;
	SaveViewState();
	ApplyAssetFilter();
}

EColumnSortMode::Type SPjcTabUnused::GetAssetSortMode(const FName Column) const {
	return Column == AssetSortColumn ? AssetSortMode : EColumnSortMode::None;
}

void SPjcTabUnused::OnAssetSearchChanged(const FText& Text) {
	AssetSearch = Text.ToString().TrimStartAndEnd();
	ApplyAssetFilter();
}

TArray<FName> SPjcTabUnused::GetDefaultColumns() {
	return {PjcUnusedColName, PjcUnusedColReason, PjcUnusedColClass, PjcUnusedColAssetSize, PjcUnusedColPath};
}

void SPjcTabUnused::RebuildHeader() {
	AssetHeader->ClearColumns();
	for (const FName Column : ColumnOrder) {
		FString Label = TEXT("Name");
		FString ToolTip;
		Label = Column == PjcUnusedColReason ? TEXT("Reason") : Label;
		Label = Column == PjcUnusedColClass ? TEXT("Class") : Label;
		Label = Column == PjcUnusedColAssetSize ? TEXT("Size") : Label;
		Label = Column == PjcUnusedColPath ? TEXT("Folder") : Label;
		ToolTip = Column == PjcUnusedColReason ? TEXT("Why it is in the list") : ToolTip;
		ToolTip = Column == PjcUnusedColPath ? TEXT("Subfolder of the selected folder") : ToolTip;
		const bool bSize = Column == PjcUnusedColAssetSize;

		// clang-format off
		SHeaderRow::FColumn::FArguments Args = SHeaderRow::Column(Column)
			.DefaultTooltip(FText::FromString(ToolTip + (ToolTip.IsEmpty() ? TEXT("") : TEXT("\n")) + TEXT("Drag onto another header to move it")))
			.SortMode(this, &SPjcTabUnused::GetAssetSortMode, Column)
			.OnSort(this, &SPjcTabUnused::OnAssetSort)
			.HAlignHeader(bSize ? HAlign_Right : HAlign_Fill)
			[
				SNew(SPjcColumnLabel)
				.Column(Column)
				.Label(Label)
				.HAlign(bSize ? HAlign_Right : HAlign_Left)
				.OnMoved(this, &SPjcTabUnused::MoveColumn)
				.OnClicked_Lambda([this, Column]() { SortByColumn(Column); })
			];
		// clang-format on
		if (Column == PjcUnusedColClass) {
			Args.FixedWidth(150.0f);
		}
		else if (bSize) {
			Args.FixedWidth(84.0f);
		}
		else {
			Args.FillWidth(Column == PjcUnusedColName ? 1.0f : (Column == PjcUnusedColReason ? 0.8f : 0.5f));
		}
		AssetHeader->AddColumn(Args);
	}
	if (ListView.IsValid()) {
		ListView->RebuildList();
	}
}

void SPjcTabUnused::MoveColumn(const FName Column, const FName Before) {
	ColumnOrder.Remove(Column);
	ColumnOrder.Insert(Column, FMath::Max(0, ColumnOrder.IndexOfByKey(Before)));
	SaveViewState();
	RebuildHeader();
}

void SPjcTabUnused::SortByColumn(const FName Column) {
	// same column again: the other way round
	const bool bSame = Column == AssetSortColumn;
	const EColumnSortMode::Type Flipped = AssetSortMode == EColumnSortMode::Ascending ? EColumnSortMode::Descending : EColumnSortMode::Ascending;
	OnAssetSort(EColumnSortPriority::Primary, Column, bSame ? Flipped : EColumnSortMode::Ascending);
}

void SPjcTabUnused::OnReasonChipClicked(const FString& Key) {
	ReasonFilter = ReasonFilter == Key ? FString {} : Key;
	ApplyAssetFilter();
}

FText SPjcTabUnused::GetWhereText() const {
	return FText::FromString(SelectedFolder.Mid(1) + (bSubfolders ? TEXT("") : TEXT(" (this folder only)")));
}

FText SPjcTabUnused::GetEmptyText() const {
	if (SetCounts.FindRef(Set) > 0) {
		return FText::FromString(TEXT("No assets ") + PjcWidgets::GetSetWord(Set) + TEXT(" here"));
	}
	switch (Set) {
		case EPjcAssetSet::Excluded : return FText::FromString(TEXT("Nothing excluded. Rules are in Scan settings > Exclusions."));
		case EPjcAssetSet::Used :	  return FText::FromString(TEXT("No used assets."));
		case EPjcAssetSet::Review :	  return FText::FromString(TEXT("Nothing to review. The scan is sure about every asset."));
		case EPjcAssetSet::Unused :
		default :					  return FText::FromString(TEXT("No unused assets. The project is clean."));
	}
}

void SPjcTabUnused::SaveViewState() const {
	if (!GConfig) return;

	GConfig->SetBool(PjcUnusedSection, TEXT("OnlyFoldersWithUnused"), bOnlyUnused, GEditorPerProjectIni);
	GConfig->SetBool(PjcUnusedSection, TEXT("FolderGuides"), bFolderGuides, GEditorPerProjectIni);
	GConfig->SetString(PjcUnusedSection, TEXT("FolderSort"), *FolderSortColumn.ToString(), GEditorPerProjectIni);
	GConfig->SetBool(PjcUnusedSection, TEXT("FolderSortDescending"), FolderSortMode == EColumnSortMode::Descending, GEditorPerProjectIni);
	GConfig->SetBool(PjcUnusedSection, TEXT("Subfolders"), bSubfolders, GEditorPerProjectIni);
	GConfig->SetBool(PjcUnusedSection, TEXT("SizeMapAll"), bMapAll, GEditorPerProjectIni);
	GConfig->SetString(PjcUnusedSection, TEXT("AssetSort"), *AssetSortColumn.ToString(), GEditorPerProjectIni);
	GConfig->SetBool(PjcUnusedSection, TEXT("AssetSortDescending"), AssetSortMode == EColumnSortMode::Descending, GEditorPerProjectIni);
	TArray<FString> Columns;
	for (const FName Column : ColumnOrder) {
		Columns.Add(Column.ToString());
	}
	GConfig->SetString(PjcUnusedSection, TEXT("Columns"), *FString::Join(Columns, TEXT(",")), GEditorPerProjectIni);
}

TSharedPtr<SWidget> SPjcTabUnused::OnAssetContextMenu() {
	const TArray<FName> Packages = GetSelectedPackages();
	if (Packages.Num() == 0) return nullptr;

	FMenuBuilder Menu {true, nullptr};
	if (Set == EPjcAssetSet::Unused || Set == EPjcAssetSet::Review) {
		const FString DeleteLabel = FString::Printf(TEXT("Delete\u2026 (%s)"), *PjcFormat::Count(Packages.Num()));
		PjcWidgets::AddMenuEntry(
			Menu, EPjcMenuIcon::Delete, DeleteLabel, TEXT("Deletes the selection with the unused assets that reference it"), [Packages]() {
				PjcActions::DeleteWithGroup(Packages);
			}
		);
		Menu.AddSeparator();
		AddExcludeEntries(Menu);
		Menu.AddSeparator();
	}
	if (Set == EPjcAssetSet::Excluded) {
		PjcWidgets::AddMenuEntry(Menu, EPjcMenuIcon::Include, TEXT("Remove exclusion"), TEXT("Removes the rules that keep the selection"), [this]() {
			RemoveSelectedExclusions();
		});
		Menu.AddSeparator();
	}
	PjcWidgets::AddMenuEntry(
		Menu, EPjcMenuIcon::Inspect, TEXT("Inspect"), TEXT("Shows why it is in the list in the Inspector"), Packages.Num() == 1, [this]() {
			InspectSelected();
		}
	);
	PjcWidgets::AddMenuEntry(Menu, EPjcMenuIcon::Browse, TEXT("Browse"), TEXT("Shows the selection in the Content Browser"), [this]() {
		BrowseSelected();
	});
	return Menu.MakeWidget();
}

FReply SPjcTabUnused::OnAssetKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) {
	const bool bDeletable = Set == EPjcAssetSet::Unused || Set == EPjcAssetSet::Review;
	if (bDeletable && KeyEvent.GetKey() == EKeys::Delete && ListView->GetNumItemsSelected() > 0) {
		PjcActions::DeleteWithGroup(GetSelectedPackages());
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

TArray<FName> SPjcTabUnused::GetSelectedPackages() const {
	TArray<FName> Packages;
	for (const auto& Row : ListRows) {
		if (ListView->IsItemSelected(Row)) {
			Packages.Add(Row->Package);
		}
	}
	return Packages;
}

void SPjcTabUnused::GetFolderUnused(const FString& Path, TArray<FName>& OutPackages) const {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	TArray<int32> Indices;
	Tree.GetListed(Result, Path, true, Indices);
	OutPackages.Reset(Indices.Num());
	for (const int32 Index : Indices) {
		OutPackages.Add(Result.Assets[Index].PackageName);
	}
}

void SPjcTabUnused::OnAssetSelectionChanged(FAssetRowPtr Row, ESelectInfo::Type SelectInfo) {
	TArray<FName> Group;
	const TArray<FName> Packages = GetSelectedPackages();
	if (Packages.Num() > 0 && (Set == EPjcAssetSet::Unused || Set == EPjcAssetSet::Review)) {
		FPjcUnusedTree::GetDeleteGroup(UPjcSubsystem::GetScanResult(), Packages, Group);
	}
	NumSelectionGroup = Group.Num();

	TSet<int32> Indices;
	for (const auto& Selected : ListView->GetSelectedItems()) {
		Indices.Add(Selected->Index);
	}
	SizeMap->SetSelected(Indices);
}

TSharedRef<SWidget> SPjcTabUnused::MakeToolbar() {
	const auto HasSelection = [this]() {
		return ListView.IsValid() && ListView->GetNumItemsSelected() > 0;
	};
	// the actions of the open list are always there, greyed until something is selected (user, 2026-11-14: on the left)
	const auto ShownFor = [this](const bool bUnused, const bool bExcluded, const bool bReview) {
		return [this, bUnused, bExcluded, bReview]() {
			const bool bShown = (Set == EPjcAssetSet::Unused && bUnused) || (Set == EPjcAssetSet::Excluded && bExcluded)
				|| (Set == EPjcAssetSet::Review && bReview);
			return bShown ? EVisibility::Visible : EVisibility::Collapsed;
		};
	};
	// an action: its icon (as in the menus) and its name
	const auto Label = [](const FName Brush, const TAttribute<FText>& Text, const FSlateColor& TextColor) -> TSharedRef<SWidget> {
		// clang-format off
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 6.0f, 0.0f})
			[
				SNew(SBox).WidthOverride(14.0f).HeightOverride(14.0f)
				[
					SNew(SImage).Image(FPjcStyles::Get().GetBrush(Brush))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(Text).ColorAndOpacity(TextColor)
			];
		// clang-format on
	};
	const auto Static = [](const TCHAR* Text) {
		return TAttribute<FText> {FText::FromString(Text)};
	};

	// clang-format off
	// the search and the actions wrap to a second line when the pane is narrow; View stays at the right
	return SNew(SBox).MinDesiredHeight(28.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D {6.0f, 4.0f})
				+ SWrapBox::Slot().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(240.0f)
					[
						SNew(SSearchBox)
						.Style(&PjcShim::GetSearchBoxStyle())
						.HintText_Lambda([this]() { return FText::FromString(Set == EPjcAssetSet::Review ? FString {TEXT("Search assets to review")} : TEXT("Search ") + PjcWidgets::GetSetWord(Set) + TEXT(" assets")); })
						.OnTextChanged(this, &SPjcTabUnused::OnAssetSearchChanged)
					]
				]
				+ SWrapBox::Slot().VAlign(VAlign_Center).Padding(FMargin {2.0f, 0.0f})
				[
					SNew(SBox).WidthOverride(1.0f).HeightOverride(22.0f)
					[
						SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.Line"))
					]
				]
				+ SWrapBox::Slot().VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(&FPjcStyles::Get().GetWidgetStyle<FButtonStyle>("ProjectCleaner.Button.DangerQuiet"))
#if ENGINE_MAJOR_VERSION < 5
					.ForegroundColor(FSlateColor::UseForeground())
#endif
					.ContentPadding(FMargin {8.0f, 1.0f, 10.0f, 1.0f})
					.ToolTipText(FText::FromString(TEXT("Deletes the selection with the unused assets that reference it (Delete)")))
					.Visibility_Lambda(ShownFor(true, false, true))
					.IsEnabled_Lambda(HasSelection)
					.OnClicked_Lambda([this]() {
						PjcActions::DeleteWithGroup(GetSelectedPackages());
						return FReply::Handled();
					})
					[
						Label("ProjectCleaner.Menu.Delete", Static(TEXT("Delete\u2026")), PjcWidgets::GetColor("ProjectCleaner.Color.DangerText"))
					]
				]
				+ SWrapBox::Slot().VAlign(VAlign_Center)
				[
					SNew(SComboButton)
					.ComboButtonStyle(&PjcShim::GetComboButtonStyle())
#if ENGINE_MAJOR_VERSION < 5
					.ForegroundColor(FSlateColor::UseForeground())
#endif
					.ToolTipText(FText::FromString(TEXT("Keeps the selection, its folder or its class from cleanup: adds exclusions")))
					.Visibility_Lambda(ShownFor(true, false, true))
					.IsEnabled_Lambda(HasSelection)
					.OnGetMenuContent(this, &SPjcTabUnused::MakeExcludeMenu)
					.ButtonContent()
					[
						Label(
							"ProjectCleaner.Menu.Exclude",
							TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateLambda([this]() {
								return FText::FromString(Set == EPjcAssetSet::Review ? TEXT("Keep") : TEXT("Exclude"));
							})),
							FSlateColor::UseForeground()
						)
					]
				]
				+ SWrapBox::Slot().VAlign(VAlign_Center)
				[
					SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
					.ButtonStyle(&PjcShim::GetButtonStyle427(false))
					.ForegroundColor(FSlateColor::UseForeground())
#endif
					.ContentPadding(FMargin {8.0f, 1.0f, 10.0f, 1.0f})
					.ToolTipText(FText::FromString(TEXT("Removes the rules that keep the selection; the next scan shows what they are without them")))
					.Visibility_Lambda(ShownFor(false, true, false))
					.IsEnabled_Lambda(HasSelection)
					.OnClicked_Lambda([this]() {
						RemoveSelectedExclusions();
						return FReply::Handled();
					})
					[
						Label("ProjectCleaner.Menu.Include", Static(TEXT("Remove exclusion")), FSlateColor::UseForeground())
					]
				]
				+ SWrapBox::Slot().VAlign(VAlign_Center)
				[
					SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
					.ButtonStyle(&PjcShim::GetButtonStyle427(false))
					.ForegroundColor(FSlateColor::UseForeground())
#endif
					.ContentPadding(FMargin {8.0f, 1.0f, 10.0f, 1.0f})
					.ToolTipText(FText::FromString(TEXT("Shows why it is in the list in the Inspector")))
					.IsEnabled_Lambda([this]() { return ListView.IsValid() && ListView->GetNumItemsSelected() == 1; })
					.OnClicked_Lambda([this]() {
						InspectSelected();
						return FReply::Handled();
					})
					[
						Label("ProjectCleaner.Menu.Inspect", Static(TEXT("Inspect")), FSlateColor::UseForeground())
					]
				]
				+ SWrapBox::Slot().VAlign(VAlign_Center)
				[
					SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
					.ButtonStyle(&PjcShim::GetButtonStyle427(false))
					.ForegroundColor(FSlateColor::UseForeground())
#endif
					.ContentPadding(FMargin {8.0f, 1.0f, 10.0f, 1.0f})
					.ToolTipText(FText::FromString(TEXT("Shows the selection in the Content Browser")))
					.IsEnabled_Lambda(HasSelection)
					.OnClicked_Lambda([this]() {
						BrowseSelected();
						return FReply::Handled();
					})
					[
						Label("ProjectCleaner.Menu.Browse", Static(TEXT("Browse")), FSlateColor::UseForeground())
					]
				]
				// the selection, or what selecting is for
				+ SWrapBox::Slot().VAlign(VAlign_Center).Padding(FMargin {4.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(SBox)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(this, &SPjcTabUnused::GetSelectionText)
							.ColorAndOpacity_Lambda([HasSelection]() { return HasSelection() ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground(); })
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(this, &SPjcTabUnused::GetSelectionGroupText)
							.ToolTipText(FText::FromString(TEXT("Unused assets that reference the selection: deleted with it")))
							.ColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.Yellow"))
						]
					]
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(SComboButton)
				.ComboButtonStyle(&PjcShim::GetComboButtonStyle())
#if ENGINE_MAJOR_VERSION < 5
				.ForegroundColor(FSlateColor::UseForeground())
#endif
				.ToolTipText(FText::FromString(TEXT("View options of the list")))
				.OnGetMenuContent(this, &SPjcTabUnused::MakeListViewMenu)
				.ButtonContent()
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("View")))
				]
			]
		];
	// clang-format on
}

TSharedRef<SWidget> SPjcTabUnused::MakeListViewMenu() {
	FMenuBuilder Menu {true, nullptr};
	PjcWidgets::AddMenuEntry(
		Menu, EPjcMenuIcon::Reset, TEXT("Reset columns"), TEXT("Puts the asset list columns back in their first order"), [this]() {
			ColumnOrder = GetDefaultColumns();
			SaveViewState();
			RebuildHeader();
		}
	);
	return Menu.MakeWidget();
}

TSharedRef<SWidget> SPjcTabUnused::MakeExcludeMenu() {
	FMenuBuilder Menu {true, nullptr};
	AddExcludeEntries(Menu);
	return Menu.MakeWidget();
}

void SPjcTabUnused::AddExcludeEntries(FMenuBuilder& Menu) {
	TSet<FString> ClassLabels;
	int32 NumSelected = 0;
	for (const auto& Row : ListRows) {
		if (ListView->IsItemSelected(Row)) {
			ClassLabels.Add(Row->ClassLabel);
			++NumSelected;
		}
	}
	const FString ClassesText = FString::Printf(TEXT("%s classes"), *PjcFormat::Count(ClassLabels.Num()));
	const FString ClassText = ClassLabels.Num() == 1 ? ClassLabels.Array()[0] : ClassesText;
	int32 Slash = INDEX_NONE;
	SelectedFolder.FindLastChar(TEXT('/'), Slash);
	const FString FolderName = SelectedFolder.Mid(Slash + 1);

	const FString Verb = Set == EPjcAssetSet::Review ? TEXT("Keep") : TEXT("Exclude");
	const FString AssetsLabel = FString::Printf(TEXT("%s assets (%s)"), *Verb, *PjcFormat::Count(NumSelected));
	PjcWidgets::AddMenuEntry(
		Menu, EPjcMenuIcon::Exclude, AssetsLabel, TEXT("Counts them as used and never deletes them, saved to Config/DefaultEditor.ini"), [this]() {
			ExcludeSelectedAssets();
		}
	);
	const FString FolderLabel = Verb + TEXT(" folder ") + FolderName;
	PjcWidgets::AddMenuEntry(
		Menu, EPjcMenuIcon::Exclude, FolderLabel, TEXT("Keeps every asset in the selected folder, saved to Config/DefaultEditor.ini"), [this]() {
			ExcludeFolder(SelectedFolder);
		}
	);
	const FString ClassLabel = Verb + TEXT(" class ") + ClassText;
	PjcWidgets::AddMenuEntry(
		Menu, EPjcMenuIcon::Exclude, ClassLabel, TEXT("Keeps every asset of the class, saved to Config/DefaultEditor.ini"), [this]() {
			ExcludeSelectedClasses();
		}
	);
}

void SPjcTabUnused::ExcludeSelectedAssets() {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	TArray<FString> Assets;
	for (const auto& Row : ListRows) {
		if (ListView->IsItemSelected(Row)) {
			Assets.Add(PjcShim::GetObjectPathString(Result.Assets[Row->Index]));
		}
	}
	if (Assets.Num() == 0) return;

	const TCHAR* Plural = Assets.Num() == 1 ? TEXT("") : TEXT("s");
	const TCHAR* Done = Set == EPjcAssetSet::Review ? TEXT("Kept") : TEXT("Excluded");
	PjcActions::Exclude({}, {}, Assets, FString::Printf(TEXT("%s %s asset%s"), Done, *PjcFormat::Count(Assets.Num()), Plural));
}

void SPjcTabUnused::ExcludeSelectedClasses() {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	TArray<FString> Classes;
	TArray<FString> Labels;
	for (const auto& Row : ListRows) {
		const UClass* AssetClass = ListView->IsItemSelected(Row) ? Result.Assets[Row->Index].GetClass() : nullptr;
		if (AssetClass && !Classes.Contains(AssetClass->GetPathName())) {
			Classes.Add(AssetClass->GetPathName());
			Labels.Add(Row->ClassLabel);
		}
	}
	if (Classes.Num() == 0) return;

	PjcActions::Exclude({}, Classes, {}, TEXT("Excluded ") + FString::Join(Labels, TEXT(", ")));
}

void SPjcTabUnused::ExcludeFolder(const FString& Path) {
	const TCHAR* Done = Set == EPjcAssetSet::Review ? TEXT("Kept") : TEXT("Excluded");
	PjcActions::Exclude({Path}, {}, {}, FString::Printf(TEXT("%s folder %s"), Done, *Path));
}

void SPjcTabUnused::RemoveSelectedExclusions() {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	TArray<FString> Folders;
	TArray<FString> Classes;
	TArray<FString> Assets;
	for (const FName Package : GetSelectedPackages()) {
		const FPjcExclusion* Exclusion = Result.Excluded.Find(Package);
		if (!Exclusion) continue;

		TArray<FString>& RulesOfKind = Exclusion->Kind == EPjcExclusionKind::Class ? Classes : Assets;
		TArray<FString>& Rules = Exclusion->Kind == EPjcExclusionKind::Folder ? Folders : RulesOfKind;
		Rules.AddUnique(Exclusion->Value);
	}
	if (Folders.Num() + Classes.Num() + Assets.Num() == 0) return;

	// folder and class rules keep other assets too: they go only after a confirm
	if (Folders.Num() + Classes.Num() > 0) {
		TArray<FString> Rules = Folders;
		for (const auto& Class : Classes) {
			Rules.Add(FSoftObjectPath {Class}.GetAssetName());
		}
		const FString Message = TEXT("These rules keep other assets too:\n\n") + FString::Join(Rules, TEXT("\n")) + TEXT("\n\nRemove them?");
		const FText Title = FText::FromString(TEXT("Remove exclusion"));
		if (PjcShim::ShowDialog(Title, FText::FromString(Message), EAppMsgType::YesNo) != EAppReturnType::Yes) return;
	}

	const int32 NumRules = Folders.Num() + Classes.Num() + Assets.Num();
	const TCHAR* Plural = NumRules == 1 ? TEXT("") : TEXT("s");
	PjcActions::RemoveExclusions(Folders, Classes, Assets, FString::Printf(TEXT("Removed %s exclusion%s"), *PjcFormat::Count(NumRules), Plural));
}

void SPjcTabUnused::InspectSelected() const {
	const TArray<FName> Packages = GetSelectedPackages();
	if (Packages.Num() == 1) {
		PjcTabs::Inspect(TabManager, Packages[0]);
	}
}

void SPjcTabUnused::BrowseSelected() const {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	TArray<FAssetData> Assets;
	for (const auto& Row : ListRows) {
		if (ListView->IsItemSelected(Row)) {
			Assets.Add(Result.Assets[Row->Index]);
		}
	}
	if (Assets.Num() > 0) {
		UPjcSubsystem::GetModuleContentBrowser().Get().SyncBrowserToAssets(Assets);
	}
}

FText SPjcTabUnused::GetSelectionText() const {
	if (!ListView.IsValid() || ListView->GetNumItemsSelected() == 0) {
		switch (Set) {
			case EPjcAssetSet::Excluded : return FText::FromString(TEXT("Select assets to remove their exclusion"));
			case EPjcAssetSet::Used :	  return FText::FromString(TEXT("Select an asset to inspect"));
			case EPjcAssetSet::Review :	  return FText::FromString(TEXT("Select assets to delete or keep"));
			case EPjcAssetSet::Unused :
			default :					  return FText::FromString(TEXT("Select assets to delete, exclude or inspect"));
		}
	}

	int32 Num = 0;
	int64 Size = 0;
	FAssetRowPtr One;
	for (const auto& Row : ListRows) {
		if (ListView->IsItemSelected(Row)) {
			++Num;
			Size += Row->Size;
			One = Row;
		}
	}
	if (Num == 1 && One.IsValid()) {
		const FString Held = One->Holder.IsEmpty() ? FString {} : TEXT(" \u00B7 held by ") + One->Holder;
		return FText::FromString(FString::Printf(TEXT("%s \u00B7 %s \u00B7 %s%s"), *One->Name, *One->ClassLabel, *PjcFormat::Size(Size), *Held));
	}
	return FText::FromString(FString::Printf(TEXT("%s selected \u00B7 %s"), *PjcFormat::Count(Num), *PjcFormat::Size(Size)));
}

FText SPjcTabUnused::GetSelectionGroupText() const {
	if (NumSelectionGroup == 0 || !ListView.IsValid() || ListView->GetNumItemsSelected() == 0) return FText::GetEmpty();

	return FText::FromString(FString::Printf(TEXT("  \u00B7  +%s go with them"), *PjcFormat::Count(NumSelectionGroup)));
}

TSharedRef<SWidget> SPjcTabUnused::MakeMapBar() {
	const auto AreaToggle = [this](const FString& Label, const bool bAll, const FString& ToolTip) {
		// clang-format off
		return SNew(SCheckBox)
			.Style(&PjcShim::GetToggleStyle())
			.ToolTipText(FText::FromString(ToolTip))
			.IsChecked_Lambda([this, bAll]() { return bMapAll == bAll ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this, bAll](ECheckBoxState) {
				bMapAll = bAll;
				SaveViewState();
				RefreshMap();
			})
			[
				SNew(SBox).Padding(FMargin {8.0f, 0.0f})
				[
					SNew(STextBlock).Text(FText::FromString(Label))
				]
			];
		// clang-format on
	};

	// clang-format off
	return SNew(SBox)
		.Visibility_Lambda([this]() { return bMapView ? EVisibility::Visible : EVisibility::Collapsed; })
		.Padding(FMargin {10.0f, 2.0f, 8.0f, 2.0f})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SAssignNew(Breadcrumb, SHorizontalBox)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {8.0f, 0.0f, 6.0f, 0.0f})
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Area:"))).ColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.Muted"))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				AreaToggle(TEXT("Listed"), false, TEXT("Only the assets of the list take space"))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				AreaToggle(TEXT("All"), true, TEXT("All assets, folders tinted by the share of the list"))
			]
		];
	// clang-format on
}

TSharedRef<SWidget> SPjcTabUnused::MakeMapLegend() const {
	const auto Entry = [](const FLinearColor& Color, const FString& Label) {
		// clang-format off
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 5.0f, 0.0f})
			[
				PjcWidgets::MakeSwatch(Color)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 14.0f, 0.0f})
			[
				SNew(STextBlock).Text(FText::FromString(Label)).ColorAndOpacity(FSlateColor::UseSubduedForeground())
			];
		// clang-format on
	};

	// clang-format off
	return SNew(SBox).Visibility_Lambda([this]() { return bMapView ? EVisibility::Visible : EVisibility::Collapsed; })
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).Visibility_Lambda([this]() { return Set == EPjcAssetSet::Unused ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					Entry(PjcWidgets::GetSetColors(EPjcAssetSet::Unused).Box, TEXT("Unused asset"))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).Visibility_Lambda([this]() { return Set == EPjcAssetSet::Excluded ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					Entry(PjcWidgets::GetSetColors(EPjcAssetSet::Excluded).Box, TEXT("Excluded asset"))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).Visibility_Lambda([this]() { return Set == EPjcAssetSet::Used ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					Entry(PjcWidgets::GetSetColors(EPjcAssetSet::Used).Box, TEXT("Used asset"))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).Visibility_Lambda([this]() { return Set == EPjcAssetSet::Review ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					Entry(PjcWidgets::GetSetColors(EPjcAssetSet::Review).Box, TEXT("Asset to review"))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).Visibility_Lambda([this]() { return bMapAll ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					Entry(PjcWidgets::GetColor(Set == EPjcAssetSet::Unused ? FName {"ProjectCleaner.Color.UsedQuiet"} : FName {"ProjectCleaner.Color.Hover"}), TEXT("Other asset"))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(STextBlock)
				.Text_Lambda([this]() {
					return FText::FromString(bMapAll ? TEXT("Folders tinted by the share of the list") : TEXT("Folders sized by the assets of the list"));
				})
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		];
	// clang-format on
}

void SPjcTabUnused::SetMapView(const bool bMap) {
	bMapView = bMap;
	SaveViewState();
	RefreshMap();
}

void SPjcTabUnused::RefreshMap() {
	if (!SizeMap.IsValid()) return;

	SizeMap->SetFolder(&UPjcSubsystem::GetScanResult(), Tree.Find(SelectedFolder), bMapAll, Set);
	RebuildBreadcrumb();
}

void SPjcTabUnused::RebuildBreadcrumb() {
	if (!Breadcrumb.IsValid()) return;

	Breadcrumb->ClearChildren();
	TArray<TSharedPtr<FPjcUnusedFolder>> Chain;
	for (TSharedPtr<FPjcUnusedFolder> Folder = Tree.Find(SelectedFolder); Folder.IsValid(); Folder = Folder->Parent.Pin()) {
		Chain.Insert(Folder, 0);
	}
	for (int32 Index = 0; Index < Chain.Num(); ++Index) {
		const TSharedPtr<FPjcUnusedFolder> Folder = Chain[Index];
		const FText Name = FText::FromString(Folder->IsRoot() ? Folder->Path.Mid(1) : Folder->Name);
		if (Index == Chain.Num() - 1) {
			// clang-format off
			Breadcrumb->AddSlot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(Name)
			];
			// clang-format on
			continue;
		}

		const FString Path = Folder->Path;
		// clang-format off
		Breadcrumb->AddSlot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SHyperlink)
			.Text(Name)
			.OnNavigate_Lambda([this, Path]() {
				SelectFolder(Path);
			})
		];
		Breadcrumb->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {4.0f, 0.0f})
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("\u203A"))).ColorAndOpacity(FSlateColor::UseSubduedForeground())
		];
		// clang-format on
	}
}

void SPjcTabUnused::OnMapAssetClicked(int32 Index) {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	if (!Result.Assets.IsValidIndex(Index)) return;

	// assets of subfolders are listed only with Subfolders on
	if (!bSubfolders) {
		bSubfolders = true;
		SaveViewState();
		RefreshAssets();
	}
	ClassBar->Reset();
	AssetSearch.Reset();
	ApplyAssetFilter();

	const FName Package = Result.Assets[Index].PackageName;
	for (const auto& Row : ListRows) {
		if (Row->Package == Package) {
			ListView->SetSelection(Row);
			ListView->RequestScrollIntoView(Row);
			break;
		}
	}
}

void SPjcTabUnused::ShowNotDeleted() {
	ShowSet(EPjcAssetSet::Unused);

	// the root of the first asset the last delete kept, with subfolders
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	const TMap<FName, FString>& Kept = UPjcSubsystem::GetLastCleanupKept();
	FString Root;
	for (const auto& Asset : Result.Assets) {
		if (Kept.Contains(Asset.PackageName)) {
			const FString Path = Asset.PackagePath.ToString();
			const int32 Slash = Path.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 1);
			Root = Slash > 0 ? Path.Left(Slash) : Path;
			break;
		}
	}
	if (Root.IsEmpty()) return;

	if (!bSubfolders) {
		bSubfolders = true;
		SaveViewState();
	}
	SelectFolder(Root);
	RefreshAssets();
	bHeldOnly = true;
	ApplyAssetFilter();
}
