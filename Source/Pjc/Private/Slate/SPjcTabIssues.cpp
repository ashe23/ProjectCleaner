// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabIssues.h"
#include "Slate/PjcWidgets.h"
#include "Slate/SPjcDialog.h"
#include "Slate/SPjcScanBar.h"
#include "Slate/SPjcToolFrame.h"
#include "PjcFormat.h"
#include "PjcSettings.h"
#include "PjcShim.h"
#include "PjcStyles.h"
#include "PjcTypes.h"
#include "PjcSubsystem.h"
// Engine Headers
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"

namespace
{
	// unity builds join files: names unique to this file
	const FName PjcIssueColFirst {TEXT("First")};

	FString PjcIssueRelative(const FString& AbsolutePath) {
		FString Path = UPjcSubsystem::PathNormalize(AbsolutePath);
		const FString ProjectDir = UPjcSubsystem::PathNormalize(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir())) + TEXT("/");
		Path.RemoveFromStart(ProjectDir);
		return Path;
	}

	// Issues row: name with tag and muted path, then cells
	class SPjcIssueRowWidget final : public SMultiColumnTableRow<TSharedPtr<FPjcIssueRow>>
	{
	  public:

		SLATE_BEGIN_ARGS(SPjcIssueRowWidget) {}
		SLATE_ARGUMENT(TSharedPtr<FPjcIssueRow>, Row)
		SLATE_ARGUMENT(TArray<FName>, Columns)
		SLATE_ARGUMENT(TArray<bool>, MonoColumns)
		SLATE_ARGUMENT(TArray<bool>, RightColumns)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable) {
			Row = InArgs._Row;
			Columns = InArgs._Columns;
			MonoColumns = InArgs._MonoColumns;
			RightColumns = InArgs._RightColumns;
			const FText ToolTip = FText::FromString(Row->ToolTip);
			SMultiColumnTableRow::FArguments RowArgs;
#if ENGINE_MAJOR_VERSION < 5
			RowArgs.Style(&PjcShim::GetRowStyle427());
#endif
			SMultiColumnTableRow::Construct(RowArgs.Padding(FMargin {0.0f, 2.0f}).ToolTipText(ToolTip), InTable);
		}

		virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Column) override {
			const FSlateColor Muted = FSlateColor::UseSubduedForeground();
			const int32 Index = Columns.IndexOfByKey(Column);

			if (Index == 0) {
				const TSharedRef<SHorizontalBox> CellBox = SNew(SHorizontalBox).Clipping(EWidgetClipping::ClipToBounds);
				// clang-format off
				CellBox->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(STextBlock).Text(FText::FromString(Row->Name))
				];
				// clang-format on
				if (!Row->Tag.IsEmpty()) {
					// clang-format off
					CellBox->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
						.BorderBackgroundColor(PjcWidgets::GetColor(Row->TagColor) * FLinearColor {0.3f, 0.3f, 0.3f, 1.0f})
						.Padding(FMargin {4.0f, 0.0f})
						.ToolTipText(FText::FromString(Row->TagToolTip))
						[
							SNew(STextBlock)
							.Text(FText::FromString(Row->Tag))
							.Font(PjcWidgets::GetFontMono(7))
							.ColorAndOpacity(PjcWidgets::GetColor(Row->TagColor))
						]
					];
					// clang-format on
				}
				if (!Row->Sub.IsEmpty()) {
					// clang-format off
					CellBox->AddSlot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(FMargin {7.0f, 0.0f, 0.0f, 0.0f})
					[
						SNew(STextBlock).Text(FText::FromString(Row->Sub)).Font(FPjcStyles::GetFont("Regular", 8)).ColorAndOpacity(Muted)
					];
					// clang-format on
				}
				return CellBox;
			}

			const int32 Cell = Index - 1;
			if (!Row->Cells.IsValidIndex(Cell)) return SNullWidget::NullWidget;

			const FName Color = Row->CellColors.IsValidIndex(Cell) ? Row->CellColors[Cell] : NAME_None;
			const bool bMono = MonoColumns.IsValidIndex(Index) && MonoColumns[Index];
			const bool bRight = RightColumns.IsValidIndex(Index) && RightColumns[Index];
			const TSharedRef<SHorizontalBox> CellBox = SNew(SHorizontalBox).Clipping(EWidgetClipping::ClipToBounds);
			// clang-format off
			CellBox->AddSlot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Row->Cells[Cell]))
				.Font(bMono ? PjcWidgets::GetFontMono(9) : FPjcStyles::GetFont("Regular", 9))
				.ColorAndOpacity(Color.IsNone() ? FSlateColor::UseForeground() : FSlateColor {PjcWidgets::GetColor(Color)})
			];
			// clang-format on
			if (Row->CellSubs.IsValidIndex(Cell) && !Row->CellSubs[Cell].IsEmpty()) {
				// clang-format off
				CellBox->AddSlot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(FMargin {7.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(STextBlock).Text(FText::FromString(Row->CellSubs[Cell])).Font(FPjcStyles::GetFont("Regular", 8)).ColorAndOpacity(Muted)
				];
				// clang-format on
			}
			// clang-format off
			return SNew(SBox).HAlign(bRight ? HAlign_Right : HAlign_Fill).Padding(FMargin {4.0f, 0.0f, bRight ? 8.0f : 4.0f, 0.0f})
				[
					CellBox
				];
			// clang-format on
		}

	  private:

		TSharedPtr<FPjcIssueRow> Row;
		TArray<FName> Columns;
		TArray<bool> MonoColumns;
		TArray<bool> RightColumns;
	};
}	// namespace

