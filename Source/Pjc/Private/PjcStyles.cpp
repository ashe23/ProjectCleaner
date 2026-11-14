// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcStyles.h"
#include "PjcConstants.h"
// Engine Headers
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
#include "Brushes/SlateBoxBrush.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateNoResource.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Styling/SlateTypes.h"
#if ENGINE_MAJOR_VERSION >= 5
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/StyleColors.h"
#endif

TSharedPtr<FSlateStyleSet> FPjcStyles::StyleInstance = nullptr;

void FPjcStyles::Initialize() {
	if (!StyleInstance.IsValid()) {
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FPjcStyles::Shutdown() {
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

void FPjcStyles::ReloadTextures() {
	if (FSlateApplication::IsInitialized()) {
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FPjcStyles::Get() {
	return *StyleInstance;
}

FName FPjcStyles::GetStyleSetName() {
	return PjcConstants::ModulePjcStylesName;
}

FSlateFontInfo FPjcStyles::GetFont(const FString& FontType, const uint32 FontSize) {
	return FCoreStyle::GetDefaultFontStyle(*FontType, FontSize);
}

FSlateIcon FPjcStyles::GetIcon(const FString& IconSpecifier) {
	return FSlateIcon(GetStyleSetName(), FName {*IconSpecifier});
}

#define IMAGE_BRUSH(RelativePath, ...) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )

TSharedRef<FSlateStyleSet> FPjcStyles::Create() {
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet(PjcConstants::ModulePjcStylesName));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin(PjcConstants::ModulePjcName.ToString())->GetBaseDir() / TEXT("Resources"));

	// cmds icons big
	Style->Set("ProjectCleaner.TabMain", new IMAGE_BRUSH(TEXT("IconBin40"), FVector2D {40.0f, 40.0f}));

	// cmds icons small
	Style->Set("ProjectCleaner.TabMain.Small", new IMAGE_BRUSH(TEXT("IconBin20"), FVector2D {20.0f, 20.0f}));

	// icons
	Style->Set("ProjectCleaner.Icon.Bin16", new IMAGE_BRUSH(TEXT("IconBin16"), FVector2D {16.0f, 16.0f}));
	Style->Set("ProjectCleaner.Icon.Warning32", new IMAGE_BRUSH(TEXT("IconWarning32"), FVector2D {32.0f, 32.0f}));
	Style->Set("ProjectCleaner.Icon.PieChart16", new IMAGE_BRUSH(TEXT("IconPieChart16"), FVector2D {16.0f, 16.0f}));
	Style->Set("ProjectCleaner.Icon.Graph16", new IMAGE_BRUSH(TEXT("IconGraph20"), FVector2D {16.0f, 16.0f}));
	// 32 px images drawn at 16: sharp on high DPI screens
	// Issues tab: the yellow warning icon (warnings, not errors), 2026-11-14
	Style->Set("ProjectCleaner.Icon.Issues16", new IMAGE_BRUSH(TEXT("IconWarning32"), FVector2D {16.0f, 16.0f}));
	Style->Set("ProjectCleaner.Icon.Help16", new IMAGE_BRUSH(TEXT("IconHelp32"), FVector2D {16.0f, 16.0f}));
	// Scan and Clean project buttons, as in 2.0
	Style->Set("ProjectCleaner.Icon.Scan20", new IMAGE_BRUSH(TEXT("IconRefresh128"), FVector2D {20.0f, 20.0f}));
	Style->Set("ProjectCleaner.Icon.Clean20", new IMAGE_BRUSH(TEXT("IconBinRed128"), FVector2D {20.0f, 20.0f}));
	// marks on circular links of the Inspector graph: white, tinted when drawn (sources: IconLoop.svg, IconPair.svg)
	Style->Set("ProjectCleaner.Graph.Disc", new IMAGE_BRUSH(TEXT("IconDisc64"), FVector2D {16.0f, 16.0f}));
	Style->Set("ProjectCleaner.Graph.Loop", new IMAGE_BRUSH(TEXT("IconLoop64"), FVector2D {16.0f, 16.0f}));
	Style->Set("ProjectCleaner.Graph.Pair", new IMAGE_BRUSH(TEXT("IconPair64"), FVector2D {16.0f, 16.0f}));

	// pallet colors
	Style->Set("ProjectCleaner.Color.Yellow", FLinearColor {FColor::FromHex(TEXT("#FFCB77"))});
	Style->Set("ProjectCleaner.Color.Red", FLinearColor {FColor::FromHex(TEXT("#FE6D73"))});
	Style->Set("ProjectCleaner.Color.Gray", FLinearColor {FColor::FromHex(TEXT("#adb5bd"))});

	// asset status: bar fills, text accents
	// used and shipped: one light green (user, 2026-11-14)
	Style->Set("ProjectCleaner.Color.UsedFill", FLinearColor {FColor::FromHex(TEXT("#3F9B68"))});
	Style->Set("ProjectCleaner.Color.EditorFill", FLinearColor {FColor::FromHex(TEXT("#3D6DB3"))});
	Style->Set("ProjectCleaner.Color.UnusedFill", FLinearColor {FColor::FromHex(TEXT("#C4484D"))});
	Style->Set("ProjectCleaner.Color.ReviewFill", FLinearColor {FColor::FromHex(TEXT("#8F6F2A"))});
	Style->Set("ProjectCleaner.Color.Review", FLinearColor {FColor::FromHex(TEXT("#E0B050"))});
	Style->Set("ProjectCleaner.Color.Ok", FLinearColor {FColor::FromHex(TEXT("#6FCF7A"))});
	Style->Set("ProjectCleaner.Color.Track", FLinearColor {FColor::FromHex(TEXT("#2D3136"))});
	Style->Set("ProjectCleaner.Color.Line", FLinearColor {FColor::FromHex(TEXT("#383B40"))});
	Style->Set("ProjectCleaner.Color.Accent", FLinearColor {FColor::FromHex(TEXT("#2F7DE1"))});
	Style->Set("ProjectCleaner.Color.ShipFill", FLinearColor {FColor::FromHex(TEXT("#3F9B68"))});
	Style->Set("ProjectCleaner.Color.KeepFill", FLinearColor {FColor::FromHex(TEXT("#56606B"))});
	Style->Set("ProjectCleaner.Color.FolderTint", FLinearColor {FColor::FromHex(TEXT("#C9A55A"))});

#if ENGINE_MAJOR_VERSION >= 5
	// tool pages: dark theme background behind the panels and cards, 4.27 sets its own below
	Style->Set("ProjectCleaner.Page", new FSlateColorBrush {FStyleColors::Background});
	// cards of the Overview, 4.27 takes the editor group border (PjcShim::GetBrushCard)
	Style->Set("ProjectCleaner.Card", new FSlateRoundedBoxBrush(FStyleColors::Panel, 4.0f, FLinearColor {FColor::FromHex(TEXT("#383B40"))}, 1.0f));
	// Inspector graph boxes, white: drawn tinted. 4.27 draws square boxes.
	Style->Set("ProjectCleaner.Graph.Node", new FSlateRoundedBoxBrush(FLinearColor::White, 3.0f));
	Style->Set("ProjectCleaner.Graph.Pill", new FSlateRoundedBoxBrush(FLinearColor::White, 23.0f));
#endif

	// rows that react to the mouse
	// clang-format off
	const FButtonStyle RowButton = FButtonStyle {}
		.SetNormal(FSlateNoResource {})
		.SetHovered(FSlateColorBrush {FLinearColor {1.0f, 1.0f, 1.0f, 0.04f}})
		.SetPressed(FSlateColorBrush {FLinearColor {1.0f, 1.0f, 1.0f, 0.07f}})
		.SetNormalPadding(FMargin {0.0f})
		.SetPressedPadding(FMargin {0.0f});
	// clang-format on
	Style->Set("ProjectCleaner.Button.Row", RowButton);

	// rounded corners as the UE5 widgets have them
#if ENGINE_MAJOR_VERSION >= 5
	const auto Round = [](const TCHAR* Value) {
		return FSlateRoundedBoxBrush {FLinearColor {FColor::FromHex(Value)}, 4.0f};
	};
#else
	// UE 4.27 has no rounded box brush: a white 16 px box with 4 px corners, nine-sliced and tinted
	const FString RoundedBox = Style->RootToContentDir(TEXT("RoundedBox16"), TEXT(".png"));
	const auto Round = [&RoundedBox](const TCHAR* Value) {
		return FSlateBoxBrush {RoundedBox, FMargin {4.0f / 16.0f}, FLinearColor {FColor::FromHex(Value)}};
	};
#endif

	// Clean project in the scan bar: dark red, so its red bin icon stands out (confirm buttons stay solid red)
	// clang-format off
	Style->Set("ProjectCleaner.Button.DangerQuiet", FButtonStyle {}
		.SetNormal(Round(TEXT("#4A2427")))
		.SetHovered(Round(TEXT("#5C2B2F")))
		.SetPressed(Round(TEXT("#3A1D20")))
		.SetDisabled(Round(TEXT("#2E2224")))
		.SetNormalPadding(FMargin {2.0f})
		.SetPressedPadding(FMargin {2.0f}));
	// clang-format on

#if ENGINE_MAJOR_VERSION < 5
	// UE 4.27 widgets in the colors of the UE5 ones (PjcShim)
	const auto Hex = [](const TCHAR* Value) {
		return FSlateColorBrush {FLinearColor {FColor::FromHex(Value)}};
	};
	const FLinearColor Foreground {FColor::FromHex(TEXT("#C0C0C0"))};
	Style->Set("ProjectCleaner.Page", new FSlateColorBrush {FLinearColor {FColor::FromHex(TEXT("#151515"))}});
	Style->Set("ProjectCleaner.Panel", new FSlateColorBrush {FLinearColor {FColor::FromHex(TEXT("#242424"))}});
	Style->Set("ProjectCleaner.PanelDark", new FSlateColorBrush {FLinearColor {FColor::FromHex(TEXT("#1A1A1A"))}});
	Style->Set("ProjectCleaner.Header", new FSlateColorBrush {FLinearColor {FColor::FromHex(TEXT("#2F2F2F"))}});

	const FMargin ButtonPadding {2.0f};
	// clang-format off
	Style->Set("ProjectCleaner.Button", FButtonStyle {}
		.SetNormal(Round(TEXT("#383838")))
		.SetHovered(Round(TEXT("#575757")))
		.SetPressed(Round(TEXT("#1F1F1F")))
		.SetDisabled(Round(TEXT("#2A2A2A")))
		.SetNormalPadding(ButtonPadding)
		.SetPressedPadding(ButtonPadding));
	// drawn tinted (ButtonColorAndOpacity)
	Style->Set("ProjectCleaner.Button.Colored", FButtonStyle {}
		.SetNormal(FSlateColorBrush {FLinearColor {0.7f, 0.7f, 0.7f}})
		.SetHovered(FSlateColorBrush {FLinearColor::White})
		.SetPressed(FSlateColorBrush {FLinearColor {0.45f, 0.45f, 0.45f}})
		.SetDisabled(FSlateColorBrush {FLinearColor {0.2f, 0.2f, 0.2f}})
		.SetNormalPadding(ButtonPadding)
		.SetPressedPadding(ButtonPadding));
	Style->Set("ProjectCleaner.Button.Primary", FButtonStyle {}
		.SetNormal(Round(TEXT("#0070E0")))
		.SetHovered(Round(TEXT("#0E86FF")))
		.SetPressed(Round(TEXT("#0050A0")))
		.SetDisabled(Round(TEXT("#2C323A")))
		.SetNormalPadding(ButtonPadding)
		.SetPressedPadding(ButtonPadding));
	Style->Set("ProjectCleaner.Button.Danger", FButtonStyle {}
		.SetNormal(Round(TEXT("#DA524E")))
		.SetHovered(Round(TEXT("#ED6966")))
		.SetPressed(Round(TEXT("#B33F3F")))
		.SetDisabled(Round(TEXT("#3A2A2A")))
		.SetNormalPadding(ButtonPadding)
		.SetPressedPadding(ButtonPadding));
	Style->Set("ProjectCleaner.Toggle", FCheckBoxStyle {}
		.SetCheckBoxType(ESlateCheckBoxType::ToggleButton)
		.SetUncheckedImage(FSlateNoResource {})
		.SetUncheckedHoveredImage(Round(TEXT("#383838")))
		.SetUncheckedPressedImage(Round(TEXT("#383838")))
		.SetCheckedImage(Round(TEXT("#0070E0")))
		.SetCheckedHoveredImage(Round(TEXT("#0E86FF")))
		.SetCheckedPressedImage(Round(TEXT("#383838"))));
	Style->Set("ProjectCleaner.Row", FTableRowStyle {FCoreStyle::Get().GetWidgetStyle<FTableRowStyle>("TableView.Row")}
		.SetActiveBrush(Hex(TEXT("#0070E0")))
		.SetActiveHoveredBrush(Hex(TEXT("#0E86FF")))
		.SetInactiveBrush(Hex(TEXT("#40576F")))
		.SetInactiveHoveredBrush(Hex(TEXT("#40576F")))
		.SetSelectedTextColor(FLinearColor::White));
	Style->Set("ProjectCleaner.Progress", FProgressBarStyle {FCoreStyle::Get().GetWidgetStyle<FProgressBarStyle>("ProgressBar")}
		.SetBackgroundImage(Hex(TEXT("#1A1A1A")))
		.SetFillImage(Hex(TEXT("#0070E0"))));
	// clang-format on

	const auto MakeDarkBox = [&Hex, &Foreground](const FEditableTextBoxStyle& Base) {
		// clang-format off
		return FEditableTextBoxStyle {Base}
			.SetBackgroundImageNormal(Hex(TEXT("#0F0F0F")))
			.SetBackgroundImageHovered(Hex(TEXT("#0F0F0F")))
			.SetBackgroundImageFocused(Hex(TEXT("#0F0F0F")))
			.SetBackgroundImageReadOnly(Hex(TEXT("#1A1A1A")))
			.SetForegroundColor(Foreground)
			.SetReadOnlyForegroundColor(Foreground);
		// clang-format on
	};
	Style->Set("ProjectCleaner.TextBox", MakeDarkBox(FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox")));
	FSearchBoxStyle SearchBox = FCoreStyle::Get().GetWidgetStyle<FSearchBoxStyle>("SearchBox");
	SearchBox.SetTextBoxStyle(MakeDarkBox(SearchBox.TextBoxStyle));
	SearchBox.GlassImage.TintColor = FSlateColor {Foreground};
	SearchBox.ClearImage.TintColor = FSlateColor {Foreground};
	Style->Set("ProjectCleaner.SearchBox", SearchBox);
#endif

	return Style;
}

#undef IMAGE_BRUSH
