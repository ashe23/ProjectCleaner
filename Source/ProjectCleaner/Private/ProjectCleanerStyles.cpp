// Copyright Ashot Barkhudaryan. All Rights Reserved.

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

FSlateFontInfo FProjectCleanerStyles::GetFont(const FString& FontType, const uint32 FontSize)
{
	return FCoreStyle::GetDefaultFontStyle(*FontType, FontSize);
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )

const FVector2D Icon8x8(8.0f, 8.0f);
const FVector2D Icon10x10(10.0f, 10.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon40x40(40.0f, 40.0f);

TSharedRef<FSlateStyleSet> FProjectCleanerStyles::Create()
{
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet(ProjectCleanerConstants::ModuleStylesName));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin(ProjectCleanerConstants::ModuleName.ToString())->GetBaseDir() / TEXT("Resources"));

	// icons
	Style->Set("ProjectCleaner.IconBin20", new IMAGE_BRUSH(TEXT("IconBin20"), Icon20x20));
	Style->Set("ProjectCleaner.IconBin40", new IMAGE_BRUSH(TEXT("IconBin40"), Icon40x40));
	Style->Set("ProjectCleaner.IconCircle8", new IMAGE_BRUSH(TEXT("IconCircle8"), Icon8x8));
	Style->Set("ProjectCleaner.IconCircle10", new IMAGE_BRUSH(TEXT("IconCircle10"), Icon10x10));
	Style->Set("ProjectCleaner.IconCircle20", new IMAGE_BRUSH(TEXT("IconCircle20"), Icon20x20));
	Style->Set("ProjectCleaner.IconCircle40", new IMAGE_BRUSH(TEXT("IconCircle40"), Icon40x40));

	// cmds
	Style->Set("ProjectCleaner.Cmd_OpenCleanerWindow", new IMAGE_BRUSH(TEXT("IconBin40"), Icon40x40));
	Style->Set("ProjectCleaner.Cmd_OpenCleanerWindow.Small", new IMAGE_BRUSH(TEXT("IconBin20"), Icon20x20));

	// pallet colors
	Style->Set("ProjectCleaner.Color.Blue", FLinearColor{FColor::FromHex(TEXT("#227c9d"))});
	Style->Set("ProjectCleaner.Color.Green", FLinearColor{FColor::FromHex(TEXT("#17c3b2"))});
	Style->Set("ProjectCleaner.Color.Yellow", FLinearColor{FColor::FromHex(TEXT("#ffcb77"))});
	Style->Set("ProjectCleaner.Color.White", FLinearColor{FColor::FromHex(TEXT("#fef9ef"))});
	Style->Set("ProjectCleaner.Color.Red", FLinearColor{FColor::FromHex(TEXT("#fe6d73"))});

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
