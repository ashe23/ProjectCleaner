// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTable.h"
#include "Slate/PjcWidgets.h"
#include "PjcFormat.h"
#include "PjcStyles.h"
#include "PjcSubsystem.h"
// Engine Headers
#include "AssetRegistry/AssetData.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

namespace
{
	// unity builds join files: names unique to this file
	constexpr float PjcTableHeaderHeight = 22.0f;
	constexpr float PjcTableRowHeight = 24.0f;

	FName PjcTableColumnId(const int32 Index) {
		return FName {*FString::Printf(TEXT("PjcTableColumn%d"), Index)};
	}

	// cells keep 10 px from the table edges and 10 px from each other
	FMargin PjcTableCellPadding(const int32 Index, const int32 Num) {
		return FMargin {Index == 0 ? 10.0f : 5.0f, 0.0f, Index == Num - 1 ? 10.0f : 5.0f, 0.0f};
	}

	FPjcTableColumn PjcTableColumn(const TCHAR* Label, const float Fill, const float FixedWidth = 0.0f) {
		FPjcTableColumn Column;
		Column.Label = Label;
		Column.Fill = Fill;
		Column.FixedWidth = FixedWidth;
		return Column;
	}

	FPjcTableColumn PjcTableMuted(const TCHAR* Label, const float Fill, const float FixedWidth = 0.0f) {
		FPjcTableColumn Column = PjcTableColumn(Label, Fill, FixedWidth);
		Column.bMuted = true;
		return Column;
	}

	FPjcTableColumn PjcTablePath(const TCHAR* Label, const float Fill) {
		FPjcTableColumn Column = PjcTableMuted(Label, Fill);
		Column.bPath = true;
		return Column;
	}

	FPjcTableColumn PjcTableNumber(const TCHAR* Label, const float FixedWidth) {
		FPjcTableColumn Column = PjcTableColumn(Label, 0.0f, FixedWidth);
		Column.bNumber = true;
		return Column;
	}

	void PjcTableAssetColumns(FPjcTable& Table, const bool bHeld, const bool bFolder) {
		if (bHeld) {
			Table.Columns = {
			  PjcTableColumn(TEXT("Name"), 1.1f),
			  PjcTableMuted(TEXT("Class"), 0.9f),
			  PjcTablePath(TEXT("Folder"), 0.8f),
			  PjcTableMuted(TEXT("Held by"), 2.2f),
			  PjcTableNumber(TEXT("Size"), 70.0f),
			};
		}
		else if (bFolder) {
			Table.Columns = {
			  PjcTableColumn(TEXT("Name"), 1.5f),
			  PjcTableMuted(TEXT("Class"), 1.0f),
			  PjcTablePath(TEXT("Folder"), 1.3f),
			  PjcTableNumber(TEXT("Size"), 74.0f),
			};
		}
		else {
			FPjcTableColumn Class = PjcTableMuted(TEXT("Class"), 1.0f);
			Class.bRight = true;
			Table.Columns = {
			  PjcTableColumn(TEXT("Name"), 1.6f),
			  Class,
			  PjcTableNumber(TEXT("Size"), 74.0f),
			};
		}
	}

	void
	PjcTableAddAsset(FPjcTable& Table, const FAssetData& Asset, const FName Package, const int64 Size, const FString* HeldBy, const bool bFolder) {
		const FString Path = Package.ToString();
		const FString Name = Asset.IsValid() ? Asset.AssetName.ToString() : FPackageName::GetShortName(Package);
		FPjcTableRow& Row = Table.Rows.AddDefaulted_GetRef();
		FString Label;
		if (Asset.IsValid()) {
			const PjcWidgets::FAssetType& Type = PjcWidgets::GetAssetType(Asset);
			Row.Icon = Type.Icon;
			Row.IconColor = PjcWidgets::GetIconColor(Type.Color);
			Label = Type.Label;
		}
		Row.Cells.Add(Name);
		Row.Cells.Add(Label);
		if (bFolder) Row.Cells.Add(FPackageName::GetLongPackagePath(Path).Mid(1));
		if (HeldBy) Row.Cells.Add(*HeldBy);
		Row.Cells.Add(PjcFormat::Size(Size));
		Row.ToolTipTitle = Name;
		Row.ToolTip = Path;
		Row.ToolTipMore = Label.IsEmpty() ? PjcFormat::Size(Size) : Label + TEXT(" \u00B7 ") + PjcFormat::Size(Size);
	}

	// One row of a table, a cell for each column
	class SPjcTableRowWidget final : public SMultiColumnTableRow<SPjcTable::FRowPtr>
	{
	  public:

		SLATE_BEGIN_ARGS(SPjcTableRowWidget) {}
		SLATE_ARGUMENT(SPjcTable::FRowPtr, Row)
		SLATE_ARGUMENT(TArray<FPjcTableColumn>, Columns)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& Owner) {
			Row = InArgs._Row;
			Columns = InArgs._Columns;
			FSuperRowType::FArguments RowArgs;
			RowArgs.Style(&FPjcStyles::Get().GetWidgetStyle<FTableRowStyle>("ProjectCleaner.TableRow")).Padding(FMargin {0.0f});
			FSuperRowType::Construct(RowArgs, Owner);
			if (!Row->ToolTipTitle.IsEmpty()) {
				SetToolTip(PjcWidgets::MakeToolTip(Row->ToolTipTitle, Row->ToolTip, Row->ToolTipMore));
			}
		}

		virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override {
			int32 Index = 0;
			while (Index < Columns.Num() && PjcTableColumnId(Index) != ColumnName) {
				++Index;
			}
			if (!Columns.IsValidIndex(Index)) return SNullWidget::NullWidget;

			// clang-format off
			const TSharedRef<SWidget> Cell = SNew(SBox)
				.HeightOverride(PjcTableRowHeight)
				.Padding(PjcTableCellPadding(Index, Columns.Num()))
				.VAlign(VAlign_Center)
				[
					MakeCell(Index)
				];
			// clang-format on
			if (!Row->Marked.IsBound()) return Cell;

			// a row that can be marked: the marked color over the row, an accent edge on the left of the first cell
			const TAttribute<bool> Marked = Row->Marked;
			const FLinearColor MarkedColor = PjcWidgets::GetColor("ProjectCleaner.Color.Marked");
			const EVisibility EdgeVisibility = Index == 0 ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
			// clang-format off
			return SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor_Lambda([Marked, MarkedColor]() { return Marked.Get() ? MarkedColor : FLinearColor::Transparent; })
				.Padding(0.0f)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						Cell
					]
					+ SOverlay::Slot().HAlign(HAlign_Left)
					[
						SNew(SBox).WidthOverride(2.0f).Visibility(EdgeVisibility)
						[
							SNew(SImage)
							.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
							.ColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.Accent"))
							.Visibility_Lambda([Marked]() { return Marked.Get() ? EVisibility::HitTestInvisible : EVisibility::Hidden; })
						]
					]
				];
			// clang-format on
		}

	  private:

		TSharedRef<SWidget> MakeCell(const int32 Index) const {
			const FPjcTableColumn& Column = Columns[Index];
			const TAttribute<FString> Text = Row->Cells.IsValidIndex(Index) ? Row->Cells[Index] : TAttribute<FString> {FString {}};
			const FSlateColor Muted = FSlateColor::UseSubduedForeground();

			if (Column.bLead) {
				// clang-format off
				return SNew(SBox).HAlign(HAlign_Center)
					[
						Row->MakeLead ? Row->MakeLead() : SNullWidget::NullWidget
					];
				// clang-format on
			}
			if (Column.bBar) {
				PjcWidgets::FBarSegment Segment;
				Segment.Value = Row->Share;
				Segment.Color = PjcWidgets::GetColor("ProjectCleaner.Color.UnusedFill");
				return PjcWidgets::MakeBar({Segment}, 4.0f, 1.0);
			}
			if (Column.bNumber) {
				// clang-format off
				return SNew(SBox).HAlign(HAlign_Right)
					[
						SNew(STextBlock)
						.Text_Lambda([Text]() { return FText::FromString(Text.Get()); })
						.Font(PjcWidgets::GetFontMono(9))
						.ColorAndOpacity(Row->TextColor)
					];
				// clang-format on
			}
			if (Column.bPath) {
				// clang-format off
				return SNew(SPjcFitText)
					.Text(Text)
					.TrimStart(true)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
					.ColorAndOpacity(Muted)
					.ToolTip(PjcWidgets::MakeToolTip(FText::FromString(Text.Get())));
				// clang-format on
			}
			if (Index > 0) {
				const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", Column.bMuted ? 8 : 9);
				// clang-format off
				return SNew(SBox).HAlign(Column.bRight ? HAlign_Right : HAlign_Fill)
					[
						SNew(SPjcFitText).Text(Text).Font(Font).ColorAndOpacity(Column.bMuted ? TAttribute<FSlateColor> {Muted} : Row->TextColor)
					];
				// clang-format on
			}

			// the first column: icon, muted prefix, name
			const TSharedRef<SHorizontalBox> Name = SNew(SHorizontalBox);
			if (Row->Icon) {
				// clang-format off
				Name->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 7.0f, 0.0f})
				[
					SNew(SBox).WidthOverride(16.0f).HeightOverride(16.0f)
					[
						SNew(SImage).Image(Row->Icon).ColorAndOpacity(Row->IconColor)
					]
				];
				// clang-format on
			}
			if (!Row->Prefix.IsEmpty()) {
				// clang-format off
				Name->AddSlot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(FText::FromString(Row->Prefix)).ColorAndOpacity(Muted)
				];
				// clang-format on
			}
			// clang-format off
			Name->AddSlot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(SPjcFitText).Text(Text).ColorAndOpacity(Row->TextColor)
			];
			// clang-format on
			return Name;
		}

		SPjcTable::FRowPtr Row;
		TArray<FPjcTableColumn> Columns;
	};
}	// namespace

