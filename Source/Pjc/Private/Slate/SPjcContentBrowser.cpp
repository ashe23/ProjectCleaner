// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcContentBrowser.h"
#include "PjcSubsystem.h"
#include "PjcConstants.h"
// Engine Headers
#include "IContentBrowserSingleton.h"

void SPjcContentBrowser::Construct(const FArguments& InArgs)
{
	Filter.Clear();
	Filter.TagsAndValues.Emplace(PjcConstants::EmptyTagName, PjcConstants::EmptyTagName.ToString());

	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.bAddFilterUI = true;
	AssetPickerConfig.bCanShowFolders = false;
	AssetPickerConfig.bAllowDragging = false;
	AssetPickerConfig.SelectionMode = ESelectionMode::Multi;
	AssetPickerConfig.bShowBottomToolbar = false;
	AssetPickerConfig.bCanShowDevelopersFolder = true;
	AssetPickerConfig.bForceShowEngineContent = false;
	AssetPickerConfig.bForceShowPluginContent = false;
	AssetPickerConfig.bAllowNullSelection = false;
	AssetPickerConfig.bCanShowClasses = false;
	AssetPickerConfig.bCanShowRealTimeThumbnails = false; // todo:ashe23 need to be configurable
	AssetPickerConfig.AssetShowWarningText = FText::FromString(TEXT("No assets"));
	AssetPickerConfig.Filter = Filter;
	AssetPickerConfig.GetCurrentSelectionDelegates.Add(&DelegateSelection);
	AssetPickerConfig.RefreshAssetViewDelegates.Add(&DelegateRefreshView);
	AssetPickerConfig.SetFilterDelegates.Add(&DelegateFilter);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(FMargin{5.0f, 0.0f})
		[
			UPjcSubsystem::GetModuleContentBrowser().Get().CreateAssetPicker(AssetPickerConfig)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Left).VAlign(VAlign_Center)
			[
				SNew(SComboButton)
				.ContentPadding(0)
				.ForegroundColor_Raw(this, &SPjcContentBrowser::GetOptionsBtnForegroundColor)
				.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
				.OnGetMenuContent(this, &SPjcContentBrowser::GetBtnActionsContent)
				.ButtonContent()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SImage).Image(FEditorStyle::GetBrush("GenericViewButton"))
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f, 0.0f, 0.0f, 0.0f).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("Actions")))
					]
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text_Raw(this, &SPjcContentBrowser::GetSummaryText)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Right).VAlign(VAlign_Center)
			[
				SNew(SComboButton)
				.ContentPadding(0)
				.ForegroundColor_Raw(this, &SPjcContentBrowser::GetOptionsBtnForegroundColor)
				.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
				.OnGetMenuContent(this, &SPjcContentBrowser::GetBtnOptionsContent)
				.ButtonContent()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SImage).Image(FEditorStyle::GetBrush("GenericViewButton"))
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f, 0.0f, 0.0f, 0.0f).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("View Options")))
					]
				]
			]
		]
	];
}

void SPjcContentBrowser::FilterUpdate(const FARFilter& InFilter)
{
	Filter = InFilter;

	DelegateFilter.Execute(Filter);
}

FText SPjcContentBrowser::GetSummaryText() const
{
	const auto SelectedItems = DelegateSelection.Execute();

	if (SelectedItems.Num() > 0)
	{
		return FText::FromString(FString::Printf(TEXT("%d selected"), SelectedItems.Num()));
	}

	return FText::GetEmpty();
}

TSharedRef<SWidget> SPjcContentBrowser::GetBtnActionsContent()
{
	const TSharedPtr<FExtender> Extender;
	FMenuBuilder MenuBuilder(true, nullptr, Extender, true);

	MenuBuilder.AddMenuSeparator(TEXT("Actions"));

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Include all assets")),
		FText::FromString(TEXT("Include all excluded assets")),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateLambda([&] { })
		),
		NAME_None,
		EUserInterfaceActionType::Button
	);

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> SPjcContentBrowser::GetBtnOptionsContent()
{
	const TSharedPtr<FExtender> Extender;
	FMenuBuilder MenuBuilder(true, nullptr, Extender, true);

	MenuBuilder.AddMenuSeparator(TEXT("Actions"));

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("RealtimeThumbnails")),
		FText::FromString(TEXT("Enable realtime thunmbnails")),
		FSlateIcon(),
		FUIAction
		(
			FExecuteAction::CreateLambda([&] { })
		),
		NAME_None,
		EUserInterfaceActionType::Button
	);

	return MenuBuilder.MakeWidget();
}

FSlateColor SPjcContentBrowser::GetOptionsBtnForegroundColor() const
{
	static const FName InvertedForegroundName("InvertedForeground");
	static const FName DefaultForegroundName("DefaultForeground");

	if (!OptionBtn.IsValid()) return FEditorStyle::GetSlateColor(DefaultForegroundName);

	return OptionBtn->IsHovered() ? FEditorStyle::GetSlateColor(InvertedForegroundName) : FEditorStyle::GetSlateColor(DefaultForegroundName);
}
