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

	// palette (proto/style): one set of colors for every engine. Surfaces, text and blue follow the UE5 editor theme, the
	// status colors are the plugin's own. Asset types keep the engine's class colors.
	const auto Color = [](const TCHAR* Value) {
		return FLinearColor {FColor::FromHex(Value)};
	};
	const FLinearColor ColorText = Color(TEXT("#C0C0C0"));
	const FLinearColor ColorMuted = Color(TEXT("#8A9099"));
	const FLinearColor ColorLink = Color(TEXT("#4D9BFF"));
	const FLinearColor ColorWarning = Color(TEXT("#FFCB77"));
	const FLinearColor ColorDangerIcon = Color(TEXT("#FF6B70"));
	const FLinearColor ColorUsed = Color(TEXT("#3F9B68"));
	const FLinearColor ColorFolder = Color(TEXT("#C9A55A"));
	// surfaces
	Style->Set("ProjectCleaner.Color.Page", Color(TEXT("#151515")));
	Style->Set("ProjectCleaner.Color.Panel", Color(TEXT("#242424")));
	Style->Set("ProjectCleaner.Color.Header", Color(TEXT("#2F2F2F")));
	Style->Set("ProjectCleaner.Color.Recess", Color(TEXT("#1A1A1A")));
	Style->Set("ProjectCleaner.Color.Input", Color(TEXT("#0F0F0F")));
	Style->Set("ProjectCleaner.Color.Hover", Color(TEXT("#383838")));
	Style->Set("ProjectCleaner.Color.Line", Color(TEXT("#383838")));
	Style->Set("ProjectCleaner.Color.Track", Color(TEXT("#383838")));
	Style->Set("ProjectCleaner.Color.Marked", Color(TEXT("#1D2A3A")));
	// text and interaction
	Style->Set("ProjectCleaner.Color.Text", ColorText);
	Style->Set("ProjectCleaner.Color.Strong", Color(TEXT("#FFFFFF")));
	Style->Set("ProjectCleaner.Color.Muted", ColorMuted);
	Style->Set("ProjectCleaner.Color.Gray", ColorMuted);
	Style->Set("ProjectCleaner.Color.Accent", Color(TEXT("#0070E0")));
	Style->Set("ProjectCleaner.Color.AccentHover", Color(TEXT("#0E86FF")));
	Style->Set("ProjectCleaner.Color.Link", ColorLink);
	// meaning
	Style->Set("ProjectCleaner.Color.Ok", Color(TEXT("#6FCF7A")));
	Style->Set("ProjectCleaner.Color.Yellow", ColorWarning);
	Style->Set("ProjectCleaner.Color.Red", Color(TEXT("#FE6D73")));
	Style->Set("ProjectCleaner.Color.Danger", Color(TEXT("#DA524E")));
	Style->Set("ProjectCleaner.Color.DangerText", Color(TEXT("#FFB3B6")));
	Style->Set("ProjectCleaner.Color.DangerIcon", ColorDangerIcon);
	Style->Set("ProjectCleaner.Color.DangerQuiet", Color(TEXT("#4A2427")));
	// asset status: a fill for bars and boxes, a light shade for text, a quiet one for backgrounds. Used and shipped are
	// one light green
	Style->Set("ProjectCleaner.Color.UsedFill", ColorUsed);
	Style->Set("ProjectCleaner.Color.UsedQuiet", Color(TEXT("#20302F")));
	Style->Set("ProjectCleaner.Color.ShipFill", ColorUsed);
	Style->Set("ProjectCleaner.Color.EditorFill", Color(TEXT("#3D6DB3")));
	Style->Set("ProjectCleaner.Color.Editor", Color(TEXT("#7FB2FF")));
	Style->Set("ProjectCleaner.Color.ReviewFill", Color(TEXT("#8F6F2A")));
	Style->Set("ProjectCleaner.Color.Review", Color(TEXT("#E0B050")));
	Style->Set("ProjectCleaner.Color.ReviewQuiet", Color(TEXT("#2E2716")));
	Style->Set("ProjectCleaner.Color.UnusedFill", Color(TEXT("#C4484D")));
	Style->Set("ProjectCleaner.Color.ExcludedFill", Color(TEXT("#6B7480")));
	Style->Set("ProjectCleaner.Color.KeepFill", Color(TEXT("#56606B")));
	Style->Set("ProjectCleaner.Color.FolderTint", ColorFolder);
	// references in the Inspector: hard ones are Muted, editor-only ones Editor
	Style->Set("ProjectCleaner.Color.SoftLink", Color(TEXT("#9B8CF0")));
	Style->Set("ProjectCleaner.Color.Circular", Color(TEXT("#FF8A3D")));
	// why used: reason chips and bars (none: Red, editor: Editor, excluded: Muted, folder labels: Yellow)
	// clang-format off
	Style->Set("ProjectCleaner.Color.Reason.Primary",		Color(TEXT("#4F9DF0")));
	Style->Set("ProjectCleaner.Color.Reason.PrimaryClass",	Color(TEXT("#7A9FD0")));
	Style->Set("ProjectCleaner.Color.Reason.Outside",		Color(TEXT("#9B8CF0")));
	Style->Set("ProjectCleaner.Color.Reason.Source",		Color(TEXT("#E0A85A")));
	Style->Set("ProjectCleaner.Color.Reason.ConfigFolder",	Color(TEXT("#C9A04A")));
	Style->Set("ProjectCleaner.Color.Reason.Localized",		Color(TEXT("#5FC4A8")));
	Style->Set("ProjectCleaner.Color.Reason.Cinematic",		Color(TEXT("#C08457")));
	Style->Set("ProjectCleaner.Color.Reason.GameplayCue",	Color(TEXT("#D679C8")));
	Style->Set("ProjectCleaner.Color.Reason.Splash",		Color(TEXT("#9FC3FF")));
	Style->Set("ProjectCleaner.Color.Reason.Other",			Color(TEXT("#6FA8A2")));
	// clang-format on

	// icon set (proto/style, originals in Media/Icons): white 64 px images, the same in every engine. Widgets draw
	// them in a color; tabs, menus and the toolbar, which the engine draws, get brushes with the color in them
	const auto Icon = [&Style](const TCHAR* Name, const float Size, const FLinearColor& Tint) {
		const FString Path = Style->RootToContentDir(FString {TEXT("Icons/")} + Name, TEXT(".png"));
		return FSlateImageBrush(Path, FVector2D {Size, Size}, Tint);
	};
	const FLinearColor White = FLinearColor::White;
	// the plugin: Tools menu, toolbar, its window tab
	Style->Set("ProjectCleaner.TabMain", new FSlateImageBrush {Icon(TEXT("App"), 40.0f, ColorDangerIcon)});
	Style->Set("ProjectCleaner.TabMain.Small", new FSlateImageBrush {Icon(TEXT("App"), 20.0f, ColorDangerIcon)});
	Style->Set("ProjectCleaner.Icon.App16", new FSlateImageBrush {Icon(TEXT("App"), 16.0f, ColorDangerIcon)});
	// tabs
	Style->Set("ProjectCleaner.Tab.Assets", new FSlateImageBrush {Icon(TEXT("Assets"), 16.0f, ColorUsed)});
	Style->Set("ProjectCleaner.Tab.Inspector", new FSlateImageBrush {Icon(TEXT("Inspector"), 16.0f, ColorLink)});
	Style->Set("ProjectCleaner.Tab.Issues", new FSlateImageBrush {Icon(TEXT("Issues"), 16.0f, ColorWarning)});
	Style->Set("ProjectCleaner.Tab.Help", new FSlateImageBrush {Icon(TEXT("Help"), 16.0f, ColorLink)});
	// the warning sign of the Issues card and of the busy message
	Style->Set("ProjectCleaner.Icon.Warning", new FSlateImageBrush {Icon(TEXT("Issues"), 32.0f, ColorWarning)});
	// white, drawn in the color of where they sit
	// clang-format off
	const TCHAR* WhiteIcons[] = {
		TEXT("Scan"), TEXT("Clean"), TEXT("Tree"), TEXT("SizeMap"), TEXT("Settings"), TEXT("Lock"), TEXT("Close"), TEXT("Add"), TEXT("Back"),
		TEXT("Forward"), TEXT("Folder"), TEXT("FolderOpen"), TEXT("Reset"), TEXT("Expand"), TEXT("ChevronUp"),
		TEXT("List"), TEXT("Grid"),
	};
	// clang-format on
	for (const TCHAR* Name : WhiteIcons) {
		Style->Set(FName {*(FString {TEXT("ProjectCleaner.Icon.")} + Name)}, new FSlateImageBrush {Icon(Name, 16.0f, White)});
	}
	Style->Set("ProjectCleaner.Icon.Scan20", new FSlateImageBrush {Icon(TEXT("Scan"), 20.0f, White)});
	Style->Set("ProjectCleaner.Icon.Clean20", new FSlateImageBrush {Icon(TEXT("Clean"), 20.0f, White)});
	// menu entries (PjcWidgets::GetMenuIcon)
	Style->Set("ProjectCleaner.Menu.Inspect", new FSlateImageBrush {Icon(TEXT("Inspector"), 16.0f, ColorLink)});
	Style->Set("ProjectCleaner.Menu.Browse", new FSlateImageBrush {Icon(TEXT("Browse"), 16.0f, ColorText)});
	Style->Set("ProjectCleaner.Menu.Delete", new FSlateImageBrush {Icon(TEXT("Clean"), 16.0f, ColorDangerIcon)});
	Style->Set("ProjectCleaner.Menu.Exclude", new FSlateImageBrush {Icon(TEXT("Exclude"), 16.0f, ColorText)});
	Style->Set("ProjectCleaner.Menu.Include", new FSlateImageBrush {Icon(TEXT("Include"), 16.0f, ColorText)});
	Style->Set("ProjectCleaner.Menu.Explorer", new FSlateImageBrush {Icon(TEXT("Explorer"), 16.0f, ColorFolder)});
	Style->Set("ProjectCleaner.Menu.SizeMap", new FSlateImageBrush {Icon(TEXT("SizeMap"), 16.0f, ColorText)});
	Style->Set("ProjectCleaner.Menu.Expand", new FSlateImageBrush {Icon(TEXT("Expand"), 16.0f, ColorText)});
	Style->Set("ProjectCleaner.Menu.Collapse", new FSlateImageBrush {Icon(TEXT("Collapse"), 16.0f, ColorText)});
	Style->Set("ProjectCleaner.Menu.Reset", new FSlateImageBrush {Icon(TEXT("Reset"), 16.0f, ColorText)});
	// tree rows (SExpanderArrow with this style set): the engine draws triangles in UE 4.27, chevrons in UE5
	Style->Set("TreeArrow_Collapsed", new FSlateImageBrush {Icon(TEXT("CaretRight"), 10.0f, ColorMuted)});
	Style->Set("TreeArrow_Collapsed_Hovered", new FSlateImageBrush {Icon(TEXT("CaretRight"), 10.0f, ColorText)});
	Style->Set("TreeArrow_Expanded", new FSlateImageBrush {Icon(TEXT("CaretDown"), 10.0f, ColorMuted)});
	Style->Set("TreeArrow_Expanded_Hovered", new FSlateImageBrush {Icon(TEXT("CaretDown"), 10.0f, ColorText)});
	// marks on circular links of the Inspector graph: white, tinted when drawn
	Style->Set("ProjectCleaner.Graph.Disc", new FSlateImageBrush {Icon(TEXT("Disc"), 16.0f, White)});
	Style->Set("ProjectCleaner.Graph.Loop", new FSlateImageBrush {Icon(TEXT("Loop"), 16.0f, White)});
	Style->Set("ProjectCleaner.Graph.Pair", new FSlateImageBrush {Icon(TEXT("Pair"), 16.0f, White)});