void SPjcTabIssues::Construct(const FArguments& InArgs) {
	TabManager = InArgs._TabManager;
	ScanFinishedHandle = UPjcSubsystem::OnScanFinished().AddSP(this, &SPjcTabIssues::OnScanFinished);

	// clang-format off
	ChildSlot
	[
		SNew(SPjcToolFrame).PageName(TEXT("Issues")).PanelOpen(false)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(200.0f)
				[
					SNew(SBorder).BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.GroupBorder"))).Padding(FMargin {0.0f, 8.0f})
					[
						MakeCategories()
					]
				]
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(1.0f)
				[
					SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.Line"))
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(SBorder).BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.GroupBorder"))).Padding(0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						MakeHeader()
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {12.0f, 0.0f, 8.0f, 6.0f})
					[
						SAssignNew(Chips, SHorizontalBox)
						.Visibility_Lambda([this]() {
							return Category == EPjcIssueCategory::External && bKept ? EVisibility::Visible : EVisibility::Collapsed;
						})
					]
					// the selection and its actions, above the list
					+ SVerticalBox::Slot().AutoHeight()
					[
						MakeFooter()
					]
					+ SVerticalBox::Slot().FillHeight(1.0f)
					[
						SAssignNew(ListBox, SBox)
					]
				]
			]
		]
	];
	// clang-format on

	OnScanFinished();
}

SPjcTabIssues::~SPjcTabIssues() {
	UPjcSubsystem::OnScanFinished().Remove(ScanFinishedHandle);
}

void SPjcTabIssues::ShowCategory(const EPjcIssueCategory InCategory, const bool bShowKept) {
	if (Category == InCategory && bKept == bShowKept) return;

	Category = InCategory;
	bKept = bShowKept && InCategory == EPjcIssueCategory::External;
	KeptReason.Reset();
	RebuildRows();
	RebuildList();
}

void SPjcTabIssues::OnScanFinished() {
	PjcIssues::GetRedirectors(UPjcSubsystem::GetScanResult(), Redirectors);
	RebuildRows();
	RebuildList();
}

int32 SPjcTabIssues::GetCount(const EPjcIssueCategory InCategory) const {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	switch (InCategory) {
		case EPjcIssueCategory::Redirectors : return Redirectors.Num();
		case EPjcIssueCategory::External :	  return Result.FilesExternal.Num();
		case EPjcIssueCategory::Corrupted :	  return Result.FilesCorrupted.Num();
		case EPjcIssueCategory::Empty :		  return Result.FoldersEmpty.Num();
	}
	return 0;
}

void SPjcTabIssues::RebuildRows() {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	AllRows.Reset();

	const auto AddFile = [this](const FString& File) -> FPjcIssueRow& {
		const FRowPtr Row = MakeShared<FPjcIssueRow>();
		const FString Relative = PjcIssueRelative(File);
		Row->Id = File;
		Row->Name = FPaths::GetCleanFilename(File);
		Row->ToolTip = Relative;
		Row->Size = UPjcSubsystem::GetFileSize(File);
		AllRows.Add(Row);
		return *Row;
	};

	switch (Category) {
		case EPjcIssueCategory::Redirectors :
			for (int32 Index = 0; Index < Redirectors.Num(); ++Index) {
				const FPjcRedirectorInfo& Info = Redirectors[Index];
				const FString Path = Info.Asset.PackageName.ToString();
				const FString Target = Info.Target.ToString();
				const FRowPtr Row = MakeShared<FPjcIssueRow>();
				Row->Id = Path;
				Row->Redirector = Index;
				Row->Name = FPackageName::GetShortName(Info.Asset.PackageName);
				Row->Sub = FPaths::GetPath(Path).Mid(1);
				Row->ToolTip = Path;
				if (const FString* Kept = FixKept.Find(Info.Asset.PackageName)) {
					Row->Tag = TEXT("FIX FAILED");
					Row->TagColor = TEXT("ProjectCleaner.Color.Red");
					Row->TagToolTip = *Kept;
				}
				Row->Cells.Add(Info.Target.IsNone() ? TEXT("outside scan") : TEXT("\u2192 ") + FPackageName::GetShortName(Info.Target));
				const TCHAR* Missing = Info.bTargetExists ? TEXT("") : TEXT(" \u00B7 missing");
				Row->CellSubs.Add(Info.Target.IsNone() ? FString {} : FPaths::GetPath(Target).Mid(1) + Missing);
				Row->CellColors.Add(Info.Target.IsNone() ? FName {} : (Info.bTargetExists ? FName {} : FName {TEXT("ProjectCleaner.Color.Red")}));
				Row->Cells.Add(Info.UsedBy.Num() > 0 ? PjcFormat::Count(Info.UsedBy.Num()) : TEXT("\u2014"));
				Row->CellSubs.Add(FString {});
				Row->CellColors.Add(NAME_None);
				Row->Cells.Add(PjcIssues::GetStateName(Info.State));
				Row->CellSubs.Add(FString {});
				FName StateColor {TEXT("ProjectCleaner.Color.Gray")};
				if (Info.State == EPjcRedirectorState::Broken) StateColor = TEXT("ProjectCleaner.Color.Red");
				if (Info.State == EPjcRedirectorState::InUse) StateColor = TEXT("ProjectCleaner.Color.Yellow");
				Row->CellColors.Add(StateColor);
				TArray<FString> Users;
				for (const FName User : Info.UsedBy) {
					Users.Add(FPackageName::GetShortName(User));
				}
				const FString UsedByText = Users.Num() > 0 ? TEXT("\nUsed by: ") + FString::Join(Users, TEXT(", ")) : FString {};
				Row->ToolTip = FString::Printf(TEXT("%s\n%s%s"), *Path, *PjcIssues::GetStateDescription(Info.State), *UsedByText);
				AllRows.Add(Row);
			}
			break;

		case EPjcIssueCategory::External :
			if (bKept) {
				for (int32 Index = 0; Index < Result.FilesExternalKept.Num(); ++Index) {
					const FString& File = Result.FilesExternalKept[Index];
					const FString Reason = Result.FilesExternalKeptReasons.IsValidIndex(Index) ? Result.FilesExternalKeptReasons[Index] : FString {};
					if (!KeptReason.IsEmpty() && Reason != KeptReason) continue;

					FPjcIssueRow& Row = AddFile(File);
					Row.Sub = FPaths::GetPath(PjcIssueRelative(File));
					Row.Reason = Reason;
					Row.Cells = {TEXT(".") + FPaths::GetExtension(File).ToLower(), PjcFormat::Size(Row.Size), Reason};
					Row.CellColors = {TEXT("ProjectCleaner.Color.Gray"), NAME_None, TEXT("ProjectCleaner.Color.Gray")};
					Row.ToolTip = PjcIssueRelative(File) + TEXT("\nKept: ") + Reason + TEXT(". The plugin never deletes it.");
				}
				break;
			}
			for (const auto& File : Result.FilesExternal) {
				FPjcIssueRow& Row = AddFile(File);
				Row.Cells = {TEXT(".") + FPaths::GetExtension(File).ToLower(), PjcFormat::Size(Row.Size), FPaths::GetPath(PjcIssueRelative(File))};
				Row.CellColors = {TEXT("ProjectCleaner.Color.Gray"), NAME_None, TEXT("ProjectCleaner.Color.Gray")};
			}
			break;

		case EPjcIssueCategory::Corrupted :
			for (const auto& File : Result.FilesCorrupted) {
				FPjcIssueRow& Row = AddFile(File);
				Row.Sub = FPaths::GetPath(PjcIssueRelative(File));
				Row.Cells = {PjcFormat::Size(Row.Size), TEXT("The asset registry cannot read it")};
				Row.CellColors = {NAME_None, TEXT("ProjectCleaner.Color.Gray")};
			}
			break;

		case EPjcIssueCategory::Empty :
			for (const auto& Folder : Result.FoldersEmpty) {
				const FRowPtr Row = MakeShared<FPjcIssueRow>();
				const FString Path = UPjcSubsystem::PathConvertToRelative(Folder);
				Row->Id = Folder;
				Row->Name = Path.StartsWith(TEXT("/")) ? Path.Mid(1) : PjcIssueRelative(Folder);
				Row->ToolTip = Folder;
				AllRows.Add(Row);
			}
			break;
	}

	ApplyFilter();
	RebuildChips();
}

