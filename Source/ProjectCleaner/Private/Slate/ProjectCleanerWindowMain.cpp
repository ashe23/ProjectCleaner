// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/ProjectCleanerWindowMain.h"
#include "Slate/ProjectCleanerTreeView.h"
#include "Slate/ProjectCleanerAssetBrowser.h"
#include "ProjectCleanerStyles.h"
#include "ProjectCleanerLibrary.h"
#include "ProjectCleanerConstants.h"
#include "ProjectCleanerScanSettings.h"
// Engine Headers
#include "ProjectCleaner.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

static constexpr int32 WidgetIndexNone = 0;
static constexpr int32 WidgetIndexLoading = 1;

void SProjectCleanerWindowMain::Construct(const FArguments& InArgs)
{
	ScanSettings = GetMutableDefault<UProjectCleanerScanSettings>();
	check(ScanSettings.IsValid());
	
	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bShowOptions = false;
	DetailsViewArgs.bAllowFavoriteSystem = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.ViewIdentifier = "ProjectCleanerScanSettings";
	
	const TSharedRef<IDetailsView> ScanSettingsProperty = PropertyEditor.CreateDetailView(DetailsViewArgs);
	ScanSettingsProperty->SetObject(ScanSettings.Get());
	
	ChildSlot
	[
		SNew(SWidgetSwitcher)
		.IsEnabled_Static(IsWidgetEnabled)
		.WidgetIndex_Static(GetWidgetIndex)
		+ SWidgetSwitcher::Slot()
		  .HAlign(HAlign_Fill)
		  .VAlign(VAlign_Fill)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin{10.0f})
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin{5.0f})
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(FMargin{0.0f, 0.0f, 5.0f, 0.0f})
					[
						SNew(SButton)
						.ContentPadding(FMargin{5.0f})
						.ButtonColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.Blue"))
						.OnClicked_Raw(this, &SProjectCleanerWindowMain::OnBtnScanProjectClicked)
						[
							SNew(STextBlock)
							.Justification(ETextJustify::Center)
							.ToolTipText(FText::FromString(TEXT("Scan project for unused assets, empty folders, non engine files or corrupted assets")))
							.ColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.White"))
							.ShadowOffset(FVector2D{1.5f, 1.5f})
							.ShadowColorAndOpacity(FLinearColor::Black)
							.Font(FProjectCleanerStyles::GetFont("Bold", 10))
							.Text(FText::FromString(TEXT("Scan Project")))
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(FMargin{0.0f, 0.0f, 5.0f, 0.0f})
					[
						SNew(SButton)
						.ContentPadding(FMargin{5.0f})
						.ButtonColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.Red"))
						[
							SNew(STextBlock)
							.Justification(ETextJustify::Center)
							.ToolTipText(FText::FromString(TEXT("Remove all unused assets and empty folders in project. This wont delete any excluded asset")))
							.ColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.White"))
							.ShadowOffset(FVector2D{1.5f, 1.5f})
							.ShadowColorAndOpacity(FLinearColor::Black)
							.Font(FProjectCleanerStyles::GetFont("Bold", 10))
							.Text(FText::FromString(TEXT("Clean Project")))
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(FMargin{0.0f, 0.0f, 5.0f, 0.0f})
					[
						SNew(SButton)
						.ContentPadding(FMargin{5.0f})
						.ButtonColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.Red"))
						[
							SNew(STextBlock)
							.Justification(ETextJustify::Center)
							.ToolTipText(FText::FromString(TEXT("Remove all empty folders in project")))
							.ColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.White"))
							.ShadowOffset(FVector2D{1.5f, 1.5f})
							.ShadowColorAndOpacity(FLinearColor::Black)
							.Font(FProjectCleanerStyles::GetFont("Bold", 10))
							.Text(FText::FromString(TEXT("Delete Empty Folders")))
						]
					]
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(FMargin{10.0f})
			[
				SNew(SSplitter)
				.Style(FEditorStyle::Get(), "ContentBrowser.Splitter")
				.PhysicalSplitterHandleSize(5.0f)
				+ SSplitter::Slot()
				.Value(0.3f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					  .FillHeight(1.0f)
					  .Padding(FMargin{5.0f})
					[
						SNew(SScrollBox)
						.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
						.AnimateWheelScrolling(true)
						.AllowOverscroll(EAllowOverscroll::No)
						+ SScrollBox::Slot()
						[
							ScanSettingsProperty
						]
					]
				]
				+ SSplitter::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					  .FillHeight(1.0f)
					  .Padding(FMargin{5.0f})
					[
						SNew(SScrollBox)
						.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
						.AnimateWheelScrolling(true)
						.AllowOverscroll(EAllowOverscroll::No)
						+ SScrollBox::Slot()
						[
							SNew(SProjectCleanerTreeView)
						]
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					.Padding(FMargin{5.0f})
					[
						SNew(SScrollBox)
						.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
						.AnimateWheelScrolling(true)
						.AllowOverscroll(EAllowOverscroll::No)
						+ SScrollBox::Slot()
						[
							SNew(SProjectCleanerAssetBrowser)
						]
					]
				]
			]
		]
		+ SWidgetSwitcher::Slot()
		  .HAlign(HAlign_Fill)
		  .VAlign(VAlign_Fill)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			  .FillWidth(1.0f)
			  .HAlign(HAlign_Center)
			  .VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Justification(ETextJustify::Center)
				.Font(FProjectCleanerStyles::GetFont("Light", 30))
				.Text(FText::FromString(ProjectCleanerConstants::MsgAssetRegistryStillWorking))
			]
		]
	];
}

SProjectCleanerWindowMain::~SProjectCleanerWindowMain()
{
}

bool SProjectCleanerWindowMain::IsWidgetEnabled()
{
	return !UProjectCleanerLibrary::IsAssetRegistryWorking();
}

int32 SProjectCleanerWindowMain::GetWidgetIndex()
{
	return UProjectCleanerLibrary::IsAssetRegistryWorking() ? WidgetIndexLoading : WidgetIndexNone;
}

FReply SProjectCleanerWindowMain::OnBtnScanProjectClicked() const
{
	UE_LOG(LogProjectCleaner, Warning, TEXT("Scan btn clicked!"));
	return FReply::Handled();
}
