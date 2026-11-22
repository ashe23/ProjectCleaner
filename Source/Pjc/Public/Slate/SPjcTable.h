// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateColor.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"

struct FAssetData;
struct FSlateBrush;
class ITableRow;
class STableViewBase;

// Column of a table: header label, width, how its cells look
struct FPjcTableColumn
{
	FString Label;
	// share of the free width, used while FixedWidth is 0
	float Fill = 1.0f;
	float FixedWidth = 0.0f;
	// sizes and counts: right aligned, mono font
	bool bNumber = false;
	bool bMuted = false;
	// right aligned text (numbers always are)
	bool bRight = false;
	// a path: the start is trimmed, the full path shows in its tooltip
	bool bPath = false;
	// a bar of the row's share in place of text
	bool bBar = false;
	// the row's lead widget (a step mark) in place of text
	bool bLead = false;
};

// Row of a table: a text for each column, an icon and a muted prefix before the first one
struct FPjcTableRow
{
	// a text can change while the table shows (bound attribute)
	TArray<TAttribute<FString>> Cells;
	const FSlateBrush* Icon = nullptr;
	FLinearColor IconColor = FLinearColor::White;
	FString Prefix;
	// 0 to 1, for a bar column
	float Share = 0.0f;
	FString ToolTipTitle;
	FString ToolTip;
	FString ToolTipMore;
	// color of the text cells that are not muted
	TAttribute<FSlateColor> TextColor = FSlateColor::UseForeground();
	// the running row: a marked background with an accent edge
	TAttribute<bool> Marked;
	// widget of a lead column
	TFunction<TSharedRef<SWidget>()> MakeLead;
};

/**
 * Table of the dialogs and of the Clean project lists: a header, rows of 24 px, a scroll bar after NumVisible rows, and a
 * line with the total. One look for every list of assets, files and folders a dialog shows.
 */
struct FPjcTable
{
	TArray<FPjcTableColumn> Columns;
	TArray<FPjcTableRow> Rows;
	int32 NumVisible = 5;
	// as high as NumVisible rows also with fewer rows (lists side by side)
	bool bFixedHeight = false;
	bool bHeader = true;
	// e.g. 3 assets and 9.60 MiB; over the rows in place of the header when bTotalOnTop
	FString Total;
	FString TotalValue;
	bool bTotalOnTop = false;

	bool IsSet() const {
		return Columns.Num() > 0;
	}

	// Assets with their class icon: Name, Class, Folder, Size. Sizes: one for each asset, the size on disk when empty.
	// HeldBy: one for each asset, adds that column
	static FPjcTable MakeAssets(
		const TArray<FAssetData>& Assets,
		const TArray<int64>& Sizes = TArray<int64> {},
		const TArray<FString>& HeldBy = TArray<FString> {},
		const bool bFolder = true
	);

	// The same for packages, by their assets in Asset Registry
	static FPjcTable MakePackages(
		const TArray<FName>& Packages,
		const TArray<int64>& Sizes = TArray<int64> {},
		const TArray<FString>& HeldBy = TArray<FString> {},
		const bool bFolder = true
	);

	// Files on disk (full paths): Name, Type, Folder in the project, Size
	static FPjcTable MakeFiles(const TArray<FString>& Files);

	// Folders on disk (full paths): Folder, Path
	static FPjcTable MakeFolders(const TArray<FString>& Folders);

	// e.g. 1 asset, 3 assets
	static FString Plural(const int32 Num, const TCHAR* One);
};

// Shows an FPjcTable
class SPjcTable final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcTable) {}
	SLATE_ARGUMENT(FPjcTable, Table)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	using FRowPtr = TSharedPtr<FPjcTableRow>;

  private:

	TSharedRef<ITableRow> MakeRow(FRowPtr Row, const TSharedRef<STableViewBase>& Owner) const;

	FPjcTable Table;
	TArray<FRowPtr> Rows;
};

// One line of text that fits its width: the end is cut off with an ellipsis, or the start for paths
class SPjcFitText final : public SLeafWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcFitText) :
		_Font(FCoreStyle::GetDefaultFontStyle("Regular", 9)),
		_ColorAndOpacity(FSlateColor::UseForeground()),
		_TrimStart(false) {}
	SLATE_ATTRIBUTE(FString, Text)
	SLATE_ARGUMENT(FSlateFontInfo, Font)
	SLATE_ATTRIBUTE(FSlateColor, ColorAndOpacity)
	SLATE_ARGUMENT(bool, TrimStart)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled
	) const override;
	virtual FVector2D ComputeDesiredSize(float) const override;
	virtual bool ComputeVolatility() const override;

  private:

	// the text as it fits in Width
	const FString& GetShown(const float Width, const float Scale) const;

	TAttribute<FString> Text;
	FSlateFontInfo Font;
	TAttribute<FSlateColor> Color;
	bool bTrimStart = false;

	mutable FString CachedText;
	mutable float CachedWidth = -1.0f;
	mutable float CachedScale = -1.0f;
	mutable FString CachedShown;
};
