// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabUnused.h"
#include "Slate/PjcTabs.h"
#include "Slate/PjcWidgets.h"
#include "Slate/SPjcDeleteDialog.h"
#include "Slate/SPjcScanBar.h"
#include "Slate/SPjcToolFrame.h"
#include "PjcFormat.h"
#include "PjcScanDraft.h"
#include "PjcSettings.h"
#include "PjcShim.h"
#include "PjcStyles.h"
#include "PjcSubsystem.h"
// Engine Headers
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IAssetTypeActions.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateIconFinder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
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

	/** Folder tree row: name, unused share bar, unused size, unused / all assets */
	class SPjcUnusedFolderRow final : public SMultiColumnTableRow<TSharedPtr<FPjcUnusedFolder>>
	{
	  public:

		SLATE_BEGIN_ARGS(SPjcUnusedFolderRow) {}
		SLATE_ARGUMENT(TSharedPtr<FPjcUnusedFolder>, Folder)
		SLATE_ATTRIBUTE(FText, HighlightText)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable) {
			Folder = InArgs._Folder;
			HighlightText = InArgs._HighlightText;
			SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments().Padding(FMargin {0.0f, 1.0f}), InTable);
		}

		virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Column) override {
			const FSlateColor Muted = FSlateColor::UseSubduedForeground();
			const bool bAnyUnused = Folder->Unused.Num > 0;

			if (Column == PjcUnusedColFolder) {
				const TSharedRef<SHorizontalBox> NameBox
					= SNew(SHorizontalBox).Clipping(EWidgetClipping::ClipToBounds).ToolTipText(FText::FromString(Folder->Path));
				NameBox->AddSlot().AutoWidth().VAlign(VAlign_Center)[SNew(SExpanderArrow, SharedThis(this)).IndentAmount(14).ShouldDrawWires(false)];
				NameBox->AddSlot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(
						FMargin {0.0f, 0.0f, 5.0f, 0.0f}
					)[SNew(SImage)
						  .Image_Lambda([this]() {
							  return PjcShim::GetBrush(
								  IsItemExpanded() ? TEXT("ContentBrowser.AssetTreeFolderOpen") : TEXT("ContentBrowser.AssetTreeFolderClosed")
							  );
						  })
						  .ColorAndOpacity(Folder->IsRoot() ? Muted : FSlateColor {PjcWidgets::GetColor("ProjectCleaner.Color.FolderTint")})];
				NameBox->AddSlot().AutoWidth().VAlign(
					VAlign_Center
				)[SNew(STextBlock).Text(FText::FromString(Folder->IsRoot() ? Folder->Path.Mid(1) : Folder->Name)).HighlightText(HighlightText)];
				if (Folder->IsAllUnused()) {
					NameBox->AddSlot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})[MakeTag(
							TEXT("all unused"), PjcWidgets::GetColor("ProjectCleaner.Color.Red"), FLinearColor {FColor::FromHex(TEXT("#1B0D0E"))}
						)];
				}
				if (Folder->bEmpty) {
					NameBox->AddSlot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})[MakeTag(
							TEXT("EMPTY"), PjcWidgets::GetColor("ProjectCleaner.Color.Track"), FLinearColor {FColor::FromHex(TEXT("#8A9099"))}
						)];
				}
				return NameBox;
			}

			if (Column == PjcUnusedColShare) {
				PjcWidgets::FBarSegment Segment;
				Segment.Value = static_cast<double>(Folder->Unused.Size);
				Segment.Color = PjcWidgets::GetColor("ProjectCleaner.Color.UnusedFill");

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
							.Text(FText::FromString(bAnyUnused ? PjcFormat::Percent(Folder->Unused.Size, Folder->All.Size) : FString {}))
							.Font(PjcWidgets::GetFontMono(8))
							.ColorAndOpacity(Muted)
							.Justification(ETextJustify::Right)
						]
					];
				// clang-format on
			}

			if (Column == PjcUnusedColSize) {
				return SNew(SBox)
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Center)
					.Padding(
						FMargin {0.0f, 0.0f, 8.0f, 0.0f}
					)[SNew(STextBlock)
						  .Text(FText::FromString(bAnyUnused ? PjcFormat::Size(Folder->Unused.Size) : TEXT("—")))
						  .Font(PjcWidgets::GetFontMono(9))
						  .ColorAndOpacity(bAnyUnused ? FSlateColor::UseForeground() : Muted)];
			}

			if (Column == PjcUnusedColAssets) {
				const FString Text = bAnyUnused
					? FString::Printf(TEXT("%s / %s"), *PjcFormat::Count(Folder->Unused.Num), *PjcFormat::Count(Folder->All.Num))
					: PjcFormat::Count(Folder->All.Num);
				return SNew(SBox)
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Center)
					.Padding(
						FMargin {0.0f, 0.0f, 8.0f, 0.0f}
					)[SNew(STextBlock).Text(FText::FromString(Text)).Font(PjcWidgets::GetFontMono(9)).ColorAndOpacity(Muted)];
			}

			return SNullWidget::NullWidget;
		}

	  private:

		static TSharedRef<SWidget> MakeTag(const FString& Text, const FLinearColor& Background, const FLinearColor& Foreground) {
			return SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(Background)
				.Padding(FMargin {5.0f, 0.0f})
				.VAlign(
					VAlign_Center
				)[SNew(STextBlock).Text(FText::FromString(Text)).Font(FPjcStyles::GetFont("Bold", 7)).ColorAndOpacity(Foreground)];
		}

		TSharedPtr<FPjcUnusedFolder> Folder;
		TAttribute<FText> HighlightText;
	};

	const FName PjcUnusedColName {TEXT("Name")};
	const FName PjcUnusedColClass {TEXT("Class")};
	const FName PjcUnusedColAssetSize {TEXT("AssetSize")};
	const FName PjcUnusedColPath {TEXT("Path")};
	const FName PjcUnusedColGroup {TEXT("Group")};

	/** Unused asset row: type color and icon, name with tags, class, size, folder */
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
			SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments().Padding(FMargin {0.0f, 1.0f}), InTable);
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
				if (Row->bLabel) {
					NameBox->AddSlot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(
							FMargin {6.0f, 0.0f, 4.0f, 0.0f}
						)[SNew(SBorder)
							  .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
							  .BorderBackgroundColor(FLinearColor {FColor::FromHex(TEXT("#2E2716"))})
							  .Padding(FMargin {4.0f, 0.0f})
							  .ToolTipText(
								  FText::FromString(TEXT("A folder label keeps it while Folder labels is on"))
							  )[SNew(STextBlock)
									.Text(FText::FromString(TEXT("LABEL")))
									.Font(PjcWidgets::GetFontMono(7))
									.ColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.Yellow"))]];
				}
				return NameBox;
			}

			if (Column == PjcUnusedColClass) {
				return SNew(SBox)
					.VAlign(VAlign_Center)
					.ToolTipText(FText::FromName(Row->ClassName))[SNew(STextBlock).Text(FText::FromString(Row->ClassLabel)).ColorAndOpacity(Muted)];
			}

			if (Column == PjcUnusedColAssetSize) {
				return SNew(SBox)
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Center)
					.Padding(
						FMargin {0.0f, 0.0f, 8.0f, 0.0f}
					)[SNew(STextBlock).Text(FText::FromString(PjcFormat::Size(Row->Size))).Font(PjcWidgets::GetFontMono(9))];
			}

			if (Column == PjcUnusedColPath) {
				return SNew(SBox).VAlign(
					VAlign_Center
				)[SNew(STextBlock).Text(FText::FromString(Row->Folder.IsEmpty() ? TEXT("·") : Row->Folder)).ColorAndOpacity(Muted)];
			}

			if (Column == PjcUnusedColGroup) {
				const FString Text = Row->Group > 0 ? TEXT("+") + PjcFormat::Count(Row->Group) : FString {};
				const FString Tip = FString::Printf(TEXT("%s unused assets reference it and are deleted with it"), *PjcFormat::Count(Row->Group));
				return SNew(SBox)
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Center)
					.Padding(FMargin {0.0f, 0.0f, 8.0f, 0.0f})
					.ToolTipText(
						Row->Group > 0 ? FText::FromString(Tip) : FText::GetEmpty()
					)[SNew(STextBlock)
						  .Text(FText::FromString(Text))
						  .Font(PjcWidgets::GetFontMono(9))
						  .ColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.Yellow"))];
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
		if (GConfig->GetString(PjcUnusedSection, TEXT("FolderSort"), SortColumn, GEditorPerProjectIni) && !SortColumn.IsEmpty()) {
			FolderSortColumn = FName {*SortColumn};
		}
		if (GConfig->GetBool(PjcUnusedSection, TEXT("FolderSortDescending"), bSortDescending, GEditorPerProjectIni)) {
			FolderSortMode = bSortDescending ? EColumnSortMode::Descending : EColumnSortMode::Ascending;
		}
		GConfig->GetBool(PjcUnusedSection, TEXT("Subfolders"), bSubfolders, GEditorPerProjectIni);
		if (GConfig->GetString(PjcUnusedSection, TEXT("AssetSort"), SortColumn, GEditorPerProjectIni) && !SortColumn.IsEmpty()) {
			AssetSortColumn = FName {*SortColumn};
		}
		if (GConfig->GetBool(PjcUnusedSection, TEXT("AssetSortDescending"), bSortDescending, GEditorPerProjectIni)) {
			AssetSortMode = bSortDescending ? EColumnSortMode::Descending : EColumnSortMode::Ascending;
		}
	}

	// clang-format off
	ChildSlot
	[
		SNew(SPjcToolFrame)
		.PageName(TEXT("AssetsUnused"))
		.PanelOpen(false)
		[
			SNew(SSplitter)
			.PhysicalSplitterHandleSize(3.0f)
			+ SSplitter::Slot().Value(0.46f)
			[
				MakeFolderPane()
			]
			+ SSplitter::Slot().Value(0.54f)
			[
				MakeAssetPane()
			]
		]
	];
	// clang-format on

	OnScanFinished();

	if (!UPjcSubsystem::GetScanResult().IsValid() && GetDefault<UPjcEditorSettings>()->bScanOnOpen) {
		SPjcScanBar::ScanProject();
	}
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
		ClassFilter.Reset();
		RefreshTree();
		RefreshAssets();
	}
	TreeView->SetSelection(Folder, ESelectInfo::Direct);
	TreeView->RequestScrollIntoView(Folder);
}

