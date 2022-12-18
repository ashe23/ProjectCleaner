// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/Tabs/SProjectCleanerTabScanInfo.h"
#include "FrontendFilters/ProjectCleanerFrontendFilterExcluded.h"
#include "ProjectCleaner.h"
#include "ProjectCleanerCmds.h"
#include "ProjectCleanerSubsystem.h"
// Engine Headers
#include "ContentBrowserModule.h"
// #include "FrontendFilterBase.h"
#include "IContentBrowserSingleton.h"
#include "ObjectTools.h"
#include "Settings/ProjectCleanerExcludeSettings.h"
#include "Toolkits/GlobalEditorCommonCommands.h"

void SProjectCleanerTabScanInfo::Construct(const FArguments& InArgs)
{
	if (!GEditor) return;
	SubsystemPtr = GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>();
	if (!SubsystemPtr) return;

	const FContentBrowserModule& ModuleContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	CommandsRegister();

	SubsystemPtr->OnProjectScanned().AddLambda([&]()
	{
		if (AssetBrowserDelegateFilter.IsBound())
		{
			AssetBrowserDelegateFilter.Execute(AssetBrowserCreateFilter());
		}
	});

	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.bAllowNullSelection = false;
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::Tile;
	AssetPickerConfig.bCanShowFolders = false;
	AssetPickerConfig.bAddFilterUI = true;
	AssetPickerConfig.bSortByPathInColumnView = true;
	AssetPickerConfig.bShowPathInColumnView = true;
	AssetPickerConfig.bShowBottomToolbar = true;
	AssetPickerConfig.bCanShowDevelopersFolder = true;
	AssetPickerConfig.bCanShowClasses = false;
	AssetPickerConfig.bAllowDragging = false;
	AssetPickerConfig.bForceShowEngineContent = false;
	AssetPickerConfig.bCanShowRealTimeThumbnails = SubsystemPtr->bShowRealtimeThumbnails;
	AssetPickerConfig.AssetShowWarningText = FText::FromName("No assets");
	AssetPickerConfig.GetCurrentSelectionDelegates.Add(&AssetBrowserDelegateSelection);
	AssetPickerConfig.RefreshAssetViewDelegates.Add(&AssetBrowserDelegateRefreshView);
	AssetPickerConfig.SetFilterDelegates.Add(&AssetBrowserDelegateFilter);
	AssetPickerConfig.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateLambda([](const FAssetData& AssetData)
	{
		if (!AssetData.IsValid()) return;
		if (!GEditor) return;

		TArray<FName> AssetNames;
		AssetNames.Add(AssetData.ToSoftObjectPath().GetAssetPathName());

		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorsForAssets(AssetNames);
	});
	AssetPickerConfig.OnGetAssetContextMenu = FOnGetAssetContextMenu::CreateRaw(this, &SProjectCleanerTabScanInfo::AssetBrowserContextMenuCreate);
	AssetPickerConfig.Filter = AssetBrowserCreateFilter();

	const TSharedPtr<FFrontendFilterCategory> DefaultCategory = MakeShareable(new FFrontendFilterCategory(FText::FromString(TEXT("ProjectCleaner Filters")), FText::FromString(TEXT(""))));
	const TSharedPtr<FFrontendFilterExcludedAssets> FilterExcluded = MakeShareable(new FFrontendFilterExcludedAssets(DefaultCategory));
	FilterExcluded->OnFilterChange().AddLambda([&](const bool bActive)
	{
		bFilterExcludeActive = bActive;
		if (AssetBrowserDelegateFilter.IsBound())
		{
			AssetBrowserDelegateFilter.Execute(AssetBrowserCreateFilter());
		}
	});
	AssetPickerConfig.ExtraFrontendFilters.Add(FilterExcluded.ToSharedRef());

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		  .Padding(FMargin{5.0f})
		  .FillHeight(1.0f)
		[
			SNew(SSplitter)
			.Style(FEditorStyle::Get(), "ContentBrowser.Splitter")
			.PhysicalSplitterHandleSize(5.0f)
			.Orientation(Orient_Vertical)
			+ SSplitter::Slot()
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("TreeView")))
				// todo:ashe23
			]
			+ SSplitter::Slot()
			[
				ModuleContentBrowser.Get().CreateAssetPicker(AssetPickerConfig)
			]
		]
	];
}

void SProjectCleanerTabScanInfo::TreeViewUpdate()
{
}

void SProjectCleanerTabScanInfo::CommandsRegister()
{
	Cmds = MakeShareable(new FUICommandList);


	FUIAction ActionPathExclude;
	ActionPathExclude.ExecuteAction = FExecuteAction::CreateLambda([&]()
	{
		// if (!TreeView.IsValid()) return;
		// if (!SubsystemPtr) return;
		//
		// UProjectCleanerExcludeSettings* ExcludeSettings = GetMutableDefault<UProjectCleanerExcludeSettings>();
		// if (!ExcludeSettings) return;
		//
		// const auto Items = TreeView->GetSelectedItems();
		// for (const auto& Item : Items)
		// {
		// 	if (!FPaths::DirectoryExists(Item->FolderPathAbs)) continue;
		//
		// 	ExcludeSettings->ExcludedFolders.Add(FDirectoryPath{Item->FolderPathRel});
		// }
		//
		// ExcludeSettings->PostEditChange();
		//
		// SubsystemPtr->ProjectScan();
	});
	ActionPathExclude.CanExecuteAction = FCanExecuteAction::CreateLambda([&]()
	{
		return false;
		// return TreeView.IsValid() && TreeView.Get()->GetSelectedItems().Num() > 0;
	});

	Cmds->MapAction(FProjectCleanerCmds::Get().PathExclude, ActionPathExclude);

	FUIAction ActionPathShowExplorer;
	ActionPathShowExplorer.ExecuteAction = FExecuteAction::CreateLambda([&]()
	{
		// if (!TreeView.IsValid()) return;
		//
		// const auto Items = TreeView->GetSelectedItems();
		// for (const auto& Item : Items)
		// {
		// 	if (!FPaths::DirectoryExists(Item->FolderPathAbs)) continue;
		//
		// 	FPlatformProcess::ExploreFolder(*Item->FolderPathAbs);
		// }
	});
	ActionPathShowExplorer.CanExecuteAction = FCanExecuteAction::CreateLambda([&]()
	{
		return false;
		// return TreeView.IsValid() && TreeView.Get()->GetSelectedItems().Num() > 0;
	});

	Cmds->MapAction(FProjectCleanerCmds::Get().PathShowInExplorer, ActionPathShowExplorer);

	FUIAction ActionAssetLocate;
	ActionAssetLocate.ExecuteAction = FExecuteAction::CreateLambda([&]()
	{
		const FContentBrowserModule& ModuleContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ModuleContentBrowser.Get().SyncBrowserToAssets(AssetBrowserDelegateSelection.Execute());
	});
	ActionAssetLocate.CanExecuteAction = FCanExecuteAction::CreateLambda([&]()
	{
		return AssetBrowserDelegateSelection.IsBound() && AssetBrowserDelegateSelection.Execute().Num() > 0;
	});

	Cmds->MapAction(FProjectCleanerCmds::Get().AssetLocateInBrowser, ActionAssetLocate);

	FUIAction ActionAssetExclude;
	ActionAssetExclude.ExecuteAction = FExecuteAction::CreateLambda([&]()
	{
		if (!SubsystemPtr) return;

		UProjectCleanerExcludeSettings* ExcludeSettings = GetMutableDefault<UProjectCleanerExcludeSettings>();
		if (!ExcludeSettings) return;

		const auto SelectedItems = AssetBrowserDelegateSelection.Execute();
		for (const auto& SelectedItem : SelectedItems)
		{
			if (!SelectedItem.IsValid()) continue;
			if (!SelectedItem.GetAsset()) continue;

			ExcludeSettings->ExcludedAssets.AddUnique(SelectedItem.GetAsset());
		}

		ExcludeSettings->PostEditChange();

		SubsystemPtr->ProjectScan();
	});
	ActionAssetExclude.CanExecuteAction = FCanExecuteAction::CreateLambda([&]()
	{
		return AssetBrowserDelegateSelection.IsBound() && AssetBrowserDelegateSelection.Execute().Num() > 0;
	});
	ActionAssetExclude.IsActionVisibleDelegate = FIsActionButtonVisible::CreateLambda([&]()
	{
		return !bFilterExcludeActive && !bFilterPrimaryActive;
	});

	Cmds->MapAction(FProjectCleanerCmds::Get().AssetExclude, ActionAssetExclude);

	FUIAction ActionAssetExcludeByType;
	ActionAssetExcludeByType.ExecuteAction = FExecuteAction::CreateLambda([&]()
	{
		if (!SubsystemPtr) return;

		UProjectCleanerExcludeSettings* ExcludeSettings = GetMutableDefault<UProjectCleanerExcludeSettings>();
		if (!ExcludeSettings) return;

		const auto SelectedItems = AssetBrowserDelegateSelection.Execute();
		for (const auto& SelectedItem : SelectedItems)
		{
			if (!SelectedItem.IsValid()) continue;
			if (!SelectedItem.GetAsset()) continue;

			const UClass* AssetClass = SubsystemPtr->GetAssetClass(SelectedItem);
			if (!AssetClass) continue;

			ExcludeSettings->ExcludedClasses.AddUnique(AssetClass);
		}

		ExcludeSettings->PostEditChange();

		SubsystemPtr->ProjectScan();
	});
	ActionAssetExcludeByType.CanExecuteAction = FCanExecuteAction::CreateLambda([&]()
	{
		return AssetBrowserDelegateSelection.IsBound() && AssetBrowserDelegateSelection.Execute().Num() > 0;
	});
	ActionAssetExcludeByType.IsActionVisibleDelegate = FIsActionButtonVisible::CreateLambda([&]()
	{
		return !bFilterExcludeActive && !bFilterPrimaryActive;
	});

	Cmds->MapAction(FProjectCleanerCmds::Get().AssetExcludeByType, ActionAssetExcludeByType);

	FUIAction ActionAssetDelete;
	ActionAssetDelete.ExecuteAction = FExecuteAction::CreateLambda([&]()
	{
		if (ObjectTools::DeleteAssets(AssetBrowserDelegateSelection.Execute()) > 0)
		{
			SubsystemPtr->ProjectScan();
		}
	});
	ActionAssetDelete.CanExecuteAction = FCanExecuteAction::CreateLambda([&]()
	{
		return AssetBrowserDelegateSelection.IsBound() && AssetBrowserDelegateSelection.Execute().Num() > 0;
	});
	ActionAssetDelete.IsActionVisibleDelegate = FIsActionButtonVisible::CreateLambda([&]()
	{
		return !bFilterExcludeActive && !bFilterPrimaryActive;
	});

	Cmds->MapAction(FProjectCleanerCmds::Get().AssetDelete, ActionAssetDelete);

	FUIAction ActionAssetDeleteLinked;
	ActionAssetDeleteLinked.ExecuteAction = FExecuteAction::CreateLambda([&]()
	{
		if (!SubsystemPtr) return;

		const auto SelectedAssets = AssetBrowserDelegateSelection.Execute();

		TArray<FAssetData> Dependencies;
		SubsystemPtr->GetAssetsDependencies(SelectedAssets, Dependencies);
		// todo:ashe23 recursive get referencers and dependencies and filter only unused

		TArray<FAssetData> LinkedAssets;
		LinkedAssets.Reserve(Dependencies.Num() + SelectedAssets.Num());

		for (const auto& Asset : SelectedAssets)
		{
			LinkedAssets.AddUnique(Asset);
		}
		
		for (const auto& Asset : Dependencies)
		{
			if (SubsystemPtr->GetScanData().AssetsUnused.Contains(Asset))
			{
				LinkedAssets.AddUnique(Asset);
			}
		}

		LinkedAssets.Shrink();

		if (ObjectTools::DeleteAssets(LinkedAssets) > 0)
		{
			SubsystemPtr->ProjectScan();
		}
	});
	ActionAssetDeleteLinked.CanExecuteAction = FCanExecuteAction::CreateLambda([&]()
	{
		return AssetBrowserDelegateSelection.IsBound() && AssetBrowserDelegateSelection.Execute().Num() > 0;
	});
	ActionAssetDeleteLinked.IsActionVisibleDelegate = FIsActionButtonVisible::CreateLambda([&]()
	{
		return !bFilterExcludeActive && !bFilterPrimaryActive;
	});

	Cmds->MapAction(FProjectCleanerCmds::Get().AssetDeleteLinked, ActionAssetDeleteLinked);
}