TArray<SPjcTabIssues::FColumn> SPjcTabIssues::GetColumns() const {
	TArray<FColumn> Columns;
	const auto Add = [&Columns](const TCHAR* Label, float Width, float Fill, bool bRight, bool bMono, const TCHAR* ToolTip = TEXT("")) {
		FColumn& Column = Columns.AddDefaulted_GetRef();
		Column.Id = Columns.Num() == 1 ? PjcIssueColFirst : FName {Label};
		Column.Label = Label;
		Column.Width = Width;
		Column.Fill = Fill;
		Column.bRight = bRight;
		Column.bMono = bMono;
		Column.ToolTip = ToolTip;
	};

	switch (Category) {
		case EPjcIssueCategory::Redirectors :
			Add(TEXT("Redirector"), 0.0f, 1.0f, false, false);
			Add(TEXT("Points to"), 0.0f, 0.8f, false, false, TEXT("The asset it points to"));
			Add(TEXT("Used by"), 80.0f, 0.0f, true, true, TEXT("Assets that still point through it"));
			Add(TEXT("State"), 130.0f, 0.0f, false, false);
			break;
		case EPjcIssueCategory::External :
			Add(TEXT("File"), 0.0f, 1.0f, false, false);
			Add(TEXT("Type"), 80.0f, 0.0f, false, true);
			Add(TEXT("Size"), 90.0f, 0.0f, true, true);
			if (bKept) {
				Add(TEXT("Kept by"), 220.0f, 0.0f, false, false, TEXT("Why the plugin never deletes it"));
			}
			else {
				Add(TEXT("Folder"), 0.0f, 0.7f, false, false);
			}
			break;
		case EPjcIssueCategory::Corrupted :
			Add(TEXT("File"), 0.0f, 1.0f, false, false);
			Add(TEXT("Size"), 90.0f, 0.0f, true, true);
			Add(TEXT("Problem"), 0.0f, 0.6f, false, false);
			break;
		case EPjcIssueCategory::Empty : Add(TEXT("Folder"), 0.0f, 1.0f, false, false); break;
	}
	return Columns;
}

void SPjcTabIssues::RebuildList() {
	const TArray<FColumn> Columns = GetColumns();
	const TSharedRef<SHeaderRow> Header = SNew(SHeaderRow);
	TArray<FName> Ids;
	TArray<bool> Mono;
	TArray<bool> Right;
	for (const auto& Column : Columns) {
		SHeaderRow::FColumn::FArguments Arguments;
		Arguments.ColumnId(Column.Id).DefaultLabel(FText::FromString(Column.Label)).DefaultTooltip(FText::FromString(Column.ToolTip));
		if (Column.bRight) {
			Arguments.HAlignHeader(HAlign_Right);
		}
		if (Column.Width > 0.0f) {
			Arguments.FixedWidth(Column.Width);
		}
		else {
			Arguments.FillWidth(Column.Fill);
		}
		Header->AddColumn(Arguments);
		Ids.Add(Column.Id);
		Mono.Add(Column.bMono);
		Right.Add(Column.bRight);
	}

	// clang-format off
	SAssignNew(ListView, SListView<FRowPtr>)
	.ListItemsSource(&Rows)
	.SelectionMode(ESelectionMode::Multi)
	.HeaderRow(Header)
	.OnGenerateRow_Lambda([Ids, Mono, Right](FRowPtr Row, const TSharedRef<STableViewBase>& Table) -> TSharedRef<ITableRow> {
		return SNew(SPjcIssueRowWidget, Table).Row(Row).Columns(Ids).MonoColumns(Mono).RightColumns(Right);
	});
	// clang-format on

	if (AllRows.Num() == 0) {
		// clang-format off
		ListBox->SetContent(
			SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("\u2713 None")))
					.Font(FPjcStyles::GetFont("Bold", 11))
					.ColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.Ok"))
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin {0.0f, 4.0f})
				[
					SNew(STextBlock)
					.Text(FText::FromString(GetTitle().ToString() + TEXT(": nothing found in this scan.")))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			]
		);
		// clang-format on
		return;
	}
	ListBox->SetContent(ListView.ToSharedRef());
}

