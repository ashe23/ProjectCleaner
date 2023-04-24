// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "SLate/AssetBrowser/SPjcAssetBrowser.h"
#include "Slate/AssetStats/SPjcAssetStats.h"
#include "EditorSettings/PjcEditorAssetExcludeSettings.h"
// #include "Slate/AssetBrowser/SPjcAssetBrowserStatItem.h"
// #include "Slate/AssetBrowser/SPjcContentBrowser.h"
// #include "Slate/AssetBrowser/SPjcTreeView.h"
// #include "PjcTypes.h"
#include "PjcStyles.h"
// #include "PjcConstants.h"
// #include "Libs/PjcLibAsset.h"
// Engine Headers
#include "EditorWidgetsModule.h"
#include "PjcSubsystem.h"
#include "Libs/PjcLibEditor.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
// #include "Widgets/Layout/SWidgetSwitcher.h"

void SPjcAssetBrowser::Construct(const FArguments& InArgs)
{
	if (!GEditor) return;

	SubsystemPtr = GEditor->GetEditorSubsystem<UPjcSubsystem>();
	if (!SubsystemPtr) return;

	SubsystemPtr->OnScanAssets().AddRaw(this, &SPjcAssetBrowser::OnScanAssets);

	FEditorWidgetsModule& EditorWidgetsModule = FModuleManager::LoadModuleChecked<FEditorWidgetsModule>("EditorWidgets");
	const TSharedRef<SWidget> AssetDiscoveryIndicator = EditorWidgetsModule.CreateAssetDiscoveryIndicator(EAssetDiscoveryIndicatorScaleMode::Scale_None, FMargin(16, 8), false);

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
				SNew(SPjcAssetStats)
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
		// + SSplitter::Slot()
		// .Value(0.7f)
		// [
		// 	SNew(SVerticalBox)
		// 	+ SVerticalBox::Slot().FillHeight(1.0f)
		// 	[
		// 		SNew(SOverlay)
		// 		+ SOverlay::Slot().Padding(5.0f)
		// 		[
		// 			SNew(SSplitter)
		// 			.Style(FEditorStyle::Get(), "DetailsView.Splitter")
		// 			.PhysicalSplitterHandleSize(3.0f)
		// 			+ SSplitter::Slot().Value(0.3f)
		// 			[
		// 				SAssignNew(TreeView, SPjcTreeView)
		// 			]
		// 			+ SSplitter::Slot().Value(0.7f)
		// 			[
		// 				SAssignNew(ContentBrowser, SPjcContentBrowser)
		// 			]
		// 		]
		// 		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(5.0f)
		// 		[
		// 			AssetDiscoveryIndicator
		// 		]
		// 	]
		// ]
	];
}

SPjcAssetBrowser::~SPjcAssetBrowser()
{
	if (!SubsystemPtr) return;

	SubsystemPtr->OnScanAssets().RemoveAll(this);
}

FReply SPjcAssetBrowser::OnBtnScanAssetsClick() const
{
	if (SubsystemPtr)
	{
		FPjcScanDataAssets Data;
		SubsystemPtr->ScanAssets(FPjcLibEditor::GetEditorAssetExcludeSettings(), Data);
	}

	return FReply::Handled();
}

void SPjcAssetBrowser::OnScanAssets(const FPjcScanDataAssets& InScaDataAssets) { }
