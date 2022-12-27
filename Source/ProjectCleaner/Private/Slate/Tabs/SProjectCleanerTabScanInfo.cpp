// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/Tabs/SProjectCleanerTabScanInfo.h"
#include "Slate/TreeView/SProjectCleanerTreeView.h"
#include "Settings/ProjectCleanerExcludeSettings.h"
#include "FrontendFilters/ProjectCleanerFrontendFilterExcluded.h"
#include "FrontendFilters/ProjectCleanerFrontendFilterPrimary.h"
#include "FrontendFilters/ProjectCleanerFrontendFilterUsed.h"
#include "FrontendFilters/ProjectCleanerFrontendFilterIndirect.h"
#include "ProjectCleanerCmds.h"
#include "ProjectCleanerSubsystem.h"
// Engine Headers
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "ObjectTools.h"
#include "Libs/ProjectCleanerLibAsset.h"
#include "Libs/ProjectCleanerLibPath.h"

void SProjectCleanerTabScanInfo::Construct(const FArguments& InArgs)
{
	if (!GEditor) return;
	SubsystemPtr = GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>();
	if (!SubsystemPtr) return;

	const FContentBrowserModule& ModuleContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	CommandsRegister();

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
	AssetPickerConfig.bCanShowRealTimeThumbnails = false;
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
	AssetPickerConfig.OnGetAssetContextMenu = FOnGetAssetContextMenu::CreateLambda([&](const TArray<FAssetData>& SelectedAssets)
	{
		FMenuBuilder MenuBuilder{true, Cmds};
		MenuBuilder.BeginSection(TEXT("AssetLocationActions"), FText::FromName(TEXT("Locations")));
		{
			MenuBuilder.AddMenuEntry(FProjectCleanerCmds::Get().AssetLocateInBrowser);
			MenuBuilder.AddMenuEntry(FProjectCleanerCmds::Get().AssetLocateInExplorer);
		}
		MenuBuilder.EndSection();
		MenuBuilder.BeginSection(TEXT("AssetExcludeActions"), FText::FromName(TEXT("Exclusion")));
		{
			MenuBuilder.AddMenuEntry(FProjectCleanerCmds::Get().AssetExclude);
			MenuBuilder.AddMenuEntry(FProjectCleanerCmds::Get().AssetExcludeByType);
		}
		MenuBuilder.EndSection();
		MenuBuilder.BeginSection(TEXT("AssetDeletionActions"), FText::FromName(TEXT("Deletion")));
		{
			MenuBuilder.AddMenuEntry(FProjectCleanerCmds::Get().AssetDelete);
			MenuBuilder.AddMenuEntry(FProjectCleanerCmds::Get().AssetDeleteLinked);
		}
		MenuBuilder.EndSection();

		return MenuBuilder.MakeWidget();
	});
	AssetPickerConfig.Filter = AssetBrowserCreateFilter();

	const TSharedPtr<FFrontendFilterCategory> DefaultCategory = MakeShareable(new FFrontendFilterCategory(FText::FromString(TEXT("ProjectCleaner Filters")), FText::FromString(TEXT(""))));
	const TSharedPtr<FFrontendFilterExcludedAssets> FilterExcluded = MakeShareable(new FFrontendFilterExcludedAssets(DefaultCategory));
	const TSharedPtr<FFrontendFilterPrimaryAssets> FilterPrimary = MakeShareable(new FFrontendFilterPrimaryAssets(DefaultCategory));
	const TSharedPtr<FFrontendFilterUsedAssets> FilterUsed = MakeShareable(new FFrontendFilterUsedAssets(DefaultCategory));
	const TSharedPtr<FFrontendFilterIndirectAssets> FilterIndirect = MakeShareable(new FFrontendFilterIndirectAssets(DefaultCategory));

	FilterExcluded->OnFilterChange().AddLambda([&](const bool bActive)
	{
		bFilterExcludeActive = bActive;
		if (AssetBrowserDelegateFilter.IsBound())
		{
			AssetBrowserDelegateFilter.Execute(AssetBrowserCreateFilter());
		}
	});
	FilterPrimary->OnFilterChange().AddLambda([&](const bool bActive)
	{
		bFilterPrimaryActive = bActive;
		if (AssetBrowserDelegateFilter.IsBound())
		{
			AssetBrowserDelegateFilter.Execute(AssetBrowserCreateFilter());
		}
	});
	FilterUsed->OnFilterChange().AddLambda([&](const bool bActive)
	{
		bFilterUsedActive = bActive;
		if (AssetBrowserDelegateFilter.IsBound())
		{
			AssetBrowserDelegateFilter.Execute(AssetBrowserCreateFilter());
		}
	});
	FilterIndirect->OnFilterChange().AddLambda([&](const bool bActive)
	{
		bFilterIndirectActive = bActive;
		if (AssetBrowserDelegateFilter.IsBound())
		{
			AssetBrowserDelegateFilter.Execute(AssetBrowserCreateFilter());
		}
	});

	AssetPickerConfig.ExtraFrontendFilters.Add(FilterExcluded.ToSharedRef());
	AssetPickerConfig.ExtraFrontendFilters.Add(FilterPrimary.ToSharedRef());
	AssetPickerConfig.ExtraFrontendFilters.Add(FilterUsed.ToSharedRef());
	AssetPickerConfig.ExtraFrontendFilters.Add(FilterIndirect.ToSharedRef());

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
				SNew(SProjectCleanerTreeView)
				.OnPathSelected_Raw(this, &SProjectCleanerTabScanInfo::OnTreeViewPathSelected)
			]
			+ SSplitter::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				  .FillHeight(1.0f)
				  .Padding(FMargin{0.0f, 5.0f})
				[
					ModuleContentBrowser.Get().CreateAssetPicker(AssetPickerConfig)
				]
			]
		]
	];
	
	SubsystemPtr->OnProjectScanned().AddRaw(this, &SProjectCleanerTabScanInfo::OnProjectScanned);
}