void SPjcTabIssues::ApplyFilter() {
	Rows.Reset();
	for (const auto& Row : AllRows) {
		if (Search.IsEmpty() || Row->Id.Contains(Search) || Row->Name.Contains(Search) || Row->Reason.Contains(Search)) {
			Rows.Add(Row);
		}
	}
	if (ListView.IsValid()) {
		ListView->RequestListRefresh();
	}
}

TSharedRef<SWidget> SPjcTabIssues::MakeCategories() {
	const TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	// clang-format off
	Box->AddSlot().AutoHeight().Padding(FMargin {12.0f, 0.0f, 12.0f, 8.0f})
	[
		PjcWidgets::MakeCardTitle(TEXT("Issues"))
	];
	// clang-format on

	struct FEntry
	{
		EPjcIssueCategory Category;
		const TCHAR* Name;
		const TCHAR* ToolTip;
	};
	const FEntry Entries[] = {
	  {EPjcIssueCategory::Redirectors,	   TEXT("Redirectors"),				 TEXT("Left by moved or renamed assets")},
	  {	  EPjcIssueCategory::External,	TEXT("External files"), TEXT("Files in content folders that are not assets")},
	  {	 EPjcIssueCategory::Corrupted, TEXT("Corrupted files"),			  TEXT("Asset files the engine cannot read")},
	  {		 EPjcIssueCategory::Empty,	 TEXT("Empty folders"),				 TEXT("Folders without assets or files")},
	};
	for (const auto& Entry : Entries) {
		const EPjcIssueCategory EntryCategory = Entry.Category;
		// clang-format off
		Box->AddSlot().AutoHeight()
		[
			SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
			.ForegroundColor(FSlateColor::UseForeground())
#endif
			.ButtonStyle(&FPjcStyles::Get().GetWidgetStyle<FButtonStyle>("ProjectCleaner.Button.Row"))
			.ContentPadding(FMargin {0.0f})
			.ToolTipText(FText::FromString(Entry.ToolTip))
			.OnClicked_Lambda([this, EntryCategory]() {
				ShowCategory(EntryCategory);
				return FReply::Handled();
			})
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor_Lambda([this, EntryCategory]() {
					return Category == EntryCategory ? FLinearColor {FColor::FromHex(TEXT("#1D2A3A"))} : FLinearColor::Transparent;
				})
				.Padding(FMargin {12.0f, 6.0f})
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(Entry.Name))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Font(PjcWidgets::GetFontMono(9))
						.Text_Lambda([this, EntryCategory]() { return FText::FromString(PjcFormat::Count(GetCount(EntryCategory))); })
						.ColorAndOpacity_Lambda([this, EntryCategory]() {
							const TCHAR* Color = GetCount(EntryCategory) > 0 ? TEXT("ProjectCleaner.Color.Yellow") : TEXT("ProjectCleaner.Color.Ok");
							return FSlateColor {PjcWidgets::GetColor(Color)};
						})
					]
				]
			]
		];
		// clang-format on
	}
	return Box;
}

TSharedRef<SWidget> SPjcTabIssues::MakeHeader() {
	// clang-format off
	return SNew(SBox).Padding(FMargin {12.0f, 8.0f, 8.0f, 8.0f})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(this, &SPjcTabIssues::GetTitle).Font(FPjcStyles::GetFont("Bold", 10))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(FMargin {10.0f, 0.0f})
			[
				SNew(STextBlock)
				.Text(this, &SPjcTabIssues::GetDescription)
				.ToolTipText(this, &SPjcTabIssues::GetDescription)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.Clipping(EWidgetClipping::ClipToBounds)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 8.0f, 0.0f})
			[
				SNew(SHorizontalBox)
				.Visibility_Lambda([this]() { return Category == EPjcIssueCategory::External ? EVisibility::Visible : EVisibility::Collapsed; })
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SCheckBox)
					.Style(&PjcShim::GetToggleStyle())
					.ToolTipText(FText::FromString(TEXT("Not assets and not used by the engine or the project")))
					.IsChecked_Lambda([this]() { return bKept ? ECheckBoxState::Unchecked : ECheckBoxState::Checked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState) {
						bKept = false;
						RebuildRows();
						RebuildList();
					})
					[
						SNew(SBox).Padding(FMargin {8.0f, 1.0f})
						[
							SNew(STextBlock)
							.Text_Lambda([]() {
								return FText::FromString(TEXT("To review  ") + PjcFormat::Count(UPjcSubsystem::GetScanResult().FilesExternal.Num()));
							})
						]
					]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SCheckBox)
					.Style(&PjcShim::GetToggleStyle())
					.ToolTipText(
						FText::FromString(
							TEXT("Files the plugin never deletes: engine folders, staged folders, names in source or config, exclusions")
						)
					)
					.IsChecked_Lambda([this]() { return bKept ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState) {
						bKept = true;
						KeptReason.Reset();
						RebuildRows();
						RebuildList();
					})
					[
						SNew(SBox).Padding(FMargin {8.0f, 1.0f})
						[
							SNew(STextBlock)
							.Text_Lambda([]() {
								return FText::FromString(TEXT("Kept  ") + PjcFormat::Count(UPjcSubsystem::GetScanResult().FilesExternalKept.Num()));
							})
						]
					]
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(200.0f)
				[
					SNew(SSearchBox)
#if ENGINE_MAJOR_VERSION < 5
					.Style(&PjcShim::GetSearchBoxStyle427())
#endif
					.HintText(FText::FromString(TEXT("Search")))
					.OnTextChanged_Lambda([this](const FText& Text) {
						Search = Text.ToString().TrimStartAndEnd();
						ApplyFilter();
					})
				]
			]
		];
	// clang-format on
}