// FPjcTable

FPjcTable FPjcTable::MakeAssets(const TArray<FAssetData>& Assets, const TArray<int64>& Sizes, const TArray<FString>& HeldBy, const bool bFolder) {
	FPjcTable Table;
	const bool bHeld = HeldBy.Num() > 0;
	PjcTableAssetColumns(Table, bHeld, bFolder);
	int64 Total = 0;
	for (int32 Index = 0; Index < Assets.Num(); ++Index) {
		const int64 Size = Sizes.IsValidIndex(Index) ? Sizes[Index] : UPjcSubsystem::GetAssetSize(Assets[Index]);
		const FString* Held = bHeld && HeldBy.IsValidIndex(Index) ? &HeldBy[Index] : nullptr;
		PjcTableAddAsset(Table, Assets[Index], Assets[Index].PackageName, Size, Held, bFolder);
		Total += Size;
	}
	Table.Total = Plural(Assets.Num(), TEXT("asset"));
	Table.TotalValue = PjcFormat::Size(Total);
	return Table;
}

FPjcTable FPjcTable::MakePackages(const TArray<FName>& Packages, const TArray<int64>& Sizes, const TArray<FString>& HeldBy, const bool bFolder) {
	FPjcTable Table;
	const bool bHeld = HeldBy.Num() > 0;
	PjcTableAssetColumns(Table, bHeld, bFolder);
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	int64 Total = 0;
	for (int32 Index = 0; Index < Packages.Num(); ++Index) {
		TArray<FAssetData> Found;
		UPjcSubsystem::GetModuleAssetRegistry().Get().GetAssetsByPackageName(Packages[Index], Found);
		const FAssetData Asset = Found.Num() > 0 ? Found[0] : FAssetData {};
		const int64 OnDisk = Asset.IsValid() ? UPjcSubsystem::GetAssetSize(Asset) : Result.GetSize(Packages[Index]);
		const int64 Size = Sizes.IsValidIndex(Index) ? Sizes[Index] : OnDisk;
		const FString* Held = bHeld && HeldBy.IsValidIndex(Index) ? &HeldBy[Index] : nullptr;
		PjcTableAddAsset(Table, Asset, Packages[Index], Size, Held, bFolder);
		Total += Size;
	}
	Table.Total = Plural(Packages.Num(), TEXT("asset"));
	Table.TotalValue = PjcFormat::Size(Total);
	return Table;
}

