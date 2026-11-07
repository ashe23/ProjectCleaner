// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Layout/Margin.h"

class SToolTip;
class SWidget;
struct FAssetData;
struct FSlateBrush;

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

	// Color of the plugin style, e.g. ProjectCleaner.Color.Red
	FLinearColor GetColor(const FName Name);

	FSlateFontInfo GetFontMono(const int32 Size);

	// Content on the card background
	TSharedRef<SWidget> MakeCard(const TSharedRef<SWidget>& Content, const FMargin& Padding);

	// Title of a card: small, bold, upper case, muted
	TSharedRef<SWidget> MakeCardTitle(const FString& Title);

	// Color sample of a legend
	TSharedRef<SWidget> MakeSwatch(const FLinearColor& Color, const float Size = 9.0f);

	struct FBarSegment
	{
		double Value = 0.0;
		FLinearColor Color = FLinearColor::White;
		TSharedPtr<SToolTip> ToolTip;
	};

	// Segments side by side, each as long as its share of Total (their sum when Total is smaller), on a dark track
	TSharedRef<SWidget> MakeBar(const TArray<FBarSegment>& Segments, const float Height, const double Total = 0.0);
}	// namespace PjcWidgets
