// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcDialog.h"
#include "Slate/PjcWidgets.h"
#include "PjcFormat.h"
#include "PjcShim.h"
#include "PjcSubsystem.h"
// Engine Headers
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	// unity builds join files: names unique to this file
	constexpr int32 PjcDialogMaxRows = 60;
}	// namespace

void SPjcDialog::Construct(const FArguments& InArgs) {
	Window = InArgs._Window;

	const TSharedRef<SVerticalBox> Body = SNew(SVerticalBox);
	for (const auto& Part : InArgs._Parts) {
		if (!Part.Heading.IsEmpty()) {
			// clang-format off
			Body->AddSlot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 4.0f})
			[
				PjcWidgets::MakeCardTitle(Part.Heading)
			];
			// clang-format on
		}
		if (!Part.Note.IsEmpty()) {
			// clang-format off
			Body->AddSlot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 6.0f})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(3.0f)
					[
						SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(PjcWidgets::GetColor(Part.NoteColor))
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(SBorder).BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.DarkGroupBorder"))).Padding(FMargin {10.0f, 6.0f})
					[
						SNew(STextBlock).Text(FText::FromString(Part.Note)).AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
				]
			];
			// clang-format on
		}
		if (Part.Rows.Num() > 0) {
			const TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
			for (int32 Index = 0; Index < Part.Rows.Num() && Index < PjcDialogMaxRows; ++Index) {
				// clang-format off
				List->AddSlot().AutoHeight().Padding(FMargin {0.0f, 1.0f})
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[
						SNew(STextBlock).Text(FText::FromString(Part.Rows[Index].Key))
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin {12.0f, 0.0f, 0.0f, 0.0f})
					[
						SNew(STextBlock).Text(FText::FromString(Part.Rows[Index].Value)).ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
				];
				// clang-format on
			}
			if (Part.Rows.Num() > PjcDialogMaxRows) {
				// clang-format off
				List->AddSlot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("and %s more"), *PjcFormat::Count(Part.Rows.Num() - PjcDialogMaxRows))))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				];
				// clang-format on
			}
			// clang-format off
			Body->AddSlot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 10.0f})
			[
				SNew(SBox).MaxDesiredHeight(160.0f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						List
					]
				]
			];
			// clang-format on
		}
	}

	const TSharedRef<SHorizontalBox> Buttons = SNew(SHorizontalBox);
	const auto Close = [this](const bool bConfirm) {
		bConfirmed = bConfirm;
		if (const TSharedPtr<SWindow> Owner = Window.Pin()) Owner->RequestDestroyWindow();
		return FReply::Handled();
	};
	if (!InArgs._ConfirmText.IsEmpty()) {
		// clang-format off
		Buttons->AddSlot().AutoWidth().Padding(FMargin {0.0f, 0.0f, 8.0f, 0.0f})
		[
			SNew(SButton)
			.ContentPadding(FMargin {14.0f, 3.0f})
			.OnClicked_Lambda([Close]() {
				return Close(false);
			})
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Cancel")))
			]
		];
		// clang-format on
	}
	const FName ConfirmColor = InArgs._ConfirmColor.IsNone() ? FName {TEXT("ProjectCleaner.Color.Accent")} : InArgs._ConfirmColor;
	// clang-format off
	Buttons->AddSlot().AutoWidth()
	[
		SNew(SButton)
		.ButtonColorAndOpacity(PjcWidgets::GetColor(ConfirmColor))
		.ContentPadding(FMargin {14.0f, 3.0f})
		.OnClicked_Lambda([Close]() {
			return Close(true);
		})
		[
			SNew(STextBlock)
			.Text(FText::FromString(InArgs._ConfirmText.IsEmpty() ? TEXT("OK") : InArgs._ConfirmText))
			.ColorAndOpacity(FLinearColor::White)
		]
	];

	ChildSlot
	[
		SNew(SBorder).BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.GroupBorder"))).Padding(0.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot().Padding(FMargin {16.0f, 14.0f, 16.0f, 4.0f})
				[
					Body
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox).HeightOverride(1.0f)
				[
					SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.Line"))
				]
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(FMargin {16.0f, 10.0f})
			[
				Buttons
			]
		]
	];
	// clang-format on
}

bool SPjcDialog::Show(const FString& Title, const TArray<FPjcDialogPart>& Parts, const FString& ConfirmText, const FName ConfirmColor) {
	int32 NumRows = 0;
	for (const auto& Part : Parts) {
		NumRows += FMath::Min(Part.Rows.Num(), 8) + 3;
	}
	// clang-format off
	const TSharedRef<SWindow> DialogWindow = SNew(SWindow)
		.Title(FText::FromString(Title))
		.ClientSize(FVector2D {560.0f, FMath::Clamp(120.0f + NumRows * 20.0f, 220.0f, 620.0f)})
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.SizingRule(ESizingRule::UserSized);
	// clang-format on
	const TSharedRef<SPjcDialog> Dialog = SNew(SPjcDialog).Parts(Parts).ConfirmText(ConfirmText).ConfirmColor(ConfirmColor).Window(DialogWindow);
	DialogWindow->SetContent(Dialog);

	FSlateApplication::Get().AddModalWindow(DialogWindow, FSlateApplication::Get().GetActiveTopLevelWindow());
	return Dialog->bConfirmed;
}

FPjcDialogPart SPjcDialog::MakeVersionControlNote() {
	FPjcDialogPart Part;
	if (UPjcSubsystem::ProjectHasVersionControl()) {
		Part.Note = TEXT("Version control found. Deleted and resaved files can be restored from it.");
		Part.NoteColor = TEXT("ProjectCleaner.Color.Ok");
	}
	else {
		Part.Note = TEXT("No version control found for this project. Changes cannot be undone: copy the project folder somewhere safe first.");
		Part.NoteColor = TEXT("ProjectCleaner.Color.Yellow");
	}
	return Part;
}
