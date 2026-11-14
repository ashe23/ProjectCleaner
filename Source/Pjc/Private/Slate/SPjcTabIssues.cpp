// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabIssues.h"
#include "FileHelpers.h"
#include "Slate/PjcActions.h"
#include "Slate/PjcTabs.h"
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
#include "PjcUnused.h"
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
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SWrapBox.h"
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

namespace
{
	// Why an asset is to review: the reason of the asset its chain starts at. Key groups the chips
	void PjcIssueReviewReason(
		const FPjcScanResult& Result,
		const FName Package,
		FString& OutKey,
		FString& OutReason,
		FString& OutDetail,
		FString& OutTip
	) {
		FName Root = Package;
		const FPjcAssetUsage* RootUsage = Result.Usage.Find(Package);
		for (int32 NumSteps = 0; RootUsage && RootUsage->Reason == EPjcUsageReason::Dependency && NumSteps < 1024; ++NumSteps) {
			Root = RootUsage->Parent;
			RootUsage = Result.Usage.Find(Root);
		}
		const bool bCue = RootUsage && RootUsage->Reason == EPjcUsageReason::GameplayCueNotFired;
		const bool bOutside = bCue && RootUsage->Detail.Contains(TEXT("outside"));
		OutKey = bCue ? TEXT("Cue never fired") : TEXT("Packaged by last cook");
		if (Root != Package) {
			const FString RootName = FPackageName::GetShortName(Root);
			OutReason = OutKey;
			OutDetail = TEXT("Only used by ") + RootName;
			OutTip = RootName + TEXT(" uses it and is to review, so it stays with it. Clean project deletes neither.");
			return;
		}
		// clang-format off
		const TCHAR* CookTip = TEXT("Nothing the scan sees uses it, but the last cook packaged it: ")
			TEXT("the game may load it in a way the scan cannot follow.");
		// clang-format on
		const TCHAR* CueTip = TEXT("GameplayCue notify whose tag no used asset, C++ or config file names. Tags can also be built at runtime.");
		const TCHAR* OutsideTip = TEXT("GameplayCue notify outside the cue notify paths: the cue manager never loads it by its tag.");
		FString Detail = RootUsage ? RootUsage->Detail : FString {};
		int32 Comma = INDEX_NONE;
		if (bCue && Detail.FindChar(TEXT(','), Comma)) {
			Detail.LeftInline(Comma);
		}
		OutReason = bOutside ? TEXT("Outside cue paths") : OutKey;
		OutDetail = bCue ? Detail : TEXT("Cooked for ") + Detail;
		OutTip = bOutside ? OutsideTip : (bCue ? CueTip : CookTip);
	}
}	// namespace

void SPjcTabIssues::Construct(const FArguments& InArgs) {
	TabManager = InArgs._TabManager;
	ScanFinishedHandle = UPjcSubsystem::OnScanFinished().AddSP(this, &SPjcTabIssues::OnScanFinished);
	SAssignNew(FilterBox, SVerticalBox);

	// clang-format off
	ChildSlot
	[
		SNew(SPjcToolFrame).PageName(TEXT("Issues")).Settings(false)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(220.0f)
				[
					SNew(SBorder).BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.GroupBorder"))).Padding(FMargin {0.0f, 8.0f})
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							MakeCategories()
						]
						+ SScrollBox::Slot().Padding(FMargin {0.0f, 6.0f, 0.0f, 0.0f})
						[
							SNew(SVerticalBox)
							.Visibility_Lambda([this]() { return bHasFilters ? EVisibility::Visible : EVisibility::Collapsed; })
							+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {6.0f, 0.0f, 6.0f, 8.0f})
							[
								SNew(SBox).HeightOverride(1.0f)
								[
									SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.Line"))
								]
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								FilterBox.ToSharedRef()
							]
						]
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
					+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {12.0f, 4.0f, 12.0f, 6.0f})
					[
						MakeToolbar()
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
	// assets to review come first when there are some
	if (NumReview > 0) {
		ShowCategory(EPjcIssueCategory::Review);
	}
}

SPjcTabIssues::~SPjcTabIssues() {
	UPjcSubsystem::OnScanFinished().Remove(ScanFinishedHandle);
}

void SPjcTabIssues::ShowCategory(const EPjcIssueCategory InCategory, const bool bShowKept, const FString& InReason) {
	const bool bSameFilter = InReason.IsEmpty() ? Filters.Num() == 0 : Filters.Num() == 1 && Filters.Contains(InReason);
	if (Category == InCategory && bKept == bShowKept && bSameFilter) return;

	if (Category != InCategory || bKept != bShowKept) {
		SortColumn = INDEX_NONE;
	}
	Category = InCategory;
	bKept = bShowKept && InCategory == EPjcIssueCategory::External;
	Filters.Reset();
	if (InCategory == EPjcIssueCategory::Review && !InReason.IsEmpty()) {
		Filters.Add(InReason);
	}
	RebuildRows();
	RebuildList();
}

