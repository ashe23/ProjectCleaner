// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "SLate/AssetBrowser/SPjcAssetBrowser.h"
#include "Slate/AssetStats/SPjcAssetStats.h"
#include "Slate/TreeView/SPjcTreeView.h"
#include "EditorSettings/PjcEditorAssetExcludeSettings.h"
#include "PjcCmds.h"
#include "PjcStyles.h"
#include "PjcConstants.h"
#include "PjcSubsystem.h"
#include "Libs/PjcLibEditor.h"
#include "Libs/PjcLibAsset.h"
#include "FrontendFilters/PjcFrontendFilterAssetsEditor.h"
#include "FrontendFilters/PjcFrontendFilterAssetsExcluded.h"
#include "FrontendFilters/PjcFrontendFilterAssetsExtReferenced.h"
#include "FrontendFilters/PjcFrontendFilterAssetsIndirect.h"
#include "FrontendFilters/PjcFrontendFilterAssetsPrimary.h"
#include "FrontendFilters/PjcFrontendFilterAssetsUsed.h"
// Engine Headers
#include "ObjectTools.h"
#include "ContentBrowserModule.h"
#include "FrontendFilterBase.h"
#include "IContentBrowserSingleton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"

void SPjcAssetBrowser::Construct(const FArguments& InArgs)
{
	if (!GEditor) return;

	SubsystemPtr = GEditor->GetEditorSubsystem<UPjcSubsystem>();
	if (!SubsystemPtr) return;

	SubsystemPtr->OnScanAssets().AddRaw(this, &SPjcAssetBrowser::OnScanAssets);

	CmdsRegister();

	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bShowOptions = true;
	DetailsViewArgs.bAllowFavoriteSystem = false;
	DetailsViewArgs.bShowPropertyMatrixButton = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.ViewIdentifier = "PjcScaSettingsEditor";

	const auto SettingsProperty = PropertyEditor.CreateDetailView(DetailsViewArgs);
	SettingsProperty->SetObject(GetMutableDefault<UPjcEditorAssetExcludeSettings>());

	const FContentBrowserModule& ModuleContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(PjcConstants::ModuleContentBrowser);

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
	AssetPickerConfig.AssetShowWarningText = FText::FromName("No Assets");
	AssetPickerConfig.OnAssetDoubleClicked.BindRaw(this, &SPjcAssetBrowser::OnAssetDblClick);
	AssetPickerConfig.GetCurrentSelectionDelegates.Add(&DelegateSelection);
	AssetPickerConfig.RefreshAssetViewDelegates.Add(&DelegateRefreshView);
	AssetPickerConfig.SetFilterDelegates.Add(&DelegateFilter);
	AssetPickerConfig.Filter = Filter;
	AssetPickerConfig.OnGetAssetContextMenu.BindRaw(this, &SPjcAssetBrowser::OnGetAssetContextMenu);

	const TSharedPtr<FFrontendFilterCategory> DefaultCategory = MakeShareable(new FFrontendFilterCategory(FText::FromString(TEXT("ProjectCleaner Filters")), FText::FromString(TEXT(""))));
	const TSharedPtr<FPjcFilterAssetsUsed> FilterUsed = MakeShareable(new FPjcFilterAssetsUsed(DefaultCategory));
	const TSharedPtr<FPjcFilterAssetsPrimary> FilterPrimary = MakeShareable(new FPjcFilterAssetsPrimary(DefaultCategory));
	const TSharedPtr<FPjcFilterAssetsIndirect> FilterIndirect = MakeShareable(new FPjcFilterAssetsIndirect(DefaultCategory));
	const TSharedPtr<FPjcFilterAssetsEditor> FilterEditor = MakeShareable(new FPjcFilterAssetsEditor(DefaultCategory));
	const TSharedPtr<FPjcFilterAssetsExtReferenced> FilterExtReferenced = MakeShareable(new FPjcFilterAssetsExtReferenced(DefaultCategory));
	const TSharedPtr<FPjcFilterAssetsExcluded> FilterExcluded = MakeShareable(new FPjcFilterAssetsExcluded(DefaultCategory));

	FilterUsed->OnFilterChanged().AddRaw(this, &SPjcAssetBrowser::OnFilterUsedChanged);
	FilterPrimary->OnFilterChanged().AddRaw(this, &SPjcAssetBrowser::OnFilterPrimaryChanged);
	FilterIndirect->OnFilterChanged().AddRaw(this, &SPjcAssetBrowser::OnFilterIndirectChanged);
	FilterEditor->OnFilterChanged().AddRaw(this, &SPjcAssetBrowser::OnFilterEditorChanged);
	FilterExtReferenced->OnFilterChanged().AddRaw(this, &SPjcAssetBrowser::OnFilterExtReferencedChanged);
	FilterExcluded->OnFilterChanged().AddRaw(this, &SPjcAssetBrowser::OnFilterExcludedChanged);

	AssetPickerConfig.ExtraFrontendFilters.Add(FilterUsed.ToSharedRef());
	AssetPickerConfig.ExtraFrontendFilters.Add(FilterPrimary.ToSharedRef());
	AssetPickerConfig.ExtraFrontendFilters.Add(FilterIndirect.ToSharedRef());
	AssetPickerConfig.ExtraFrontendFilters.Add(FilterEditor.ToSharedRef());
	AssetPickerConfig.ExtraFrontendFilters.Add(FilterExtReferenced.ToSharedRef());
	AssetPickerConfig.ExtraFrontendFilters.Add(FilterExcluded.ToSharedRef());

	const TSharedRef<SWidget> ContentBrowserView = ModuleContentBrowser.Get().CreateAssetPicker(AssetPickerConfig);

	FilterUpdate();

	TreeViewPtr = SNew(SPjcTreeView);
	TreeViewPtr->OnPathSelectionChanged().BindRaw(this, &SPjcAssetBrowser::OnPathSelectionChanged);

	ChildSlot
	[
		SNew(SSplitter)
		.Style(FEditorStyle::Get(), "DetailsView.Splitter")
		.PhysicalSplitterHandleSize(3.0f)
		+ SSplitter::Slot()
		.Value(0.3f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
					.ContentPadding(FMargin{3.0f})
					.OnClicked_Raw(this, &SPjcAssetBrowser::OnBtnScanAssetsClick)
					.ButtonColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Blue"))
					[
						SNew(STextBlock)
						.Justification(ETextJustify::Center)
						.ToolTipText(FText::FromString(TEXT("Scan project for unused assets and empty folders")))
						.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.White"))
						.ShadowOffset(FVector2D{1.5f, 1.5f})
						.ShadowColorAndOpacity(FLinearColor::Black)
						.Font(FPjcStyles::GetFont("Bold", 10))
						.Text(FText::FromString(TEXT("Scan Project")))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{5.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(SButton)
					.ContentPadding(FMargin{3.0f})
					.ButtonColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Red"))
					[
						SNew(STextBlock)
						.Justification(ETextJustify::Center)
						.ToolTipText(FText::FromString(TEXT("Cleanup project based on CleanupMethod setting.")))
						.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.White"))
						.ShadowOffset(FVector2D{1.5f, 1.5f})
						.ShadowColorAndOpacity(FLinearColor::Black)
						.Font(FPjcStyles::GetFont("Bold", 10))
						.Text(FText::FromString(TEXT("Clean Project")))
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
			[
				SNew(SSeparator).Thickness(5.0f)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
			[
				SNew(SPjcAssetStats).Padding(FMargin{5.0f})
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
			[
				SNew(SSeparator).Thickness(5.0f)
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5.0f)
			[
				SNew(SBox)
				[
					SNew(SScrollBox)
					.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
					.AnimateWheelScrolling(true)
					.AllowOverscroll(EAllowOverscroll::No)
					+ SScrollBox::Slot()
					[
						SettingsProperty
					]
				]
			]
		]
		+ SSplitter::Slot()
		.Value(0.7f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5.0f)
			[
				SNew(SSplitter)
				.Style(FEditorStyle::Get(), "DetailsView.Splitter")
				.PhysicalSplitterHandleSize(3.0f)
				+ SSplitter::Slot().Value(0.45f)
				[
					TreeViewPtr.ToSharedRef()
				]
				+ SSplitter::Slot().Value(0.55f)
				[
					ContentBrowserView
				]
			]
		]
	];
}

SPjcAssetBrowser::~SPjcAssetBrowser()
{
	if (SubsystemPtr)
	{
		SubsystemPtr->OnScanAssets().RemoveAll(this);
	}

	if (TreeViewPtr)
	{
		TreeViewPtr->OnPathSelectionChanged().Unbind();
	}
}

FReply SPjcAssetBrowser::OnBtnScanAssetsClick() const
{
	if (SubsystemPtr)
	{
		SubsystemPtr->ScanProjectAssets();
	}

	return FReply::Handled();
}

void SPjcAssetBrowser::OnScanAssets()
{
	FilterUpdate();
}

void SPjcAssetBrowser::OnPathSelectionChanged(const TArray<FName>& InSelectedPaths)
{
	SelectedPaths.Empty();
	SelectedPaths.Append(InSelectedPaths);

	FilterUpdate();
}

void SPjcAssetBrowser::FilterUpdate()
{
	if (!SubsystemPtr) return;

	Filter.Clear();

	if (AnyFilterEnabled())
	{
		if (bFilterUsedActive)
		{
			Filter.PackageNames.Reserve(SubsystemPtr->GetAssetsUsed().Num());

			for (const auto& Asset : SubsystemPtr->GetAssetsUsed())
			{
				Filter.PackageNames.Emplace(Asset.PackageName);
			}
		}

		if (bFilterPrimaryActive)
		{
			Filter.PackageNames.Reserve(Filter.PackageNames.Num() + SubsystemPtr->GetAssetsPrimary().Num());

			for (const auto& Asset : SubsystemPtr->GetAssetsPrimary())
			{
				Filter.PackageNames.Emplace(Asset.PackageName);
			}
		}

		if (bFilterIndirectActive)
		{
			Filter.PackageNames.Reserve(Filter.PackageNames.Num() + SubsystemPtr->GetAssetsIndirect().Num());

			for (const auto& Asset : SubsystemPtr->GetAssetsIndirect())
			{
				Filter.PackageNames.Emplace(Asset.PackageName);
			}
		}

		if (bFilterEditorActive)
		{
			Filter.PackageNames.Reserve(Filter.PackageNames.Num() + SubsystemPtr->GetAssetsEditor().Num());

			for (const auto& Asset : SubsystemPtr->GetAssetsEditor())
			{
				Filter.PackageNames.Emplace(Asset.PackageName);
			}
		}

		if (bFilterExtReferencedActive)
		{
			Filter.PackageNames.Reserve(Filter.PackageNames.Num() + SubsystemPtr->GetAssetsExtReferenced().Num());

			for (const auto& Asset : SubsystemPtr->GetAssetsExtReferenced())
			{
				Filter.PackageNames.Emplace(Asset.PackageName);
			}
		}

		if (bFilterExcludedActive)
		{
			Filter.PackageNames.Reserve(Filter.PackageNames.Num() + SubsystemPtr->GetAssetsExcluded().Num());

			for (const auto& Asset : SubsystemPtr->GetAssetsExcluded())
			{
				Filter.PackageNames.Emplace(Asset.PackageName);
			}
		}
	}
	else
	{
		Filter.PackageNames.Reserve(SubsystemPtr->GetAssetsUnused().Num());

		for (const auto& Asset : SubsystemPtr->GetAssetsUnused())
		{
			Filter.PackageNames.Emplace(Asset.PackageName);
		}
	}

	if (Filter.PackageNames.Num() == 0)
	{
		// this is needed for disabling showing primary assets in browser, when there is no unused assets
		Filter.TagsAndValues.Add(PjcConstants::EmptyTagName, PjcConstants::EmptyTagName.ToString());
	}

	if (SelectedPaths.Num() > 0)
	{
		Filter.PackagePaths.Append(SelectedPaths);
	}

	DelegateFilter.Execute(Filter);
}

void SPjcAssetBrowser::OnAssetDblClick(const FAssetData& AssetData)
{
	FPjcLibEditor::OpenAssetEditor(AssetData);
}

void SPjcAssetBrowser::OnFilterUsedChanged(const bool bActive)
{
	bFilterUsedActive = bActive;

	FilterUpdate();
}

void SPjcAssetBrowser::OnFilterPrimaryChanged(const bool bActive)
{
	bFilterPrimaryActive = bActive;

	FilterUpdate();
}

void SPjcAssetBrowser::OnFilterIndirectChanged(const bool bActive)
{
	bFilterIndirectActive = bActive;

	FilterUpdate();
}

void SPjcAssetBrowser::OnFilterEditorChanged(const bool bActive)
{
	bFilterEditorActive = bActive;

	FilterUpdate();
}

void SPjcAssetBrowser::OnFilterExtReferencedChanged(const bool bActive)
{
	bFilterExtReferencedActive = bActive;

	FilterUpdate();
}

void SPjcAssetBrowser::OnFilterExcludedChanged(const bool bActive)
{
	bFilterExcludedActive = bActive;

	FilterUpdate();
}

void SPjcAssetBrowser::CmdsRegister()
{
	Cmds = MakeShareable(new FUICommandList);

	Cmds->MapAction
	(
		FPjcCmds::Get().OpenSizeMap,
		FUIAction
		(
			FExecuteAction::CreateLambda([&]()
			{
				FPjcLibEditor::OpenSizeMapViewer(DelegateSelection.Execute());
			})
		)
	);

	Cmds->MapAction
	(
		FPjcCmds::Get().OpenReferenceViewer,
		FUIAction
		(
			FExecuteAction::CreateLambda([&]()
			{
				FPjcLibEditor::OpenReferenceViewer(DelegateSelection.Execute());
			})
		)
	);

	Cmds->MapAction
	(
		FPjcCmds::Get().OpenAssetAudit,
		FUIAction
		(
			FExecuteAction::CreateLambda([&]()
			{
				FPjcLibEditor::OpenAssetAuditViewer(DelegateSelection.Execute());
			})
		)
	);

	Cmds->MapAction
	(
		FPjcCmds::Get().AssetsDelete,
		FUIAction
		(
			FExecuteAction::CreateLambda([&]()
			{
				const TArray<FAssetData> AssetsSelected = DelegateSelection.Execute();

				const int32 NumAssetsTotal = AssetsSelected.Num();
				const int32 NumAssetsDeleted = ObjectTools::DeleteAssets(AssetsSelected, true);

				if (NumAssetsDeleted > 0)
				{
					const FString Msg = FString::Printf(TEXT("Deleted %d of %d assets."), NumAssetsDeleted, NumAssetsTotal);

					if (NumAssetsTotal == NumAssetsDeleted)
					{
						FPjcLibEditor::ShowNotification(Msg, SNotificationItem::CS_Success, 5.0f);
					}
					else
					{
						FPjcLibEditor::ShowNotificationWithOutputLog(Msg, SNotificationItem::CS_Fail, 5.0f);
					}

					SubsystemPtr->ScanProjectAssets();
				}
			})
		)
	);

	Cmds->MapAction
	(
		FPjcCmds::Get().AssetsExclude,
		FUIAction
		(
			FExecuteAction::CreateLambda([&]()
			{
				UPjcEditorAssetExcludeSettings* AssetExcludeSettings = GetMutableDefault<UPjcEditorAssetExcludeSettings>();
				if (!AssetExcludeSettings) return;

				const TArray<FAssetData>& AssetsSelected = DelegateSelection.Execute();

				for (const auto& Asset : AssetsSelected)
				{
					AssetExcludeSettings->ExcludedAssets.Emplace(Asset.ToSoftObjectPath());
				}

				AssetExcludeSettings->PostEditChange();

				SubsystemPtr->ScanProjectAssets();
			}),
			FCanExecuteAction::CreateLambda([&]()
			{
				return SubsystemPtr && !bFilterExcludedActive;
			}),
			FGetActionCheckState::CreateLambda([&]()
			{
				return ECheckBoxState::Checked;
			}),
			FIsActionButtonVisible::CreateLambda([&]()
			{
				return !bFilterExcludedActive;
			})
		)
	);

	Cmds->MapAction
	(
		FPjcCmds::Get().AssetsExcludeByClass,
		FUIAction
		(
			FExecuteAction::CreateLambda([&]()
			{
				UPjcEditorAssetExcludeSettings* AssetExcludeSettings = GetMutableDefault<UPjcEditorAssetExcludeSettings>();
				if (!AssetExcludeSettings) return;

				const TArray<FAssetData>& AssetsSelected = DelegateSelection.Execute();

				for (const auto& Asset : AssetsSelected)
				{
					const UClass* AssetExactClass = FPjcLibAsset::GetClassByName(FPjcLibAsset::GetAssetClassName(Asset));
					if (!AssetExactClass) continue;

					AssetExcludeSettings->ExcludedClasses.Emplace(AssetExactClass);
				}

				AssetExcludeSettings->PostEditChange();

				SubsystemPtr->ScanProjectAssets();
			}),
			FCanExecuteAction::CreateLambda([&]()
			{
				return SubsystemPtr && !bFilterExcludedActive;
			}),
			FGetActionCheckState::CreateLambda([&]()
			{
				return ECheckBoxState::Checked;
			}),
			FIsActionButtonVisible::CreateLambda([&]()
			{
				return !bFilterExcludedActive;
			})
		)
	);
}

bool SPjcAssetBrowser::AnyFilterEnabled() const
{
	return bFilterUsedActive || bFilterPrimaryActive || bFilterIndirectActive || bFilterEditorActive || bFilterExtReferencedActive || bFilterExcludedActive;
}

TSharedPtr<SWidget> SPjcAssetBrowser::OnGetAssetContextMenu(const TArray<FAssetData>& Assets) const
{
	FMenuBuilder MenuBuilder{true, Cmds};

	MenuBuilder.BeginSection(TEXT("PjcSectionAssetInfo"), FText::FromName(TEXT("Info")));
	{
		MenuBuilder.AddMenuEntry(FPjcCmds::Get().OpenSizeMap);
		MenuBuilder.AddMenuEntry(FPjcCmds::Get().OpenReferenceViewer);
		MenuBuilder.AddMenuEntry(FPjcCmds::Get().OpenAssetAudit);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection(TEXT("PjcSectionAssetExclusion"), FText::FromName(TEXT("Exclusion")));
	{
		MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsExclude);
		MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsExcludeByClass);
		// MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsInclude);
		// MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsIncludeByClass);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection(TEXT("PjcSectionAssetCleanup"), FText::FromName(TEXT("Cleanup")));
	{
		MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsDelete);
		// MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsExcludeByClass);
		// MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsInclude);
		// MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsIncludeByClass);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}