void SPjcTabUnused::OnScanFinished() {
	Tree = FPjcUnusedTree::Make(UPjcSubsystem::GetScanResult());

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

	// assets only folder labels keep are unused while labels do not count: tagged LABEL
	LabelOnly.Reset();
	GroupSizes.Reset();
	if (!GetDefault<UPjcScanSettings>()->bFolderLabelsUsed) {
		for (const auto& Asset : UPjcSubsystem::GetScanResult().FolderLabelOnly) {
			LabelOnly.Add(Asset.PackageName);
		}
	}

	RefreshTree();
	RefreshAssets();
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
		.DefaultLabel(FText::FromString(TEXT("Unused")))
		.DefaultTooltip(FText::FromString(TEXT("Share of the folder size that is unused")))
		.FixedWidth(128.0f)
		.SortMode(this, &SPjcTabUnused::GetFolderSortMode, PjcUnusedColShare)
		.OnSort(this, &SPjcTabUnused::OnFolderSort)
		+ SHeaderRow::Column(PjcUnusedColSize)
		.DefaultLabel(FText::FromString(TEXT("Size")))
		.DefaultTooltip(FText::FromString(TEXT("Size of the unused assets, subfolders included")))
		.FixedWidth(80.0f)
		.HAlignHeader(HAlign_Right)
		.SortMode(this, &SPjcTabUnused::GetFolderSortMode, PjcUnusedColSize)
		.OnSort(this, &SPjcTabUnused::OnFolderSort)
		+ SHeaderRow::Column(PjcUnusedColAssets)
		.DefaultLabel(FText::FromString(TEXT("Assets")))
		.DefaultTooltip(FText::FromString(TEXT("Unused assets / all assets, subfolders included")))
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

	return SNew(SBorder)
		.BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.GroupBorder")))
		.Padding(0.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {8.0f, 6.0f})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SSearchBox)
					.HintText(FText::FromString(TEXT("Search folders")))
					.OnTextChanged(this, &SPjcTabUnused::OnFolderSearchChanged)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(SComboButton)
					.ToolTipText(FText::FromString(TEXT("View options")))
					.OnGetMenuContent(this, &SPjcTabUnused::MakeViewMenu)
					.ButtonContent()
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("View")))
					]
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
				TreeView.ToSharedRef()
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

	return !bOnlyUnused || Folder.Unused.Num > 0 || (bShowEmpty && Folder.bEmpty) || Folder.Path.Equals(SelectedFolder);
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
			KeyA = A->All.Size > 0 ? static_cast<double>(A->Unused.Size) / static_cast<double>(A->All.Size) : 0.0;
			KeyB = B->All.Size > 0 ? static_cast<double>(B->Unused.Size) / static_cast<double>(B->All.Size) : 0.0;
		}
		else if (Column == PjcUnusedColAssets) {
			KeyA = A->Unused.Num;
			KeyB = B->Unused.Num;
		}
		else {
			KeyA = static_cast<double>(A->Unused.Size);
			KeyB = static_cast<double>(B->Unused.Size);
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
	return SNew(SPjcUnusedFolderRow, Table).Folder(Folder).HighlightText_Lambda([this]() {
		return FText::FromString(FolderSearch);
	});
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
	ClassFilter.Reset();
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
			const bool bShown = !bOnlyUnused || Folder.Unused.Num > 0 || (bShowEmpty && Folder.bEmpty);
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
	Menu.AddMenuEntry(
		FText::FromString(FString::Printf(TEXT("Delete unused… (%s)"), *PjcFormat::Count(Folder->Unused.Num))),
		FText::FromString(TEXT("Deletes the unused assets of the folder and its subfolders, with the unused assets that reference them")),
		FSlateIcon {},
		FUIAction {
		  FExecuteAction::CreateLambda([this, Folder]() {
			  TArray<FName> Packages;
			  GetFolderUnused(Folder->Path, Packages);
			  DeleteWithGroup(Packages);
		  }),
		  FCanExecuteAction::CreateLambda([Folder]() {
			  return Folder->Unused.Num > 0;
		  })
		}
	);
	Menu.AddMenuEntry(
		FText::FromString(TEXT("Exclude folder")),
		FText::FromString(TEXT("Keeps every asset in the folder, saved to Config/DefaultEditor.ini")),
		FSlateIcon {},
		FUIAction {FExecuteAction::CreateLambda([this, Folder]() {
			ExcludeFolder(Folder->Path);
		})}
	);
	Menu.AddSeparator();
	Menu.AddMenuEntry(
		FText::FromString(TEXT("Expand all")),
		FText::FromString(TEXT("Opens every subfolder")),
		FSlateIcon {},
		FUIAction {FExecuteAction::CreateLambda([this, Folder]() {
			SetExpansionRecursive(Folder, true);
		})}
	);
	Menu.AddMenuEntry(
		FText::FromString(TEXT("Collapse all")),
		FText::FromString(TEXT("Closes every subfolder")),
		FSlateIcon {},
		FUIAction {FExecuteAction::CreateLambda([this, Folder]() {
			SetExpansionRecursive(Folder, false);
		})}
	);
	Menu.AddSeparator();
	Menu.AddMenuEntry(
		FText::FromString(TEXT("Show in Explorer")),
		FText::FromString(TEXT("Opens the folder in the file explorer")),
		FSlateIcon {},
		FUIAction {FExecuteAction::CreateLambda([Folder]() {
			UPjcSubsystem::OpenPathInFileExplorer(UPjcSubsystem::PathConvertToAbsolute(Folder->Path));
		})}
	);
	return Menu.MakeWidget();
}

TSharedRef<SWidget> SPjcTabUnused::MakeViewMenu() {
	int32 NumEmpty = 0;
	for (const auto& Pair : Tree.Folders) {
		NumEmpty += Pair.Value->bEmpty ? 1 : 0;
	}

	FMenuBuilder Menu {true, nullptr};
	Menu.AddMenuEntry(
		FText::FromString(TEXT("Only folders with unused")),
		FText::FromString(TEXT("Hides folders without unused assets")),
		FSlateIcon {},
		FUIAction {
		  FExecuteAction::CreateLambda([this]() {
			  bOnlyUnused = !bOnlyUnused;
			  SaveViewState();
			  OnFolderSearchChanged(FText::FromString(FolderSearch));
		  }),
		  FCanExecuteAction {},
		  FIsActionChecked::CreateLambda([this]() {
			  return bOnlyUnused;
		  })
		},
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);
	Menu.AddMenuEntry(
		FText::FromString(FString::Printf(TEXT("Empty folders (%s)"), *PjcFormat::Count(NumEmpty))),
		FText::FromString(TEXT("Shows empty folders, cleanup deletes them")),
		FSlateIcon {},
		FUIAction {
		  FExecuteAction::CreateLambda([this]() {
			  bShowEmpty = !bShowEmpty;
			  OnFolderSearchChanged(FText::FromString(FolderSearch));
		  }),
		  FCanExecuteAction {},
		  FIsActionChecked::CreateLambda([this]() {
			  return bShowEmpty;
		  })
		},
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);
	Menu.AddSeparator();
	Menu.AddMenuEntry(FText::FromString(TEXT("Expand all")), FText::GetEmpty(), FSlateIcon {}, FUIAction {FExecuteAction::CreateLambda([this]() {
						  for (const auto& Root : TreeRoots) {
							  SetExpansionRecursive(Root, true);
						  }
					  })});
	Menu.AddMenuEntry(FText::FromString(TEXT("Collapse all")), FText::GetEmpty(), FSlateIcon {}, FUIAction {FExecuteAction::CreateLambda([this]() {
						  for (const auto& Root : TreeRoots) {
							  SetExpansionRecursive(Root, false);
							  TreeView->SetItemExpansion(Root, true);
						  }
					  })});
	return Menu.MakeWidget();
}