SProjectCleanerTabScanInfo::~SProjectCleanerTabScanInfo()
{
	SubsystemPtr->OnProjectScanned().RemoveAll(this);
	SubsystemPtr = nullptr;
}

void SProjectCleanerTabScanInfo::CommandsRegister()
{
	Cmds = MakeShareable(new FUICommandList);

	Cmds->MapAction
	(
		FProjectCleanerCmds::Get().AssetLocateInBrowser,
		FUIAction
		(
			FExecuteAction::CreateLambda([&]()
			{
				const FContentBrowserModule& ModuleContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
				ModuleContentBrowser.Get().SyncBrowserToAssets(AssetBrowserDelegateSelection.Execute());
			}),
			FCanExecuteAction::CreateLambda([&]
			{
				return AssetBrowserDelegateSelection.IsBound() && AssetBrowserDelegateSelection.Execute().Num() > 0;
			}),
			FIsActionChecked::CreateLambda([] { return true; }),
			FIsActionButtonVisible::CreateLambda([&]() { return true; })
		)
	);
	Cmds->MapAction
	(
		FProjectCleanerCmds::Get().AssetLocateInExplorer,
		FUIAction
		(
			FExecuteAction::CreateLambda([&]()
			{
				if (!SubsystemPtr) return;

				const auto& SelectedItems = AssetBrowserDelegateSelection.Execute();
				for (const auto& Asset : SelectedItems)
				{
					const FString FolderPathAbs = UProjectCleanerLibPath::ConvertToAbs(Asset.PackagePath.ToString());

					if (!FolderPathAbs.IsEmpty() && FPaths::DirectoryExists(FolderPathAbs))
					{
						FPlatformProcess::ExploreFolder(*FolderPathAbs);
					}
				}
			}),
			FCanExecuteAction::CreateLambda([&]
			{
				return AssetBrowserDelegateSelection.IsBound() && AssetBrowserDelegateSelection.Execute().Num() > 0;
			}),
			FIsActionChecked::CreateLambda([] { return true; }),
			FIsActionButtonVisible::CreateLambda([&]() { return true; })
		)
	);
	Cmds->MapAction
	(
		FProjectCleanerCmds::Get().AssetExclude,
		FUIAction
		(
			FExecuteAction::CreateLambda([&]()
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
			}),
			FCanExecuteAction::CreateLambda([&]
			{
				return SubsystemPtr && AssetBrowserDelegateSelection.IsBound() && AssetBrowserDelegateSelection.Execute().Num() > 0;
			}),
			FIsActionChecked::CreateLambda([] { return true; }),
			FIsActionButtonVisible::CreateLambda([&]() { return FilterAllDisabled(); })
		)
	);
	Cmds->MapAction
	(
		FProjectCleanerCmds::Get().AssetExcludeByType,
		FUIAction
		(
			FExecuteAction::CreateLambda([&]()
			{
				if (!SubsystemPtr) return;
				
				UProjectCleanerExcludeSettings* ExcludeSettings = GetMutableDefault<UProjectCleanerExcludeSettings>();
				if (!ExcludeSettings) return;
				
				const auto SelectedItems = AssetBrowserDelegateSelection.Execute();
				for (const auto& SelectedItem : SelectedItems)
				{
					if (!SelectedItem.IsValid()) continue;
					if (!SelectedItem.GetAsset()) continue;
				
					const UClass* AssetClass = UProjectCleanerLibAsset::GetAssetClass(SelectedItem);
					if (!AssetClass) continue;
				
					ExcludeSettings->ExcludedClasses.AddUnique(AssetClass);
				}
				
				ExcludeSettings->PostEditChange();
				
				SubsystemPtr->ProjectScan();
			}),
			FCanExecuteAction::CreateLambda([&]
			{
				return SubsystemPtr && AssetBrowserDelegateSelection.IsBound() && AssetBrowserDelegateSelection.Execute().Num() > 0;
			}),
			FIsActionChecked::CreateLambda([] { return true; }),
			FIsActionButtonVisible::CreateLambda([&]() { return FilterAllDisabled(); })
		)
	);
	Cmds->MapAction
	(
		FProjectCleanerCmds::Get().AssetDelete,
		FUIAction
		(
			FExecuteAction::CreateLambda([&]()
			{
				if (ObjectTools::DeleteAssets(AssetBrowserDelegateSelection.Execute()) > 0)
				{
					SubsystemPtr->ProjectScan();
				}
			}),
			FCanExecuteAction::CreateLambda([&]
			{
				return SubsystemPtr && AssetBrowserDelegateSelection.IsBound() && AssetBrowserDelegateSelection.Execute().Num() > 0;
			}),
			FIsActionChecked::CreateLambda([] { return true; }),
			FIsActionButtonVisible::CreateLambda([&]() { return FilterAllDisabled(); })
		)
	);
	Cmds->MapAction
	(
		FProjectCleanerCmds::Get().AssetDeleteLinked,
		FUIAction
		(
			FExecuteAction::CreateLambda([&]()
			{
				// todo:ashe23 very sloppy logic here, research about how to handle this case
				if (!SubsystemPtr) return;

				// const auto SelectedAssets = AssetBrowserDelegateSelection.Execute();
				// // for selected assets find all dependency assets that have 0 referencers and remove them
				//
				// TArray<FAssetData> Referencers;
				// TArray<FAssetData> Dependencies;
				// SubsystemPtr->GetAssetsReferencers(SelectedAssets, Referencers);
				// SubsystemPtr->GetAssetsDependencies(SelectedAssets, Dependencies);

				// return;

				// TArray<FAssetData> LinkedAssets;
				// LinkedAssets.Reserve(SelectedAssets.Num() + Referencers.Num());
				//
				// for (const auto& Asset : SelectedAssets)
				// {
				// 	LinkedAssets.AddUnique(Asset);
				// }
				//
				// for (const auto& Asset : Referencers)
				// {
				// 	if (SubsystemPtr->GetScanData().AssetsUnused.Contains(Asset))
				// 	{
				// 		LinkedAssets.AddUnique(Asset);
				// 	}
				// }
				//
				// LinkedAssets.Reserve(LinkedAssets.Num() + Dependencies.Num());
				// SubsystemPtr->GetAssetsDependencies(LinkedAssets, Dependencies);
				//
				// for (const auto& Asset : Dependencies)
				// {
				// 	if (SubsystemPtr->GetScanData().AssetsUnused.Contains(Asset))
				// 	{
				// 		LinkedAssets.AddUnique(Asset);
				// 	}
				// }
				//
				// LinkedAssets.Shrink();
				//
				// if (ObjectTools::DeleteAssets(LinkedAssets) > 0)
				// {
				// 	SubsystemPtr->ProjectScan();
				// }
			}),
			FCanExecuteAction::CreateLambda([&]
			{
				return SubsystemPtr && AssetBrowserDelegateSelection.IsBound() && AssetBrowserDelegateSelection.Execute().Num() > 0;
			}),
			FIsActionChecked::CreateLambda([] { return true; }),
			FIsActionButtonVisible::CreateLambda([&]() { return FilterAllDisabled(); })
		)
	);
}