void SPjcTabIssues::OnScanFinished() {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	PjcIssues::GetRedirectors(Result, Redirectors);
	NumReview = 0;
	for (const auto& Asset : Result.Assets) {
		NumReview += FPjcUnusedTree::IsInSet(Result, Asset.PackageName, EPjcAssetSet::Review) ? 1 : 0;
	}
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
		case EPjcIssueCategory::Review :	  return NumReview;
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
		case EPjcIssueCategory::Review :
			ReviewReasons.Reset();
			for (int32 Index = 0; Index < Result.Assets.Num(); ++Index) {
				const FAssetData& Asset = Result.Assets[Index];
				if (!FPjcUnusedTree::IsInSet(Result, Asset.PackageName, EPjcAssetSet::Review)) continue;

				FString Key;
				FString Reason;
				FString Detail;
				FString Tip;
				PjcIssueReviewReason(Result, Asset.PackageName, Key, Reason, Detail, Tip);
				ReviewReasons.FindOrAdd(Key)++;
				if (Filters.Num() > 0 && !Filters.Contains(Key)) continue;

				const FRowPtr Row = MakeShared<FPjcIssueRow>();
				Row->Id = Asset.PackageName.ToString();
				Row->Package = Asset.PackageName;
				Row->Asset = Index;
				Row->Name = Asset.AssetName.ToString();
				Row->Sub = PjcWidgets::GetAssetType(Asset).Label + TEXT(" \u00B7 ") + Asset.PackagePath.ToString().Mid(1);
				Row->Reason = Key;
				Row->Size = Result.GetSize(Asset.PackageName);
				Row->Cells = {Reason, Detail, PjcFormat::Size(Row->Size)};
				Row->CellColors = {TEXT("ProjectCleaner.Color.Review"), TEXT("ProjectCleaner.Color.Gray"), NAME_None};
				Row->ToolTip = Row->Id + TEXT("\n") + Tip;
				AllRows.Add(Row);
			}
			AllRows.Sort([](const FRowPtr& A, const FRowPtr& B) {
				return A->Size != B->Size ? A->Size > B->Size : A->Name < B->Name;
			});
			break;

		case EPjcIssueCategory::Redirectors :
			for (int32 Index = 0; Index < Redirectors.Num(); ++Index) {
				const FPjcRedirectorInfo& Info = Redirectors[Index];
				const FString StateName = PjcIssues::GetStateName(Info.State);
				if (Filters.Num() > 0 && !Filters.Contains(StateName)) continue;
				const FString Path = Info.Asset.PackageName.ToString();
				const FString Final = Info.FinalTarget.ToString();
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
				Row->Cells.Add(Info.FinalTarget.IsNone() ? TEXT("outside scan") : TEXT("\u2192 ") + FPackageName::GetShortName(Info.FinalTarget));
				const TCHAR* Missing = Info.bTargetExists ? TEXT("") : TEXT(" \u00B7 missing");
				const FString Through
					= Info.Via.Num() > 0 ? FString::Printf(TEXT(" \u00B7 through %s"), *PjcFormat::Count(Info.Via.Num())) : FString {};
				Row->CellSubs.Add(Info.FinalTarget.IsNone() ? FString {} : FPaths::GetPath(Final).Mid(1) + Missing + Through);
				Row->CellColors.Add(
					Info.FinalTarget.IsNone() ? FName {} : (Info.bTargetExists ? FName {} : FName {TEXT("ProjectCleaner.Color.Red")})
				);
				Row->Cells.Add(Info.UsedBy.Num() > 0 ? PjcFormat::Count(Info.UsedBy.Num()) : TEXT("\u2014"));
				Row->CellSubs.Add(FString {});
				Row->CellColors.Add(NAME_None);
				Row->Cells.Add(PjcIssues::GetStateName(Info.State));
				Row->CellSubs.Add(FString {});
				FName StateColor {TEXT("ProjectCleaner.Color.Gray")};
				if (Info.State == EPjcRedirectorState::Broken) StateColor = TEXT("ProjectCleaner.Color.Red");
				if (Info.State == EPjcRedirectorState::InUse) StateColor = TEXT("ProjectCleaner.Color.Yellow");
				if (Info.State == EPjcRedirectorState::NamedInFiles) StateColor = TEXT("ProjectCleaner.Color.Reason.Source");
				if (Info.State == EPjcRedirectorState::UsedOutside) StateColor = TEXT("ProjectCleaner.Color.Reason.Outside");
				Row->CellColors.Add(StateColor);
				TArray<FString> Users;
				for (const FName User : Info.UsedBy) {
					Users.Add(FPackageName::GetShortName(User));
				}
				const FString UsedByText = Users.Num() > 0 ? TEXT("\nUsed by: ") + FString::Join(Users, TEXT(", ")) : FString {};
				const FString KeptText = Info.KeptBy.IsEmpty() ? FString {} : TEXT("\n") + Info.KeptBy;
				TArray<FString> ViaNames;
				for (const FName Via : Info.Via) {
					ViaNames.Add(FPackageName::GetShortName(Via));
				}
				const FString ViaText = ViaNames.Num() > 0 ? TEXT("\nThrough ") + FString::Join(ViaNames, TEXT(" \u2192 ")) : FString {};
				Row->ToolTip
					= FString::Printf(TEXT("%s\n%s%s%s%s"), *Path, *PjcIssues::GetStateDescription(Info.State), *KeptText, *ViaText, *UsedByText);
				AllRows.Add(Row);
			}
			break;

		case EPjcIssueCategory::External :
			if (bKept) {
				for (int32 Index = 0; Index < Result.FilesExternalKept.Num(); ++Index) {
					const FString& File = Result.FilesExternalKept[Index];
					const FString Reason = Result.FilesExternalKeptReasons.IsValidIndex(Index) ? Result.FilesExternalKeptReasons[Index] : FString {};
					if (Filters.Num() > 0 && !Filters.Contains(Reason)) continue;

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
	RebuildFilters();
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
		case EPjcIssueCategory::Review :
			Add(TEXT("Asset"), 0.0f, 1.0f, false, false);
			Add(TEXT("Reason"), 170.0f, 0.0f, false, false, TEXT("Why the scan is not sure"));
			Add(TEXT("Detail"), 0.0f, 0.6f, false, false);
			Add(TEXT("Size"), 90.0f, 0.0f, true, true);
			break;
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
		const int32 Index = Ids.Num();
		Arguments.ColumnId(Column.Id).DefaultLabel(FText::FromString(Column.Label)).DefaultTooltip(FText::FromString(Column.ToolTip));
		Arguments.SortMode_Lambda([this, Index]() {
			return SortColumn == Index ? SortMode : EColumnSortMode::None;
		});
		Arguments.OnSort(this, &SPjcTabIssues::OnSort);
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
	.OnContextMenuOpening(this, &SPjcTabIssues::OnContextMenu)
	.OnKeyDownHandler(this, &SPjcTabIssues::OnListKeyDown)
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
	// sorted by the picked column: names and texts a to z, sizes and counts by value
	const TArray<FColumn> Columns = GetColumns();
	if (Columns.IsValidIndex(SortColumn)) {
		const int32 Cell = SortColumn - 1;
		const FString& Label = Columns[SortColumn].Label;
		const bool bSize = Label == TEXT("Size");
		const bool bCount = Label == TEXT("Used by");
		const bool bAscending = SortMode == EColumnSortMode::Ascending;
		Rows.Sort([Cell, bSize, bCount, bAscending](const FRowPtr& A, const FRowPtr& B) {
			int32 Compare = 0;
			if (Cell < 0) {
				Compare = A->Name.Compare(B->Name, ESearchCase::IgnoreCase);
			}
			else if (bSize) {
				Compare = A->Size < B->Size ? -1 : (A->Size > B->Size ? 1 : 0);
			}
			else {
				const FString X = A->Cells.IsValidIndex(Cell) ? A->Cells[Cell] : FString {};
				const FString Y = B->Cells.IsValidIndex(Cell) ? B->Cells[Cell] : FString {};
				Compare = bCount ? FCString::Atoi(*X) - FCString::Atoi(*Y) : X.Compare(Y, ESearchCase::IgnoreCase);
			}
			if (Compare == 0) return A->Name < B->Name;
			return bAscending ? Compare < 0 : Compare > 0;
		});
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
		FName Icon;
		FName IconColor;
	};
	const FName White {TEXT("ProjectCleaner.Color.Strong")};
	const FEntry Entries[] = {
	  {		EPjcIssueCategory::Review,
	   TEXT("Assets to review"),
	   TEXT("Assets the scan is not sure about: Clean project keeps them until you delete or keep them"),
	   TEXT("ProjectCleaner.Icon.Warning"),
	   White							 },
	  {EPjcIssueCategory::Redirectors,
	   TEXT("Redirectors"),
	   TEXT("Left by moved or renamed assets"),
	   TEXT("ProjectCleaner.Icon.Forward"),
	   TEXT("ProjectCleaner.Color.Muted")},
	  {	  EPjcIssueCategory::External,
	   TEXT("External files"),
	   TEXT("Files in content folders that are not assets"),
	   TEXT("ProjectCleaner.Menu.Explorer"),
	   White							 },
	  {	 EPjcIssueCategory::Corrupted,
	   TEXT("Corrupted files"),
	   TEXT("Asset files the engine cannot read"),
	   TEXT("ProjectCleaner.Icon.Close"),
	   TEXT("ProjectCleaner.Color.Red")	 },
	  {		 EPjcIssueCategory::Empty,
	   TEXT("Empty folders"),
	   TEXT("Folders without assets or files"),
	   TEXT("ProjectCleaner.Icon.Folder"),
	   TEXT("ProjectCleaner.Color.Muted")},
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
					return Category == EntryCategory ? PjcWidgets::GetColor("ProjectCleaner.Color.Marked") : FLinearColor::Transparent;
				})
				.Padding(FMargin {12.0f, 6.0f})
				[
					SNew(SHorizontalBox)
					// what kind of thing is listed
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 8.0f, 0.0f})
					[
						SNew(SBox).WidthOverride(14.0f).HeightOverride(14.0f)
						[
							SNew(SImage).Image(FPjcStyles::Get().GetBrush(Entry.Icon)).ColorAndOpacity(PjcWidgets::GetColor(Entry.IconColor))
						]
					]
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
	return SNew(SBox).Padding(FMargin {12.0f, 8.0f, 12.0f, 4.0f})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(this, &SPjcTabIssues::GetTitle).Font(FPjcStyles::GetFont("Bold", 10))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(this, &SPjcTabIssues::GetDescription)
					.ToolTipText(this, &SPjcTabIssues::GetDescription)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					.Clipping(EWidgetClipping::ClipToBounds)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {12.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock).Text(this, &SPjcTabIssues::GetCountText).ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		];
	// clang-format on
}