TSharedRef<SWidget> SPjcTabUnused::MakeAssetPane() {
	// clang-format off
	const TSharedRef<SHeaderRow> Header = SNew(SHeaderRow)
		+ SHeaderRow::Column(PjcUnusedColName)
		.DefaultLabel(FText::FromString(TEXT("Name")))
		.FillWidth(1.0f)
		.SortMode(this, &SPjcTabUnused::GetAssetSortMode, PjcUnusedColName)
		.OnSort(this, &SPjcTabUnused::OnAssetSort)
		+ SHeaderRow::Column(PjcUnusedColClass)
		.DefaultLabel(FText::FromString(TEXT("Class")))
		.FixedWidth(150.0f)
		.SortMode(this, &SPjcTabUnused::GetAssetSortMode, PjcUnusedColClass)
		.OnSort(this, &SPjcTabUnused::OnAssetSort)
		+ SHeaderRow::Column(PjcUnusedColAssetSize)
		.DefaultLabel(FText::FromString(TEXT("Size")))
		.FixedWidth(84.0f)
		.HAlignHeader(HAlign_Right)
		.SortMode(this, &SPjcTabUnused::GetAssetSortMode, PjcUnusedColAssetSize)
		.OnSort(this, &SPjcTabUnused::OnAssetSort)
		+ SHeaderRow::Column(PjcUnusedColPath)
		.DefaultLabel(FText::FromString(TEXT("Folder")))
		.DefaultTooltip(FText::FromString(TEXT("Subfolder of the selected folder")))
		.FillWidth(0.5f)
		.SortMode(this, &SPjcTabUnused::GetAssetSortMode, PjcUnusedColPath)
		.OnSort(this, &SPjcTabUnused::OnAssetSort)
		+ SHeaderRow::Column(PjcUnusedColGroup)
		.DefaultLabel(FText::FromString(TEXT("Group")))
		.DefaultTooltip(FText::FromString(TEXT("Assets deleted with it: the unused assets that reference it")))
		.FixedWidth(66.0f)
		.HAlignHeader(HAlign_Right)
		.SortMode(this, &SPjcTabUnused::GetAssetSortMode, PjcUnusedColGroup)
		.OnSort(this, &SPjcTabUnused::OnAssetSort);

	SAssignNew(ListView, SListView<FAssetRowPtr>)
		.ListItemsSource(&ListRows)
		.SelectionMode(ESelectionMode::Multi)
		.HeaderRow(Header)
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
			// where: folder, its unused total, subfolders, search
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {10.0f, 6.0f, 8.0f, 6.0f})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(this, &SPjcTabUnused::GetWhereText).Font(FPjcStyles::GetFont("Bold", 9))
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(this, &SPjcTabUnused::GetWhereInfoText).ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {8.0f, 0.0f})
				[
					SNew(SCheckBox)
					.ToolTipText(FText::FromString(TEXT("Lists the unused assets of subfolders too")))
					.IsChecked_Lambda([this]() { return bSubfolders ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](const ECheckBoxState State) {
						bSubfolders = State == ECheckBoxState::Checked;
						SaveViewState();
						RefreshAssets();
					})
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("Subfolders")))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(200.0f)
					[
						SNew(SSearchBox)
						.HintText(FText::FromString(TEXT("Search unused assets")))
						.OnTextChanged(this, &SPjcTabUnused::OnAssetSearchChanged)
					]
				]
			]
			// class filters, biggest first
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {8.0f, 0.0f, 8.0f, 6.0f})
			[
				SNew(SScrollBox)
				.Orientation(Orient_Horizontal)
				.ScrollBarThickness(FVector2D {4.0f, 4.0f})
				+ SScrollBox::Slot()
				[
					SAssignNew(ChipsBox, SHorizontalBox)
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
				ListView.ToSharedRef()
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				MakeFooter()
			]
		];
	// clang-format on
}

