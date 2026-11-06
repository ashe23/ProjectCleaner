// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabUnused.h"
#include "Slate/PjcWidgets.h"
#include "Slate/SPjcScanBar.h"
#include "Slate/SPjcToolFrame.h"
#include "PjcFormat.h"
#include "PjcSettings.h"
#include "PjcShim.h"
#include "PjcStyles.h"
#include "PjcSubsystem.h"
// Engine Headers
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Misc/ConfigCacheIni.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
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
				const TSharedRef<SHorizontalBox> NameBox = SNew(SHorizontalBox).ToolTipText(FText::FromString(Folder->Path));
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

	SelectedFolder = Path;
	for (FFolderPtr Parent = Folder->Parent.Pin(); Parent.IsValid(); Parent = Parent->Parent.Pin()) {
		ExpandedFolders.Add(Parent->Path);
		TreeView->SetItemExpansion(Parent, true);
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

	RefreshTree();
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
	if (Folder.IsValid()) {
		SelectedFolder = Folder->Path;
	}
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
	return SNew(SBorder)
		.BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.GroupBorder")))
		.Padding(0.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {10.0f, 9.0f})
			[
				SNew(STextBlock).Text(this, &SPjcTabUnused::GetWhereText)
			]
		];
	// clang-format on
}

FText SPjcTabUnused::GetWhereText() const {
	const FFolderPtr Folder = Tree.Find(SelectedFolder);
	if (!Folder.IsValid()) return FText::GetEmpty();

	return FText::FromString(
		FString::Printf(
			TEXT("%s · %s unused · %s"), *SelectedFolder.Mid(1), *PjcFormat::Count(Folder->Unused.Num), *PjcFormat::Size(Folder->Unused.Size)
		)
	);
}

void SPjcTabUnused::SaveViewState() const {
	if (!GConfig) return;

	GConfig->SetBool(PjcUnusedSection, TEXT("OnlyFoldersWithUnused"), bOnlyUnused, GEditorPerProjectIni);
	GConfig->SetString(PjcUnusedSection, TEXT("FolderSort"), *FolderSortColumn.ToString(), GEditorPerProjectIni);
	GConfig->SetBool(PjcUnusedSection, TEXT("FolderSortDescending"), FolderSortMode == EColumnSortMode::Descending, GEditorPerProjectIni);
}