#if ENGINE_MAJOR_VERSION >= 5
	// tool pages: dark theme background behind the panels and cards, 4.27 sets its own below
	Style->Set("ProjectCleaner.Page", new FSlateColorBrush {FStyleColors::Background});
	// cards of the Overview, 4.27 takes the editor group border (PjcShim::GetBrushCard)
	Style->Set("ProjectCleaner.Card", new FSlateRoundedBoxBrush(FStyleColors::Panel, 4.0f, FLinearColor {FColor::FromHex(TEXT("#383838"))}, 1.0f));
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

	// rounded corners as the UE5 widgets have them: a white 16 px box with 4 px corners, nine-sliced and tinted. The same image
	// in every engine (UE 4.27 has no rounded box brush), so the plugin's own buttons and toggles look the same everywhere
	const FString RoundedBox = Style->RootToContentDir(TEXT("RoundedBox16"), TEXT(".png"));
	const auto Round = [&RoundedBox](const TCHAR* Value) {
		return FSlateBoxBrush {RoundedBox, FMargin {4.0f / 16.0f}, FLinearColor {FColor::FromHex(Value)}};
	};

	// Clean project in the scan bar: dark red, so its red bin icon stands out (confirm buttons stay solid red)
	// clang-format off
	Style->Set("ProjectCleaner.Button.DangerQuiet", FButtonStyle {}
		.SetNormal(Round(TEXT("#4A2427")))
		.SetHovered(Round(TEXT("#5C2B2F")))
		.SetPressed(Round(TEXT("#3A1D20")))
		.SetDisabled(Round(TEXT("#2E2224")))
		.SetNormalPadding(FMargin {2.0f})
		.SetPressedPadding(FMargin {2.0f}));
	// on and off buttons (Tree and Size map, filter chips): nothing when off, a box on hover, blue when on, as UE5
	// ToggleButtonCheckbox
	Style->Set("ProjectCleaner.Toggle", FCheckBoxStyle {}
		.SetCheckBoxType(ESlateCheckBoxType::ToggleButton)
		.SetUncheckedImage(FSlateNoResource {})
		.SetUncheckedHoveredImage(Round(TEXT("#383838")))
		.SetUncheckedPressedImage(Round(TEXT("#383838")))
		.SetCheckedImage(Round(TEXT("#0070E0")))
		.SetCheckedHoveredImage(Round(TEXT("#0E86FF")))
		.SetCheckedPressedImage(Round(TEXT("#383838")))
		.SetPadding(FMargin {16.0f, 2.0f})
		.SetForegroundColor(ColorText));
	// checkboxes: a dark rounded box, blue with a white check when on, a dash when mixed (the engine's differ per version)
	const auto Check = [&Icon](const TCHAR* Name) {
		return Icon(Name, 16.0f, FLinearColor::White);
	};
	Style->Set("ProjectCleaner.CheckBox", FCheckBoxStyle {}
		.SetCheckBoxType(ESlateCheckBoxType::CheckBox)
		.SetUncheckedImage(Check(TEXT("CheckOff")))
		.SetUncheckedHoveredImage(Check(TEXT("CheckOffHover")))
		.SetUncheckedPressedImage(Check(TEXT("CheckOffHover")))
		.SetCheckedImage(Check(TEXT("CheckOn")))
		.SetCheckedHoveredImage(Check(TEXT("CheckOnHover")))
		.SetCheckedPressedImage(Check(TEXT("CheckOnHover")))
		.SetUndeterminedImage(Check(TEXT("CheckMixed")))
		.SetUndeterminedHoveredImage(Check(TEXT("CheckMixedHover")))
		.SetUndeterminedPressedImage(Check(TEXT("CheckMixedHover")))
		.SetPadding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})
		.SetForegroundColor(ColorText));
	// clang-format on

	// pills (counts on tabs, filters): a white circle nine-sliced into a capsule, tinted where it is drawn
	Style->Set("ProjectCleaner.Pill", new FSlateBoxBrush {Style->RootToContentDir(TEXT("Pill16"), TEXT(".png")), FMargin {0.5f}});
	// a round dot (filter pills), the same circle drawn whole
	Style->Set(
		"ProjectCleaner.Dot",
		new FSlateImageBrush {
		  Style->RootToContentDir(TEXT("Pill16"), TEXT(".png")), FVector2D {8.0f, 8.0f}
	}
	);
	// grid tiles draw their own border and selection: the table row under them draws nothing
	{
		FTableRowStyle TileRow = FCoreStyle::Get().GetWidgetStyle<FTableRowStyle>("TableView.Row");
		const FSlateNoResource None;
		TileRow.SetSelectorFocusedBrush(None).SetActiveHoveredBrush(None).SetActiveBrush(None).SetInactiveHoveredBrush(None).SetInactiveBrush(None);
		TileRow.SetEvenRowBackgroundHoveredBrush(None).SetEvenRowBackgroundBrush(None).SetOddRowBackgroundHoveredBrush(None).SetOddRowBackgroundBrush(
			None
		);
		TileRow.SetActiveHighlightedBrush(None).SetInactiveHighlightedBrush(None);
		Style->Set("ProjectCleaner.TileRow", TileRow);
	}
	// splitters between panes: a gap with a thin line in its middle, blue while hovered or dragged. Both brushes are the same
	// line: UE5 draws the normal brush in the highlight tint when hovered, UE 4.27 the highlight brush
	const FString SplitterLine = Style->RootToContentDir(TEXT("SplitterLine"), TEXT(".png"));
	const FVector2D LineSize {12.0f, 4.0f};
	FSlateImageBrush LineNormal {SplitterLine, LineSize};
	LineNormal.Tiling = ESlateBrushTileType::Vertical;
	LineNormal.TintColor = FSlateColor {FLinearColor {FColor::FromHex(TEXT("#383838"))}};
	FSlateImageBrush LineLit = LineNormal;
	LineLit.TintColor = FSlateColor {FLinearColor {FColor::FromHex(TEXT("#0070E0"))}};
	Style->Set("ProjectCleaner.Splitter", FSplitterStyle {}.SetHandleNormalBrush(LineNormal).SetHandleHighlightBrush(LineLit));

	// search boxes: rounded, the lens of the icon set on the left, the same in every engine
	FSearchBoxStyle SearchBox = FCoreStyle::Get().GetWidgetStyle<FSearchBoxStyle>("SearchBox");
	FEditableTextBoxStyle SearchText = SearchBox.TextBoxStyle;
	SearchText.SetBackgroundImageNormal(Round(TEXT("#0F0F0F")));
	SearchText.SetBackgroundImageHovered(Round(TEXT("#0F0F0F")));
	SearchText.SetBackgroundImageFocused(Round(TEXT("#0F0F0F")));
	SearchText.SetBackgroundImageReadOnly(Round(TEXT("#1A1A1A")));
	SearchText.SetForegroundColor(ColorText);
	SearchBox.SetTextBoxStyle(SearchText);
	SearchBox.SetGlassImage(Icon(TEXT("Scan"), 14.0f, ColorMuted));
	SearchBox.SetClearImage(Icon(TEXT("Close"), 12.0f, ColorMuted));
	// room left of the lens, as UE5 has it (4.27 has none)
	SearchBox.SetImagePadding(FMargin {4.0f, 0.0f, 0.0f, 0.0f});
