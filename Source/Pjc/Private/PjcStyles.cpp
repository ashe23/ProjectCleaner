// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcStyles.h"
#include "PjcConstants.h"
// Engine Headers
#include "Styling/SlateStyleRegistry.h"
#include "Interfaces/IPluginManager.h"

TSharedPtr<FSlateStyleSet> FPjcStyles::StyleInstance = nullptr;

void FPjcStyles::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FPjcStyles::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

void FPjcStyles::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FPjcStyles::Get()
{
	return *StyleInstance;
}

FName FPjcStyles::GetStyleSetName()
{
	return PjcConstants::ModulePjcStylesName;
}

FSlateFontInfo FPjcStyles::GetFont(const FString& FontType, const uint32 FontSize)
{
	return FCoreStyle::GetDefaultFontStyle(*FontType, FontSize);
}

FSlateIcon FPjcStyles::GetIcon(const FString& IconSpecifier)
{
	return FSlateIcon(GetStyleSetName(), FName{*IconSpecifier});
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )

TSharedRef<FSlateStyleSet> FPjcStyles::Create()
{
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet(PjcConstants::ModulePjcStylesName));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin(PjcConstants::ModulePjcName.ToString())->GetBaseDir() / TEXT("Resources"));

	// cmds icons big
	Style->Set("ProjectCleaner.TabMain", new IMAGE_BRUSH(TEXT("IconBin40"), FVector2D{40.0f, 40.0f}));

	// cmds icons small
	Style->Set("ProjectCleaner.TabMain.Small", new IMAGE_BRUSH(TEXT("IconBin20"), FVector2D{20.0f, 20.0f}));

	// icons
	Style->Set("ProjectCleaner.Icon.Bin16", new IMAGE_BRUSH(TEXT("IconBin16"), FVector2D{16.0f, 16.0f}));
	Style->Set("ProjectCleaner.Icon.Bin20", new IMAGE_BRUSH(TEXT("IconBin20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.Icon.Bin40", new IMAGE_BRUSH(TEXT("IconBin40"), FVector2D{40.0f, 40.0f}));

	// pallet colors
	Style->Set("ProjectCleaner.Color.Blue", FLinearColor{FColor::FromHex(TEXT("#227C9D"))});
	Style->Set("ProjectCleaner.Color.Green", FLinearColor{FColor::FromHex(TEXT("#17C3B2"))});
	Style->Set("ProjectCleaner.Color.GreenBright", FLinearColor{FColor::FromHex(TEXT("#16db65"))});
	Style->Set("ProjectCleaner.Color.Yellow", FLinearColor{FColor::FromHex(TEXT("#FFCB77"))});
	Style->Set("ProjectCleaner.Color.White", FLinearColor{FColor::FromHex(TEXT("#fef9ef"))});
	Style->Set("ProjectCleaner.Color.Red", FLinearColor{FColor::FromHex(TEXT("#FE6D73"))});
	Style->Set("ProjectCleaner.Color.Violet", FLinearColor{FColor::FromHex(TEXT("#932f6d"))});
	Style->Set("ProjectCleaner.Color.Gray", FLinearColor{FColor::FromHex(TEXT("#adb5bd"))});
	Style->Set("ProjectCleaner.Color.BlueLight", FLinearColor{FColor::FromHex(TEXT("#0496ff"))});
	Style->Set("ProjectCleaner.Color.DarkGray", FLinearColor{FColor::FromHex(TEXT("#567189"))});

	// backgrounds
	Style->Set("ProjectCleaner.BgProgressbar", new BOX_BRUSH("BgProgressbar", FMargin(5.f/12.f)));
	Style->Set("ProjectCleaner.BgWhite", new BOX_BRUSH("BgWhite", FMargin(16.0f/16.f)));

	return Style;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT
