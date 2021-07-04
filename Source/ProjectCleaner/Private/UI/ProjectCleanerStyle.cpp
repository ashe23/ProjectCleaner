// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "UI/ProjectCleanerStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"

TSharedPtr< FSlateStyleSet > FProjectCleanerStyle::StyleInstance = nullptr;

void FProjectCleanerStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FProjectCleanerStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FProjectCleanerStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("ProjectCleanerStyle"));
	return StyleSetName;
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )

const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon40x40(40.0f, 40.0f);

TSharedRef< FSlateStyleSet > FProjectCleanerStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("ProjectCleanerStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("ProjectCleaner")->GetBaseDir() / TEXT("Resources"));

	// icons
	Style->Set("ProjectCleaner.OpenCleanerWindow", new IMAGE_BRUSH(TEXT("ButtonIcon_40x"), Icon40x40));
	Style->Set("ProjectCleaner.OpenCleanerWindow.Small", new IMAGE_BRUSH(TEXT("ButtonIcon_20x"), Icon20x20));

	// fonts
	Style->Set("ProjectCleaner.Font.Light30", FCoreStyle::GetDefaultFontStyle("Light", 30));
	Style->Set("ProjectCleaner.Font.Light20", FCoreStyle::GetDefaultFontStyle("Light", 20));
	Style->Set("ProjectCleaner.Font.Light15", FCoreStyle::GetDefaultFontStyle("Light", 15));
	Style->Set("ProjectCleaner.Font.Light10", FCoreStyle::GetDefaultFontStyle("Light", 10));
	Style->Set("ProjectCleaner.Font.Light8", FCoreStyle::GetDefaultFontStyle("Light", 8));

	return Style;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT

void FProjectCleanerStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FProjectCleanerStyle::Get()
{
	return *StyleInstance;
}