TSharedRef<SWidget> SPjcTabIssues::MakeFooter() {
	const auto Is = [this](const EPjcIssueCategory InCategory) {
		return [this, InCategory]() {
			return Category == InCategory ? EVisibility::Visible : EVisibility::Collapsed;
		};
	};
	const auto Button = [](const FString& Label, const FString& ToolTip, const FName Color, TFunction<void()> OnClick, TFunction<bool()> IsEnabled) {
		// clang-format off
		return SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
			.ButtonStyle(&PjcShim::GetButtonStyle427(!Color.IsNone()))
			.ForegroundColor(FSlateColor::UseForeground())
#endif
			.ButtonColorAndOpacity(Color.IsNone() ? FLinearColor::White : PjcWidgets::GetColor(Color))
			.ContentPadding(FMargin {10.0f, 2.0f})
			.ToolTipText(FText::FromString(ToolTip))
			.IsEnabled_Lambda([IsEnabled]() { return IsEnabled(); })
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
	};

	const auto SelectedRedirectors = [this]() {
		TArray<int32> Indices;
		for (const auto& Row : GetSelected()) {
			if (Row->Redirector != INDEX_NONE) Indices.Add(Row->Redirector);
		}
		return Indices;
	};

	// clang-format off
	return SNew(SBorder)
		.BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.DarkGroupBorder")))
		.Padding(FMargin {12.0f, 4.0f})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(SBox).MinDesiredHeight(24.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(this, &SPjcTabIssues::GetSelectionText)
					.ColorAndOpacity_Lambda([this]() { return GetSelected().Num() > 0 ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground(); })
				]
			]
			// redirectors
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(SBox).Visibility_Lambda([this]() {
					return Category == EPjcIssueCategory::Redirectors && GetSelected().Num() > 0 ? EVisibility::Visible : EVisibility::Collapsed;
				})
				[
					Button(
						TEXT("Fix"),
						TEXT("Fixes the selected redirectors"),
						TEXT("ProjectCleaner.Color.Accent"),
						[this, SelectedRedirectors]() { FixRedirectors(SelectedRedirectors()); },
						[this, SelectedRedirectors]() {
							return SelectedRedirectors().ContainsByPredicate([this](const int32 Index) { return Redirectors[Index].CanFix(); });
						}
					)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(SBox).Visibility_Lambda(Is(EPjcIssueCategory::Redirectors))
				[
					Button(
						TEXT("Fix all"),
						TEXT("Fixes every redirector that can be fixed"),
						NAME_None,
						[this]() {
							TArray<int32> All;
							for (int32 Index = 0; Index < Redirectors.Num(); ++Index) All.Add(Index);
							FixRedirectors(All);
						},
						[this]() { return Redirectors.ContainsByPredicate([](const FPjcRedirectorInfo& Info) { return Info.CanFix(); }); }
					)
				]
			]
			// files and folders
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(SBox).Visibility_Lambda([this]() {
					const bool bFiles = Category == EPjcIssueCategory::Corrupted || Category == EPjcIssueCategory::Empty;
					const bool bHere = bFiles || (Category == EPjcIssueCategory::External && !bKept);
					return bHere && GetSelected().Num() > 0 ? EVisibility::Visible : EVisibility::Collapsed;
				})
				[
					Button(
						TEXT("Delete\u2026"),
						TEXT("Deletes the selection from disk"),
						TEXT("ProjectCleaner.Color.UnusedFill"),
						[this]() { DeleteRows(GetSelected()); },
						[this]() { return GetSelected().Num() > 0; }
					)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(SBox).Visibility_Lambda([this]() {
					return Category == EPjcIssueCategory::External && !bKept && GetSelected().Num() > 0 ? EVisibility::Visible : EVisibility::Collapsed;
				})
				[
					SNew(SComboButton)
#if ENGINE_MAJOR_VERSION < 5
					.ButtonStyle(&PjcShim::GetButtonStyle427(false))
					.ForegroundColor(FSlateColor::UseForeground())
#endif
					.ToolTipText(FText::FromString(TEXT("Keeps them: by file, extension or folder, saved to Config/DefaultEditor.ini")))
					.IsEnabled_Lambda([this]() { return GetSelected().Num() > 0; })
					.OnGetMenuContent(this, &SPjcTabIssues::MakeExcludeMenu)
					.ButtonContent()
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("Exclude")))
					]
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(SBox).Visibility_Lambda(Is(EPjcIssueCategory::Empty))
				[
					Button(
						TEXT("Delete all"),
						TEXT("Deletes every empty folder"),
						NAME_None,
						[this]() { DeleteRows(AllRows); },
						[this]() { return AllRows.Num() > 0; }
					)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(SBox).Visibility_Lambda([this]() {
					const bool bFiles = Category == EPjcIssueCategory::External || Category == EPjcIssueCategory::Corrupted;
					return bFiles ? EVisibility::Visible : EVisibility::Collapsed;
				})
				[
					Button(
						TEXT("Show in Explorer"),
						TEXT("Opens the folder of the selected file"),
						NAME_None,
						[this]() {
							const TArray<FRowPtr> Selected = GetSelected();
							if (Selected.Num() > 0) UPjcSubsystem::OpenPathInFileExplorer(FPaths::GetPath(Selected[0]->Id));
						},
						[this]() { return GetSelected().Num() > 0; }
					)
				]
			]
		];
	// clang-format on
}