#if ENGINE_MAJOR_VERSION >= 5
	SearchBox.SetLeftAlignGlassImageAndClearButton(true);
#else
	SearchBox.SetLeftAlignButtons(true);
#endif
	Style->Set("ProjectCleaner.SearchBox", SearchBox);

	// drop-down buttons (View, Exclude): a dark box with a thin outline and the chevron of the icon set, as UE5's combo
	// buttons, in every engine (4.27 draws a flat box with a triangle)
	const auto Outline = [&Style](const TCHAR* Name) {
		return FSlateBoxBrush {Style->RootToContentDir(Name, TEXT(".png")), FMargin {4.0f / 16.0f}};
	};
	// clang-format off
	const FButtonStyle DropButton = FButtonStyle {}
		.SetNormal(Outline(TEXT("RoundedOutline16")))
		.SetHovered(Outline(TEXT("RoundedOutlineHover16")))
		.SetPressed(Outline(TEXT("RoundedOutlineHover16")))
		.SetDisabled(Outline(TEXT("RoundedOutline16")))
		.SetNormalPadding(FMargin {8.0f, 1.0f, 8.0f, 1.0f})
		.SetPressedPadding(FMargin {8.0f, 2.0f, 8.0f, 0.0f});
	// clang-format on
	FComboButtonStyle DropDown = FCoreStyle::Get().GetWidgetStyle<FComboButtonStyle>("ComboButton");
	DropDown.SetButtonStyle(DropButton);
	DropDown.SetDownArrowImage(Icon(TEXT("Expand"), 10.0f, ColorText));
