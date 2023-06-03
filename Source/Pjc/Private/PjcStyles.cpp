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
	Style->Set("ProjectCleaner.Refresh", new IMAGE_BRUSH(TEXT("IconRefresh32"), FVector2D{32.0f, 32.0f}));
	Style->Set("ProjectCleaner.Delete", new IMAGE_BRUSH(TEXT("IconCross32"), FVector2D{32.0f, 32.0f}));
	Style->Set("ProjectCleaner.Exclude", new IMAGE_BRUSH(TEXT("IconMinus32"), FVector2D{32.0f, 32.0f}));
	Style->Set("ProjectCleaner.ExcludeByExt", new IMAGE_BRUSH(TEXT("IconMinus32"), FVector2D{32.0f, 32.0f}));
	Style->Set("ProjectCleaner.ExcludeByClass", new IMAGE_BRUSH(TEXT("IconMinus32"), FVector2D{32.0f, 32.0f}));
	Style->Set("ProjectCleaner.ClearSelection", new IMAGE_BRUSH(TEXT("IconNone32"), FVector2D{32.0f, 32.0f}));
	Style->Set("ProjectCleaner.ScanProject", new IMAGE_BRUSH(TEXT("IconRefresh32"), FVector2D{32.0f, 32.0f}));
	Style->Set("ProjectCleaner.CleanProject", new IMAGE_BRUSH(TEXT("IconBinRed32"), FVector2D{32.0f, 32.0f}));
	Style->Set("ProjectCleaner.DeleteEmptyFolders", new IMAGE_BRUSH(TEXT("IconFolderRemove32"), FVector2D{32.0f, 32.0f}));
	Style->Set("ProjectCleaner.ClearExcludeSettings", new IMAGE_BRUSH(TEXT("IconFilterClear32"), FVector2D{32.0f, 32.0f}));
	Style->Set("ProjectCleaner.OpenViewerAssetsIndirect", new IMAGE_BRUSH(TEXT("IconArrows32"), FVector2D{32.0f, 32.0f}));
	Style->Set("ProjectCleaner.OpenViewerAssetsCorrupted", new IMAGE_BRUSH(TEXT("IconCorruptedFile32"), FVector2D{32.0f, 32.0f}));
	Style->Set("ProjectCleaner.AssetsExclude", new IMAGE_BRUSH(TEXT("IconMinus32"), FVector2D{32.0f, 32.0f}));
	Style->Set("ProjectCleaner.AssetsExcludeByClass", new IMAGE_BRUSH(TEXT("IconMinus32"), FVector2D{32.0f, 32.0f}));
	Style->Set("ProjectCleaner.AssetsInclude", new IMAGE_BRUSH(TEXT("IconPlus32"), FVector2D{32.0f, 32.0f}));
	Style->Set("ProjectCleaner.AssetsIncludeByClass", new IMAGE_BRUSH(TEXT("IconPlus32"), FVector2D{32.0f, 32.0f}));
	Style->Set("ProjectCleaner.PathsExclude", new IMAGE_BRUSH(TEXT("IconMinus32"), FVector2D{32.0f, 32.0f}));
	Style->Set("ProjectCleaner.PathsInclude", new IMAGE_BRUSH(TEXT("IconPlus32"), FVector2D{32.0f, 32.0f}));
	Style->Set("ProjectCleaner.PathsReveal", new IMAGE_BRUSH(TEXT("IconFolderReveal32"), FVector2D{32.0f, 32.0f}));
	Style->Set("ProjectCleaner.PathsExpandRecursive", new IMAGE_BRUSH(TEXT("IconExpand32"), FVector2D{32.0f, 32.0f}));
	Style->Set("ProjectCleaner.PathsCollapseRecursive", new IMAGE_BRUSH(TEXT("IconCollapse32"), FVector2D{32.0f, 32.0f}));
	Style->Set("ProjectCleaner.AssetsDelete", new IMAGE_BRUSH(TEXT("IconCross32"), FVector2D{32.0f, 32.0f}));
	Style->Set("ProjectCleaner.OpenViewerSizeMap", new IMAGE_BRUSH(TEXT("IconTreeMap32"), FVector2D{32.0f, 32.0f}));
	Style->Set("ProjectCleaner.OpenViewerReference", new IMAGE_BRUSH(TEXT("IconGraph32"), FVector2D{32.0f, 32.0f}));
	Style->Set("ProjectCleaner.OpenViewerAssetsAudit", new IMAGE_BRUSH(TEXT("IconStat32"), FVector2D{32.0f, 32.0f}));
	Style->Set("ProjectCleaner.OpenGithub", new IMAGE_BRUSH(TEXT("IconGithub32"), FVector2D{32.0f, 32.0f}));
	Style->Set("ProjectCleaner.OpenWiki", new IMAGE_BRUSH(TEXT("IconWiki32"), FVector2D{32.0f, 32.0f}));
	Style->Set("ProjectCleaner.OpenBugReport", new IMAGE_BRUSH(TEXT("IconBug32"), FVector2D{32.0f, 32.0f}));

	// cmds icons small
	Style->Set("ProjectCleaner.TabMain.Small", new IMAGE_BRUSH(TEXT("IconBin20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.Refresh.Small", new IMAGE_BRUSH(TEXT("IconRefresh20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.Delete.Small", new IMAGE_BRUSH(TEXT("IconCross20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.Exclude.Small", new IMAGE_BRUSH(TEXT("IconMinus20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.ExcludeByExt.Small", new IMAGE_BRUSH(TEXT("IconMinus20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.ExcludeByClass.Small", new IMAGE_BRUSH(TEXT("IconMinus20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.ClearSelection.Small", new IMAGE_BRUSH(TEXT("IconNone20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.ScanProject.Small", new IMAGE_BRUSH(TEXT("IconRefresh20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.CleanProject.Small", new IMAGE_BRUSH(TEXT("IconBinRed20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.DeleteEmptyFolders.Small", new IMAGE_BRUSH(TEXT("IconFolderRemove20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.ClearExcludeSettings.Small", new IMAGE_BRUSH(TEXT("IconFilterClear20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.OpenViewerAssetsIndirect.Small", new IMAGE_BRUSH(TEXT("IconArrows20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.OpenViewerAssetsCorrupted.Small", new IMAGE_BRUSH(TEXT("IconCorruptedFile20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.AssetsExclude.Small", new IMAGE_BRUSH(TEXT("IconMinus20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.AssetsExcludeByClass.Small", new IMAGE_BRUSH(TEXT("IconMinus20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.AssetsInclude.Small", new IMAGE_BRUSH(TEXT("IconPlus20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.AssetsIncludeByClass.Small", new IMAGE_BRUSH(TEXT("IconPlus20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.PathsExclude.Small", new IMAGE_BRUSH(TEXT("IconMinus20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.PathsInclude.Small", new IMAGE_BRUSH(TEXT("IconPlus20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.PathsReveal.Small", new IMAGE_BRUSH(TEXT("IconFolderReveal20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.PathsExpandRecursive.Small", new IMAGE_BRUSH(TEXT("IconExpand20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.PathsCollapseRecursive.Small", new IMAGE_BRUSH(TEXT("IconCollapse20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.AssetsDelete.Small", new IMAGE_BRUSH(TEXT("IconCross20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.OpenViewerSizeMap.Small", new IMAGE_BRUSH(TEXT("IconTreeMap20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.OpenViewerReference.Small", new IMAGE_BRUSH(TEXT("IconGraph20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.OpenViewerAssetsAudit.Small", new IMAGE_BRUSH(TEXT("IconStat20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.OpenGithub.Small", new IMAGE_BRUSH(TEXT("IconGithub20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.OpenWiki.Small", new IMAGE_BRUSH(TEXT("IconWiki20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.OpenBugReport.Small", new IMAGE_BRUSH(TEXT("IconBug20"), FVector2D{20.0f, 20.0f}));

	// icons
	Style->Set("ProjectCleaner.Icon.Bin16", new IMAGE_BRUSH(TEXT("IconBin16"), FVector2D{16.0f, 16.0f}));
	Style->Set("ProjectCleaner.Icon.Bin20", new IMAGE_BRUSH(TEXT("IconBin20"), FVector2D{20.0f, 20.0f}));
	Style->Set("ProjectCleaner.Icon.Bin40", new IMAGE_BRUSH(TEXT("IconBin40"), FVector2D{40.0f, 40.0f}));
	Style->Set("ProjectCleaner.Icon.Warning32", new IMAGE_BRUSH(TEXT("IconWarning32"), FVector2D{32.0f, 32.0f}));
	Style->Set("ProjectCleaner.Icon.Check32", new IMAGE_BRUSH(TEXT("IconCheck32"), FVector2D{32.0f, 32.0f}));
	Style->Set("ProjectCleaner.Icon.PieChart16", new IMAGE_BRUSH(TEXT("IconPieChart16"), FVector2D{16.0f, 16.0f}));
	Style->Set("ProjectCleaner.Icon.File16", new IMAGE_BRUSH(TEXT("IconFile16"), FVector2D{16.0f, 16.0f}));
	Style->Set("ProjectCleaner.Icon.Stat16", new IMAGE_BRUSH(TEXT("IconStat16"), FVector2D{16.0f, 16.0f}));
	Style->Set("ProjectCleaner.Icon.Arrows16", new IMAGE_BRUSH(TEXT("IconArrows16"), FVector2D{16.0f, 16.0f}));
	Style->Set("ProjectCleaner.Icon.CorruptedFile16", new IMAGE_BRUSH(TEXT("IconCorruptedFile20"), FVector2D{16.0f, 16.0f}));

	// pallet colors
	Style->Set("ProjectCleaner.Color.Black", FLinearColor{FColor::FromHex(TEXT("#000000"))});
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