FText SPjcTabIssues::GetTitle() const {
	switch (Category) {
		case EPjcIssueCategory::Redirectors : return FText::FromString(TEXT("Redirectors"));
		case EPjcIssueCategory::External :	  return FText::FromString(TEXT("External files"));
		case EPjcIssueCategory::Corrupted :	  return FText::FromString(TEXT("Corrupted files"));
		case EPjcIssueCategory::Empty :		  return FText::FromString(TEXT("Empty folders"));
	}
	return FText::GetEmpty();
}

FText SPjcTabIssues::GetDescription() const {
	switch (Category) {
		case EPjcIssueCategory::Redirectors :
			return FText::FromString(TEXT("Found in your project. Scanning follows them; fixing resaves the assets that still use the old path."));
		case EPjcIssueCategory::External :
			{
				const TCHAR* Kept = TEXT("Files the plugin never deletes: engine folders, staged folders, names in source or config, exclusions.");
				const TCHAR* ToReview = TEXT("Not assets and not used by the engine or the project. Review before deleting.");
				return FText::FromString(bKept ? Kept : ToReview);
			}
		case EPjcIssueCategory::Corrupted :
			return FText::FromString(TEXT("Asset files the engine cannot load. Deleting them is the only fix the plugin offers."));
		case EPjcIssueCategory::Empty : return FText::FromString(TEXT("Folders without assets or files, often left after moving assets."));
	}
	return FText::GetEmpty();
}

TArray<SPjcTabIssues::FRowPtr> SPjcTabIssues::GetSelected() const {
	TArray<FRowPtr> Selected;
	if (!ListView.IsValid()) return Selected;

	for (const auto& Row : Rows) {
		if (ListView->IsItemSelected(Row)) {
			Selected.Add(Row);
		}
	}
	return Selected;
}

FText SPjcTabIssues::GetSelectionText() const {
	if (AllRows.Num() == 0) return FText::GetEmpty();

	const TArray<FRowPtr> Selected = GetSelected();
	if (Selected.Num() == 0) {
		// what selecting is for
		if (Category == EPjcIssueCategory::Redirectors) return FText::FromString(TEXT("Select redirectors to fix"));
		if (Category == EPjcIssueCategory::Empty) return FText::FromString(TEXT("Select folders to delete"));
		if (Category == EPjcIssueCategory::External && bKept) return FText::FromString(TEXT("Never deleted"));

		return FText::FromString(
			Category == EPjcIssueCategory::External ? TEXT("Select files to delete or exclude") : TEXT("Select files to delete")
		);
	}

	int64 Size = 0;
	for (const auto& Row : Selected) {
		Size += FMath::Max<int64>(Row->Size, 0);
	}
	const FString SelectedText = FString::Printf(TEXT("%s selected"), *PjcFormat::Count(Selected.Num()));
	return FText::FromString(Size > 0 ? SelectedText + TEXT(" \u00B7 ") + PjcFormat::Size(Size) : SelectedText);
}

