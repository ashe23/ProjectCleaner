// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Layout/Margin.h"
#include "Textures/SlateIcon.h"
#include "Widgets/Views/SHeaderRow.h"

class FMenuBuilder;
class SToolTip;
class SWindow;
class SWidget;
struct FAssetData;
struct FSlateBrush;
enum class EPjcAssetSet : uint8;

// What a menu entry does, for its icon
enum class EPjcMenuIcon : uint8
{
	None,
	Inspect,
	Browse,
	Delete,
	Exclude,
	// remove an exclusion
	Include,
	Explorer,
	SizeMap,
	Expand,
	Collapse,
	Reset,
};

// Small widgets the Project Cleaner tools share
namespace PjcWidgets
{
	// Type of an asset as the Content Browser shows it
	struct FAssetType
	{
		FString Label;
		FLinearColor Color = FLinearColor::Gray;
		const FSlateBrush* Icon = nullptr;
	};

	// Name, color and icon of the asset type, cached per class
	const FAssetType& GetAssetType(const FAssetData& Asset);

	// Tooltip with a bold title, the text, and optional muted lines
	TSharedRef<SToolTip> MakeToolTip(const FString& Title, const FString& Text, const FString& More = FString {});

	// Tooltip with only text, in the same look; shows nothing while the text is empty
	TSharedRef<SToolTip> MakeToolTip(const TAttribute<FText>& Text);

	// A header column with a tooltip in the same look
	SHeaderRow::FColumn::FArguments MakeColumn(SHeaderRow::FColumn::FArguments Column, const TAttribute<FText>& ToolTip);

	// Color of the plugin style, e.g. ProjectCleaner.Color.Red
	FLinearColor GetColor(const FName Name);

	// Class color of an asset icon, a bit lighter so dark class colors stay readable on dark rows
	FLinearColor GetIconColor(const FLinearColor& ClassColor);

	// The size of a window's content before the window opens. An auto sized window is centered at the size it opens
	// with, so it opens with this size to stay centered.
	FVector2D GetContentSize(const TSharedRef<SWidget>& Content);

	// Shows a modal window centered on the editor window. The engine centers it on a work area it selects, at the size
	// it opens with, and an auto sized window then grows or shrinks from its top left corner.
	void ShowModal(const TSharedRef<SWindow>& Window);

	// One number of a stats card: the value (it can change while the card shows), its label and tooltip, the palette
	// color of a large value
	struct FStat
	{
		TAttribute<FString> Value;
		FString Label;
		FString ToolTip;
		FName Color = TEXT("ProjectCleaner.Color.Strong");
	};

	// Stats card (Clean project, Scan): the numbers large with the label above and the unit smaller, the last
	// two as label and value lines on the right. A value of 0 or a dash is grey
	TSharedRef<SWidget> MakeStats(const TArray<FStat>& Stats, const float LinesFill = 1.1f);

	// Mark of a step in a list of steps: a check when done, a spinner while it runs, a dot while it waits
	TSharedRef<SWidget> MakeStepMark(TFunction<bool()> IsDone, TFunction<bool()> IsRunning);

	// Colors of an Assets set: bars, size map boxes and their text, folders tinted by their share
	struct FSetColors
	{
		FLinearColor Bar;
		FLinearColor Box;
		FLinearColor BoxText;
		FLinearColor Tint;
	};
	FSetColors GetSetColors(const EPjcAssetSet Set);

	// Name of an Assets set (Unused), and the word lists use (unused, to review)
	FString GetSetLabel(const EPjcAssetSet Set);
	FString GetSetWord(const EPjcAssetSet Set);

	FSlateFontInfo GetFontMono(const int32 Size);

	// Content on the card background
	TSharedRef<SWidget> MakeCard(const TSharedRef<SWidget>& Content, const FMargin& Padding);

	// Title of a card: small, bold, upper case, muted
	TSharedRef<SWidget> MakeCardTitle(const FString& Title);

	// Color sample of a legend
	TSharedRef<SWidget> MakeSwatch(const FLinearColor& Color, const float Size = 9.0f);

	// Filter of a list in a side column, one above the other: a dot in its color, the name, a count, as a pill. Picked: in its
	// color; grey while other filters are picked (Assets, Issues)
	TSharedRef<SWidget> MakeFilterPill(
		const FString& Label,
		const FString& ToolTip,
		const FString& More,
		const FLinearColor& Color,
		const int32 Num,
		const bool bDot,
		TFunction<bool()> IsOn,
		TFunction<bool()> IsGrey,
		TFunction<void()> OnClick
	);

	// Heading over a group of filters: small, bold, upper case, muted
	TSharedRef<SWidget> MakeFilterHeading(const TAttribute<FText>& Text, const TAttribute<FText>& ToolTip);

	struct FBarSegment
	{
		double Value = 0.0;
		FLinearColor Color = FLinearColor::White;
		TSharedPtr<SToolTip> ToolTip;
	};

	// Segments side by side, each as long as its share of Total (their sum when Total is smaller), on a dark track
	TSharedRef<SWidget> MakeBar(const TArray<FBarSegment>& Segments, const float Height, const double Total = 0.0);

	// The editor's own icon for a menu entry
	FSlateIcon GetMenuIcon(const EPjcMenuIcon Icon);

	// Menu entry that runs OnClick, greyed out when not enabled
	void AddMenuEntry(FMenuBuilder& Menu, const EPjcMenuIcon Icon, const FString& Label, const FString& ToolTip, TFunction<void()> OnClick);
	void AddMenuEntry(
		FMenuBuilder& Menu,
		const EPjcMenuIcon Icon,
		const FString& Label,
		const FString& ToolTip,
		const bool bEnabled,
		TFunction<void()> OnClick
	);

	// Menu entry with a check mark while *bChecked is true; OnClick changes it
	void AddMenuToggle(FMenuBuilder& Menu, const FString& Label, const FString& ToolTip, const bool* bChecked, TFunction<void()> OnClick);
}	// namespace PjcWidgets