void SPjcTabUnused::RefreshAssets() {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	TArray<int32> Indices;
	Tree.GetUnused(Result, SelectedFolder, bSubfolders, Indices);

	FolderRows.Reset(Indices.Num());
	for (const int32 Index : Indices) {
		const FAssetData& Asset = Result.Assets[Index];
		const FTypeInfo& Type = GetTypeInfo(Asset);
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
		Row->bLabel = LabelOnly.Contains(Asset.PackageName);
		if (const int32* Group = GroupSizes.Find(Asset.PackageName)) {
			Row->Group = *Group;
		}
		else {
			TArray<FName> With;
			FPjcUnusedTree::GetDeleteGroup(Result, {Asset.PackageName}, With);
			Row->Group = GroupSizes.Add(Asset.PackageName, With.Num());
		}
		FolderRows.Add(Row);
	}

	// filters of classes no longer here go
	TSet<FName> Classes;
	for (const auto& Row : FolderRows) {
		Classes.Add(Row->ClassName);
	}
	ClassFilter = ClassFilter.Intersect(Classes);

	RebuildChips();
	ApplyAssetFilter();
}

void SPjcTabUnused::ApplyAssetFilter() {
	// selection stays over refreshes
	TSet<FName> Selected;
	for (const auto& Row : ListView->GetSelectedItems()) {
		Selected.Add(Row->Package);
	}

	ListRows.Reset();
	for (const auto& Row : FolderRows) {
		if (ClassFilter.Num() > 0 && !ClassFilter.Contains(Row->ClassName)) continue;
		if (!AssetSearch.IsEmpty() && !Row->Name.Contains(AssetSearch) && !Row->ClassLabel.Contains(AssetSearch)) continue;

		ListRows.Add(Row);
	}

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
		else if (Column == PjcUnusedColGroup) {
			Compare = A->Group < B->Group ? -1 : (A->Group > B->Group ? 1 : 0);
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
		FTypeInfo Type;
		int32 Num = 0;
		int64 Size = 0;
	};
	TMap<FName, FChip> Chips;
	for (const auto& Row : FolderRows) {
		FChip& Chip = Chips.FindOrAdd(Row->ClassName);
		Chip.ClassName = Row->ClassName;
		Chip.Type = FTypeInfo {Row->ClassLabel, Row->Color, Row->Icon};
		++Chip.Num;
		Chip.Size += Row->Size;
	}
	TArray<FChip> Sorted;
	Chips.GenerateValueArray(Sorted);
	Sorted.Sort([](const FChip& A, const FChip& B) {
		return A.Size != B.Size ? A.Size > B.Size : A.Type.Label < B.Type.Label;
	});

	const auto MakeChip
		= [this](const FName ClassName, const FString& Label, const TOptional<FLinearColor>& Color, const int32 Num, const FString& ToolTip) {
			  const TSharedRef<SHorizontalBox> Content = SNew(SHorizontalBox);
			  if (Color.IsSet()) {
				  Content->AddSlot()
					  .AutoWidth()
					  .VAlign(VAlign_Center)
					  .Padding(FMargin {0.0f, 0.0f, 5.0f, 0.0f})[PjcWidgets::MakeSwatch(Color.GetValue(), 8.0f)];
			  }
			  Content->AddSlot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(Label))];
			  Content->AddSlot()
				  .AutoWidth()
				  .VAlign(VAlign_Center)
				  .Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})[SNew(STextBlock)
																 .Text(FText::FromString(PjcFormat::Count(Num)))
																 .Font(PjcWidgets::GetFontMono(8))
																 .ColorAndOpacity(FSlateColor::UseSubduedForeground())];

			  // clang-format off
		return SNew(SCheckBox)
			.Style(&PjcShim::GetStyle().GetWidgetStyle<FCheckBoxStyle>(TEXT("ToggleButtonCheckbox")))
			.ToolTipText(FText::FromString(ToolTip))
			.IsChecked_Lambda([this, ClassName]() {
				const bool bOn = ClassName.IsNone() ? ClassFilter.Num() == 0 : ClassFilter.Contains(ClassName);
				return bOn ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
			.OnCheckStateChanged_Lambda([this, ClassName](ECheckBoxState) { OnChipClicked(ClassName); })
			[
				SNew(SBox).Padding(FMargin {8.0f, 2.0f})
				[
					Content
				]
			];
			  // clang-format on
		  };

	ChipsBox->ClearChildren();
	ChipsBox->AddSlot().AutoWidth().Padding(
		FMargin {0.0f, 0.0f, 4.0f, 0.0f}
	)[MakeChip(NAME_None, TEXT("All"), {}, FolderRows.Num(), TEXT("Every unused asset here"))];
	for (const auto& Chip : Sorted) {
		ChipsBox->AddSlot().AutoWidth().Padding(FMargin {0.0f, 0.0f, 4.0f, 0.0f})[MakeChip(
			Chip.ClassName,
			Chip.Type.Label,
			Chip.Type.Color,
			Chip.Num,
			FString::Printf(TEXT("%s · %s\nCtrl+click adds it to the filter"), *Chip.ClassName.ToString(), *PjcFormat::Size(Chip.Size))
		)];
	}
}