void SProjectCleanerTabScanInfo::OnProjectScanned() const
{
	if (AssetBrowserDelegateFilter.IsBound())
	{
		AssetBrowserDelegateFilter.Execute(AssetBrowserCreateFilter());
	}
}

void SProjectCleanerTabScanInfo::OnTreeViewPathSelected(const TSet<FString>& InSelectedPaths)
{
	SelectedPaths = InSelectedPaths;
	
	if (AssetBrowserDelegateFilter.IsBound())
	{
		AssetBrowserDelegateFilter.Execute(AssetBrowserCreateFilter());
	}
}

FARFilter SProjectCleanerTabScanInfo::AssetBrowserCreateFilter() const
{
	FARFilter Filter;

	if (SelectedPaths.Num() > 0)
	{
		for (const auto& SelectedPath : SelectedPaths)
		{
			Filter.PackagePaths.AddUnique(FName{*SelectedPath});
		}
	}

	if (FilterAnyEnabled())
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

		if (bFilterUsedActive)
		{
			for (const auto& Asset : SubsystemPtr->GetScanData().AssetsUsed)
			{
				Filter.PackageNames.Emplace(Asset.PackageName);
			}
		}

		if (bFilterIndirectActive)
		{
			for (const auto& Asset : SubsystemPtr->GetScanData().AssetsIndirect)
			{
				Filter.PackageNames.Emplace(Asset.PackageName);
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

bool SProjectCleanerTabScanInfo::FilterAnyEnabled() const
{
	return bFilterExcludeActive || bFilterPrimaryActive || bFilterUsedActive || bFilterIndirectActive;
}

bool SProjectCleanerTabScanInfo::FilterAllDisabled() const
{
	return !bFilterExcludeActive && !bFilterPrimaryActive && !bFilterUsedActive && !bFilterIndirectActive;
}

bool SProjectCleanerTabScanInfo::FilterAllEnabled() const
{
	return bFilterExcludeActive && bFilterPrimaryActive && bFilterUsedActive && bFilterIndirectActive;
}
