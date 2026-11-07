// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcStyles.h"
#include "PjcConstants.h"
// Engine Headers
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
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
	Style->Set("ProjectCleaner.OpenGithub", new IMAGE_BRUSH(TEXT("IconGithub32"), FVector2D {32.0f, 32.0f}));
	Style->Set("ProjectCleaner.OpenWiki", new IMAGE_BRUSH(TEXT("IconWiki32"), FVector2D {32.0f, 32.0f}));
	Style->Set("ProjectCleaner.OpenBugReport", new IMAGE_BRUSH(TEXT("IconBug32"), FVector2D {32.0f, 32.0f}));

	// cmds icons small
	Style->Set("ProjectCleaner.TabMain.Small", new IMAGE_BRUSH(TEXT("IconBin20"), FVector2D {20.0f, 20.0f}));
	Style->Set("ProjectCleaner.OpenGithub.Small", new IMAGE_BRUSH(TEXT("IconGithub20"), FVector2D {20.0f, 20.0f}));
	Style->Set("ProjectCleaner.OpenWiki.Small", new IMAGE_BRUSH(TEXT("IconWiki20"), FVector2D {20.0f, 20.0f}));
	Style->Set("ProjectCleaner.OpenBugReport.Small", new IMAGE_BRUSH(TEXT("IconBug20"), FVector2D {20.0f, 20.0f}));

	// icons
	Style->Set("ProjectCleaner.Icon.Bin16", new IMAGE_BRUSH(TEXT("IconBin16"), FVector2D {16.0f, 16.0f}));
	Style->Set("ProjectCleaner.Icon.Warning32", new IMAGE_BRUSH(TEXT("IconWarning32"), FVector2D {32.0f, 32.0f}));
	Style->Set("ProjectCleaner.Icon.PieChart16", new IMAGE_BRUSH(TEXT("IconPieChart16"), FVector2D {16.0f, 16.0f}));
	Style->Set("ProjectCleaner.Icon.Stat16", new IMAGE_BRUSH(TEXT("IconStat16"), FVector2D {16.0f, 16.0f}));
	Style->Set("ProjectCleaner.Icon.Graph16", new IMAGE_BRUSH(TEXT("IconGraph20"), FVector2D {16.0f, 16.0f}));
	Style->Set("ProjectCleaner.Icon.CorruptedFile16", new IMAGE_BRUSH(TEXT("IconCorruptedFile20"), FVector2D {16.0f, 16.0f}));

	// pallet colors
	Style->Set("ProjectCleaner.Color.Yellow", FLinearColor {FColor::FromHex(TEXT("#FFCB77"))});
	Style->Set("ProjectCleaner.Color.Red", FLinearColor {FColor::FromHex(TEXT("#FE6D73"))});
	Style->Set("ProjectCleaner.Color.Gray", FLinearColor {FColor::FromHex(TEXT("#adb5bd"))});

	// asset status: bar fills, text accents
	Style->Set("ProjectCleaner.Color.UsedFill", FLinearColor {FColor::FromHex(TEXT("#2C766F"))});
	Style->Set("ProjectCleaner.Color.EditorFill", FLinearColor {FColor::FromHex(TEXT("#3D6DB3"))});
	Style->Set("ProjectCleaner.Color.UnusedFill", FLinearColor {FColor::FromHex(TEXT("#C4484D"))});
	Style->Set("ProjectCleaner.Color.Ok", FLinearColor {FColor::FromHex(TEXT("#6FCF7A"))});
	Style->Set("ProjectCleaner.Color.Track", FLinearColor {FColor::FromHex(TEXT("#2D3136"))});
	Style->Set("ProjectCleaner.Color.Line", FLinearColor {FColor::FromHex(TEXT("#383B40"))});
	Style->Set("ProjectCleaner.Color.Accent", FLinearColor {FColor::FromHex(TEXT("#2F7DE1"))});
	Style->Set("ProjectCleaner.Color.ShipFill", FLinearColor {FColor::FromHex(TEXT("#3A8D5C"))});
	Style->Set("ProjectCleaner.Color.KeepFill", FLinearColor {FColor::FromHex(TEXT("#56606B"))});
	Style->Set("ProjectCleaner.Color.FolderTint", FLinearColor {FColor::FromHex(TEXT("#C9A55A"))});

#if ENGINE_MAJOR_VERSION >= 5
	// cards of the Overview, 4.27 takes the editor group border (PjcShim::GetBrushCard)
	Style->Set("ProjectCleaner.Card", new FSlateRoundedBoxBrush(FStyleColors::Panel, 4.0f, FLinearColor {FColor::FromHex(TEXT("#383B40"))}, 1.0f));
	// Inspector graph boxes, white: drawn tinted. 4.27 draws square boxes.
	Style->Set("ProjectCleaner.Graph.Node", new FSlateRoundedBoxBrush(FLinearColor::White, 3.0f));
	Style->Set("ProjectCleaner.Graph.Pill", new FSlateRoundedBoxBrush(FLinearColor::White, 23.0f));
#endif

	// rows that react to the mouse
	Style->Set(
		"ProjectCleaner.Button.Row",
		FButtonStyle {}
			.SetNormal(FSlateNoResource {})
			.SetHovered(FSlateColorBrush {FLinearColor {1.0f, 1.0f, 1.0f, 0.04f}})
			.SetPressed(FSlateColorBrush {FLinearColor {1.0f, 1.0f, 1.0f, 0.07f}})
			.SetNormalPadding(FMargin {0.0f})
			.SetPressedPadding(FMargin {0.0f})
	);

	return Style;
}

#undef IMAGE_BRUSH
