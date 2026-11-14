// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcDeleteDialog.h"
#include "Slate/PjcWidgets.h"
#include "PjcFormat.h"
#include "PjcShim.h"
#include "PjcStyles.h"
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
	constexpr int32 PjcDeleteDialogMaxListed = 60;
	const TCHAR* const PjcDeleteDialogVcsFound = TEXT("Version control found. Deleted assets can be restored from it.");
	// clang-format off
	const TCHAR* const PjcDeleteDialogVcsMissing = TEXT("No version control found for this project. ")
		TEXT("Deleted files cannot be restored: copy the project folder somewhere safe first.");
	// clang-format on

	TSharedRef<SWidget> PjcDeleteDialogNote(const FString& Text, const FName Color) {
		// clang-format off
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(3.0f)
				[
					SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(PjcWidgets::GetColor(Color))
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(SBorder).BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.DarkGroupBorder"))).Padding(FMargin {10.0f, 6.0f})
				[
					SNew(STextBlock).Text(FText::FromString(Text)).AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			];
		// clang-format on
	}

	TSharedRef<SWidget> PjcDeleteDialogHeading(const FString& Text) {
		return PjcWidgets::MakeCardTitle(Text);
	}
}	// namespace

void SPjcDeleteDialog::Construct(const FArguments& InArgs) {
	Window = InArgs._Window;

	int64 Size = 0;
	for (const auto& Asset : InArgs._Selected) {
		Size += UPjcSubsystem::GetAssetSize(Asset);
	}
	for (const auto& Asset : InArgs._With) {
		Size += UPjcSubsystem::GetAssetSize(Asset);
	}
	const int32 NumAll = InArgs._Selected.Num() + InArgs._With.Num();

	const TSharedRef<SVerticalBox> Body = SNew(SVerticalBox);
	// clang-format off
	Body->AddSlot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 10.0f})
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin {0.0f, 0.0f, 24.0f, 0.0f})
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(FText::FromString(PjcFormat::Count(NumAll))).Font(PjcWidgets::GetFontMono(14))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("assets"))).ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(FText::FromString(PjcFormat::Size(Size))).Font(PjcWidgets::GetFontMono(14))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("on disk"))).ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		]
	];
	Body->AddSlot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 4.0f})
	[
		PjcDeleteDialogHeading(FString::Printf(TEXT("Selected \u00B7 %s"), *PjcFormat::Count(InArgs._Selected.Num())))
	];
	Body->AddSlot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 10.0f})
	[
		MakeList(InArgs._Selected)
	];
	if (InArgs._With.Num() > 0) {
		Body->AddSlot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 4.0f})
		[
			PjcDeleteDialogHeading(FString::Printf(TEXT("Deleted with them \u00B7 %s"), *PjcFormat::Count(InArgs._With.Num())))
		];
		Body->AddSlot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 6.0f})
		[
			PjcDeleteDialogNote(
				TEXT("Unused assets that reference the selection. The engine cannot delete an asset something still references."),
				TEXT("ProjectCleaner.Color.Yellow")
			)
		];
		Body->AddSlot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 10.0f})
		[
			MakeList(InArgs._With)
		];
	}
	if (InArgs._Held.Num() > 0) {
		Body->AddSlot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 8.0f})
		[
			PjcDeleteDialogNote(
				FString::Printf(
					TEXT("%s of them failed to delete last time: %s."),
					*PjcFormat::Count(InArgs._Held.Num()),
					*InArgs._Held[0]
				),
				TEXT("ProjectCleaner.Color.Yellow")
			)
		];
	}
	const bool bVersionControl = UPjcSubsystem::ProjectHasVersionControl();
	const TCHAR* VcsNote = bVersionControl ? PjcDeleteDialogVcsFound : PjcDeleteDialogVcsMissing;
	const FName VcsColor = bVersionControl ? TEXT("ProjectCleaner.Color.Ok") : TEXT("ProjectCleaner.Color.Yellow");
	Body->AddSlot().AutoHeight()
	[
		PjcDeleteDialogNote(VcsNote, VcsColor)
	];

	ChildSlot
	[
		SNew(SBorder).BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.GroupBorder"))).Padding(0.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot().Padding(FMargin {16.0f, 14.0f, 16.0f, 10.0f})
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
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin {0.0f, 0.0f, 8.0f, 0.0f})
				[
					SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
					.ButtonStyle(&PjcShim::GetButtonStyle427(false))
					.ForegroundColor(FSlateColor::UseForeground())
#endif
					.ContentPadding(FMargin {14.0f, 3.0f})
					.OnClicked_Lambda([this]() {
						if (const TSharedPtr<SWindow> Owner = Window.Pin()) Owner->RequestDestroyWindow();
						return FReply::Handled();
					})
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("Cancel")))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
					.ButtonStyle(&PjcShim::GetButtonStyle427(true))
					.ForegroundColor(FSlateColor::UseForeground())
