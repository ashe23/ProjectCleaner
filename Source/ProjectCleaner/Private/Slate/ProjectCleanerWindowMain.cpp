// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/ProjectCleanerWindowMain.h"
#include "ProjectCleanerTypes.h"
#include "ProjectCleanerStyles.h"
#include "ProjectCleanerConstants.h"
#include "Libs/ProjectCleanerAssetLibrary.h"
#include "Slate/ProjectCleanerTreeView.h"
// Engine Headers
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

static constexpr int32 WidgetIndexNone = 0;
static constexpr int32 WidgetIndexLoading = 1;

void SProjectCleanerWindowMain::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SWidgetSwitcher)
		.IsEnabled_Static(IsWidgetEnabled)
		.WidgetIndex_Static(GetWidgetIndex)
		+ SWidgetSwitcher::Slot()
		  .HAlign(HAlign_Fill)
		  .VAlign(VAlign_Fill)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			  .FillHeight(1.0f)
			  .Padding(FMargin{5.0f})
			[
				SNew(SScrollBox)
				.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
				.AnimateWheelScrolling(true)
				.AllowOverscroll(EAllowOverscroll::No)
				+ SScrollBox::Slot()
				[
					SNew(SProjectCleanerTreeView)
				]
			]
		]
		+ SWidgetSwitcher::Slot()
		  .HAlign(HAlign_Fill)
		  .VAlign(VAlign_Fill)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			  .FillWidth(1.0f)
			  .HAlign(HAlign_Center)
			  .VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Justification(ETextJustify::Center)
				.Font(FProjectCleanerStyles::Get().GetFontStyle("ProjectCleaner.Font.Light30"))
				.Text(FText::FromString(ProjectCleanerConstants::MsgAssetRegistryStillWorking))
			]
		]
	];
}

SProjectCleanerWindowMain::~SProjectCleanerWindowMain()
{
}

bool SProjectCleanerWindowMain::IsWidgetEnabled()
{
	return !UProjectCleanerAssetLibrary::AssetRegistryIsLoadingAssets();
}

int32 SProjectCleanerWindowMain::GetWidgetIndex()
{
	return UProjectCleanerAssetLibrary::AssetRegistryIsLoadingAssets() ? WidgetIndexLoading : WidgetIndexNone;
}
