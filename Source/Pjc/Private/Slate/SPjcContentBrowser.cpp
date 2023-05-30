// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcContentBrowser.h"
#include "Slate/SPjcTreeView.h"
#include "PjcCmds.h"
#include "PjcSubsystem.h"
#include "PjcConstants.h"
#include "PjcFrontendFilters.h"
// Engine Headers
#include "FrontendFilterBase.h"
#include "ObjectTools.h"
#include "IContentBrowserSingleton.h"
#include "Widgets/Layout/SSeparator.h"


void SPjcContentBrowser::Construct(const FArguments& InArgs)
{
	TreeViewPtr = InArgs._TreeViewPtr;
	SubsystemPtr = GEditor->GetEditorSubsystem<UPjcSubsystem>();

	if (TreeViewPtr.IsValid())
	{
		TreeViewPtr->OnSelectionChanged().BindLambda([&]()
		{
			UpdateView();
		});
	}

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

			UpdateView();
		}),
		FCanExecuteAction::CreateLambda([&]()
		{
			return !bFilterAssetsExcludedActive && DelegateSelection.Execute().Num() > 0;
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
			return !bFilterAssetsUnusedActive && DelegateSelection.Execute().Num() > 0;
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
			return !bFilterAssetsUnusedActive && DelegateSelection.Execute().Num() > 0;
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
			return bFilterAssetsUnusedActive && DelegateSelection.Execute().Num() > 0;
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
			return bFilterAssetsUnusedActive && DelegateSelection.Execute().Num() > 0;
		})
	);

	CreateContentBrowser();

	UpdateView();

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
	];
}

SPjcContentBrowser::~SPjcContentBrowser()
{
	if (TreeViewPtr)
	{
		TreeViewPtr->OnSelectionChanged().Unbind();
	}
}

