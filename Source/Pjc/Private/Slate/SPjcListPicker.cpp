// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcListPicker.h"
#include "Slate/PjcWidgets.h"
#include "PjcShim.h"
// Engine Headers
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

namespace
{
	// unity builds join files: names unique to this file
	constexpr int32 PjcPickerMaxRows = 60;
}	// namespace

void SPjcListPicker::Construct(const FArguments& InArgs) {
	Items = InArgs._Items;
	Added = TSet<FString> {InArgs._Added};
	Query = InArgs._Query;
	MaxBeforeTyping = InArgs._MaxBeforeTyping;
	bAllowTyped = InArgs._bAllowTyped;
	bTypedIsPath = InArgs._bTypedIsPath;
	OnPicked = InArgs._OnPicked;
	OnClose = InArgs._OnClose;

	// clang-format off
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(PjcShim::GetBrush(TEXT("Menu.Background")))
		.Padding(FMargin {6.0f})
		[
			SNew(SBox).WidthOverride(320.0f).MaxDesiredHeight(320.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 4.0f})
				[
					SAssignNew(Search, SEditableTextBox)
					.Text(FText::FromString(Query))
					.HintText(FText::FromString(InArgs._Hint))
					.OnTextChanged_Lambda([this](const FText& Text) {
						Query = Text.ToString();
						Highlight = 0;
						Refresh();
					})
					.OnKeyDownHandler(this, &SPjcListPicker::OnSearchKeyDown)
				]
				+ SVerticalBox::Slot().FillHeight(1.0f)
				[
					SAssignNew(List, SListView<FItemPtr>)
					.ListItemsSource(&Rows)
					.SelectionMode(ESelectionMode::Single)
					.OnGenerateRow(this, &SPjcListPicker::OnGenerateRow)
					.OnMouseButtonClick_Lambda([this](FItemPtr Item) { Pick(Item); })
				]
			]
		]
	];
	// clang-format on

	Refresh();

	// typing starts right away once the picker is shown
	const auto FocusSearch = [this](double, float) {
		FSlateApplication::Get().SetKeyboardFocus(Search, EFocusCause::SetDirectly);
		return EActiveTimerReturnType::Stop;
	};
	RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateLambda(FocusSearch));
}

void SPjcListPicker::Refresh() {
	const FString Typed = Query.TrimStartAndEnd();
	Rows.Reset();

	// before typing the biggest, then what matches, the ones that start with it first
	TArray<const FPjcPickerItem*> Matches;
	for (const auto& Item : Items) {
		const FString& Name = Item.Label.IsEmpty() ? Item.Value : Item.Label;
		if (Typed.IsEmpty() || Item.Value.Contains(Typed) || Name.Contains(Typed)) {
			Matches.Add(&Item);
		}
	}
	if (Typed.IsEmpty() && MaxBeforeTyping > 0) {
		Matches.Sort([](const FPjcPickerItem& A, const FPjcPickerItem& B) {
			return A.Sort > B.Sort;
		});
		Matches.SetNum(FMath::Min(Matches.Num(), MaxBeforeTyping));
	}
	if (!Typed.IsEmpty()) {
		Matches.StableSort([&Typed](const FPjcPickerItem& A, const FPjcPickerItem& B) {
			const bool bStartsA = (A.Label.IsEmpty() ? A.Value : A.Label).StartsWith(Typed);
			const bool bStartsB = (B.Label.IsEmpty() ? B.Value : B.Label).StartsWith(Typed);
			return bStartsA && !bStartsB;
		});
	}
	for (int32 Index = 0; Index < Matches.Num() && Index < PjcPickerMaxRows; ++Index) {
		Rows.Add(MakeShared<FPjcPickerItem>(*Matches[Index]));
	}

	// a value that is not in the project can be typed (a class or folder without assets yet)
	const bool bFound = Items.ContainsByPredicate([&Typed](const FPjcPickerItem& Item) {
		return Item.Value.Equals(Typed, ESearchCase::IgnoreCase);
	});
	const bool bTypedValid = !bTypedIsPath || Typed.StartsWith(TEXT("/"));
	if (bAllowTyped && !Typed.IsEmpty() && !bFound && bTypedValid) {
		const TSharedPtr<FPjcPickerItem> TypedItem = MakeShared<FPjcPickerItem>();
		TypedItem->Value = Typed;
		TypedItem->Meta = bTypedIsPath ? TEXT("path") : TEXT("class name");
		TypedItem->bTyped = true;
		Rows.Add(TypedItem);
	}

	Highlight = FMath::Clamp(Highlight, 0, FMath::Max(0, Rows.Num() - 1));
	if (List.IsValid()) {
		List->RequestListRefresh();
		if (Rows.IsValidIndex(Highlight)) {
			List->SetSelection(Rows[Highlight], ESelectInfo::Direct);
			List->RequestScrollIntoView(Rows[Highlight]);
		}
	}
}

void SPjcListPicker::Pick(const FItemPtr& Item) {
	if (!Item.IsValid() || Added.Contains(Item->Value)) return;

	Added.Add(Item->Value);
	OnPicked.ExecuteIfBound(Item->Value);
}

FReply SPjcListPicker::OnSearchKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) {
	const FKey Key = KeyEvent.GetKey();
	if (Key == EKeys::Escape) {
		OnClose.ExecuteIfBound();
		return FReply::Handled();
	}
	if ((Key == EKeys::Down || Key == EKeys::Up) && Rows.Num() > 0) {
		Highlight = (Highlight + (Key == EKeys::Down ? 1 : -1) + Rows.Num()) % Rows.Num();
		List->SetSelection(Rows[Highlight], ESelectInfo::Direct);
		List->RequestScrollIntoView(Rows[Highlight]);
		return FReply::Handled();
	}
	if (Key == EKeys::Enter && Rows.IsValidIndex(Highlight)) {
		Pick(Rows[Highlight]);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

TSharedRef<ITableRow> SPjcListPicker::OnGenerateRow(FItemPtr Item, const TSharedRef<STableViewBase>& Table) {
	const bool bAdded = Added.Contains(Item->Value);
	const FString Name = Item->bTyped ? TEXT("Add ") + Item->Value : (Item->Label.IsEmpty() ? Item->Value : Item->Label);
	const FSlateColor NameColor = bAdded ? FSlateColor::UseSubduedForeground() : FSlateColor::UseForeground();
	// clang-format off
	return SNew(STableRow<FItemPtr>, Table)
		.Padding(FMargin {4.0f, 2.0f})
		.ToolTipText(FText::FromString(Item->Value))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin {0.0f, 0.0f, 6.0f, 0.0f})
			[
				SNew(SBox).WidthOverride(12.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(bAdded ? TEXT("\\u2713") : TEXT("")))
					.ColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.Ok"))
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Name))
				.HighlightText(FText::FromString(Query.TrimStartAndEnd()))
				.ColorAndOpacity(NameColor)
				.Clipping(EWidgetClipping::ClipToBounds)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin {8.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock).Text(FText::FromString(Item->Meta)).Font(PjcWidgets::GetFontMono(8)).ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		];
	// clang-format on
}