#endif
					.ButtonColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.UnusedFill"))
					.ContentPadding(FMargin {14.0f, 3.0f})
					.OnClicked_Lambda([this]() {
						bConfirmed = true;
						if (const TSharedPtr<SWindow> Owner = Window.Pin()) Owner->RequestDestroyWindow();
						return FReply::Handled();
					})
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("Delete"))).ColorAndOpacity(FLinearColor::White)
					]
				]
			]
		]
	];
	// clang-format on
}

bool SPjcDeleteDialog::Confirm(const TArray<FAssetData>& Selected, const TArray<FAssetData>& With, const TArray<FString>& Held) {
	if (Selected.Num() == 0) return false;

	// clang-format off
	const TSharedRef<SWindow> DialogWindow = SNew(SWindow)
		.Title(GetTitle(Selected.Num() + With.Num()))
		.ClientSize(FVector2D {560.0f, With.Num() > 0 ? 560.0f : 420.0f})
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.SizingRule(ESizingRule::UserSized);
	// clang-format on
	const TSharedRef<SPjcDeleteDialog> Dialog = SNew(SPjcDeleteDialog).Selected(Selected).With(With).Held(Held).Window(DialogWindow);
	DialogWindow->SetContent(Dialog);

	FSlateApplication::Get().AddModalWindow(DialogWindow, FSlateApplication::Get().GetActiveTopLevelWindow());
	return Dialog->bConfirmed;
}

FText SPjcDeleteDialog::GetTitle(const int32 NumAssets) {
	return FText::FromString(FString::Printf(TEXT("Delete %s asset%s"), *PjcFormat::Count(NumAssets), NumAssets == 1 ? TEXT("") : TEXT("s")));
}

TSharedRef<SWidget> SPjcDeleteDialog::MakeList(const TArray<FAssetData>& Assets) {
	const TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
	for (int32 Index = 0; Index < Assets.Num() && Index < PjcDeleteDialogMaxListed; ++Index) {
		const FAssetData& Asset = Assets[Index];
		// clang-format off
		List->AddSlot().AutoHeight().Padding(FMargin {0.0f, 1.0f})
		[
			SNew(SHorizontalBox).ToolTipText(FText::FromName(Asset.PackageName))
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(STextBlock).Text(FText::FromName(Asset.AssetName))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin {12.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock)
				.Text(FText::FromString(PjcFormat::Size(UPjcSubsystem::GetAssetSize(Asset))))
				.Font(PjcWidgets::GetFontMono(9))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		];
		// clang-format on
	}
	if (Assets.Num() > PjcDeleteDialogMaxListed) {
		// clang-format off
		List->AddSlot().AutoHeight().Padding(FMargin {0.0f, 2.0f})
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("and %s more"), *PjcFormat::Count(Assets.Num() - PjcDeleteDialogMaxListed))))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		];
		// clang-format on
	}
	// clang-format off
	return SNew(SBox).MaxDesiredHeight(160.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				List
			]
		];
	// clang-format on
}
