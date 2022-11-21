// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerStyles.h"
#include "ProjectCleanerConstants.h"
// Engine Headers
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"

TSharedPtr<FSlateStyleSet> FProjectCleanerStyles::StyleInstance = nullptr;

void FProjectCleanerStyles::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FProjectCleanerStyles::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FProjectCleanerStyles::GetStyleSetName()
{
	return ProjectCleanerConstants::ModuleStylesName;
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )

const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon40x40(40.0f, 40.0f);

TSharedRef<FSlateStyleSet> FProjectCleanerStyles::Create()
{
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet(ProjectCleanerConstants::ModuleStylesName));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin(ProjectCleanerConstants::ModuleName.ToString())->GetBaseDir() / TEXT("Resources"));

	// icons
	Style->Set("ProjectCleaner.IconBin20", new IMAGE_BRUSH(TEXT("IconBin256"), Icon20x20));
	Style->Set("ProjectCleaner.IconBin40", new IMAGE_BRUSH(TEXT("IconBin256"), Icon40x40));

	// cmds
	Style->Set("ProjectCleaner.Cmd_OpenCleanerWindow", new IMAGE_BRUSH(TEXT("IconBin256"), Icon40x40));
	Style->Set("ProjectCleaner.Cmd_OpenCleanerWindow.Small", new IMAGE_BRUSH(TEXT("IconBin256"), Icon20x20));

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

void FProjectCleanerStyles::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FProjectCleanerStyles::Get()
{
	return *StyleInstance;
}
