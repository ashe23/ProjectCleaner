// // Copyright Ashot Barkhudaryan. All Rights Reserved.
//
// #include "Slate/SPjcTabAssetsUnused.h"
// #include "Slate/Shared/SPjcTreeView.h"
// #include "Slate/Shared/SPjcStatsBasic.h"
// #include "Subsystems/PjcSubsystemScanner.h"
// #include "PjcCmds.h"
// #include "PjcTypes.h"
// // Engine Headers
// #include "ContentBrowserModule.h"
// #include "IContentBrowserSingleton.h"
// #include "ObjectTools.h"
// #include "Pjc.h"
// #include "PjcStyles.h"
// #include "Settings/ContentBrowserSettings.h"
// #include "Subsystems/PjcSubsystemHelper.h"
// #include "Widgets/Layout/SScrollBox.h"
// #include "Widgets/Layout/SSeparator.h"
//
// void SPjcTabAssetsUnused::Construct(const FArguments& InArgs)
// {
// 	if (GEngine)
// 	{
// 		ScannerSubsystemPtr = GEngine->GetEngineSubsystem<UPjcScannerSubsystem>();
//
// 		if (ScannerSubsystemPtr)
// 		{
// 			ScannerSubsystemPtr->OnProjectAssetsScanSuccess().AddRaw(this, &SPjcTabAssetsUnused::OnProjectScanSuccess);
// 			ScannerSubsystemPtr->OnProjectAssetsScanFail().AddRaw(this, &SPjcTabAssetsUnused::OnProjectScanFail);
// 		}
// 	}
//
// 	Cmds = MakeShareable(new FUICommandList);
//
// 	Cmds->MapAction(
// 		FPjcCmds::Get().TabAssetsUnusedBtnScan,
// 		FExecuteAction::CreateLambda([&]()
// 		{
// 			if (ScannerSubsystemPtr)
// 			{
// 				ScannerSubsystemPtr->ScanProjectAssets();
// 			}
// 		})
// 	);
//
// 	Cmds->MapAction(
// 		FPjcCmds::Get().TabAssetsUnusedBtnClean,
// 		FExecuteAction::CreateLambda([&]()
// 		{
// 			UPjcHelperSubsystem::ShaderCompilationDisable();
// 			ObjectTools::DeleteAssets(ScannerSubsystemPtr->GetAssetsByCategory(EPjcAssetCategory::Unused).Array(), true);
// 			UPjcHelperSubsystem::ShaderCompilationEnable();
//
// 			// const TSharedRef<SWindow> Window = SNew(SWindow).Title(FText::FromString(TEXT("Some Title"))).ClientSize(FVector2D{600, 400});
// 			// const TSharedRef<SWidget> Content =
// 			// 	SNew(SVerticalBox)
// 			// 	+ SVerticalBox::Slot()
// 			// 	[
// 			// 		SNew(STextBlock).Text(FText::FromString(TEXT("Some Content")))
// 			// 	];
// 			//
// 			// Window->SetContent(Content);
// 			//
// 			// if (GEditor)
// 			// {
// 			// 	GEditor->EditorAddModalWindow(Window);
// 			// }
// 		}),
// 		FCanExecuteAction::CreateLambda([]()
// 		{
// 			return true;
// 		})
// 	);
//
// 	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
// 	FDetailsViewArgs DetailsViewArgs;
// 	DetailsViewArgs.bUpdatesFromSelection = false;
// 	DetailsViewArgs.bLockable = false;
// 	DetailsViewArgs.bAllowSearch = false;
// 	DetailsViewArgs.bShowOptions = true;
// 	DetailsViewArgs.bAllowFavoriteSystem = false;
// 	DetailsViewArgs.bShowPropertyMatrixButton = false;
// 	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
// 	DetailsViewArgs.ViewIdentifier = "PjcEditorAssetExcludeSettings";
//
// 	const auto SettingsProperty = PropertyEditor.CreateDetailView(DetailsViewArgs);
// 	SettingsProperty->SetObject(GetMutableDefault<UPjcEditorAssetExcludeSettings>());
//
// 	const FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
//
// 	UContentBrowserSettings* ContentBrowserSettings = GetMutableDefault<UContentBrowserSettings>();
// 	if (ContentBrowserSettings)
// 	{
// 		ContentBrowserSettings->SetDisplayDevelopersFolder(true);
// 		ContentBrowserSettings->SetDisplayEngineFolder(false);
// 		ContentBrowserSettings->SetDisplayCppFolders(false);
// 		ContentBrowserSettings->SetDisplayPluginFolders(false);
// 		ContentBrowserSettings->PostEditChange();
// 	}
//
// 	FPathPickerConfig PathPickerConfig;
// 	PathPickerConfig.bAllowClassesFolder = false;
// 	PathPickerConfig.bAllowContextMenu = false;
// 	PathPickerConfig.bFocusSearchBoxWhenOpened = false;
//
// 	FAssetPickerConfig AssetPickerConfig;
// 	AssetPickerConfig.bAllowDragging = false;
// 	AssetPickerConfig.bCanShowClasses = false;
// 	AssetPickerConfig.bCanShowFolders = false;
// 	AssetPickerConfig.bCanShowDevelopersFolder = true;
// 	AssetPickerConfig.bForceShowEngineContent = false;
// 	AssetPickerConfig.bForceShowPluginContent = false;
// 	AssetPickerConfig.bAddFilterUI = true;
// 	AssetPickerConfig.OnGetAssetContextMenu.BindRaw(this, &SPjcTabAssetsUnused::GetContentBrowserContextMenu);
//
// 	SAssignNew(TreeViewPtr, SPjcTreeView).HeaderPadding(FMargin{5.0f});
// 	if (TreeViewPtr.IsValid())
// 	{
// 		TreeViewPtr->OnTreeViewSelectionChanged().BindLambda([&](const TSet<FString>& InSelectedPaths)
// 		{
// 			// todo:ashe23 change content browser filter here
// 			
// 		});
// 	}
// 	StatItemsUpdate();
//
// 	ChildSlot
// 	[
// 		SNew(SVerticalBox)
// 		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5.0f)
// 		[
// 			SNew(SSplitter)
// 			.PhysicalSplitterHandleSize(3.0f)
// 			.Style(FEditorStyle::Get(), "DetailsView.Splitter")
// 			+ SSplitter::Slot().Value(0.2f)
// 			[
// 				SNew(SVerticalBox)
// 				+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
// 				[
// 					CreateToolbar()
// 				]
// 				+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
// 				[
// 					SNew(SSeparator).Thickness(5.0f)
// 				]
// 				+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
// 				[
// 					SAssignNew(StatsViewPtr, SPjcStatsBasic)
// 					.Title(FText::FromString(TEXT("Asset Statistics Summary")))
// 					.HeaderMargin(FMargin{10.0f})
// 					.InitialItems(&StatItems)
// 				]
// 				+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
// 				[
// 					SNew(SSeparator).Thickness(5.0f)
// 				]
// 				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(3.0f)
// 				[
// 					SNew(SBox)
// 					[
// 						SNew(SScrollBox)
// 						.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
// 						.AnimateWheelScrolling(true)
// 						.AllowOverscroll(EAllowOverscroll::No)
// 						+ SScrollBox::Slot()
// 						[
// 							SettingsProperty
// 						]
// 					]
// 				]
// 			]
// 			+ SSplitter::Slot().Value(0.35f)
// 			[
// 				TreeViewPtr.ToSharedRef()
// 			]
// 			+ SSplitter::Slot().Value(0.45f)
// 			[
// 				SNew(SVerticalBox)
// 				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5.0f)
// 				[
// 					ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
// 				]
// 			]
// 		]
// 	];
// }
//
// SPjcTabAssetsUnused::~SPjcTabAssetsUnused()
// {
// 	if (ScannerSubsystemPtr)
// 	{
// 		ScannerSubsystemPtr->OnProjectAssetsScanSuccess().RemoveAll(this);
// 		ScannerSubsystemPtr->OnProjectAssetsScanFail().RemoveAll(this);
// 	}
//
// 	if (TreeViewPtr)
// 	{
// 		TreeViewPtr->OnTreeViewSelectionChanged().Unbind();
// 	}
// }
//
// void SPjcTabAssetsUnused::OnProjectScanSuccess()
// {
// 	StatItemsUpdate();
// }
//
// void SPjcTabAssetsUnused::OnProjectScanFail(const FString& InScanErrMsg)
// {
// 	UPjcHelperSubsystem::ShowNotificationWithOutputLog(InScanErrMsg, SNotificationItem::CS_Fail, 5.0f);
// }
//
// void SPjcTabAssetsUnused::StatItemsUpdate()
// {
// 	if (!ScannerSubsystemPtr) return;
//
// 	const TSet<FAssetData>& AssetsUnused = ScannerSubsystemPtr->GetAssetsByCategory(EPjcAssetCategory::Unused);
// 	const TSet<FAssetData>& AssetsUsed = ScannerSubsystemPtr->GetAssetsByCategory(EPjcAssetCategory::Used);
// 	const TSet<FAssetData>& AssetsPrimary = ScannerSubsystemPtr->GetAssetsByCategory(EPjcAssetCategory::Primary);
// 	const TSet<FAssetData>& AssetsEditor = ScannerSubsystemPtr->GetAssetsByCategory(EPjcAssetCategory::Editor);
// 	const TSet<FAssetData>& AssetsIndirect = ScannerSubsystemPtr->GetAssetsByCategory(EPjcAssetCategory::Indirect);
// 	const TSet<FAssetData>& AssetsExtReferenced = ScannerSubsystemPtr->GetAssetsByCategory(EPjcAssetCategory::ExtReferenced);
// 	const TSet<FAssetData>& AssetsExcluded = ScannerSubsystemPtr->GetAssetsByCategory(EPjcAssetCategory::Excluded);
// 	const TSet<FAssetData>& AssetsAny = ScannerSubsystemPtr->GetAssetsByCategory(EPjcAssetCategory::Any);
//
//
// 	const int64 SizeAssetsUnused = UPjcHelperSubsystem::GetAssetsTotalSize(AssetsUnused);
// 	const int64 SizeAssetsUsed = UPjcHelperSubsystem::GetAssetsTotalSize(AssetsUsed);
// 	const int64 SizeAssetsPrimary = UPjcHelperSubsystem::GetAssetsTotalSize(AssetsPrimary);
// 	const int64 SizeAssetsEditor = UPjcHelperSubsystem::GetAssetsTotalSize(AssetsEditor);
// 	const int64 SizeAssetsIndirect = UPjcHelperSubsystem::GetAssetsTotalSize(AssetsIndirect);
// 	const int64 SizeAssetsExtReferenced = UPjcHelperSubsystem::GetAssetsTotalSize(AssetsExtReferenced);
// 	const int64 SizeAssetsExcluded = UPjcHelperSubsystem::GetAssetsTotalSize(AssetsExcluded);
// 	const int64 SizeAssetsAny = UPjcHelperSubsystem::GetAssetsTotalSize(AssetsAny);
//
// 	StatItems.Reset();
//
// 	const FMargin FirstLvl{5.0f, 0.0f, 0.0f, 0.0f};
// 	const FMargin SecondLvl{20.0f, 0.0f, 0.0f, 0.0f};
// 	const auto ColorRed = FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Red").GetSpecifiedColor();
// 	const auto ColorYellow = FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Yellow").GetSpecifiedColor();
//
// 	StatItems.Emplace(
// 		MakeShareable(
// 			new FPjcStatItem{
// 				FText::FromString(TEXT("Unused")),
// 				FText::AsNumber(AssetsUnused.Num()),
// 				FText::AsMemory(SizeAssetsUnused, IEC),
// 				FText::FromString(TEXT("Unused Assets")),
// 				FText::FromString(TEXT("Total number of unused assets")),
// 				FText::FromString(TEXT("Total size of unused assets")),
// 				AssetsUnused.Num() > 0 ? ColorRed : FLinearColor::White,
// 				FirstLvl
// 			}
// 		)
// 	);
//
// 	StatItems.Emplace(
// 		MakeShareable(
// 			new FPjcStatItem{
// 				FText::FromString(TEXT("Used")),
// 				FText::AsNumber(AssetsUsed.Num()),
// 				FText::AsMemory(SizeAssetsUsed, IEC),
// 				FText::FromString(TEXT("Used Assets")),
// 				FText::FromString(TEXT("Total number of used assets")),
// 				FText::FromString(TEXT("Total size of used assets")),
// 				FLinearColor::White,
// 				FirstLvl
// 			}
// 		)
// 	);
//
// 	StatItems.Emplace(
// 		MakeShareable(
// 			new FPjcStatItem{
// 				FText::FromString(TEXT("Primary")),
// 				FText::AsNumber(AssetsPrimary.Num()),
// 				FText::AsMemory(SizeAssetsPrimary, IEC),
// 				FText::FromString(TEXT("Primary Assets")),
// 				FText::FromString(TEXT("Total number of primary assets")),
// 				FText::FromString(TEXT("Total size of primary assets")),
// 				FLinearColor::White,
// 				SecondLvl
// 			}
// 		)
// 	);
//
// 	StatItems.Emplace(
// 		MakeShareable(
// 			new FPjcStatItem{
// 				FText::FromString(TEXT("Editor")),
// 				FText::AsNumber(AssetsEditor.Num()),
// 				FText::AsMemory(SizeAssetsEditor, IEC),
// 				FText::FromString(TEXT("Editor Assets")),
// 				FText::FromString(TEXT("Total number of Editor assets")),
// 				FText::FromString(TEXT("Total size of Editor assets")),
// 				FLinearColor::White,
// 				SecondLvl
// 			}
// 		)
// 	);
//
// 	StatItems.Emplace(
// 		MakeShareable(
// 			new FPjcStatItem{
// 				FText::FromString(TEXT("Indirect")),
// 				FText::AsNumber(AssetsIndirect.Num()),
// 				FText::AsMemory(SizeAssetsIndirect, IEC),
// 				FText::FromString(TEXT("Indirect Assets")),
// 				FText::FromString(TEXT("Total number of Indirect assets")),
// 				FText::FromString(TEXT("Total size of Indirect assets")),
// 				FLinearColor::White,
// 				SecondLvl
// 			}
// 		)
// 	);
//
// 	StatItems.Emplace(
// 		MakeShareable(
// 			new FPjcStatItem{
// 				FText::FromString(TEXT("ExtReferenced")),
// 				FText::AsNumber(AssetsExtReferenced.Num()),
// 				FText::AsMemory(SizeAssetsExtReferenced, IEC),
// 				FText::FromString(TEXT("ExtReferenced Assets")),
// 				FText::FromString(TEXT("Total number of ExtReferenced assets")),
// 				FText::FromString(TEXT("Total size of ExtReferenced assets")),
// 				FLinearColor::White,
// 				SecondLvl
// 			}
// 		)
// 	);
//
// 	StatItems.Emplace(
// 		MakeShareable(
// 			new FPjcStatItem{
// 				FText::FromString(TEXT("Excluded")),
// 				FText::AsNumber(AssetsExcluded.Num()),
// 				FText::AsMemory(SizeAssetsExcluded, IEC),
// 				FText::FromString(TEXT("Excluded Assets")),
// 				FText::FromString(TEXT("Total number of Excluded assets")),
// 				FText::FromString(TEXT("Total size of Excluded assets")),
// 				AssetsExcluded.Num() > 0 ? ColorYellow : FLinearColor::White,
// 				SecondLvl
// 			}
// 		)
// 	);
//
// 	StatItems.Emplace(
// 		MakeShareable(
// 			new FPjcStatItem{
// 				FText::FromString(TEXT("Total")),
// 				FText::AsNumber(AssetsAny.Num()),
// 				FText::AsMemory(SizeAssetsAny, IEC),
// 				FText::FromString(TEXT("All Assets")),
// 				FText::FromString(TEXT("Total number of assets")),
// 				FText::FromString(TEXT("Total size of assets")),
// 				FLinearColor::White,
// 				FirstLvl
// 			}
// 		)
// 	);
//
// 	if (StatsViewPtr.IsValid())
// 	{
// 		StatsViewPtr->StatItemsUpdate(StatItems);
// 	}
// }
//
// TSharedRef<SWidget> SPjcTabAssetsUnused::CreateToolbar() const
// {
// 	FToolBarBuilder ToolBarBuilder{Cmds, FMultiBoxCustomization::None};
// 	ToolBarBuilder.BeginSection("PjcTabAssetUnusedScanActions");
// 	{
// 		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().TabAssetsUnusedBtnScan);
// 		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().TabAssetsUnusedBtnClean);
// 	}
// 	ToolBarBuilder.EndSection();
//
// 	return ToolBarBuilder.MakeWidget();
// }
//
// TSharedPtr<SWidget> SPjcTabAssetsUnused::GetContentBrowserContextMenu(const TArray<FAssetData>& Assets) const
// {
// 	FMenuBuilder MenuBuilder{true, Cmds};
//
// 	MenuBuilder.BeginSection(TEXT("PjcAssetsInfo"), FText::FromString(TEXT("Info")));
// 	{
// 		MenuBuilder.AddMenuEntry(FPjcCmds::Get().OpenViewerSizeMap);
// 		MenuBuilder.AddMenuEntry(FPjcCmds::Get().OpenViewerReference);
// 		MenuBuilder.AddMenuEntry(FPjcCmds::Get().OpenViewerAudit);
// 	}
// 	MenuBuilder.EndSection();
//
// 	MenuBuilder.BeginSection(TEXT("PjcAssetActions"), FText::FromString(TEXT("Actions")));
// 	{
// 		MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsExclude);
// 		MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsExcludeByClass);
// 		MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsInclude);
// 		MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsIncludeByClass);
// 		MenuBuilder.AddMenuEntry(FPjcCmds::Get().AssetsDelete);
// 	}
// 	MenuBuilder.EndSection();
//
// 	return MenuBuilder.MakeWidget();
// }