FPjcTable FPjcTable::MakeFiles(const TArray<FString>& Files) {
	FPjcTable Table;
	Table.Columns = {
	  PjcTableColumn(TEXT("Name"), 1.4f),
	  PjcTableMuted(TEXT("Type"), 0.0f, 56.0f),
	  PjcTablePath(TEXT("Folder"), 1.4f),
	  PjcTableNumber(TEXT("Size"), 74.0f),
	};
	const FString ProjectDir = UPjcSubsystem::PathNormalize(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir())) + TEXT("/");
	int64 Total = 0;
	for (const FString& File : Files) {
		FString Relative = UPjcSubsystem::PathNormalize(File);
		Relative.RemoveFromStart(ProjectDir);
		const int64 Size = FMath::Max<int64>(UPjcSubsystem::GetFileSize(File), 0);
		FPjcTableRow& Row = Table.Rows.AddDefaulted_GetRef();
		Row.Cells = {FPaths::GetBaseFilename(File), FPaths::GetExtension(File).ToLower(), FPaths::GetPath(Relative), PjcFormat::Size(Size)};
		Row.ToolTipTitle = FPaths::GetCleanFilename(File);
		Row.ToolTip = Relative;
		Total += Size;
	}
	Table.Total = Plural(Files.Num(), TEXT("file"));
	Table.TotalValue = PjcFormat::Size(Total);
	return Table;
}

FPjcTable FPjcTable::MakeFolders(const TArray<FString>& Folders) {
	FPjcTable Table;
	Table.Columns = {
	  PjcTableColumn(TEXT("Folder"), 1.0f),
	  PjcTablePath(TEXT("Path"), 1.6f),
	};
	const FString ProjectDir = UPjcSubsystem::PathNormalize(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir())) + TEXT("/");
	const FSlateBrush* Icon = FPjcStyles::Get().GetBrush("ProjectCleaner.Icon.Folder");
	const FLinearColor IconColor = PjcWidgets::GetColor("ProjectCleaner.Color.FolderTint");
	for (const FString& Folder : Folders) {
		// the content path (Game/Maps), or the path in the project for folders outside content
		FString Path = UPjcSubsystem::PathConvertToRelative(Folder);
		if (Path.StartsWith(TEXT("/"))) {
			Path = Path.Mid(1);
		}
		else {
			Path = UPjcSubsystem::PathNormalize(Folder);
			Path.RemoveFromStart(ProjectDir);
		}
		FPjcTableRow& Row = Table.Rows.AddDefaulted_GetRef();
		Row.Icon = Icon;
		Row.IconColor = IconColor;
		Row.Cells = {FPaths::GetCleanFilename(Path), FPaths::GetPath(Path)};
		Row.ToolTipTitle = FPaths::GetCleanFilename(Path);
		Row.ToolTip = Path;
	}
	Table.Total = Plural(Folders.Num(), TEXT("folder"));
	Table.TotalValue = PjcFormat::Size(0);
	return Table;
}

FString FPjcTable::Plural(const int32 Num, const TCHAR* One) {
	return PjcFormat::Count(Num) + TEXT(" ") + One + (Num == 1 ? TEXT("") : TEXT("s"));
}

