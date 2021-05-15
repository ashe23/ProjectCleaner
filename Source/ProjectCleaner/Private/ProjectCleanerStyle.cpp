// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "ProjectCleanerStyle.h"
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

FName FProjectCleanerStyle::GetContextName()
{
	return FName(TEXT("ProjectCleaner.PluginAction"));
}

//void FProjectCleanerStyle::SetIcon(const FString& StyleName, const FString& ResourcePath)
//{
//	FSlateStyleSet* Style = StyleInstance.Get();
//
//	FString Name(GetContextName().ToString());
//	Name = Name + "." + StyleName;
//	Style->Set(*Name, new IMAGE_BRUSH(TEXT("ButtonIcon_40x"), Icon40x40));
//
//	Name += ".Small";
//	Style->Set(*Name, new IMAGE_BRUSH(TEXT("ButtonIcon_20x"), Icon20x20));
//
//	FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
//}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon40x40(40.0f, 40.0f);

TSharedRef< FSlateStyleSet > FProjectCleanerStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("ProjectCleanerStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("ProjectCleaner")->GetBaseDir() / TEXT("Resources"));

	Style->Set("ProjectCleaner.PluginAction", new IMAGE_BRUSH(TEXT("ButtonIcon_40x"), Icon40x40));
	Style->Set("ProjectCleaner.PluginAction.Small", new IMAGE_BRUSH(TEXT("ButtonIcon_20x"), Icon20x20));

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
