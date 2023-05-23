// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabAssetsIndirect.h"
#include "Slate/SPjcItemFileInfo.h"
#include "PjcStyles.h"
#include "PjcSubsystem.h"
#include "PjcConstants.h"
// Engine Headers
#include "IContentBrowserSingleton.h"
#include "PjcCmds.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"

void SPjcTabAssetsIndirect::Construct(const FArguments& InArgs)
{
	Cmds = MakeShareable(new FUICommandList);

	Cmds->MapAction(
		FPjcCmds::Get().Refresh,
		FExecuteAction::CreateLambda([&]()
		{
			UPjcSubsystem::GetAssetsByCategory(EPjcAssetCategory::Indirect, AssetsIndirect);

			Filter.Clear();

			Filter.ObjectPaths.Reserve(AssetsIndirect.Num());

			for (const auto& Asset : AssetsIndirect)
			{
				Filter.ObjectPaths.Emplace(Asset.ToSoftObjectPath().GetAssetPathName());
			}

			DelegateFilter.Execute(Filter);
		})
	);

	Filter.TagsAndValues.Emplace(PjcConstants::EmptyTagName, PjcConstants::EmptyTagName.ToString());

	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.bAllowDragging = false;
	AssetPickerConfig.bCanShowClasses = false;
	AssetPickerConfig.bCanShowFolders = false;
	AssetPickerConfig.bShowBottomToolbar = false;
	AssetPickerConfig.bAddFilterUI = true;
	AssetPickerConfig.bCanShowDevelopersFolder = true;
	AssetPickerConfig.bForceShowEngineContent = false;
	AssetPickerConfig.bForceShowPluginContent = false;
	AssetPickerConfig.bCanShowRealTimeThumbnails = true;
	AssetPickerConfig.SelectionMode = ESelectionMode::SingleToggle;
	AssetPickerConfig.GetCurrentSelectionDelegates.Add(&DelegateSelection);
	AssetPickerConfig.RefreshAssetViewDelegates.Add(&DelegateRefreshView);
	AssetPickerConfig.SetFilterDelegates.Add(&DelegateFilter);
	AssetPickerConfig.AssetShowWarningText = FText::FromString(TEXT("No assets"));
	AssetPickerConfig.Filter = Filter;
	AssetPickerConfig.bAllowNullSelection = true;
	AssetPickerConfig.OnAssetDoubleClicked.BindLambda([](const FAssetData& InAsset)
	{
		UPjcSubsystem::OpenAssetEditor(InAsset);
	});
	AssetPickerConfig.OnAssetSelected.BindLambda([&](const FAssetData& InAsset)
	{
		Items.Reset();

		if (!InAsset.IsValid()) return;
		if (!ListView.IsValid()) return;

		TArray<FPjcFileInfo> Infos;
		UPjcSubsystem::GetAssetIndirectInfo(InAsset, Infos);

		for (const auto& Info : Infos)
		{
			Items.Emplace(MakeShareable(new FPjcFileInfo{Info.FileNum, Info.FilePath}));
		}

		ListView->RebuildList();
	});

	SAssignNew(ListView, SListView<TSharedPtr<FPjcFileInfo>>)
	.ListItemsSource(&Items)
	.SelectionMode(ESelectionMode::None)
	.OnGenerateRow(this, &SPjcTabAssetsIndirect::OnGenerateRow)
	.HeaderRow(GetHeaderRow());

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
		[
			CreateToolbar()
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(10.0f, 0.0f)
		[
			SNew(SSeparator).Thickness(5.0f)
		]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5.0f)
		[
			SNew(SSplitter)
			.PhysicalSplitterHandleSize(3.0f)
			.Style(FEditorStyle::Get(), "DetailsView.Splitter")
			+ SSplitter::Slot().Value(0.5f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5.0f)
				[
					UPjcSubsystem::GetModuleContentBrowser().Get().CreateAssetPicker(AssetPickerConfig)
				]
			]
			+ SSplitter::Slot().Value(0.5f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin{10.0f, 0.0f, 5.0f, 5.0f})
				[
					SNew(STextBlock)
					.Justification(ETextJustify::Center)
					.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Gray"))
					.ShadowOffset(FVector2D{0.5f, 0.5f})
					.ShadowColorAndOpacity(FLinearColor::Black)
					.Font(FPjcStyles::GetFont("Bold", 15))
					.Text(FText::FromString(TEXT("List of assets used in source code or config files.")))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin{10.0f, 0.0f, 5.0f, 5.0f})
				[
					SNew(STextBlock)
					.Justification(ETextJustify::Center)
					.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Gray"))
					.ShadowOffset(FVector2D{0.5f, 0.5f})
					.ShadowColorAndOpacity(FLinearColor::Black)
					.Font(FPjcStyles::GetFont("Bold", 10))
					.Text(FText::FromString(TEXT("Select asset in order to see their usage info.")))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin{10.0f, 0.0f, 5.0f, 5.0f})
				[
					SNew(SSearchBox)
					.HintText(FText::FromString(TEXT("Search files...")))
				]
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(FMargin{10.0f, 0.0f, 5.0f, 5.0f})
				[
					SNew(SScrollBox)
					.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
					.AnimateWheelScrolling(true)
					.AllowOverscroll(EAllowOverscroll::No)
					+ SScrollBox::Slot()
					[
						ListView.ToSharedRef()
					]
				]
			]
		]
	];
}

TSharedRef<SWidget> SPjcTabAssetsIndirect::CreateToolbar() const
{
	FToolBarBuilder ToolBarBuilder{Cmds, FMultiBoxCustomization::None};
	ToolBarBuilder.BeginSection(NAME_None);
	{
		ToolBarBuilder.AddToolBarButton(
			FPjcCmds::Get().Refresh,
			NAME_None,
			FText::FromString(TEXT("Scan")),
			FText::FromString(TEXT("Scan for indirect assets and their usage info"))
		);
	}
	ToolBarBuilder.EndSection();

	return ToolBarBuilder.MakeWidget();
}

TSharedRef<SHeaderRow> SPjcTabAssetsIndirect::GetHeaderRow()
{
	return
		SNew(SHeaderRow)
		+ SHeaderRow::Column(TEXT("FilePath"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .FillWidth(0.8f)
		  .HeaderContentPadding(FMargin{5.0f})
		// .OnSort_Raw(this, &SPjcTreeView::OnTreeSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("File Path")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
			.ToolTipText(FText::FromString(TEXT("Absolute file path where asset is used.")))
		]
		+ SHeaderRow::Column(TEXT("FileLine"))
		  .HAlignHeader(HAlign_Center)
		  .VAlignHeader(VAlign_Center)
		  .FillWidth(0.2f)
		  .HeaderContentPadding(FMargin{5.0f})
		// .OnSort_Raw(this, &SPjcTreeView::OnTreeSort)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("File Line")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", 10.0f))
			.ToolTipText(FText::FromString(TEXT("File line number where asset is used")))
		];
}

TSharedRef<ITableRow> SPjcTabAssetsIndirect::OnGenerateRow(TSharedPtr<FPjcFileInfo> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SPjcItemFileInfo, OwnerTable).Item(Item);
}
