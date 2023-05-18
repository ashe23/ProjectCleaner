// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcContentBrowser.h"
#include "PjcCmds.h"
#include "PjcSubsystem.h"
#include "PjcConstants.h"
// Engine Headers
#include "FrontendFilterBase.h"
#include "ObjectTools.h"
#include "IContentBrowserSingleton.h"
#include "Pjc.h"
#include "PjcFrontendFilters.h"
#include "Widgets/Layout/SSeparator.h"

void SPjcContentBrowser::Construct(const FArguments& InArgs)
{
	SubsystemPtr = GEditor->GetEditorSubsystem<UPjcSubsystem>();
	check(SubsystemPtr);

	Cmds = MakeShareable(new FUICommandList);

	Cmds->MapAction(
		FPjcCmds::Get().OpenViewerSizeMap,
		FExecuteAction::CreateLambda([&]()
		{
			UPjcSubsystem::OpenSizeMapViewer(DelegateSelection.Execute());
		}),
		FCanExecuteAction::CreateLambda([&]()
		{
			return DelegateSelection.Execute().Num() > 0;
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().OpenViewerReference,
		FExecuteAction::CreateLambda([&]()
		{
			UPjcSubsystem::OpenReferenceViewer(DelegateSelection.Execute());
		}),
		FCanExecuteAction::CreateLambda([&]()
		{
			return DelegateSelection.Execute().Num() > 0;
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().OpenViewerAssetsAudit,
		FExecuteAction::CreateLambda([&]()
		{
			UPjcSubsystem::OpenAssetAuditViewer(DelegateSelection.Execute());
		}),
		FCanExecuteAction::CreateLambda([&]()
		{
			return DelegateSelection.Execute().Num() > 0;
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().AssetsExclude,
		FExecuteAction::CreateLambda([&]()
		{
			UPjcAssetExcludeSettings* ExcludeSettings = GetMutableDefault<UPjcAssetExcludeSettings>();
			if (!ExcludeSettings) return;

			for (const auto& Asset : DelegateSelection.Execute())
			{
				ExcludeSettings->ExcludedAssets.Emplace(Asset.ToSoftObjectPath());
			}
			ExcludeSettings->PostEditChange();

			// todo:ashe23 rescan project
		}),
		FCanExecuteAction::CreateLambda([&]()
		{
			return bUnusedAssetsMode && DelegateSelection.Execute().Num() > 0;
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().AssetsExcludeByClass,
		FExecuteAction::CreateLambda([&]()
		{
			UPjcAssetExcludeSettings* ExcludeSettings = GetMutableDefault<UPjcAssetExcludeSettings>();
			if (!ExcludeSettings) return;

			for (const auto& Asset : DelegateSelection.Execute())
			{
				const UClass* AssetClass = UPjcSubsystem::GetAssetClassByName(UPjcSubsystem::GetAssetExactClassName(Asset));
				if (!AssetClass) continue;

				ExcludeSettings->ExcludedClasses.Emplace(AssetClass);
			}

			ExcludeSettings->PostEditChange();

			// todo:ashe23 rescan project
		}),
		FCanExecuteAction::CreateLambda([&]()
		{
			return bUnusedAssetsMode && DelegateSelection.Execute().Num() > 0;
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().AssetsInclude,
		FExecuteAction::CreateLambda([&]()
		{
			// todo:ashe23 check if selected assets are not exclude by paths

			UPjcAssetExcludeSettings* ExcludeSettings = GetMutableDefault<UPjcAssetExcludeSettings>();
			if (!ExcludeSettings) return;

			TSet<FName> ObjectPaths;

			for (const auto& Asset : DelegateSelection.Execute())
			{
				ObjectPaths.Emplace(Asset.ToSoftObjectPath().GetAssetPathName());
			}

			ExcludeSettings->ExcludedAssets.RemoveAllSwap([&](const TSoftObjectPtr<UObject>& ExcludedAsset)
			{
				return ObjectPaths.Contains(ExcludedAsset.ToSoftObjectPath().GetAssetPathName());
			}, false);
			ExcludeSettings->PostEditChange();

			// todo:ashe23 rescan project
		}),
		FCanExecuteAction::CreateLambda([&]()
		{
			return !bUnusedAssetsMode && DelegateSelection.Execute().Num() > 0;
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().AssetsIncludeByClass,
		FExecuteAction::CreateLambda([&]()
		{
			UPjcAssetExcludeSettings* ExcludeSettings = GetMutableDefault<UPjcAssetExcludeSettings>();
			if (!ExcludeSettings) return;

			TSet<FName> ClassNames;

			for (const auto& Asset : DelegateSelection.Execute())
			{
				ClassNames.Emplace(UPjcSubsystem::GetAssetExactClassName(Asset));
			}

			ExcludeSettings->ExcludedClasses.RemoveAllSwap([&](const TSoftClassPtr<UObject>& ExcludedClass)
			{
				return ClassNames.Contains(ExcludedClass.Get()->GetFName());
			}, false);

			ExcludeSettings->PostEditChange();

			// todo:ashe23 rescan project
		}),
		FCanExecuteAction::CreateLambda([&]()
		{
			return !bUnusedAssetsMode && DelegateSelection.Execute().Num() > 0;
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().AssetsDelete,
		FExecuteAction::CreateLambda([&]()
		{
			// todo:ashe23 show separate view for delete confirmation
			ObjectTools::DeleteAssets(DelegateSelection.Execute());

			// todo:ashe23 rescan project
		}),
		FCanExecuteAction::CreateLambda([&]()
		{
			return DelegateSelection.Execute().Num() > 0;
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().ThumbnailSizeTiny,
		FExecuteAction::CreateLambda([&]()
		{
			ThumbnailSize = 0.01f;
			CreateContentBrowser();
			UpdateView();
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().ThumbnailSizeSmall,
		FExecuteAction::CreateLambda([&]()
		{
			ThumbnailSize = 0.1f;
			CreateContentBrowser();
			UpdateView();
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().ThumbnailSizeMedium,
		FExecuteAction::CreateLambda([&]()
		{
			ThumbnailSize = 0.2f;
			CreateContentBrowser();
			UpdateView();
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().ThumbnailSizeLarge,
		FExecuteAction::CreateLambda([&]()
		{
			ThumbnailSize = 0.3f;
			CreateContentBrowser();
			UpdateView();
		})
	);

	CreateContentBrowser();
	UpdateView();
}

void SPjcContentBrowser::FilterUpdate(const FARFilter& InFilter)
{
	Filter = InFilter;

	DelegateFilter.Execute(Filter);
}

void SPjcContentBrowser::UpdateView()
{
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
		[
			CreateToolbar()
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
		[
			SNew(SSeparator).Thickness(5.0f)
		]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5.0f)
		[
			ContentBrowserPtr.ToSharedRef()
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Left).VAlign(VAlign_Center)
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

	DelegateFilter.Execute(Filter);
}

FText SPjcContentBrowser::GetSummaryText() const
{
	const auto ItemsSelected = DelegateSelection.Execute();

	TArray<FAssetData> AssetsTotal;
	UPjcSubsystem::GetModuleAssetRegistry().Get().GetAssets(Filter, AssetsTotal);

	const FString NumAssetsTotal = FText::AsNumber(AssetsTotal.Num()).ToString();
	const FString NumAssetsSelected = FText::AsNumber(ItemsSelected.Num()).ToString();

	if (ItemsSelected.Num() > 0)
	{
		return FText::FromString(FString::Printf(TEXT("Selected %s of %s items"), *NumAssetsSelected, *NumAssetsTotal));
	}

	return FText::FromString(FString::Printf(TEXT("Items - %s"), *NumAssetsTotal));
}

TSharedRef<SWidget> SPjcContentBrowser::CreateToolbar() const
{
	FToolBarBuilder ToolBarBuilder{Cmds, FMultiBoxCustomization::None};
	ToolBarBuilder.BeginSection("PjcSectionActionsAssets");
	{
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().AssetsDelete);
		ToolBarBuilder.AddSeparator();
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().AssetsExclude);
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().AssetsExcludeByClass);
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().AssetsInclude);
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().AssetsIncludeByClass);
		ToolBarBuilder.AddSeparator();
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().OpenViewerSizeMap);
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().OpenViewerReference);
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().OpenViewerAssetsAudit);
		ToolBarBuilder.AddSeparator();
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().OpenViewerAssetsIndirect);
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().OpenViewerAssetsCorrupted);
	}
	ToolBarBuilder.EndSection();

	return ToolBarBuilder.MakeWidget();
}

TSharedRef<SWidget> SPjcContentBrowser::GetBtnOptionsContent()
{
	const TSharedPtr<FExtender> Extender;
	FMenuBuilder MenuBuilder(true, Cmds, Extender, true);

	MenuBuilder.BeginSection(TEXT("Thumbnails"), FText::FromString(TEXT("Thumbnails")));
	{
		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("RealtimeThumbnails")),
			FText::FromString(TEXT("Enable realtime thunmbnails")),
			FSlateIcon(),
			FUIAction
			(
				FExecuteAction::CreateLambda([&]
				{
					SubsystemPtr->bShowRealtimeThumbnails = !SubsystemPtr->bShowRealtimeThumbnails;
					SubsystemPtr->PostEditChange();
				}),
				FCanExecuteAction::CreateLambda([&]()
				{
					return true;
				}),
				FGetActionCheckState::CreateLambda([&]()
				{
					return SubsystemPtr->bShowRealtimeThumbnails ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddSubMenu(
			FText::FromString(TEXT("Thumbnail Size")),
			FText::FromString(TEXT("Select Thumbnail Size")),
			FNewMenuDelegate::CreateRaw(this, &SPjcContentBrowser::MakeSubmenu)
		);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}


void SPjcContentBrowser::CreateContentBrowser()
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
	AssetPickerConfig.bCanShowRealTimeThumbnails = SubsystemPtr->bShowRealtimeThumbnails;
	AssetPickerConfig.AssetShowWarningText = FText::FromString(TEXT("No assets"));
	AssetPickerConfig.Filter = Filter;
	AssetPickerConfig.ThumbnailScale = ThumbnailSize;
	AssetPickerConfig.bAllowNullSelection = true;
	AssetPickerConfig.GetCurrentSelectionDelegates.Add(&DelegateSelection);
	AssetPickerConfig.RefreshAssetViewDelegates.Add(&DelegateRefreshView);
	AssetPickerConfig.SetFilterDelegates.Add(&DelegateFilter);
	AssetPickerConfig.OnAssetDoubleClicked.BindLambda([](const FAssetData& InAsset)
	{
		UPjcSubsystem::OpenAssetEditor(InAsset);
	});
	AssetPickerConfig.OnGetAssetContextMenu.BindLambda([&](const TArray<FAssetData>&)
	{
		const TSharedPtr<FExtender> Extender;
		FMenuBuilder MenuBuilder(true, Cmds, Extender, true);

		MenuBuilder.BeginSection(TEXT("Info"), FText::FromString(TEXT("Info")));
		{
			MenuBuilder.AddMenuEntry(FPjcCmds::Get().OpenViewerSizeMap);
			MenuBuilder.AddMenuEntry(FPjcCmds::Get().OpenViewerReference);
			MenuBuilder.AddMenuEntry(FPjcCmds::Get().OpenViewerAssetsAudit);
		}
		MenuBuilder.EndSection();

		MenuBuilder.BeginSection(TEXT("Exclusion"), FText::FromString(TEXT("Exclusion")));
		{
			MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsExclude);
			MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsExcludeByClass);
		}
		MenuBuilder.EndSection();

		MenuBuilder.BeginSection(TEXT("Inclusion"), FText::FromString(TEXT("Inclusion")));
		{
			MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsInclude);
			MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsIncludeByClass);
		}
		MenuBuilder.EndSection();

		MenuBuilder.BeginSection(TEXT("Deletion"), FText::FromString(TEXT("Deletion")));
		{
			MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsDelete);
		}
		MenuBuilder.EndSection();

		return MenuBuilder.MakeWidget();
	});

	// todo:ashe23 maybe make this frontend filter optional?

	const TSharedPtr<FFrontendFilterCategory> DefaultCategory = MakeShareable(new FFrontendFilterCategory(FText::FromString(TEXT("ProjectCleaner Filters")), FText::FromString(TEXT(""))));
	const TSharedPtr<FPjcFilterAssetsUsed> FilterUsed = MakeShareable(new FPjcFilterAssetsUsed(DefaultCategory));
	FilterUsed->OnFilterChanged().AddLambda([](const bool bActive)
	{
		UE_LOG(LogProjectCleaner, Warning, TEXT("Fiter Used Assets Changed"));
	});

	AssetPickerConfig.ExtraFrontendFilters.Emplace(FilterUsed.ToSharedRef());

	ContentBrowserPtr.Reset();
	ContentBrowserPtr = UPjcSubsystem::GetModuleContentBrowser().Get().CreateAssetPicker(AssetPickerConfig);
}

void SPjcContentBrowser::MakeSubmenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(FPjcCmds::Get().ThumbnailSizeTiny);
	MenuBuilder.AddMenuEntry(FPjcCmds::Get().ThumbnailSizeSmall);
	MenuBuilder.AddMenuEntry(FPjcCmds::Get().ThumbnailSizeMedium);
	MenuBuilder.AddMenuEntry(FPjcCmds::Get().ThumbnailSizeLarge);
}

FSlateColor SPjcContentBrowser::GetOptionsBtnForegroundColor() const
{
	static const FName InvertedForegroundName("InvertedForeground");
	static const FName DefaultForegroundName("DefaultForeground");

	if (!OptionBtn.IsValid()) return FEditorStyle::GetSlateColor(DefaultForegroundName);

	return OptionBtn->IsHovered() ? FEditorStyle::GetSlateColor(InvertedForegroundName) : FEditorStyle::GetSlateColor(DefaultForegroundName);
}