void SPjcContentBrowser::UpdateView()
{
	if (!TreeViewPtr.IsValid()) return;

	Filter.Clear();

	TArray<FAssetData> AssetsUnused;
	UPjcSubsystem::GetAssetsUnused(AssetsUnused, false);

	const TSet<FString>& SelectedPaths = TreeViewPtr->GetSelectedPaths();

	if (SelectedPaths.Num() > 0)
	{
		Filter.bRecursivePaths = true;

		for (const auto& SelectedPath : TreeViewPtr->GetSelectedPaths())
		{
			Filter.PackagePaths.Emplace(FName{*SelectedPath});
		}
	}

	if (AnyFilterActive())
	{
		if (bFilterAssetsUsedActive)
		{
			TArray<FAssetData> AssetsUsed;
			UPjcSubsystem::GetAssetsUsed(AssetsUsed, false);

			Filter.ObjectPaths.Reserve(AssetsUsed.Num());

			for (const auto& Asset : AssetsUsed)
			{
				Filter.ObjectPaths.Emplace(Asset.ToSoftObjectPath().GetAssetPathName());
			}
		}

		DelegateFilter.Execute(Filter);

		return;
	}

	if (AssetsUnused.Num() > 0 && bFilterAssetsUnusedActive)
	{
		Filter.ObjectPaths.Reserve(AssetsUnused.Num());

		for (const auto& Asset : AssetsUnused)
		{
			Filter.ObjectPaths.Emplace(Asset.ToSoftObjectPath().GetAssetPathName());
		}
	}
	else
	{
		Filter.TagsAndValues.Emplace(PjcConstants::EmptyTagName, PjcConstants::EmptyTagName.ToString());
	}

	DelegateFilter.Execute(Filter);
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
	}
	ToolBarBuilder.EndSection();

	return ToolBarBuilder.MakeWidget();
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
	AssetPickerConfig.bShowBottomToolbar = true;
	AssetPickerConfig.bCanShowDevelopersFolder = true;
	AssetPickerConfig.bForceShowEngineContent = false;
	AssetPickerConfig.bForceShowPluginContent = false;
	AssetPickerConfig.bAllowNullSelection = false;
	AssetPickerConfig.bCanShowClasses = false;
	AssetPickerConfig.bCanShowRealTimeThumbnails = false;
	AssetPickerConfig.AssetShowWarningText = FText::FromString(TEXT("No assets"));
	AssetPickerConfig.Filter = Filter;
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

	const TSharedPtr<FFrontendFilterCategory> DefaultCategory = MakeShareable(new FFrontendFilterCategory(FText::FromString(TEXT("ProjectCleaner Filters")), FText::FromString(TEXT(""))));
	const TSharedPtr<FPjcFilterAssetsUsed> FilterUsed = MakeShareable(new FPjcFilterAssetsUsed(DefaultCategory));
	const TSharedPtr<FPjcFilterAssetsPrimary> FilterPrimary = MakeShareable(new FPjcFilterAssetsPrimary(DefaultCategory));
	const TSharedPtr<FPjcFilterAssetsIndirect> FilterIndirect = MakeShareable(new FPjcFilterAssetsIndirect(DefaultCategory));
	const TSharedPtr<FPjcFilterAssetsCircular> FilterCircular = MakeShareable(new FPjcFilterAssetsCircular(DefaultCategory));
	const TSharedPtr<FPjcFilterAssetsEditor> FilterEditor = MakeShareable(new FPjcFilterAssetsEditor(DefaultCategory));
	const TSharedPtr<FPjcFilterAssetsExcluded> FilterExcluded = MakeShareable(new FPjcFilterAssetsExcluded(DefaultCategory));
	const TSharedPtr<FPjcFilterAssetsExtReferenced> FilterExtReferenced = MakeShareable(new FPjcFilterAssetsExtReferenced(DefaultCategory));


	FilterUsed->OnFilterChanged().AddLambda([&](const bool bActive)
	{
		bFilterAssetsUsedActive = bActive;
		bFilterAssetsUnusedActive = !AnyFilterActive();

		UpdateView();
	});

	FilterPrimary->OnFilterChanged().AddLambda([&](const bool bActive)
	{
		bFilterAssetsPrimaryActive = bActive;
		bFilterAssetsUnusedActive = !AnyFilterActive();

		UpdateView();
	});

	FilterIndirect->OnFilterChanged().AddLambda([&](const bool bActive)
	{
		bFilterAssetsIndirectActive = bActive;
		bFilterAssetsUnusedActive = !AnyFilterActive();

		UpdateView();
	});

	FilterCircular->OnFilterChanged().AddLambda([&](const bool bActive)
	{
		bFilterAssetsCircularActive = bActive;
		bFilterAssetsUnusedActive = !AnyFilterActive();

		UpdateView();
	});

	FilterEditor->OnFilterChanged().AddLambda([&](const bool bActive)
	{
		bFilterAssetsEditorActive = bActive;
		bFilterAssetsUnusedActive = !AnyFilterActive();

		UpdateView();
	});

	FilterExcluded->OnFilterChanged().AddLambda([&](const bool bActive)
	{
		bFilterAssetsExcludedActive = bActive;
		bFilterAssetsUnusedActive = !AnyFilterActive();

		UpdateView();
	});

	FilterExtReferenced->OnFilterChanged().AddLambda([&](const bool bActive)
	{
		bFilterAssetsExtReferencedActive = bActive;
		bFilterAssetsUnusedActive = !AnyFilterActive();

		UpdateView();
	});


	AssetPickerConfig.ExtraFrontendFilters.Emplace(FilterUsed.ToSharedRef());
	AssetPickerConfig.ExtraFrontendFilters.Emplace(FilterPrimary.ToSharedRef());
	AssetPickerConfig.ExtraFrontendFilters.Emplace(FilterIndirect.ToSharedRef());
	AssetPickerConfig.ExtraFrontendFilters.Emplace(FilterCircular.ToSharedRef());
	AssetPickerConfig.ExtraFrontendFilters.Emplace(FilterEditor.ToSharedRef());
	AssetPickerConfig.ExtraFrontendFilters.Emplace(FilterExcluded.ToSharedRef());
	AssetPickerConfig.ExtraFrontendFilters.Emplace(FilterExtReferenced.ToSharedRef());

	ContentBrowserPtr.Reset();
	ContentBrowserPtr = UPjcSubsystem::GetModuleContentBrowser().Get().CreateAssetPicker(AssetPickerConfig);
}

bool SPjcContentBrowser::AnyFilterActive() const
{
	return
		bFilterAssetsUsedActive ||
		bFilterAssetsPrimaryActive ||
		bFilterAssetsEditorActive ||
		bFilterAssetsIndirectActive ||
		bFilterAssetsExcludedActive ||
		bFilterAssetsExtReferencedActive;
}
