// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/PjcWidgets.h"
#include "PjcShim.h"
#include "PjcStyles.h"
// Engine Headers
#include "AssetRegistry/AssetData.h"
#include "AssetToolsModule.h"
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
			const TSharedPtr<IAssetTypeActions> Actions
				= FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().GetAssetTypeActionsForClass(AssetClass).Pin();
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
		Lines->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Title)).Font(FPjcStyles::GetFont("Bold", 9))];
		if (!Text.IsEmpty()) {
			Lines->AddSlot().AutoHeight().Padding(
				FMargin {0.0f, 3.0f, 0.0f, 0.0f}
			)[SNew(STextBlock).Text(FText::FromString(Text)).WrapTextAt(280.0f)];
		}
		if (!More.IsEmpty()) {
			Lines->AddSlot().AutoHeight().Padding(
				FMargin {0.0f, 5.0f, 0.0f, 0.0f}
			)[SNew(STextBlock).Text(FText::FromString(More)).ColorAndOpacity(FSlateColor::UseSubduedForeground())];
		}
		return SNew(SToolTip)[Lines];
	}

	FLinearColor GetColor(const FName Name) {
		return FPjcStyles::Get().GetColor(Name);
	}

	FSlateFontInfo GetFontMono(const int32 Size) {
		return FCoreStyle::GetDefaultFontStyle("Mono", Size);
	}

	TSharedRef<SWidget> MakeCard(const TSharedRef<SWidget>& Content, const FMargin& Padding) {
		return SNew(SBorder).BorderImage(PjcShim::GetBrushCard()).Padding(Padding)[Content];
	}

	TSharedRef<SWidget> MakeCardTitle(const FString& Title) {
		return SNew(STextBlock)
			.Text(FText::FromString(Title.ToUpper()))
			.Font(FPjcStyles::GetFont("Bold", 8))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground());
	}

	TSharedRef<SWidget> MakeSwatch(const FLinearColor& Color, const float Size) {
		return SNew(SBox).WidthOverride(Size).HeightOverride(
			Size
		)[SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(Color)];
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
				Fill->AddSlot().FillWidth(
					Share
				)[SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(Segment.Color).ToolTip(Segment.ToolTip)];
			}
			if (Scale > Sum) {
				Fill->AddSlot().FillWidth(static_cast<float>((Scale - Sum) / Scale))[SNew(SSpacer)];
			}
		}

		// clang-format off
		return SNew(SBox)
			.HeightOverride(Height)
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
}	// namespace PjcWidgets
