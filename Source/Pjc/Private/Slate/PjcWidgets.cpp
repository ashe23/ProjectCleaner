// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/PjcWidgets.h"
#include "PjcUnused.h"
#include "PjcFormat.h"
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
#include "Widgets/Input/SButton.h"
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
		Type.Color = GetColor("ProjectCleaner.Color.Muted");
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

	FSetColors GetSetColors(const EPjcAssetSet Set) {
		// the set's palette color for bars and folder tints; size map boxes a shade darker, so white names read on them
		const auto Make = [](const FName Fill) {
			FSetColors Colors;
			Colors.Bar = GetColor(Fill);
			Colors.Box = Colors.Bar * FLinearColor {0.5f, 0.5f, 0.5f, 1.0f};
			Colors.BoxText = GetColor("ProjectCleaner.Color.Strong");
			Colors.Tint = Colors.Bar;
			return Colors;
		};
		switch (Set) {
			case EPjcAssetSet::Excluded : return Make("ProjectCleaner.Color.ExcludedFill");
			case EPjcAssetSet::Used :	  return Make("ProjectCleaner.Color.UsedFill");
			case EPjcAssetSet::Review :	  return Make("ProjectCleaner.Color.ReviewFill");
			case EPjcAssetSet::Unused :
			default :					  return Make("ProjectCleaner.Color.UnusedFill");
		}
	}

	FString GetSetLabel(const EPjcAssetSet Set) {
		switch (Set) {
			case EPjcAssetSet::Excluded : return TEXT("Excluded");
			case EPjcAssetSet::Used :	  return TEXT("Used");
			case EPjcAssetSet::Review :	  return TEXT("Review");
			case EPjcAssetSet::Unused :
			default :					  return TEXT("Unused");
		}
	}

	FString GetSetWord(const EPjcAssetSet Set) {
		return Set == EPjcAssetSet::Review ? TEXT("to review") : GetSetLabel(Set).ToLower();
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
	) {
		const FLinearColor Recess = GetColor("ProjectCleaner.Color.Recess");
		const FLinearColor Line = GetColor("ProjectCleaner.Color.Line");
		const FLinearColor Grey = GetColor("ProjectCleaner.Color.Hover");
		const FLinearColor Fill = FMath::Lerp(Recess, Color, 0.22f);
		// clang-format off
		return SNew(SButton)
			.ButtonStyle(&PjcShim::GetStyle().GetWidgetStyle<FButtonStyle>(TEXT("NoBorder")))
#if ENGINE_MAJOR_VERSION < 5
			.ForegroundColor(FSlateColor::UseForeground())
#endif
			.ContentPadding(FMargin {0.0f})
			.ToolTip(MakeToolTip(Label, ToolTip, More))
			.OnClicked_Lambda([OnClick]() {
				OnClick();
				return FReply::Handled();
			})
			[
				SNew(SBorder)
				.BorderImage(FPjcStyles::Get().GetBrush("ProjectCleaner.Pill"))
				.BorderBackgroundColor_Lambda([IsOn, Color, Line]() { return FSlateColor {IsOn() ? Color : Line}; })
				.Padding(FMargin {1.0f})
				[
					SNew(SBorder)
					.BorderImage(FPjcStyles::Get().GetBrush("ProjectCleaner.Pill"))
					.BorderBackgroundColor_Lambda([IsOn, Fill, Recess]() { return FSlateColor {IsOn() ? Fill : Recess}; })
					.Padding(FMargin {bDot ? 8.0f : 10.0f, 2.0f, 10.0f, 2.0f})
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 6.0f, 0.0f})
						[
							SNew(SBox).WidthOverride(7.0f).HeightOverride(7.0f).Visibility(bDot ? EVisibility::Visible : EVisibility::Collapsed)
							[
								SNew(SImage)
								.Image(FPjcStyles::Get().GetBrush("ProjectCleaner.Dot"))
								.ColorAndOpacity_Lambda([IsGrey, Color, Grey]() { return FSlateColor {IsGrey() ? Grey : Color}; })
							]
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
						[
							SNew(SBox).Clipping(EWidgetClipping::ClipToBounds)
							[
								SNew(STextBlock)
								.Text(FText::FromString(Label))
								.ColorAndOpacity_Lambda([IsGrey]() { return IsGrey() ? FSlateColor::UseSubduedForeground() : FSlateColor::UseForeground(); })
							]
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {7.0f, 0.0f, 0.0f, 0.0f})
						[
							SNew(STextBlock)
							.Text(FText::FromString(PjcFormat::Count(Num)))
							.Font(GetFontMono(8))
							.ColorAndOpacity_Lambda([IsOn]() { return IsOn() ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground(); })
						]
					]
				]
			];
		// clang-format on
	}

	TSharedRef<SWidget> MakeFilterHeading(const TAttribute<FText>& Text, const TAttribute<FText>& ToolTip) {
		return SNew(STextBlock)
			.Text(Text)
			.ToolTipText(ToolTip)
			.Font(FPjcStyles::GetFont("Bold", 8))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground());
	}

	FSlateIcon GetMenuIcon(const EPjcMenuIcon Icon) {
		switch (Icon) {
			case EPjcMenuIcon::Inspect :  return FPjcStyles::GetIcon("ProjectCleaner.Menu.Inspect");
			case EPjcMenuIcon::Browse :	  return FPjcStyles::GetIcon("ProjectCleaner.Menu.Browse");
			case EPjcMenuIcon::Delete :	  return FPjcStyles::GetIcon("ProjectCleaner.Menu.Delete");
			case EPjcMenuIcon::Exclude :  return FPjcStyles::GetIcon("ProjectCleaner.Menu.Exclude");
			case EPjcMenuIcon::Include :  return FPjcStyles::GetIcon("ProjectCleaner.Menu.Include");
			case EPjcMenuIcon::Explorer : return FPjcStyles::GetIcon("ProjectCleaner.Menu.Explorer");
			case EPjcMenuIcon::SizeMap :  return FPjcStyles::GetIcon("ProjectCleaner.Menu.SizeMap");
			case EPjcMenuIcon::Expand :	  return FPjcStyles::GetIcon("ProjectCleaner.Menu.Expand");
			case EPjcMenuIcon::Collapse : return FPjcStyles::GetIcon("ProjectCleaner.Menu.Collapse");
			case EPjcMenuIcon::Reset :	  return FPjcStyles::GetIcon("ProjectCleaner.Menu.Reset");
			default :					  return FSlateIcon {};
		}
	}

	void AddMenuEntry(FMenuBuilder& Menu, const EPjcMenuIcon Icon, const FString& Label, const FString& ToolTip, TFunction<void()> OnClick) {
		AddMenuEntry(Menu, Icon, Label, ToolTip, true, MoveTemp(OnClick));
	}

	void AddMenuEntry(
		FMenuBuilder& Menu,
		const EPjcMenuIcon Icon,
		const FString& Label,
		const FString& ToolTip,
		const bool bEnabled,
		TFunction<void()> OnClick
	) {
		const FCanExecuteAction CanClick = FCanExecuteAction::CreateLambda([bEnabled]() {
			return bEnabled;
		});
		const FUIAction Action {FExecuteAction::CreateLambda(MoveTemp(OnClick)), CanClick};
		Menu.AddMenuEntry(FText::FromString(Label), FText::FromString(ToolTip), GetMenuIcon(Icon), Action);
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
