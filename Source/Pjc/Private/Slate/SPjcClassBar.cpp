// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcClassBar.h"
#include "Slate/PjcWidgets.h"
#include "PjcFormat.h"
#include "PjcShim.h"
#include "PjcStyles.h"
// Engine Headers
#include "Runtime/Launch/Resources/Version.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	// unity builds join files: names unique to this file
	const FLinearColor PjcClassBarOff {0.05f, 0.05f, 0.05f, 1.0f};
	const FLinearColor PjcClassBarOffText {0.13f, 0.13f, 0.13f, 1.0f};
}	// namespace

void SPjcClassBar::Construct(const FArguments& InArgs) {
	OnChanged = InArgs._OnChanged;

	// clang-format off
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {6.0f, 4.0f, 6.0f, 2.0f})
		[
			SNew(SCheckBox)
			.ToolTipText(FText::FromString(TEXT("Click: every class on. Click again: every class off")))
			.IsChecked_Lambda([this]() {
				if (bNone) return ECheckBoxState::Unchecked;

				return Picked.Num() == 0 ? ECheckBoxState::Checked : ECheckBoxState::Undetermined;
			})
			.OnCheckStateChanged_Lambda([this](ECheckBoxState) { Toggle(NAME_None); })
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(FMargin {4.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("All classes")))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {8.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(STextBlock)
					.Text_Lambda([this]() {
						int32 Total = 0;
						for (const auto& Item : Items) {
							Total += Item.Num;
						}
						return FText::FromString(PjcFormat::Count(Total));
					})
					.Font(PjcWidgets::GetFontMono(8))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SAssignNew(Rows, SVerticalBox)
			]
		]
	];
	// clang-format on
}

void SPjcClassBar::SetItems(const TArray<FPjcClassBarItem>& InItems) {
	Items = InItems;
	TSet<FName> Names;
	for (const auto& Item : Items) {
		Names.Add(Item.ClassName);
	}
	Picked = Picked.Intersect(Names);

	Rows->ClearChildren();
	for (const auto& Item : Items) {
		// clang-format off
		Rows->AddSlot().AutoHeight()
		[
			MakeRow(Item)
		];
		// clang-format on
	}
}

bool SPjcClassBar::IsShown(const FName ClassName) const {
	return !bNone && (Picked.Num() == 0 || Picked.Contains(ClassName));
}

void SPjcClassBar::Reset() {
	Picked.Reset();
	bNone = false;
}

void SPjcClassBar::Toggle(const FName ClassName) {
	if (ClassName.IsNone()) {
		// every class on; when they all are already: every class off
		const bool bAllOn = IsAllOn();
		Picked.Reset();
		bNone = bAllOn;
	}
	else {
		bNone = false;
		if (Picked.Contains(ClassName)) {
			Picked.Remove(ClassName);
		}
		else {
			Picked.Add(ClassName);
		}
	}
	OnChanged.ExecuteIfBound();
}

TSharedRef<SWidget> SPjcClassBar::MakeRow(const FPjcClassBarItem& Item) {
	const FName ClassName = Item.ClassName;
	const FLinearColor Color = Item.Color;
	// with every class on they all show their color: a color means "listed"
	const auto IsOn = [this, ClassName]() {
		return IsShown(ClassName);
	};

	// clang-format off
	return SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
		.ForegroundColor(FSlateColor::UseForeground())
#endif
		.ButtonStyle(&PjcShim::GetStyle().GetWidgetStyle<FButtonStyle>(TEXT("NoBorder")))
		.ContentPadding(FMargin {8.0f, 2.0f})
		.ToolTipText(FText::FromString(Item.ToolTip.IsEmpty() ? Item.Label : Item.ToolTip))
		.OnClicked_Lambda([this, ClassName]() {
			Toggle(ClassName);
			return FReply::Handled();
		})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(10.0f).HeightOverride(10.0f)
				[
					SNew(SImage)
					.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.ColorAndOpacity_Lambda([IsOn, Color]() { return FSlateColor {IsOn() ? Color : PjcClassBarOff}; })
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(FMargin {7.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock)
				.Text(FText::FromString(Item.Label))
				.Clipping(EWidgetClipping::ClipToBounds)
				.ColorAndOpacity_Lambda([IsOn]() { return IsOn() ? FSlateColor::UseForeground() : FSlateColor {PjcClassBarOffText}; })
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {8.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock)
				.Text(FText::FromString(PjcFormat::Count(Item.Num)))
				.Font(PjcWidgets::GetFontMono(8))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		];
	// clang-format on
}
