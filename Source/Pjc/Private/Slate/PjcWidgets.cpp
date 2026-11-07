// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/PjcWidgets.h"
#include "PjcShim.h"
#include "PjcStyles.h"
// Engine Headers
#include "AssetRegistry/AssetData.h"
#include "AssetToolsModule.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IAssetTypeActions.h"
#include "Misc/Paths.h"
#include "Styling/SlateIconFinder.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Text/STextBlock.h"

namespace PjcWidgets
{
	const FAssetType& GetAssetType(const FAssetData& Asset) {
		static TMap<FName, FAssetType> Types;
		const FName ClassName = PjcShim::GetAssetClassName(Asset);
		if (const FAssetType* Found = Types.Find(ClassName)) return *Found;

		// as the Content Browser shows the type: its name, color and icon
		FAssetType Type;
		Type.Label = FPaths::GetExtension(ClassName.ToString()).IsEmpty() ? ClassName.ToString() : FPaths::GetExtension(ClassName.ToString());
		Type.Color = FLinearColor {FColor::FromHex(TEXT("#8A9099"))};
		const UClass* AssetClass = Asset.GetClass();
		if (AssetClass) {
			IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
			const TSharedPtr<IAssetTypeActions> Actions = AssetTools.GetAssetTypeActionsForClass(AssetClass).Pin();
			if (Actions.IsValid()) {
				Type.Label = Actions->GetName().ToString();
				Type.Color = FLinearColor {Actions->GetTypeColor()};
			}
			Type.Icon = FSlateIconFinder::FindIconBrushForClass(AssetClass);
		}
		return Types.Add(ClassName, Type);
	}

	TSharedRef<SToolTip> MakeToolTip(const FString& Title, const FString& Text, const FString& More) {
		const TSharedRef<SVerticalBox> Lines = SNew(SVerticalBox);
		// clang-format off
		Lines->AddSlot().AutoHeight()
		[
			SNew(STextBlock).Text(FText::FromString(Title)).Font(FPjcStyles::GetFont("Bold", 9))
		];
		// clang-format on
		if (!Text.IsEmpty()) {
			// clang-format off
			Lines->AddSlot().AutoHeight().Padding(FMargin {0.0f, 3.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock).Text(FText::FromString(Text)).WrapTextAt(280.0f)
			];
			// clang-format on
		}
		if (!More.IsEmpty()) {
			// clang-format off
			Lines->AddSlot().AutoHeight().Padding(FMargin {0.0f, 5.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock).Text(FText::FromString(More)).ColorAndOpacity(FSlateColor::UseSubduedForeground())
			];
			// clang-format on
		}
		// clang-format off
		return SNew(SToolTip)
			[
				Lines
			];
		// clang-format on
	}

	FLinearColor GetColor(const FName Name) {
		return FPjcStyles::Get().GetColor(Name);
	}

	FSlateFontInfo GetFontMono(const int32 Size) {
		return FCoreStyle::GetDefaultFontStyle("Mono", Size);
	}

	TSharedRef<SWidget> MakeCard(const TSharedRef<SWidget>& Content, const FMargin& Padding) {
		// clang-format off
		return SNew(SBorder).BorderImage(PjcShim::GetBrushCard()).Padding(Padding)
			[
				Content
			];
		// clang-format on
	}

	TSharedRef<SWidget> MakeCardTitle(const FString& Title) {
		// clang-format off
		return SNew(STextBlock)
			.Text(FText::FromString(Title.ToUpper()))
			.Font(FPjcStyles::GetFont("Bold", 8))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground());
		// clang-format on
	}

	TSharedRef<SWidget> MakeSwatch(const FLinearColor& Color, const float Size) {
		// clang-format off
		return SNew(SBox).WidthOverride(Size).HeightOverride(Size)
			[
				SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(Color)
			];
		// clang-format on
	}

	TSharedRef<SWidget> MakeBar(const TArray<FBarSegment>& Segments, const float Height, const double Total) {
		double Sum = 0.0;
		for (const auto& Segment : Segments) {
			Sum += FMath::Max(Segment.Value, 0.0);
		}
		const double Scale = FMath::Max(Sum, Total);

		const TSharedRef<SHorizontalBox> Fill = SNew(SHorizontalBox);
		if (Scale > 0.0) {
			for (const auto& Segment : Segments) {
				if (Segment.Value <= 0.0) continue;

				// a tiny share stays visible
				const float Share = FMath::Max(static_cast<float>(Segment.Value / Scale), 0.003f);
				// clang-format off
				Fill->AddSlot().FillWidth(Share)
				[
					SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(Segment.Color).ToolTip(Segment.ToolTip)
				];
				// clang-format on
			}
			if (Scale > Sum) {
				// clang-format off
				Fill->AddSlot().FillWidth(static_cast<float>((Scale - Sum) / Scale))
				[
					SNew(SSpacer)
				];
				// clang-format on
			}
		}

		// clang-format off
		return SNew(SBox).HeightOverride(Height)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(GetColor("ProjectCleaner.Color.Track"))
				]
				+ SOverlay::Slot()
				[
					Fill
				]
			];
		// clang-format on
	}

	void AddMenuEntry(FMenuBuilder& Menu, const FString& Label, const FString& ToolTip, TFunction<void()> OnClick) {
		AddMenuEntry(Menu, Label, ToolTip, true, MoveTemp(OnClick));
	}

	void AddMenuEntry(FMenuBuilder& Menu, const FString& Label, const FString& ToolTip, const bool bEnabled, TFunction<void()> OnClick) {
		const FCanExecuteAction CanClick = FCanExecuteAction::CreateLambda([bEnabled]() {
			return bEnabled;
		});
		const FUIAction Action {FExecuteAction::CreateLambda(MoveTemp(OnClick)), CanClick};
		Menu.AddMenuEntry(FText::FromString(Label), FText::FromString(ToolTip), FSlateIcon {}, Action);
	}

	void AddMenuToggle(FMenuBuilder& Menu, const FString& Label, const FString& ToolTip, const bool* bChecked, TFunction<void()> OnClick) {
		const FIsActionChecked IsChecked = FIsActionChecked::CreateLambda([bChecked]() {
			return bChecked && *bChecked;
		});
		const FUIAction Action {FExecuteAction::CreateLambda(MoveTemp(OnClick)), FCanExecuteAction {}, IsChecked};
		const FText Tip = FText::FromString(ToolTip);
		Menu.AddMenuEntry(FText::FromString(Label), Tip, FSlateIcon {}, Action, NAME_None, EUserInterfaceActionType::ToggleButton);
	}
}	// namespace PjcWidgets