#if ENGINE_MAJOR_VERSION >= 5
	// UE 4.27 pads the content by 5 unless told otherwise, UE5 takes it from the style (0 in the engine's)
	DropDown.SetContentPadding(FMargin {5.0f});
#endif
	Style->Set("ProjectCleaner.ComboButton", DropDown);

#if ENGINE_MAJOR_VERSION < 5
	// UE 4.27 widgets in the colors of the UE5 ones (PjcShim)
	const auto Hex = [](const TCHAR* Value) {
		return FSlateColorBrush {FLinearColor {FColor::FromHex(Value)}};
	};
	const FLinearColor Foreground = ColorText;
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

	const auto MakeDarkBox = [&Round, &Foreground](const FEditableTextBoxStyle& Base) {
		// clang-format off
		return FEditableTextBoxStyle {Base}
			.SetBackgroundImageNormal(Round(TEXT("#0F0F0F")))
			.SetBackgroundImageHovered(Round(TEXT("#0F0F0F")))
			.SetBackgroundImageFocused(Round(TEXT("#0F0F0F")))
			.SetBackgroundImageReadOnly(Round(TEXT("#1A1A1A")))
			.SetForegroundColor(Foreground)
			.SetReadOnlyForegroundColor(Foreground);
		// clang-format on
	};
	Style->Set("ProjectCleaner.TextBox", MakeDarkBox(FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox")));
#endif

	return Style;
}

#undef IMAGE_BRUSH