// SPjcTable

void SPjcTable::Construct(const FArguments& InArgs) {
	Table = InArgs._Table;
	Rows.Reset(Table.Rows.Num());
	for (const FPjcTableRow& Row : Table.Rows) {
		Rows.Add(MakeShared<FPjcTableRow>(Row));
	}

	const int32 NumColumns = Table.Columns.Num();
	// clang-format off
	const TSharedRef<SHeaderRow> Header = SNew(SHeaderRow)
		.Style(&FPjcStyles::Get().GetWidgetStyle<FHeaderRowStyle>("ProjectCleaner.TableHeader"))
		.Visibility(Table.bHeader ? EVisibility::Visible : EVisibility::Collapsed);
	// clang-format on
	for (int32 Index = 0; Index < NumColumns; ++Index) {
		const FPjcTableColumn& Column = Table.Columns[Index];
		const EHorizontalAlignment Align = Column.bNumber || Column.bRight ? HAlign_Right : HAlign_Left;
		SHeaderRow::FColumn::FArguments Args = SHeaderRow::Column(PjcTableColumnId(Index));
		if (Column.FixedWidth > 0.0f) {
			Args.FixedWidth(Column.FixedWidth);
		}
		else {
			Args.FillWidth(Column.Fill);
		}
		// clang-format off
		Args.HeaderContentPadding(PjcTableCellPadding(Index, NumColumns))
			.HeaderContent()
			[
				SNew(SBox).HeightOverride(PjcTableHeaderHeight).VAlign(VAlign_Center).HAlign(Align)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Column.Label))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			];
		// clang-format on
		Header->AddColumn(Args);
	}

	const int32 NumShown = Table.bFixedHeight ? Table.NumVisible : FMath::Clamp(Rows.Num(), 1, Table.NumVisible);
	const float ListHeight = (Table.bHeader ? PjcTableHeaderHeight : 0.0f) + NumShown * PjcTableRowHeight;
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	const FLinearColor Line = PjcWidgets::GetColor("ProjectCleaner.Color.Line");
	const EVisibility TotalVisibility = Table.Total.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;

	// clang-format off
	const TSharedRef<SWidget> Total = SNew(SBox)
		.HeightOverride(PjcTableRowHeight)
		.Visibility(TotalVisibility)
		[
			SNew(SBorder)
			.BorderImage(White)
			.BorderBackgroundColor(PjcWidgets::GetColor("ProjectCleaner.Color.Header"))
			.Padding(FMargin {10.0f, 0.0f})
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Table.Total))
					.Font(FPjcStyles::GetFont("Bold", 9))
					.ColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.Strong"))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {12.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(STextBlock).Text(FText::FromString(Table.TotalValue)).Font(PjcWidgets::GetFontMono(9))
				]
			]
		];
	const TSharedRef<SWidget> TotalLine = SNew(SBox)
		.HeightOverride(1.0f)
		.Visibility(TotalVisibility)
		[
			SNew(SImage).Image(White).ColorAndOpacity(Line)
		];

	const TSharedRef<SVerticalBox> Content = SNew(SVerticalBox);
	if (Table.bTotalOnTop) {
		Content->AddSlot().AutoHeight()
		[
			Total
		];
		Content->AddSlot().AutoHeight()
		[
			TotalLine
		];
	}
	Content->AddSlot().AutoHeight()
	[
		SNew(SBox).HeightOverride(ListHeight)
		[
			SNew(SListView<FRowPtr>)
			.ListItemsSource(&Rows)
			.SelectionMode(ESelectionMode::None)
			.ScrollbarVisibility(Rows.Num() > NumShown ? EVisibility::Visible : EVisibility::Collapsed)
			.HeaderRow(Header)
			.OnGenerateRow_Lambda([this](FRowPtr Row, const TSharedRef<STableViewBase>& Owner) { return MakeRow(Row, Owner); })
		]
	];
	if (!Table.bTotalOnTop) {
		Content->AddSlot().AutoHeight()
		[
			TotalLine
		];
		Content->AddSlot().AutoHeight()
		[
			Total
		];
	}

	ChildSlot
	[
		SNew(SBorder).BorderImage(White).BorderBackgroundColor(Line).Padding(1.0f)
		[
			SNew(SBorder).BorderImage(White).BorderBackgroundColor(PjcWidgets::GetColor("ProjectCleaner.Color.Recess")).Padding(0.0f)
			[
				Content
			]
		]
	];
	// clang-format on
}