const SPjcTabUnused::FTypeInfo& SPjcTabUnused::GetTypeInfo(const FAssetData& Asset) {
	const FName ClassName = PjcShim::GetAssetClassName(Asset);
	if (const FTypeInfo* Found = TypeInfos.Find(ClassName)) return *Found;

	// as the Content Browser shows the type: its name, color and icon
	FTypeInfo Info;
	Info.Label = FPaths::GetExtension(ClassName.ToString()).IsEmpty() ? ClassName.ToString() : FPaths::GetExtension(ClassName.ToString());
	Info.Color = FLinearColor {FColor::FromHex(TEXT("#8A9099"))};
	const UClass* AssetClass = Asset.GetClass();
	if (AssetClass) {
		const TSharedPtr<IAssetTypeActions> Actions
			= FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().GetAssetTypeActionsForClass(AssetClass).Pin();
		if (Actions.IsValid()) {
			Info.Label = Actions->GetName().ToString();
			Info.Color = FLinearColor {Actions->GetTypeColor()};
		}
		Info.Icon = FSlateIconFinder::FindIconBrushForClass(AssetClass);
	}
	return TypeInfos.Add(ClassName, Info);
}

TSharedRef<ITableRow> SPjcTabUnused::OnGenerateAssetRow(FAssetRowPtr Row, const TSharedRef<STableViewBase>& Table) {
	return SNew(SPjcUnusedAssetRow, Table).Row(Row).HighlightText_Lambda([this]() {
		return FText::FromString(AssetSearch);
	});
}

