// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabAssetsUnused.h"
#include "Slate/SPjcTreeView.h"
#include "Slate/SPjcStatAssets.h"
#include "Slate/SPjcContentBrowser.h"
#include "PjcCmds.h"
#include "PjcTypes.h"
#include "PjcSubsystem.h"
// Engine Headers
#include "FileHelpers.h"
#include "Settings/ContentBrowserSettings.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"

void SPjcTabAssetsUnused::Construct(const FArguments& InArgs)
{
	UContentBrowserSettings* ContentBrowserSettings = GetMutableDefault<UContentBrowserSettings>();
	if (ContentBrowserSettings)
	{
		ContentBrowserSettings->SetDisplayDevelopersFolder(true);
		ContentBrowserSettings->SetDisplayEngineFolder(false);
		ContentBrowserSettings->SetDisplayCppFolders(false);
		ContentBrowserSettings->SetDisplayPluginFolders(false);
		ContentBrowserSettings->PostEditChange();
	}

	Cmds = MakeShareable(new FUICommandList);

	Cmds->MapAction(
		FPjcCmds::Get().ScanProject,
		FExecuteAction::CreateLambda([&]()
		{
			if (UPjcSubsystem::GetModuleAssetRegistry().Get().IsLoadingAssets())
			{
				UPjcSubsystem::ShowNotification(TEXT("Failed to scan project. AssetRegistry is still discovering assets. Please try again after it has finished."), SNotificationItem::CS_Fail, 5.0f);
				return;
			}

			TArray<FAssetData> Redirectors;
			UPjcSubsystem::GetProjectRedirectors(Redirectors);
			UPjcSubsystem::FixProjectRedirectors(Redirectors);

			if (UPjcSubsystem::ProjectHasRedirectors())
			{
				UPjcSubsystem::ShowNotificationWithOutputLog(TEXT("Failed to scan project, because not all redirectors are fixed."), SNotificationItem::CS_Fail, 5.0f);
				return;
			}

			if (!FEditorFileUtils::SaveDirtyPackages(true, true, true, false, false, false))
			{
				UPjcSubsystem::ShowNotificationWithOutputLog(TEXT("Failed to scan project, because not all assets have been saved."), SNotificationItem::CS_Fail, 5.0f);
				return;
			}

			UpdateView();
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().CleanProject,
		FExecuteAction::CreateLambda([&]()
		{
			// todo:ashe23 show window here 
		}),
		FCanExecuteAction::CreateLambda([&]()
		{
			return bCanCleanProject;
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().ClearExcludeSettings,
		FExecuteAction::CreateLambda([&]()
		{
			UPjcAssetExcludeSettings* AssetExcludeSettings = GetMutableDefault<UPjcAssetExcludeSettings>();
			if (!AssetExcludeSettings) return;

			AssetExcludeSettings->ExcludedAssets.Empty();
			AssetExcludeSettings->ExcludedClasses.Empty();
			AssetExcludeSettings->ExcludedFolders.Empty();
			AssetExcludeSettings->PostEditChange();

			UpdateView();
		})
	);

	FPropertyEditorModule& PropertyEditor = UPjcSubsystem::GetModulePropertyEditor();
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bShowOptions = true;
	DetailsViewArgs.bAllowFavoriteSystem = false;
	DetailsViewArgs.bShowPropertyMatrixButton = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.ViewIdentifier = "PjcEditorAssetExcludeSettings";

	const auto SettingsProperty = PropertyEditor.CreateDetailView(DetailsViewArgs);
	SettingsProperty->SetObject(GetMutableDefault<UPjcAssetExcludeSettings>());

	SAssignNew(TreeViewPtr, SPjcTreeView);
	SAssignNew(ContentBrowserPtr, SPjcContentBrowser).TreeViewPtr(TreeViewPtr);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5.0f)
		[
			SNew(SSplitter)
			.PhysicalSplitterHandleSize(3.0f)
			.Style(FEditorStyle::Get(), "DetailsView.Splitter")
			+ SSplitter::Slot().Value(0.2f)
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
				+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
				[
					SAssignNew(StatAssetsPtr, SPjcStatAssets)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
				[
					SNew(SSeparator).Thickness(5.0f)
				]
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(3.0f)
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
			+ SSplitter::Slot().Value(0.35f)
			[
				TreeViewPtr.ToSharedRef()
			]
			+ SSplitter::Slot().Value(0.45f)
			[
				ContentBrowserPtr.ToSharedRef()
			]
		]
	];
}


// void SPjcTabAssetsUnused::OnCleanProject()
// {
// 	// todo:ashe23 cleanup setting window here, with option to delete empty folders afterwards
//
// 	// ObjectTools::DeleteAssets(AssetsCategoryMapping[EPjcAssetCategory::Unused].Array(), true);
//
// 	// const TSharedRef<SWindow> Window = SNew(SWindow).Title(FText::FromString(TEXT("Some Title"))).ClientSize(FVector2D{600, 400});
// 	// const TSharedRef<SWidget> Content =
// 	// 	SNew(SVerticalBox)
// 	// 	+ SVerticalBox::Slot()
// 	// 	[
// 	// 		SNew(STextBlock).Text(FText::FromString(TEXT("Some Content")))
// 	// 	];
// 	//
// 	// Window->SetContent(Content);
// 	//
// 	// if (GEditor)
// 	// {
// 	// 	GEditor->EditorAddModalWindow(Window);
// 	// }
// }

TSharedRef<SWidget> SPjcTabAssetsUnused::CreateToolbar() const
{
	FToolBarBuilder ToolBarBuilder{Cmds, FMultiBoxCustomization::None};
	ToolBarBuilder.BeginSection("PjcSectionTabAssetUnusedActions");
	{
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().ScanProject);
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().CleanProject);
		ToolBarBuilder.AddSeparator();
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().ClearExcludeSettings);
	}
	ToolBarBuilder.EndSection();

	return ToolBarBuilder.MakeWidget();
}

void SPjcTabAssetsUnused::UpdateView()
{
	if (StatAssetsPtr.IsValid())
	{
		StatAssetsPtr->UpdateView();
	}

	if (TreeViewPtr.IsValid())
	{
		TreeViewPtr->UpdateData();
		TreeViewPtr->UpdateView();
	}

	if (ContentBrowserPtr.IsValid())
	{
		ContentBrowserPtr->UpdateView();
	}

	TArray<FAssetData> AssetsUnused;
	UPjcSubsystem::GetAssetsUnused(AssetsUnused, false);

	bCanCleanProject = AssetsUnused.Num() > 0;
}