FText SPjcTabIssues::GetCountText() const {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	const int32 Num = Category == EPjcIssueCategory::External && bKept ? Result.FilesExternalKept.Num() : GetCount(Category);
	if (Num == 0) return FText::GetEmpty();

	const TCHAR* Noun = TEXT("file");
	Noun = Category == EPjcIssueCategory::Redirectors ? TEXT("redirector") : Noun;
	Noun = Category == EPjcIssueCategory::Empty ? TEXT("folder") : Noun;
	Noun = Category == EPjcIssueCategory::Review ? TEXT("asset") : Noun;
	return FText::FromString(FString::Printf(TEXT("%s %s%s"), *PjcFormat::Count(Num), Noun, Num == 1 ? TEXT("") : TEXT("s")));
}

TSharedRef<SWidget> SPjcTabIssues::MakeToolbar() {
	const auto Is = [this](const EPjcIssueCategory InCategory) {
		return [this, InCategory]() {
			return Category == InCategory ? EVisibility::Visible : EVisibility::Collapsed;
		};
	};
	const auto HasSelection = [this]() {
		return GetSelected().Num() > 0;
	};
	const auto SelectedRedirectors = [this]() {
		TArray<int32> Indices;
		for (const auto& Row : GetSelected()) {
			if (Row->Redirector != INDEX_NONE) Indices.Add(Row->Redirector);
		}
		return Indices;
	};
	const auto Label = [](const FName Brush, const TCHAR* Text, const FSlateColor& TextColor) -> TSharedRef<SWidget> {
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
				SNew(STextBlock).Text(FText::FromString(Text)).ColorAndOpacity(TextColor)
			];
		// clang-format on
	};
	const auto Button = [&Label](
							const FName Brush,
							const TCHAR* Text,
							const TCHAR* ToolTip,
							TFunction<EVisibility()> Visible,
							TFunction<bool()> Enabled,
							TFunction<void()> OnClick
						) -> TSharedRef<SWidget> {
		// clang-format off
		return SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
			.ButtonStyle(&PjcShim::GetButtonStyle427(false))
			.ForegroundColor(FSlateColor::UseForeground())
#endif
			.ContentPadding(FMargin {8.0f, 1.0f, 10.0f, 1.0f})
			.ToolTipText(FText::FromString(ToolTip))
			.Visibility_Lambda([Visible]() { return Visible(); })
			.IsEnabled_Lambda([Enabled]() { return Enabled(); })
			.OnClicked_Lambda([OnClick]() {
				OnClick();
				return FReply::Handled();
			})
			[
				Label(Brush, Text, FSlateColor::UseForeground())
			];
		// clang-format on
	};
	const auto DeleteButton = [&Label](const TCHAR* ToolTip, TFunction<EVisibility()> Visible, TFunction<bool()> Enabled, TFunction<void()> OnClick) {
		// clang-format off
		return SNew(SButton)
			.ButtonStyle(&FPjcStyles::Get().GetWidgetStyle<FButtonStyle>("ProjectCleaner.Button.DangerQuiet"))
#if ENGINE_MAJOR_VERSION < 5
			.ForegroundColor(FSlateColor::UseForeground())
#endif
			.ContentPadding(FMargin {8.0f, 1.0f, 10.0f, 1.0f})
			.ToolTipText(FText::FromString(ToolTip))
			.Visibility_Lambda([Visible]() { return Visible(); })
			.IsEnabled_Lambda([Enabled]() { return Enabled(); })
			.OnClicked_Lambda([OnClick]() {
				OnClick();
				return FReply::Handled();
			})
			[
				Label("ProjectCleaner.Menu.Delete", TEXT("Delete\u2026"), PjcWidgets::GetColor("ProjectCleaner.Color.DangerText"))
			];
		// clang-format on
	};
	const auto DeletesHere = [this]() {
		const bool bFiles = Category == EPjcIssueCategory::Corrupted || Category == EPjcIssueCategory::Empty || Category == EPjcIssueCategory::Review;
		return bFiles || (Category == EPjcIssueCategory::External && !bKept) ? EVisibility::Visible : EVisibility::Collapsed;
	};
	const auto ExplorerHere = [this]() {
		const bool bFiles = Category == EPjcIssueCategory::External || Category == EPjcIssueCategory::Corrupted;
		return bFiles ? EVisibility::Visible : EVisibility::Collapsed;
	};

	// clang-format off
	return SNew(SBox).MinDesiredHeight(28.0f)
		[
			SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D {6.0f, 4.0f})
			+ SWrapBox::Slot().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(240.0f)
				[
					SNew(SSearchBox)
					.Style(&PjcShim::GetSearchBoxStyle())
					.HintText_Lambda([this]() { return FText::FromString(TEXT("Search ") + GetTitle().ToString().ToLower()); })
					.OnTextChanged_Lambda([this](const FText& Text) {
						Search = Text.ToString().TrimStartAndEnd();
						ApplyFilter();
					})
				]
			]
			+ SWrapBox::Slot().VAlign(VAlign_Center).Padding(FMargin {2.0f, 0.0f})
			[
				SNew(SBox).WidthOverride(1.0f).HeightOverride(22.0f)
				[
					SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.Line"))
				]
			]
			// delete: assets to review with the unused assets that reference them, files and folders from disk
			+ SWrapBox::Slot().VAlign(VAlign_Center)
			[
				DeleteButton(TEXT("Deletes the selection (Delete)"), DeletesHere, HasSelection, [this]() {
					if (Category == EPjcIssueCategory::Review) {
						DeleteAssets(GetSelected());
					}
					else {
						DeleteRows(GetSelected());
					}
				})
			]
			+ SWrapBox::Slot().VAlign(VAlign_Center)
			[
				Button("ProjectCleaner.Menu.Exclude", TEXT("Keep"), TEXT("Keeps them for good: adds them to the exclusions (Config/DefaultEditor.ini), so they count as used"),
					Is(EPjcIssueCategory::Review), HasSelection, [this]() { KeepAssets(GetSelected()); })
			]
			+ SWrapBox::Slot().VAlign(VAlign_Center)
			[
				Button("ProjectCleaner.Menu.Inspect", TEXT("Inspect"), TEXT("Shows why the scan is not sure in the Inspector"),
					Is(EPjcIssueCategory::Review), [this]() { return GetSelected().Num() == 1; }, [this]() {
						const TArray<FRowPtr> Selected = GetSelected();
						if (Selected.Num() == 1) PjcTabs::Inspect(TabManager, Selected[0]->Package);
					})
			]
			// redirectors
			+ SWrapBox::Slot().VAlign(VAlign_Center)
			[
				SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
				.ButtonStyle(&PjcShim::GetButtonStyle427(true))
				.ForegroundColor(FSlateColor::UseForeground())
#endif
				.ButtonColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.Accent"))
				.ContentPadding(FMargin {8.0f, 1.0f, 10.0f, 1.0f})
				.ToolTipText(FText::FromString(TEXT("Fixes the selected redirectors")))
				.Visibility_Lambda(Is(EPjcIssueCategory::Redirectors))
				.IsEnabled_Lambda([this, SelectedRedirectors]() {
					return SelectedRedirectors().ContainsByPredicate([this](const int32 Index) { return Redirectors[Index].CanFix(); });
				})
				.OnClicked_Lambda([this, SelectedRedirectors]() {
					FixRedirectors(SelectedRedirectors());
					return FReply::Handled();
				})
				[
					Label("ProjectCleaner.Graph.Loop", TEXT("Fix\u2026"), FSlateColor {FLinearColor::White})
				]
			]
			+ SWrapBox::Slot().VAlign(VAlign_Center)
			[
				Button("ProjectCleaner.Graph.Loop", TEXT("Fix all\u2026"), TEXT("Fixes every redirector that can be fixed"), Is(EPjcIssueCategory::Redirectors),
					[this]() { return Redirectors.ContainsByPredicate([](const FPjcRedirectorInfo& Info) { return Info.CanFix(); }); },
					[this]() {
						TArray<int32> All;
						for (int32 Index = 0; Index < Redirectors.Num(); ++Index) All.Add(Index);
						FixRedirectors(All);
					})
			]
			// external files: keep them by file, extension or folder
			+ SWrapBox::Slot().VAlign(VAlign_Center)
			[
				SNew(SComboButton)
				.ComboButtonStyle(&PjcShim::GetComboButtonStyle())
#if ENGINE_MAJOR_VERSION < 5
				.ForegroundColor(FSlateColor::UseForeground())
#endif
				.ToolTipText(FText::FromString(TEXT("Keeps them: by file, extension or folder, saved to Config/DefaultEditor.ini")))
				.Visibility_Lambda([this]() { return Category == EPjcIssueCategory::External && !bKept ? EVisibility::Visible : EVisibility::Collapsed; })
				.IsEnabled_Lambda(HasSelection)
				.OnGetMenuContent(this, &SPjcTabIssues::MakeExcludeMenu)
				.ButtonContent()
				[
					Label("ProjectCleaner.Menu.Exclude", TEXT("Exclude"), FSlateColor::UseForeground())
				]
			]
			+ SWrapBox::Slot().VAlign(VAlign_Center)
			[
				Button("ProjectCleaner.Menu.Delete", TEXT("Delete all\u2026"), TEXT("Deletes every empty folder"), Is(EPjcIssueCategory::Empty),
					[this]() { return AllRows.Num() > 0; }, [this]() { DeleteRows(AllRows); })
			]
			+ SWrapBox::Slot().VAlign(VAlign_Center)
			[
				Button("ProjectCleaner.Menu.Explorer", TEXT("Show in Explorer"), TEXT("Opens the folder of the selected file"), ExplorerHere, HasSelection, [this]() {
					const TArray<FRowPtr> Selected = GetSelected();
					if (Selected.Num() > 0) UPjcSubsystem::OpenPathInFileExplorer(FPaths::GetPath(Selected[0]->Id));
				})
			]
			// the selection, or what selecting is for
			+ SWrapBox::Slot().VAlign(VAlign_Center).Padding(FMargin {4.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock)
				.Text(this, &SPjcTabIssues::GetSelectionText)
				.ColorAndOpacity_Lambda([this]() { return GetSelected().Num() > 0 ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground(); })
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
		case EPjcIssueCategory::Review :	  return FText::FromString(TEXT("Assets to review"));
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
		case EPjcIssueCategory::Review :
			return FText::FromString(TEXT("The scan is not sure about them. Clean project keeps them: delete them, or keep them for good."));
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
		if (Category == EPjcIssueCategory::Review) return FText::FromString(TEXT("Select assets to delete, keep or inspect"));
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
	// fixable, named in files (fixed only with a redirect), kept
	TArray<int32> Can;
	TArray<int32> Named;
	TArray<int32> Cannot;
	for (const int32 Index : Indices) {
		const FPjcRedirectorInfo& Info = Redirectors[Index];
		if (!Info.CanFix()) {
			Cannot.Add(Index);
		}
		else if (Info.State == EPjcRedirectorState::NamedInFiles) {
			Named.Add(Index);
		}
		else {
			Can.Add(Index);
		}
	}
	if (Can.Num() == 0 && Named.Num() == 0 && Cannot.Num() == 0) return;

	TSet<FName> Users;
	for (const int32 Index : Can) {
		Users.Append(Redirectors[Index].UsedBy);
	}
	for (const int32 Index : Named) {
		Users.Append(Redirectors[Index].UsedBy);
	}

	static bool bAddRedirects = true;
	static bool bDeleteEmptyFolders = true;
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
	if (Named.Num() > 0) {
		FPjcDialogPart& List = Parts.AddDefaulted_GetRef();
		List.Heading = FString::Printf(TEXT("Named in files \u00B7 %s"), *PjcFormat::Count(Named.Num()));
		List.Note = TEXT(
			"Files name their old paths. With the option below they are fixed and a redirect keeps the old paths working; without it they stay."
		);
		List.NoteColor = TEXT("ProjectCleaner.Color.Reason.Source");
		for (const int32 Index : Named) {
			List.Rows.Emplace(FPackageName::GetShortName(Redirectors[Index].Asset.PackageName), Redirectors[Index].KeptBy);
		}
	}
	if (Cannot.Num() > 0) {
		FPjcDialogPart& List = Parts.AddDefaulted_GetRef();
		List.Heading = FString::Printf(TEXT("Kept \u00B7 %s"), *PjcFormat::Count(Cannot.Num()));
		List.Note = TEXT("Fixing cannot do these safely: repair broken references by hand, or scan the content that uses them.");
		List.NoteColor = TEXT("ProjectCleaner.Color.Red");
		for (const int32 Index : Cannot) {
			const FPjcRedirectorInfo& Info = Redirectors[Index];
			const FString UseText = FString::Printf(TEXT("%s use it, target missing"), *PjcFormat::Count(Info.UsedBy.Num()));
			List.Rows.Emplace(
				FPackageName::GetShortName(Info.Asset.PackageName), Info.State == EPjcRedirectorState::UsedOutside ? Info.KeptBy : UseText
			);
		}
	}
	if (Named.Num() > 0) {
		FPjcDialogPart& Option = Parts.AddDefaulted_GetRef();
		Option.CheckLabel = TEXT("Keep old paths named in files working: add redirects to Config/DefaultEngine.ini");
		Option.CheckValue = &bAddRedirects;
	}
	FPjcDialogPart& Folders = Parts.AddDefaulted_GetRef();
	Folders.CheckLabel = TEXT("Delete folders left empty");
	Folders.CheckValue = &bDeleteEmptyFolders;
	Parts.Add(SPjcDialog::MakeVersionControlNote());

	const int32 NumToFix = Can.Num() + Named.Num();
	if (!SPjcDialog::Show(TEXT("Fix redirectors"), Parts, NumToFix > 0 ? TEXT("Fix") : FString {}) || NumToFix == 0) return;

	// unsaved referencers are saved first, or nothing is fixed
	TArray<UPackage*> Dirty;
	for (const FName User : Users) {
		UPackage* Package = FindPackage(nullptr, *User.ToString());
		if (Package && Package->IsDirty()) {
			Dirty.Add(Package);
		}
	}
	if (Dirty.Num() > 0) {
		const FEditorFileUtils::EPromptReturnCode Saved = FEditorFileUtils::PromptForCheckoutAndSave(Dirty, false, true);
		if (Saved != FEditorFileUtils::PR_Success) return;
	}

	TArray<FAssetData> Assets;
	for (const int32 Index : Can) {
		Assets.Add(Redirectors[Index].Asset);
	}
	for (const int32 Index : Named) {
		Assets.Add(Redirectors[Index].Asset);
	}
	FPjcRedirectorFixOptions Options;
	Options.bAddRedirects = bAddRedirects;
	Options.bDeleteEmptyFolders = bDeleteEmptyFolders;
	const FPjcRedirectorFixResult Result = UPjcSubsystem::FixRedirectorsWithReport(Assets, true, Options);
	FixKept = Result.Kept;
	SPjcScanBar::ScanProject();

	const FString Redirected
		= Result.NumRedirects > 0 ? FString::Printf(TEXT(" \u00B7 %s redirects added"), *PjcFormat::Count(Result.NumRedirects)) : FString {};
	const FString FoldersText = Result.NumFoldersDeleted > 0
		? FString::Printf(TEXT(" \u00B7 %s empty folders deleted"), *PjcFormat::Count(Result.NumFoldersDeleted))
		: FString {};
	if (Result.Kept.Num() == 0 && Result.NotResaved.Num() == 0) {
		const FString ResavedText = PjcFormat::Count(Result.NumResaved);
		const FString DeletedText = PjcFormat::Count(Result.Deleted.Num());
		const FString FixedText
			= FString::Printf(TEXT("Fixed %s redirectors \u00B7 resaved %s assets"), *DeletedText, *ResavedText) + Redirected + FoldersText;
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
	const FString ReportTitle
		= FString::Printf(TEXT("Fixed %s of %s"), *PjcFormat::Count(Result.Deleted.Num()), *PjcFormat::Count(Assets.Num())) + Redirected;
	SPjcDialog::Show(ReportTitle, Report, FString {});
}

void SPjcTabIssues::RebuildFilters() {
	if (!FilterBox.IsValid()) return;

	FilterBox->ClearChildren();
	FilterKeys.Reset();
	bHasFilters = false;
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	const auto AddHeading = [this](const TCHAR* Text, const TCHAR* ToolTip) {
		// clang-format off
		FilterBox->AddSlot().AutoHeight().Padding(FMargin {10.0f, 2.0f, 10.0f, 5.0f})
		[
			PjcWidgets::MakeFilterHeading(FText::FromString(Text), FText::FromString(ToolTip))
		];
		// clang-format on
	};
	// a filter: All (empty key) or one value; picked ones in their color, the others grey while one is picked
	const auto AddPill = [this](const FString& Key, const FString& Label, const FString& ToolTip, const FLinearColor& Color, const int32 Num) {
		if (!Key.IsEmpty()) {
			FilterKeys.Add(Key);
		}
		const auto IsOn = [this, Key]() {
			return Key.IsEmpty() ? Filters.Num() == 0 : Filters.Contains(Key);
		};
		const auto IsGrey = [this, Key, IsOn]() {
			return !Key.IsEmpty() && !IsOn() && Filters.Num() > 0;
		};
		const FString More = Key.IsEmpty() ? TEXT("Click: every item of the list") : TEXT("Click: show them, click more to add them");
		// clang-format off
		FilterBox->AddSlot().AutoHeight().Padding(FMargin {6.0f, 0.0f, 6.0f, 4.0f})
		[
			PjcWidgets::MakeFilterPill(Label, ToolTip, More, Color, Num, !Key.IsEmpty(), IsOn, IsGrey, [this, Key]() { OnFilterClicked(Key); })
		];
		// clang-format on
	};
	const FLinearColor Accent = PjcWidgets::GetColor("ProjectCleaner.Color.Accent");

	if (Category == EPjcIssueCategory::Review && NumReview > 0) {
		AddHeading(TEXT("REASON FILTERS"), TEXT("Why the scan is not sure about them"));
		AddPill(FString {}, TEXT("All"), TEXT("Every asset to review"), Accent, NumReview);
		TArray<FString> Keys;
		ReviewReasons.GenerateKeyArray(Keys);
		Keys.Sort();
		for (const auto& Key : Keys) {
			const bool bCue = Key.Contains(TEXT("Cue"));
			const FLinearColor Color = PjcWidgets::GetColor(bCue ? "ProjectCleaner.Color.Reason.GameplayCue" : "ProjectCleaner.Color.Review");
			AddPill(Key, Key, FString {}, Color, ReviewReasons[Key]);
		}
		bHasFilters = true;
	}
	else if (Category == EPjcIssueCategory::Redirectors && Redirectors.Num() > 0) {
		AddHeading(TEXT("STATUS FILTERS"), TEXT("What each redirector needs"));
		AddPill(FString {}, TEXT("All"), TEXT("Every redirector"), Accent, Redirectors.Num());
		const TPair<EPjcRedirectorState, FName> States[] = {
		  TPair<EPjcRedirectorState, FName> {		 EPjcRedirectorState::InUse,		 FName {TEXT("ProjectCleaner.Color.Yellow")}},
		  TPair<EPjcRedirectorState, FName> { EPjcRedirectorState::UsedByUnused,		 FName {TEXT("ProjectCleaner.Color.Editor")}},
		  TPair<EPjcRedirectorState, FName> { EPjcRedirectorState::NamedInFiles,  FName {TEXT("ProjectCleaner.Color.Reason.Source")}},
		  TPair<EPjcRedirectorState, FName> {  EPjcRedirectorState::UsedOutside, FName {TEXT("ProjectCleaner.Color.Reason.Outside")}},
		  TPair<EPjcRedirectorState, FName> {EPjcRedirectorState::NotReferenced,		  FName {TEXT("ProjectCleaner.Color.Muted")}},
		  TPair<EPjcRedirectorState, FName> { EPjcRedirectorState::TargetUnused,		 FName {TEXT("ProjectCleaner.Color.Editor")}},
		  TPair<EPjcRedirectorState, FName> {		EPjcRedirectorState::Broken,			FName {TEXT("ProjectCleaner.Color.Red")}},
		};
		for (const auto& State : States) {
			const int32 Num = Redirectors
								  .FilterByPredicate([&State](const FPjcRedirectorInfo& Info) {
									  return Info.State == State.Key;
								  })
								  .Num();
			if (Num == 0) continue;
			const FString Name = PjcIssues::GetStateName(State.Key);
			AddPill(Name, Name, PjcIssues::GetStateDescription(State.Key), PjcWidgets::GetColor(State.Value), Num);
		}
		bHasFilters = true;
	}
	else if (Category == EPjcIssueCategory::External) {
		// to review or kept: two lists with their own actions
		AddHeading(TEXT("STATUS FILTERS"), TEXT("Files to review, or the files the plugin never deletes"));
		const auto AddMode = [this](const bool bKeptMode, const TCHAR* Label, const TCHAR* ToolTip, const FName Color, const int32 Num) {
			const auto IsOn = [this, bKeptMode]() {
				return bKept == bKeptMode;
			};
			// clang-format off
			FilterBox->AddSlot().AutoHeight().Padding(FMargin {6.0f, 0.0f, 6.0f, 4.0f})
			[
				PjcWidgets::MakeFilterPill(Label, ToolTip, FString {}, PjcWidgets::GetColor(Color), Num, true, IsOn, []() { return false; },
					[this, bKeptMode]() { ShowCategory(EPjcIssueCategory::External, bKeptMode); })
			];
			// clang-format on
		};
		AddMode(
			false,
			TEXT("To review"),
			TEXT("Not assets and not used by the engine or the project. Review before deleting."),
			TEXT("ProjectCleaner.Color.Yellow"),
			Result.FilesExternal.Num()
		);
		AddMode(
			true,
			TEXT("Kept"),
			TEXT("Files the plugin never deletes: engine folders, staged folders, names in source or config, exclusions"),
			TEXT("ProjectCleaner.Color.Ok"),
			Result.FilesExternalKept.Num()
		);
		if (bKept && Result.FilesExternalKept.Num() > 0) {
			TMap<FString, int32> ByReason;
			for (const auto& Reason : Result.FilesExternalKeptReasons) {
				ByReason.FindOrAdd(Reason)++;
			}
			ByReason.ValueSort([](const int32 A, const int32 B) {
				return A > B;
			});
			// clang-format off
			FilterBox->AddSlot().AutoHeight().Padding(FMargin {10.0f, 6.0f, 10.0f, 5.0f})
			[
				PjcWidgets::MakeFilterHeading(FText::FromString(TEXT("KEPT BY")), FText::FromString(TEXT("Why the plugin never deletes them")))
			];
			// clang-format on
			AddPill(FString {}, TEXT("All"), TEXT("Every kept file"), Accent, Result.FilesExternalKept.Num());
			for (const auto& Pair : ByReason) {
				AddPill(Pair.Key, Pair.Key, FString {}, PjcWidgets::GetColor("ProjectCleaner.Color.Muted"), Pair.Value);
			}
		}
		bHasFilters = true;
	}
}

void SPjcTabIssues::OnFilterClicked(const FString& Key) {
	// All: every item; a value: added to the picked ones, or taken off; every value picked is All again
	if (Key.IsEmpty()) {
		Filters.Reset();
	}
	else {
		if (Filters.Contains(Key)) {
			Filters.Remove(Key);
		}
		else {
			Filters.Add(Key);
		}
		if (FilterKeys.Num() > 0 && FilterKeys.Difference(Filters).Num() == 0) {
			Filters.Reset();
		}
	}
	RebuildRows();
	RebuildList();
}

void SPjcTabIssues::OnSort(EColumnSortPriority::Type Priority, const FName& Column, EColumnSortMode::Type Mode) {
	const TArray<FColumn> Columns = GetColumns();
	const int32 Index = Columns.IndexOfByPredicate([&Column](const FColumn& Each) {
		return Each.Id == Column;
	});
	if (Index == INDEX_NONE) return;

	// a new column starts with the biggest sizes and counts, names a to z
	if (Index != SortColumn) {
		const bool bNumber = Columns[Index].Label == TEXT("Size") || Columns[Index].Label == TEXT("Used by");
		Mode = bNumber ? EColumnSortMode::Descending : EColumnSortMode::Ascending;
	}
	SortColumn = Index;
	SortMode = Mode;
	ApplyFilter();
}

TSharedPtr<SWidget> SPjcTabIssues::OnContextMenu() {
	const TArray<FRowPtr> Selected = GetSelected();
	if (Selected.Num() == 0) return nullptr;

	// the toolbar's actions for the selection
	FMenuBuilder Menu {true, nullptr};
	const FString DeleteLabel = FString::Printf(TEXT("Delete\u2026 (%s)"), *PjcFormat::Count(Selected.Num()));
	switch (Category) {
		case EPjcIssueCategory::Review :
			PjcWidgets::AddMenuEntry(
				Menu, EPjcMenuIcon::Delete, DeleteLabel, TEXT("Deletes them with the unused assets that reference them"), [this]() {
					DeleteAssets(GetSelected());
				}
			);
			PjcWidgets::AddMenuEntry(Menu, EPjcMenuIcon::Exclude, TEXT("Keep"), TEXT("Keeps them for good: adds them to the exclusions"), [this]() {
				KeepAssets(GetSelected());
			});
			PjcWidgets::AddMenuEntry(
				Menu, EPjcMenuIcon::Inspect, TEXT("Inspect"), TEXT("Shows why the scan is not sure"), Selected.Num() == 1, [this]() {
					const TArray<FRowPtr> Rows = GetSelected();
					if (Rows.Num() == 1) PjcTabs::Inspect(TabManager, Rows[0]->Package);
				}
			);
			break;
		case EPjcIssueCategory::Redirectors :
			{
				TArray<int32> Indices;
				for (const auto& Row : Selected) {
					if (Row->Redirector != INDEX_NONE) Indices.Add(Row->Redirector);
				}
				const bool bCanFix = Indices.ContainsByPredicate([this](const int32 Index) {
					return Redirectors[Index].CanFix();
				});
				const FString FixLabel = FString::Printf(TEXT("Fix\u2026 (%s)"), *PjcFormat::Count(Indices.Num()));
				PjcWidgets::AddMenuEntry(Menu, EPjcMenuIcon::None, FixLabel, TEXT("Fixes the selected redirectors"), bCanFix, [this, Indices]() {
					FixRedirectors(Indices);
				});
			}
			break;
		case EPjcIssueCategory::External :
			if (!bKept) {
				PjcWidgets::AddMenuEntry(Menu, EPjcMenuIcon::Delete, DeleteLabel, TEXT("Deletes the selection from disk"), [this]() {
					DeleteRows(GetSelected());
				});
				Menu.AddSeparator();
				AddExcludeEntries(Menu);
				Menu.AddSeparator();
			}
			PjcWidgets::AddMenuEntry(Menu, EPjcMenuIcon::Explorer, TEXT("Show in Explorer"), TEXT("Opens the folder of the file"), [this]() {
				const TArray<FRowPtr> Rows = GetSelected();
				if (Rows.Num() > 0) UPjcSubsystem::OpenPathInFileExplorer(FPaths::GetPath(Rows[0]->Id));
			});
			break;
		case EPjcIssueCategory::Corrupted :
			PjcWidgets::AddMenuEntry(Menu, EPjcMenuIcon::Delete, DeleteLabel, TEXT("Deletes the selection from disk"), [this]() {
				DeleteRows(GetSelected());
			});
			PjcWidgets::AddMenuEntry(Menu, EPjcMenuIcon::Explorer, TEXT("Show in Explorer"), TEXT("Opens the folder of the file"), [this]() {
				const TArray<FRowPtr> Rows = GetSelected();
				if (Rows.Num() > 0) UPjcSubsystem::OpenPathInFileExplorer(FPaths::GetPath(Rows[0]->Id));
			});
			break;
		case EPjcIssueCategory::Empty :
			PjcWidgets::AddMenuEntry(Menu, EPjcMenuIcon::Delete, DeleteLabel, TEXT("Deletes the selected empty folders"), [this]() {
				DeleteRows(GetSelected());
			});
			break;
	}
	return Menu.MakeWidget();
}

FReply SPjcTabIssues::OnListKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) {
	if (KeyEvent.GetKey() != EKeys::Delete || GetSelected().Num() == 0) return FReply::Unhandled();

	if (Category == EPjcIssueCategory::Review) {
		DeleteAssets(GetSelected());
		return FReply::Handled();
	}
	const bool bFiles = Category == EPjcIssueCategory::Corrupted || Category == EPjcIssueCategory::Empty;
	if (bFiles || (Category == EPjcIssueCategory::External && !bKept)) {
		DeleteRows(GetSelected());
		return FReply::Handled();
	}
	return FReply::Unhandled();
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

void SPjcTabIssues::KeepAssets(const TArray<FRowPtr>& InRows) {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	TArray<FString> Assets;
	for (const auto& Row : InRows) {
		if (Result.Assets.IsValidIndex(Row->Asset)) {
			Assets.Add(PjcShim::GetObjectPathString(Result.Assets[Row->Asset]));
		}
	}
	if (Assets.Num() == 0) return;

	const TCHAR* Plural = Assets.Num() == 1 ? TEXT("") : TEXT("s");
	PjcActions::Exclude({}, {}, Assets, FString::Printf(TEXT("Kept %s asset%s"), *PjcFormat::Count(Assets.Num()), Plural));
}

void SPjcTabIssues::DeleteAssets(const TArray<FRowPtr>& InRows) {
	TArray<FName> Packages;
	for (const auto& Row : InRows) {
		Packages.Add(Row->Package);
	}
	if (Packages.Num() > 0) {
		PjcActions::DeleteWithGroup(Packages);
	}
}

TSharedRef<SWidget> SPjcTabIssues::MakeExcludeMenu() {
	FMenuBuilder Menu {true, nullptr};
	AddExcludeEntries(Menu);
	return Menu.MakeWidget();
}

void SPjcTabIssues::AddExcludeEntries(FMenuBuilder& Menu) {
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
