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
	return PjcConstants::ModuleStylesName;
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
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet(PjcConstants::ModuleStylesName));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin(PjcConstants::ModuleName.ToString())->GetBaseDir() / TEXT("Resources"));

	// cmds
	Style->Set("ProjectCleaner.OpenMainWindow", new IMAGE_BRUSH(TEXT("IconBin40"), FVector2D{40.0f, 40.0f}));
	Style->Set("ProjectCleaner.OpenMainWindow.Small", new IMAGE_BRUSH(TEXT("IconBin20"), FVector2D{20.0f, 20.0f}));

	// icons
	Style->Set("ProjectCleaner.IconBin16", new IMAGE_BRUSH(TEXT("IconBin16"), FVector2D{16.0f, 16.0f}));
	Style->Set("ProjectCleaner.IconBin20", new IMAGE_BRUSH(TEXT("IconBin20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.IconBin40", new IMAGE_BRUSH(TEXT("IconBin40"), FVector2D{40.0f, 40.0f}));
	Style->Set("ProjectCleaner.IconSettings16", new IMAGE_BRUSH(TEXT("IconSettings16"), FVector2D{16.0f, 16.0f}));
	Style->Set("ProjectCleaner.IconSettings20", new IMAGE_BRUSH(TEXT("IconSettings20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.IconSettings40", new IMAGE_BRUSH(TEXT("IconSettings40"), FVector2D{40.0f, 40.0f}));
	Style->Set("ProjectCleaner.IconTabAssetsBrowser16", new IMAGE_BRUSH(TEXT("IconTabAssetsBrowser16"), FVector2D{16.0f, 16.0f}));
	Style->Set("ProjectCleaner.IconTabAssetsBrowser20", new IMAGE_BRUSH(TEXT("IconTabAssetsBrowser20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.IconTabAssetsBrowser40", new IMAGE_BRUSH(TEXT("IconTabAssetsBrowser40"), FVector2D{40.0f, 40.0f}));
	Style->Set("ProjectCleaner.IconTabIndirect16", new IMAGE_BRUSH(TEXT("IconTabIndirect16"), FVector2D{16.0f, 16.0f}));
	Style->Set("ProjectCleaner.IconTabIndirect20", new IMAGE_BRUSH(TEXT("IconTabIndirect20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.IconTabIndirect40", new IMAGE_BRUSH(TEXT("IconTabIndirect40"), FVector2D{40.0f, 40.0f}));
	Style->Set("ProjectCleaner.IconTabCorrupted16", new IMAGE_BRUSH(TEXT("IconTabCorrupted16"), FVector2D{16.0f, 16.0f}));
	Style->Set("ProjectCleaner.IconTabCorrupted20", new IMAGE_BRUSH(TEXT("IconTabCorrupted20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.IconTabCorrupted40", new IMAGE_BRUSH(TEXT("IconTabCorrupted40"), FVector2D{40.0f, 40.0f}));
	Style->Set("ProjectCleaner.IconTabNonEngine16", new IMAGE_BRUSH(TEXT("IconTabNonEngine16"), FVector2D{16.0f, 16.0f}));
	Style->Set("ProjectCleaner.IconTabNonEngine20", new IMAGE_BRUSH(TEXT("IconTabNonEngine20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.IconTabNonEngine40", new IMAGE_BRUSH(TEXT("IconTabNonEngine40"), FVector2D{40.0f, 40.0f}));
	Style->Set("ProjectCleaner.IconWarning16", new IMAGE_BRUSH(TEXT("IconWarning16"), FVector2D{16.0f, 16.0f}));
	Style->Set("ProjectCleaner.IconWarning20", new IMAGE_BRUSH(TEXT("IconWarning20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.IconCheck16", new IMAGE_BRUSH(TEXT("IconCheck16"), FVector2D{16.0f, 16.0f}));
	Style->Set("ProjectCleaner.IconCheck20", new IMAGE_BRUSH(TEXT("IconCheck20"), FVector2D{20.0f, 20.0f}));

	// pallet colors
	Style->Set("ProjectCleaner.Color.Blue", FLinearColor{FColor::FromHex(TEXT("#227c9d"))});
	Style->Set("ProjectCleaner.Color.Green", FLinearColor{FColor::FromHex(TEXT("#17c3b2"))});
	Style->Set("ProjectCleaner.Color.GreenBright", FLinearColor{FColor::FromHex(TEXT("#16db65"))});
	Style->Set("ProjectCleaner.Color.Yellow", FLinearColor{FColor::FromHex(TEXT("#ffcb77"))});
	Style->Set("ProjectCleaner.Color.White", FLinearColor{FColor::FromHex(TEXT("#fef9ef"))});
	Style->Set("ProjectCleaner.Color.Red", FLinearColor{FColor::FromHex(TEXT("#fe6d73"))});
	Style->Set("ProjectCleaner.Color.Violet", FLinearColor{FColor::FromHex(TEXT("#932f6d"))});
	Style->Set("ProjectCleaner.Color.Gray", FLinearColor{FColor::FromHex(TEXT("#adb5bd"))});
	Style->Set("ProjectCleaner.Color.DarkGray", FLinearColor{FColor::FromHex(TEXT("#567189"))});

	// progressbar
	Style->Set("ProjectCleaner.Progressbar", new BOX_BRUSH("ProgressbarBackground", FMargin(5.f/12.f)));

	return Style;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT
