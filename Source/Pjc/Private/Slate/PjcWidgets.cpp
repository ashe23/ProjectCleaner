// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/PjcWidgets.h"
#include "PjcUnused.h"
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

	FSetColors GetSetColors(const EPjcAssetSet Set) {
		const auto Hex = [](const TCHAR* Value) {
			return FLinearColor {FColor::FromHex(Value)};
		};
		const FLinearColor UsedBar = GetColor("ProjectCleaner.Color.UsedFill");
		const FLinearColor ReviewBar = GetColor("ProjectCleaner.Color.ReviewFill");
		const FLinearColor UnusedBar = GetColor("ProjectCleaner.Color.UnusedFill");
		switch (Set) {
			case EPjcAssetSet::Excluded : return FSetColors {Hex(TEXT("#6B7480")), Hex(TEXT("#4F555D")), Hex(TEXT("#E6E8EA")), Hex(TEXT("#6B7480"))};
			case EPjcAssetSet::Used :	  return FSetColors {UsedBar, Hex(TEXT("#2A5F5A")), Hex(TEXT("#D8F1EE")), Hex(TEXT("#2C766F"))};
			case EPjcAssetSet::Review :	  return FSetColors {ReviewBar, Hex(TEXT("#7A5F26")), Hex(TEXT("#FFF1D6")), Hex(TEXT("#8F6F2A"))};
			case EPjcAssetSet::Unused :
			default :					  return FSetColors {UnusedBar, Hex(TEXT("#7A3236")), Hex(TEXT("#FFE1E2")), Hex(TEXT("#8E3A3F"))};
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

	FSlateIcon GetMenuIcon(const EPjcMenuIcon Icon) {
		switch (Icon) {
			case EPjcMenuIcon::Inspect :  return PjcShim::GetEditorIcon(TEXT("Icons.Search"), TEXT("Symbols.SearchGlass"));
			case EPjcMenuIcon::Browse :	  return PjcShim::GetEditorIcon(TEXT("Icons.BrowseContent"), TEXT("SystemWideCommands.FindInContentBrowser"));
			case EPjcMenuIcon::Delete :	  return PjcShim::GetEditorIcon(TEXT("Icons.Delete"), TEXT("ContentBrowser.AssetActions.Delete"));
			case EPjcMenuIcon::Exclude :  return PjcShim::GetEditorIcon(TEXT("Icons.Hidden"), TEXT("Level.NotVisibleIcon16x"));
			case EPjcMenuIcon::Include :  return PjcShim::GetEditorIcon(TEXT("Icons.Visible"), TEXT("Level.VisibleIcon16x"));
			case EPjcMenuIcon::Explorer : return PjcShim::GetEditorIcon(TEXT("Icons.FolderOpen"), TEXT("ContentBrowser.AssetTreeFolderOpen"));
			case EPjcMenuIcon::SizeMap :  return PjcShim::GetEditorIcon(TEXT("ContentBrowser.SizeMap"), TEXT(""));
			case EPjcMenuIcon::Expand :	  return PjcShim::GetEditorIcon(TEXT("Icons.ChevronDown"), TEXT("TreeArrow_Expanded"));
			case EPjcMenuIcon::Collapse : return PjcShim::GetEditorIcon(TEXT("Icons.ChevronRight"), TEXT("TreeArrow_Collapsed"));
			case EPjcMenuIcon::Reset :	  return PjcShim::GetEditorIcon(TEXT("Icons.Refresh"), TEXT("Icons.Refresh"));
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