void SPjcTabUnused::OnAssetSort(EColumnSortPriority::Type Priority, const FName& Column, EColumnSortMode::Type Mode) {
	// a new column starts with the biggest (names a to z)
	if (Column != AssetSortColumn) {
		Mode = Column == PjcUnusedColAssetSize || Column == PjcUnusedColGroup ? EColumnSortMode::Descending : EColumnSortMode::Ascending;
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

void SPjcTabUnused::OnChipClicked(const FName ClassName) {
	// a click shows one class (again: all), Ctrl+click adds or removes it
	if (ClassName.IsNone()) {
		ClassFilter.Reset();
	}
	else if (FSlateApplication::Get().GetModifierKeys().IsControlDown()) {
		if (ClassFilter.Contains(ClassName)) {
			ClassFilter.Remove(ClassName);
		}
		else {
			ClassFilter.Add(ClassName);
		}
	}
	else {
		const bool bOnly = ClassFilter.Num() == 1 && ClassFilter.Contains(ClassName);
		ClassFilter.Reset();
		if (!bOnly) {
			ClassFilter.Add(ClassName);
		}
	}
	ApplyAssetFilter();
}

FText SPjcTabUnused::GetWhereText() const {
	return FText::FromString(SelectedFolder.Mid(1));
}

FText SPjcTabUnused::GetWhereInfoText() const {
	int64 Size = 0;
	for (const auto& Row : FolderRows) {
		Size += Row->Size;
	}
	return FText::FromString(FString::Printf(TEXT("  ·  %s unused · %s"), *PjcFormat::Count(FolderRows.Num()), *PjcFormat::Size(Size)));
}

void SPjcTabUnused::SaveViewState() const {
	if (!GConfig) return;

	GConfig->SetBool(PjcUnusedSection, TEXT("OnlyFoldersWithUnused"), bOnlyUnused, GEditorPerProjectIni);
	GConfig->SetString(PjcUnusedSection, TEXT("FolderSort"), *FolderSortColumn.ToString(), GEditorPerProjectIni);
	GConfig->SetBool(PjcUnusedSection, TEXT("FolderSortDescending"), FolderSortMode == EColumnSortMode::Descending, GEditorPerProjectIni);
	GConfig->SetBool(PjcUnusedSection, TEXT("Subfolders"), bSubfolders, GEditorPerProjectIni);
	GConfig->SetString(PjcUnusedSection, TEXT("AssetSort"), *AssetSortColumn.ToString(), GEditorPerProjectIni);
	GConfig->SetBool(PjcUnusedSection, TEXT("AssetSortDescending"), AssetSortMode == EColumnSortMode::Descending, GEditorPerProjectIni);
}

TSharedPtr<SWidget> SPjcTabUnused::OnAssetContextMenu() {
	const TArray<FName> Packages = GetSelectedPackages();
	if (Packages.Num() == 0) return nullptr;

	FMenuBuilder Menu {true, nullptr};
	Menu.AddMenuEntry(
		FText::FromString(FString::Printf(TEXT("Delete… (%s)"), *PjcFormat::Count(Packages.Num()))),
		FText::FromString(TEXT("Deletes the selection with the unused assets that reference it")),
		FSlateIcon {},
		FUIAction {FExecuteAction::CreateLambda([this, Packages]() {
			DeleteWithGroup(Packages);
		})}
	);
	Menu.AddSeparator();
	AddExcludeEntries(Menu);
	Menu.AddSeparator();
	Menu.AddMenuEntry(
		FText::FromString(TEXT("Inspect")),
		FText::FromString(TEXT("Shows why it is unused in Asset Usage")),
		FSlateIcon {},
		FUIAction {FExecuteAction::CreateSP(this, &SPjcTabUnused::InspectSelected), FCanExecuteAction::CreateLambda([Packages]() {
					   return Packages.Num() == 1;
				   })}
	);
	Menu.AddMenuEntry(
		FText::FromString(TEXT("Browse")),
		FText::FromString(TEXT("Shows the selection in the Content Browser")),
		FSlateIcon {},
		FUIAction {FExecuteAction::CreateSP(this, &SPjcTabUnused::BrowseSelected)}
	);
	return Menu.MakeWidget();
}

FReply SPjcTabUnused::OnAssetKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) {
	if (KeyEvent.GetKey() == EKeys::Delete && ListView->GetNumItemsSelected() > 0) {
		DeleteWithGroup(GetSelectedPackages());
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
	Tree.GetUnused(Result, Path, true, Indices);
	OutPackages.Reset(Indices.Num());
	for (const int32 Index : Indices) {
		OutPackages.Add(Result.Assets[Index].PackageName);
	}
}

void SPjcTabUnused::DeleteWithGroup(const TArray<FName>& Packages) {
	if (Packages.Num() == 0) return;

	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	TArray<FName> GroupPackages;
	FPjcUnusedTree::GetDeleteGroup(Result, Packages, GroupPackages);

	// asset data of scanned assets and redirectors
	TMap<FName, const FAssetData*> ByPackage;
	for (const auto& Asset : Result.Assets) {
		ByPackage.Add(Asset.PackageName, &Asset);
	}
	for (const auto& Asset : Result.Redirectors) {
		ByPackage.Add(Asset.PackageName, &Asset);
	}
	const auto ToAssets = [&ByPackage](const TArray<FName>& Names, TArray<FAssetData>& Out) {
		for (const FName Name : Names) {
			if (const FAssetData* const* Found = ByPackage.Find(Name)) {
				Out.Add(**Found);
			}
		}
	};
	TArray<FAssetData> Selected;
	TArray<FAssetData> With;
	ToAssets(Packages, Selected);
	ToAssets(GroupPackages, With);

	// what the last delete could not remove
	TArray<FString> Held;
	const TMap<FName, FString>& Kept = UPjcSubsystem::GetLastCleanupKept();
	for (const auto& Asset : Selected) {
		if (const FString* Holder = Kept.Find(Asset.PackageName)) {
			Held.Add(*Holder);
		}
	}
	for (const auto& Asset : With) {
		if (const FString* Holder = Kept.Find(Asset.PackageName)) {
			Held.Add(*Holder);
		}
	}

	if (!SPjcDeleteDialog::Confirm(Selected, With, Held)) return;

	TArray<FAssetData> All = Selected;
	All.Append(With);
	UPjcSubsystem::DeleteAssets(All, true, true);
	SPjcScanBar::ScanProject();
}

void SPjcTabUnused::OnAssetSelectionChanged(FAssetRowPtr Row, ESelectInfo::Type SelectInfo) {
	TArray<FName> Group;
	const TArray<FName> Packages = GetSelectedPackages();
	if (Packages.Num() > 0) {
		FPjcUnusedTree::GetDeleteGroup(UPjcSubsystem::GetScanResult(), Packages, Group);
	}
	NumSelectionGroup = Group.Num();
}

TSharedRef<SWidget> SPjcTabUnused::MakeFooter() {
	const auto HasSelection = [this]() {
		return ListView.IsValid() && ListView->GetNumItemsSelected() > 0;
	};

	// clang-format off
	return SNew(SBorder)
		.BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.DarkGroupBorder")))
		.Padding(FMargin {10.0f, 6.0f})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(this, &SPjcTabUnused::GetSelectionText)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SPjcTabUnused::GetSelectionGroupText)
				.ToolTipText(FText::FromString(TEXT("Unused assets that reference the selection: deleted with it")))
				.ColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.Yellow"))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(SButton)
				.ButtonColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.UnusedFill"))
				.ContentPadding(FMargin {10.0f, 2.0f})
				.ToolTipText(FText::FromString(TEXT("Deletes the selection with the unused assets that reference it (Delete)")))
				.IsEnabled_Lambda(HasSelection)
				.OnClicked_Lambda([this]() {
					DeleteWithGroup(GetSelectedPackages());
					return FReply::Handled();
				})
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("Delete…"))).ColorAndOpacity(FLinearColor::White)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(SComboButton)
				.ToolTipText(FText::FromString(TEXT("Keeps the selection, its folder or its class from cleanup")))
				.IsEnabled_Lambda(HasSelection)
				.OnGetMenuContent(this, &SPjcTabUnused::MakeExcludeMenu)
				.ButtonContent()
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("Exclude")))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(SButton)
				.ContentPadding(FMargin {10.0f, 2.0f})
				.ToolTipText(FText::FromString(TEXT("Shows why it is unused in Asset Usage")))
				.IsEnabled_Lambda([this]() { return ListView.IsValid() && ListView->GetNumItemsSelected() == 1; })
				.OnClicked_Lambda([this]() {
					InspectSelected();
					return FReply::Handled();
				})
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("Inspect")))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(SButton)
				.ContentPadding(FMargin {10.0f, 2.0f})
				.ToolTipText(FText::FromString(TEXT("Shows the selection in the Content Browser")))
				.IsEnabled_Lambda(HasSelection)
				.OnClicked_Lambda([this]() {
					BrowseSelected();
					return FReply::Handled();
				})
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("Browse")))
				]
			]
		];
	// clang-format on
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
	const FString ClassText
		= ClassLabels.Num() == 1 ? ClassLabels.Array()[0] : FString::Printf(TEXT("%s classes"), *PjcFormat::Count(ClassLabels.Num()));
	int32 Slash = INDEX_NONE;
	SelectedFolder.FindLastChar(TEXT('/'), Slash);
	const FString FolderName = SelectedFolder.Mid(Slash + 1);

	Menu.AddMenuEntry(
		FText::FromString(FString::Printf(TEXT("Exclude assets (%s)"), *PjcFormat::Count(NumSelected))),
		FText::FromString(TEXT("Keeps the selected assets, saved to Config/DefaultEditor.ini")),
		FSlateIcon {},
		FUIAction {FExecuteAction::CreateSP(this, &SPjcTabUnused::ExcludeSelectedAssets)}
	);
	Menu.AddMenuEntry(
		FText::FromString(FString::Printf(TEXT("Exclude folder %s"), *FolderName)),
		FText::FromString(TEXT("Keeps every asset in the selected folder, saved to Config/DefaultEditor.ini")),
		FSlateIcon {},
		FUIAction {FExecuteAction::CreateLambda([this]() {
			ExcludeFolder(SelectedFolder);
		})}
	);
	Menu.AddMenuEntry(
		FText::FromString(FString::Printf(TEXT("Exclude class %s"), *ClassText)),
		FText::FromString(TEXT("Keeps every asset of the class, saved to Config/DefaultEditor.ini")),
		FSlateIcon {},
		FUIAction {FExecuteAction::CreateSP(this, &SPjcTabUnused::ExcludeSelectedClasses)}
	);
}