FARFilter SProjectCleanerTabScanInfo::AssetBrowserCreateFilter() const
{
	FARFilter Filter;

	if (bFilterExcludeActive || bFilterPrimaryActive)
	{
		if (SubsystemPtr->GetScanData().AssetsExcluded.Num() == 0 && SubsystemPtr->GetScanData().AssetsPrimary.Num() == 0)
		{
			Filter.TagsAndValues.Add(FName{"ProjectCleanerEmptyTag"}, FString{"ProjectCleanerEmptyTag"});
		}
		else
		{
			Filter.PackageNames.Reserve(SubsystemPtr->GetScanData().AssetsExcluded.Num() + SubsystemPtr->GetScanData().AssetsPrimary.Num());

			if (bFilterExcludeActive)
			{
				for (const auto& Asset : SubsystemPtr->GetScanData().AssetsExcluded)
				{
					Filter.PackageNames.Emplace(Asset.PackageName);
				}
			}

			if (bFilterPrimaryActive)
			{
				for (const auto& Asset : SubsystemPtr->GetScanData().AssetsPrimary)
				{
					Filter.PackageNames.Emplace(Asset.PackageName);
				}
			}
		}

		return Filter;
	}

	if (SubsystemPtr->GetScanData().AssetsUnused.Num() == 0)
	{
		// this is needed for disabling showing primary assets in browser, when there is no unused assets
		Filter.TagsAndValues.Add(FName{TEXT("ProjectCleanerEmptyTag")}, FString{TEXT("ProjectCleanerEmptyTag")});
	}
	else
	{
		Filter.PackageNames.Reserve(SubsystemPtr->GetScanData().AssetsUnused.Num());

		for (const auto& Asset : SubsystemPtr->GetScanData().AssetsUnused)
		{
			Filter.PackageNames.Add(Asset.PackageName);
		}
	}

	return Filter;
}

TSharedPtr<SWidget> SProjectCleanerTabScanInfo::AssetBrowserContextMenuCreate(const TArray<FAssetData>& SelectedAssets) const
{
	FMenuBuilder MenuBuilder{true, Cmds};
	MenuBuilder.BeginSection(TEXT("Asset"), FText::FromName(TEXT("Asset Actions")));
	{
		MenuBuilder.AddMenuEntry(FProjectCleanerCmds::Get().AssetLocateInBrowser);
		MenuBuilder.AddMenuEntry(FProjectCleanerCmds::Get().AssetExclude);
		MenuBuilder.AddMenuEntry(FProjectCleanerCmds::Get().AssetExcludeByType);
		MenuBuilder.AddMenuEntry(FProjectCleanerCmds::Get().AssetDelete);
		MenuBuilder.AddMenuEntry(FProjectCleanerCmds::Get().AssetDeleteLinked);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}