void SPjcTabIssues::FixRedirectors(const TArray<int32>& Indices) {
	TArray<int32> Can;
	TArray<int32> Cannot;
	for (const int32 Index : Indices) {
		(Redirectors[Index].CanFix() ? Can : Cannot).Add(Index);
	}
	if (Can.Num() == 0 && Cannot.Num() == 0) return;

	TSet<FName> Users;
	for (const int32 Index : Can) {
		Users.Append(Redirectors[Index].UsedBy);
	}

	TArray<FPjcDialogPart> Parts;
	FPjcDialogPart& What = Parts.AddDefaulted_GetRef();
	const TCHAR* Plural = Users.Num() == 1 ? TEXT("") : TEXT("s");
	const FString Resaves = FString::Printf(TEXT("Resaves %s asset%s so they point to the real asset"), *PjcFormat::Count(Users.Num()), Plural);
	const FString ResaveNote = Resaves + TEXT(", then deletes the redirectors.");
	What.Note = Users.Num() > 0 ? ResaveNote : FString {TEXT("Nothing points through them: they are deleted.")};
	FPjcDialogPart& Team = Parts.AddDefaulted_GetRef();
	Team.Note = TEXT("Team project: teammates' unsubmitted work may still use the old paths. Fix after everyone has checked in.");
	Team.NoteColor = TEXT("ProjectCleaner.Color.Yellow");
	if (Can.Num() > 0) {
		FPjcDialogPart& List = Parts.AddDefaulted_GetRef();
		List.Heading = FString::Printf(TEXT("Redirectors \u00B7 %s"), *PjcFormat::Count(Can.Num()));
		for (const int32 Index : Can) {
			List.Rows.Emplace(FPackageName::GetShortName(Redirectors[Index].Asset.PackageName), PjcIssues::GetStateName(Redirectors[Index].State));
		}
	}
	if (Users.Num() > 0) {
		FPjcDialogPart& List = Parts.AddDefaulted_GetRef();
		List.Heading = FString::Printf(TEXT("Resaved \u00B7 %s"), *PjcFormat::Count(Users.Num()));
		for (const FName User : Users) {
			List.Rows.Emplace(FPackageName::GetShortName(User), FPaths::GetPath(User.ToString()).Mid(1));
		}
	}
	if (Cannot.Num() > 0) {
		FPjcDialogPart& List = Parts.AddDefaulted_GetRef();
		List.Heading = FString::Printf(TEXT("Not fixable \u00B7 %s"), *PjcFormat::Count(Cannot.Num()));
		List.Note = TEXT("Target missing and still used: repair the assets that use them by hand.");
		List.NoteColor = TEXT("ProjectCleaner.Color.Red");
		for (const int32 Index : Cannot) {
			const FString UseText = FString::Printf(TEXT("%s use it"), *PjcFormat::Count(Redirectors[Index].UsedBy.Num()));
			List.Rows.Emplace(FPackageName::GetShortName(Redirectors[Index].Asset.PackageName), UseText);
		}
	}
	Parts.Add(SPjcDialog::MakeVersionControlNote());

	const FString Title = FString::Printf(TEXT("Fix %s redirector%s"), *PjcFormat::Count(Can.Num()), Can.Num() == 1 ? TEXT("") : TEXT("s"));
	if (!SPjcDialog::Show(Title, Parts, Can.Num() > 0 ? TEXT("Fix") : FString {}) || Can.Num() == 0) return;

	TArray<FAssetData> Assets;
	for (const int32 Index : Can) {
		Assets.Add(Redirectors[Index].Asset);
	}
	const FPjcRedirectorFixResult Result = UPjcSubsystem::FixRedirectorsWithReport(Assets, true);
	FixKept = Result.Kept;
	SPjcScanBar::ScanProject();

	if (Result.Kept.Num() == 0 && Result.NotResaved.Num() == 0) {
		const FString ResavedText = PjcFormat::Count(Result.NumResaved);
		const FString DeletedText = PjcFormat::Count(Result.Deleted.Num());
		const FString FixedText = FString::Printf(TEXT("Fixed %s redirectors \u00B7 resaved %s assets"), *DeletedText, *ResavedText);
		UPjcSubsystem::ShowNotification(FixedText, SNotificationItem::CS_Success, 4.0f);
		return;
	}

	// what stays, and why: never a bare "failed"
	TArray<FPjcDialogPart> Report;
	FPjcDialogPart& Note = Report.AddDefaulted_GetRef();
	const TCHAR* KeptPlural = Result.Kept.Num() == 1 ? TEXT("") : TEXT("s");
	const FString Stay = FString::Printf(TEXT("%s redirector%s stay."), *PjcFormat::Count(Result.Kept.Num()), KeptPlural);
	Note.Note = Stay + TEXT(" The project still works: a redirector keeps the old path pointing to the real asset.");
	Note.NoteColor = TEXT("ProjectCleaner.Color.Red");
	if (Result.NotResaved.Num() > 0) {
		FPjcDialogPart& List = Report.AddDefaulted_GetRef();
		List.Heading = FString::Printf(TEXT("Not resaved \u00B7 %s"), *PjcFormat::Count(Result.NotResaved.Num()));
		for (const auto& Pair : Result.NotResaved) {
			List.Rows.Emplace(FPackageName::GetShortName(Pair.Key), Pair.Value);
		}
	}
	FPjcDialogPart& Kept = Report.AddDefaulted_GetRef();
	Kept.Heading = FString::Printf(TEXT("Redirectors kept \u00B7 %s"), *PjcFormat::Count(Result.Kept.Num()));
	for (const auto& Pair : Result.Kept) {
		Kept.Rows.Emplace(FPackageName::GetShortName(Pair.Key), Pair.Value);
	}
	const FString ReportTitle = FString::Printf(TEXT("Fixed %s of %s"), *PjcFormat::Count(Result.Deleted.Num()), *PjcFormat::Count(Can.Num()));
	SPjcDialog::Show(ReportTitle, Report, FString {});
}

void SPjcTabIssues::RebuildChips() {
	if (!Chips.IsValid()) return;

	Chips->ClearChildren();
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	TMap<FString, int32> ByReason;
	for (const auto& Reason : Result.FilesExternalKeptReasons) {
		ByReason.FindOrAdd(Reason)++;
	}
	ByReason.ValueSort([](const int32 A, const int32 B) {
		return A > B;
	});

	const auto Chip = [this](const FString& Reason, const FString& Label, const int32 Num) {
		// clang-format off
		return SNew(SCheckBox)
			.Style(&PjcShim::GetToggleStyle())
			.IsChecked_Lambda([this, Reason]() { return KeptReason == Reason ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this, Reason](ECheckBoxState) {
				KeptReason = Reason;
				RebuildRows();
				RebuildList();
			})
			[
				SNew(SBox).Padding(FMargin {8.0f, 1.0f})
				[
					SNew(STextBlock).Text(FText::FromString(Label + TEXT("  ") + PjcFormat::Count(Num)))
				]
			];
		// clang-format on
	};
	// clang-format off
	Chips->AddSlot().AutoWidth().Padding(FMargin {0.0f, 0.0f, 4.0f, 0.0f})
	[
		Chip(FString {}, TEXT("All"), Result.FilesExternalKept.Num())
	];
	// clang-format on
	for (const auto& Pair : ByReason) {
		// clang-format off
		Chips->AddSlot().AutoWidth().Padding(FMargin {0.0f, 0.0f, 4.0f, 0.0f})
		[
			Chip(Pair.Key, Pair.Key, Pair.Value)
		];
		// clang-format on
	}
}