void SPjcTabUnused::Exclude(const TArray<FString>& Folders, const TArray<FString>& Classes, const TArray<FString>& Assets, const FString& Message) {
	UPjcAssetExcludeSettings* Settings = GetMutableDefault<UPjcAssetExcludeSettings>();
	for (const auto& Folder : Folders) {
		if (!Settings->ExcludedFolders.ContainsByPredicate([&Folder](const FDirectoryPath& Path) {
				return Path.Path.Equals(Folder);
			})) {
			Settings->ExcludedFolders.Add(FDirectoryPath {Folder});
		}
	}
	for (const auto& Class : Classes) {
		Settings->ExcludedClasses.AddUnique(TSoftClassPtr<UObject> {FSoftObjectPath {Class}});
	}
	for (const auto& Asset : Assets) {
		Settings->ExcludedAssets.AddUnique(TSoftObjectPtr<UObject> {FSoftObjectPath {Asset}});
	}
	PjcShim::UpdateDefaultConfigFile(Settings);

	// an edited draft keeps its edits and gets the exclusions too, so Rescan does not drop them
	FPjcScanDraft& Draft = FPjcScanDraft::Get();
	for (const auto& Folder : Folders) {
		Draft.ExcludedFolders.AddUnique(Folder);
	}
	for (const auto& Class : Classes) {
		Draft.ExcludedClasses.AddUnique(Class);
	}
	for (const auto& Asset : Assets) {
		Draft.ExcludedAssets.AddUnique(Asset);
	}
	FPjcScanDraft::OnEdited().Broadcast();

	UPjcSubsystem::ShowNotification(Message + TEXT(" · saved to Config/DefaultEditor.ini"), SNotificationItem::CS_Success, 4.0f);
	SPjcScanBar::ScanProject();
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

	Exclude({}, {}, Assets, FString::Printf(TEXT("Excluded %s asset%s"), *PjcFormat::Count(Assets.Num()), Assets.Num() == 1 ? TEXT("") : TEXT("s")));
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

	Exclude({}, Classes, {}, TEXT("Excluded ") + FString::Join(Labels, TEXT(", ")));
}

void SPjcTabUnused::ExcludeFolder(const FString& Path) {
	Exclude({Path}, {}, {}, TEXT("Excluded folder ") + Path);
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
		return FText::FromString(TEXT("No selection · Ctrl or Shift selects more"));
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
		return FText::FromString(FString::Printf(TEXT("%s · %s · %s"), *One->Name, *One->ClassLabel, *PjcFormat::Size(Size)));
	}
	return FText::FromString(FString::Printf(TEXT("%s selected · %s"), *PjcFormat::Count(Num), *PjcFormat::Size(Size)));
}

FText SPjcTabUnused::GetSelectionGroupText() const {
	if (NumSelectionGroup == 0 || !ListView.IsValid() || ListView->GetNumItemsSelected() == 0) return FText::GetEmpty();

	return FText::FromString(FString::Printf(TEXT("  ·  +%s go with them"), *PjcFormat::Count(NumSelectionGroup)));
}