TSharedRef<ITableRow> SPjcTable::MakeRow(FRowPtr Row, const TSharedRef<STableViewBase>& Owner) const {
	return SNew(SPjcTableRowWidget, Owner).Row(Row).Columns(Table.Columns);
}

// SPjcFitText

void SPjcFitText::Construct(const FArguments& InArgs) {
	Text = InArgs._Text;
	Font = InArgs._Font;
	Color = InArgs._ColorAndOpacity;
	bTrimStart = InArgs._TrimStart;
}

const FString& SPjcFitText::GetShown(const float Width, const float Scale) const {
	const FString Full = Text.Get();
	if (Full == CachedText && FMath::IsNearlyEqual(Width, CachedWidth) && FMath::IsNearlyEqual(Scale, CachedScale)) return CachedShown;

	CachedText = Full;
	CachedWidth = Width;
	CachedScale = Scale;
	CachedShown = Full;
	// measured at the scale the text is drawn: glyph widths do not scale exactly with the DPI
	const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const auto MeasureX = [&Measure, this, Scale](const FString& Value) {
		return static_cast<float>(Measure->Measure(Value, Font, Scale).X) / Scale;
	};
	if (Width <= 0.0f || MeasureX(Full) <= Width + 0.5f) return CachedShown;

	// the longest part that fits with the ellipsis
	const FString Dots = TEXT("\u2026");
	const auto Cut = [this, &Full, &Dots](const int32 Num) {
		return bTrimStart ? Dots + Full.Right(Num) : Full.Left(Num) + Dots;
	};
	int32 Low = 0;
	int32 High = Full.Len();
	while (Low < High) {
		const int32 Mid = (Low + High + 1) / 2;
		if (MeasureX(Cut(Mid)) <= Width) {
			Low = Mid;
		}
		else {
			High = Mid - 1;
		}
	}
	CachedShown = Low > 0 ? Cut(Low) : FString {};
	return CachedShown;
}

int32 SPjcFitText::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled
) const {
	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const FString& Shown = GetShown(LocalSize.X, FMath::Max(AllottedGeometry.Scale, 0.01f));
	if (Shown.IsEmpty()) return LayerId;

	const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const float Height = Measure->GetMaxCharacterHeight(Font);
	const FVector2D Offset {0.0f, FMath::RoundToFloat((LocalSize.Y - Height) * 0.5f)};
	const FPaintGeometry Geometry = AllottedGeometry.ToPaintGeometry(FVector2D {LocalSize.X, Height}, FSlateLayoutTransform {Offset});
	const FLinearColor Tint = Color.Get().GetColor(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint();
	const ESlateDrawEffect Effect = ShouldBeEnabled(bParentEnabled) ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
	FSlateDrawElement::MakeText(OutDrawElements, LayerId, Geometry, Shown, Font, Effect, Tint);
	return LayerId;
}

FVector2D SPjcFitText::ComputeDesiredSize(float) const {
	const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FVector2D Size = Measure->Measure(Text.Get(), Font);
	return FVector2D {Size.X, static_cast<float>(Measure->GetMaxCharacterHeight(Font))};
}

bool SPjcFitText::ComputeVolatility() const {
	return Text.IsBound() || Color.IsBound();
}