void SPjcTabIssues::DeleteRows(const TArray<FRowPtr>& InRows) {
	if (InRows.Num() == 0) return;

	const bool bFolders = Category == EPjcIssueCategory::Empty;
	TArray<FString> Paths;
	int64 Size = 0;
	FPjcDialogPart List;
	for (const auto& Row : InRows) {
		Paths.Add(Row->Id);
		Size += FMath::Max<int64>(Row->Size, 0);
		List.Rows.Emplace(Row->Name, Row->Size >= 0 ? PjcFormat::Size(Row->Size) : FString {});
	}
	const FString PathsText = PjcFormat::Count(Paths.Num());
	const FString FilesText = FString::Printf(TEXT("Files \u00B7 %s \u00B7 %s"), *PathsText, *PjcFormat::Size(Size));
	List.Heading = bFolders ? FString::Printf(TEXT("Folders \u00B7 %s"), *PathsText) : FilesText;

	const TCHAR* Noun = bFolders ? TEXT("folder") : TEXT("file");
	const FString What = FString::Printf(TEXT("%s %s%s"), *PjcFormat::Count(Paths.Num()), Noun, Paths.Num() == 1 ? TEXT("") : TEXT("s"));
	const TArray<FPjcDialogPart> DeleteParts {List, SPjcDialog::MakeVersionControlNote()};
	if (!SPjcDialog::Show(TEXT("Delete ") + What, DeleteParts, TEXT("Delete"), TEXT("ProjectCleaner.Color.UnusedFill"))) {
		return;
	}

	const int32 NumDeleted = bFolders ? UPjcSubsystem::DeleteFolders(Paths) : UPjcSubsystem::DeleteFiles(Paths);
	if (NumDeleted == Paths.Num()) {
		UPjcSubsystem::ShowNotification(TEXT("Deleted ") + What, SNotificationItem::CS_Success, 4.0f);
	}
	else {
		const FString Partly = FString::Printf(TEXT("Deleted %s of %s, see Output Log for the others"), *PjcFormat::Count(NumDeleted), *What);
		UPjcSubsystem::ShowNotificationWithOutputLog(Partly, SNotificationItem::CS_Fail, 6.0f);
	}
	SPjcScanBar::ScanProject();
}

TSharedRef<SWidget> SPjcTabIssues::MakeExcludeMenu() {
	TArray<FString> Files;
	TSet<FString> Extensions;
	TSet<FString> Folders;
	for (const auto& Row : GetSelected()) {
		Files.Add(Row->Id);
		Extensions.Add(FPaths::GetExtension(Row->Id).ToLower());
		Folders.Add(UPjcSubsystem::PathConvertToRelative(FPaths::GetPath(Row->Id)));
	}
	TArray<FString> ExtensionList = Extensions.Array();
	TArray<FString> FolderList = Folders.Array();
	FolderList.RemoveAll([](const FString& Folder) {
		return !Folder.StartsWith(TEXT("/"));
	});
	TArray<FString> Dotted;
	for (const auto& Extension : ExtensionList) {
		Dotted.Add(TEXT(".") + Extension);
	}

	FMenuBuilder Menu {true, nullptr};
	const FString FilesLabel = FString::Printf(TEXT("Exclude files (%s)"), *PjcFormat::Count(Files.Num()));
	const FString FilesWhat = FString::Printf(TEXT("%s file%s"), *PjcFormat::Count(Files.Num()), Files.Num() == 1 ? TEXT("") : TEXT("s"));
	PjcWidgets::AddMenuEntry(Menu, EPjcMenuIcon::Exclude, FilesLabel, TEXT("Keeps the selected files"), [this, Files, FilesWhat]() {
		ExcludeFiles(Files, {}, {}, FilesWhat);
	});
	const FString DottedText = FString::Join(Dotted, TEXT(" "));
	const FString ExtensionLabel = TEXT("Exclude extension ") + DottedText;
	PjcWidgets::AddMenuEntry(
		Menu, EPjcMenuIcon::Exclude, ExtensionLabel, TEXT("Keeps every file with the extension"), [this, ExtensionList, DottedText]() {
			ExcludeFiles({}, ExtensionList, {}, TEXT("extension ") + DottedText);
		}
	);
	const FString FoldersText = FString::Printf(TEXT("%s folders"), *PjcFormat::Count(FolderList.Num()));
	const FString FirstFolder = FolderList.Num() == 1 ? FPaths::GetCleanFilename(FolderList[0]) : FString {};
	const FString ExcludeLabel = FolderList.Num() == 1 ? TEXT("Exclude folder ") + FirstFolder : TEXT("Exclude ") + FoldersText;
	const FString FolderWhat = FolderList.Num() == 1 ? FolderList[0] : FoldersText;
	const TCHAR* FolderTip = TEXT("Keeps every file in the folder of the selection");
	PjcWidgets::AddMenuEntry(Menu, EPjcMenuIcon::Exclude, ExcludeLabel, FolderTip, FolderList.Num() > 0, [this, FolderList, FolderWhat]() {
		ExcludeFiles({}, {}, FolderList, FolderWhat);
	});
	return Menu.MakeWidget();
}

void SPjcTabIssues::ExcludeFiles(
	const TArray<FString>& Files,
	const TArray<FString>& Extensions,
	const TArray<FString>& Folders,
	const FString& What
) {
	UPjcFileExcludeSettings* Settings = GetMutableDefault<UPjcFileExcludeSettings>();
	const FString ProjectDir = UPjcSubsystem::PathNormalize(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir())) + TEXT("/");
	for (const auto& File : Files) {
		// relative to the project, like the settings picker
		FString Relative = UPjcSubsystem::PathNormalize(File);
		Relative.RemoveFromStart(ProjectDir);
		if (!Settings->ExcludedFiles.ContainsByPredicate([&Relative](const FFilePath& Path) {
				return Path.FilePath == Relative;
			})) {
			FFilePath Path;
			Path.FilePath = Relative;
			Settings->ExcludedFiles.Add(Path);
		}
	}
	for (const auto& Extension : Extensions) {
		Settings->ExcludedExtensions.AddUnique(Extension);
	}
	for (const auto& Folder : Folders) {
		if (!Settings->ExcludedFolders.ContainsByPredicate([&Folder](const FDirectoryPath& Path) {
				return Path.Path == Folder;
			})) {
			Settings->ExcludedFolders.Add(FDirectoryPath {Folder});
		}
	}
	PjcShim::UpdateDefaultConfigFile(Settings);

	const FString Note = TEXT("Excluded ") + What + TEXT(" \u00B7 saved to Config/DefaultEditor.ini");
	UPjcSubsystem::ShowNotification(Note, SNotificationItem::CS_Success, 4.0f);
	SPjcScanBar::ScanProject();
}
