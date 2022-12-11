// // Copyright Ashot Barkhudaryan. All Rights Reserved.
//
// #include "Slate/Tabs/SProjectCleanerTabUnused.h"
// #include "Slate/TreeView/SProjectCleanerTreeView.h"
// #include "ProjectCleaner.h"
// #include "ProjectCleanerScanner.h"
// #include "ProjectCleanerConstants.h"
// #include "Settings/ProjectCleanerScanSettings.h"
// #include "Settings/ProjectCleanerExcludeSettings.h"
// // Engine Headers
// // #include "ContentBrowserModule.h"
// // #include "ContentBrowserItem.h"
// // #include "FrontendFilterBase.h"
// // #include "IContentBrowserSingleton.h"
// #include "ProjectCleanerStyles.h"
// #include "Slate/ProjectCleanerAssetBrowser.h"
// #include "Widgets/Layout/SSeparator.h"
//
// void SProjectCleanerTabUnused::Construct(const FArguments& InArgs)
// {
// 	Scanner = InArgs._Scanner;
// 	if (!Scanner.IsValid()) return;
//
// 	ScanSettings = GetMutableDefault<UProjectCleanerScanSettings>();
// 	ExcludeSettings = GetMutableDefault<UProjectCleanerExcludeSettings>();
//
// 	check(ScanSettings);
// 	check(ExcludeSettings);
//
// 	Scanner->OnScanFinished().AddLambda([&]()
// 	{
// 		UpdateView();
// 	});
//
// 	SelectedPaths.Add(ProjectCleanerConstants::PathRelRoot.ToString());
//
// 	UpdateView();
// }
//
// void SProjectCleanerTabUnused::UpdateView()
// {
// 	if (!Scanner.IsValid()) return;
// 	if (!ScanSettings) return;
//
// 	// making sure tree view is valid
//
// 	if (!ProjectCleanerTreeView.IsValid())
// 	{
// 		SAssignNew(ProjectCleanerTreeView, SProjectCleanerTreeView).Scanner(Scanner);
// 	}
//
// 	ChildSlot
// 	[
// 		SNew(SVerticalBox)
// 		+ SVerticalBox::Slot()
// 		  .Padding(FMargin{5.0f})
// 		  .FillHeight(1.0f)
// 		[
// 			SNew(SSplitter)
// 			.Style(FEditorStyle::Get(), "ContentBrowser.Splitter")
// 			.PhysicalSplitterHandleSize(5.0f)
// 			.Orientation(Orient_Vertical)
// 			+ SSplitter::Slot()
// 			[
// 				ProjectCleanerTreeView.ToSharedRef()
// 			]
// 			+ SSplitter::Slot()
// 			[
// 				SAssignNew(ProjectCleanerAssetBrowser, SProjectCleanerAssetBrowser).Scanner(Scanner)
// 			]
// 		]
// 	];
// }
//
// void SProjectCleanerTabUnused::UpdateFilter()
// {
// 	// Filter.Clear();
// 	//
// 	// if (Scanner->GetAssetsUnused().Num() == 0)
// 	// {
// 	// 	// this is needed for disabling showing primary assets in browser, when there is no unused assets
// 	// 	Filter.TagsAndValues.Add(FName{"ProjectCleanerEmptyTag"}, FString{"ProjectCleanerEmptyTag"});
// 	//
// 	// 	return;
// 	// }
// 	//
// 	// for (const auto& UnusedAsset : Scanner->GetAssetsUnused())
// 	// {
// 	// 	Filter.ObjectPaths.Add(UnusedAsset.ObjectPath);
// 	// }
// 	//
// 	// if (SelectedPaths.Num() == 0)
// 	// {
// 	// 	Filter.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
// 	// }
// 	// else
// 	// {
// 	// 	for (const auto& SelectedPath : SelectedPaths)
// 	// 	{
// 	// 		Filter.PackagePaths.Add(FName{*SelectedPath});
// 	// 	}
// 	// }
// }
