// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/Tabs/SProjectCleanerTabScanSettings.h"
#include "ProjectCleanerStyles.h"
#include "ProjectCleanerSubsystem.h"
#include "Settings/ProjectCleanerExcludeSettings.h"
// Engine Headers
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"

void SProjectCleanerTabScanSettings::Construct(const FArguments& InArgs)
{
	if (!GEditor) return;
	SubsystemPtr = GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>();
	if (!SubsystemPtr) return;

	SubsystemPtr->OnProjectScanned().AddLambda([&]()
	{
		UpdateData();
	});

	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bShowOptions = false;
	DetailsViewArgs.bAllowFavoriteSystem = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.ViewIdentifier = "ProjectCleanerExcludeSettings";

	const auto ExcludeSettingsProperty = PropertyEditor.CreateDetailView(DetailsViewArgs);
	ExcludeSettingsProperty->SetObject(GetMutableDefault<UProjectCleanerExcludeSettings>());

	UpdateData();

	ChildSlot
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
				.OnClicked_Raw(this, &SProjectCleanerTabScanSettings::OnBtnScanProjectClick)
				.ButtonColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.Blue"))
				[
					SNew(STextBlock)
					.Justification(ETextJustify::Center)
					.ToolTipText(FText::FromString(TEXT("Scan the project with the specified scan settings.")))
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
				// .OnClicked_Raw(this, &SProjectCleanerTabScanSettings::OnBtnCleanProjectClick)
				// .IsEnabled_Raw(this, &SProjectCleanerTabScanSettings::BtnCleanProjectEnabled)
				.ButtonColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.Red"))
				[
					SNew(STextBlock)
					.Justification(ETextJustify::Center)
					.ToolTipText(FText::FromString(TEXT("Remove all unused assets from the project. This won't delete any excluded assets.")))
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
				// .OnClicked_Raw(this, &SProjectCleanerTabScanSettings::OnBtnDeleteEmptyFoldersClick)
				// .IsEnabled_Raw(this, &SProjectCleanerTabScanSettings::BtnCleanEmptyFoldersEnabled)
				.ButtonColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.Red"))
				[
					SNew(STextBlock)
					.Justification(ETextJustify::Center)
					.ToolTipText(FText::FromString(TEXT("Remove all empty folders in the project.")))
					.ColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.White"))
					.ShadowOffset(FVector2D{1.5f, 1.5f})
					.ShadowColorAndOpacity(FLinearColor::Black)
					.Font(FProjectCleanerStyles::GetFont("Bold", 10))
					.Text(FText::FromString(TEXT("Clean Empty Folders")))
				]
			]
		]
		+ SVerticalBox::Slot()
		  .Padding(FMargin{5.0f})
		  .AutoHeight()
		[
			SNew(SSeparator)
			.Thickness(5.0f)
		]
		+ SVerticalBox::Slot()
		  .Padding(FMargin{5.0f})
		  .AutoHeight()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Font(FProjectCleanerStyles::GetFont("Bold", 13))
				.Text_Raw(this, &SProjectCleanerTabScanSettings::GetTextAssetsTotal)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Font(FProjectCleanerStyles::GetFont("Bold", 13))
				.Text_Raw(this, &SProjectCleanerTabScanSettings::GetTextAssetsIndirect)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Font(FProjectCleanerStyles::GetFont("Bold", 13))
				.ColorAndOpacity_Raw(this, &SProjectCleanerTabScanSettings::GetTextColorAssetsExcluded)
				.Text_Raw(this, &SProjectCleanerTabScanSettings::GetTextAssetsExcluded)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Font(FProjectCleanerStyles::GetFont("Bold", 13))
				.ColorAndOpacity_Raw(this, &SProjectCleanerTabScanSettings::GetTextColorAssetsUnused)
				.Text_Raw(this, &SProjectCleanerTabScanSettings::GetTextAssetsUnused)
			]
			+ SVerticalBox::Slot()
			  .AutoHeight()
			  .Padding(FMargin{0.0f, 10.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock)
				.Font(FProjectCleanerStyles::GetFont("Bold", 13))
				.Text_Raw(this, &SProjectCleanerTabScanSettings::GetTextFoldersTotal)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Font(FProjectCleanerStyles::GetFont("Bold", 13))
				.ColorAndOpacity_Raw(this, &SProjectCleanerTabScanSettings::GetTextColorFoldersEmpty)
				.Text_Raw(this, &SProjectCleanerTabScanSettings::GetTextFoldersEmpty)
			]
			+ SVerticalBox::Slot()
			  .AutoHeight()
			  .Padding(FMargin{0.0f, 10.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock)
				.Font(FProjectCleanerStyles::GetFont("Bold", 13))
				.Text_Raw(this, &SProjectCleanerTabScanSettings::GetTextFilesCorrupted)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Font(FProjectCleanerStyles::GetFont("Bold", 13))
				.Text_Raw(this, &SProjectCleanerTabScanSettings::GetTextFilesNonEngine)
			]
		]
		+ SVerticalBox::Slot()
		  .Padding(FMargin{5.0f})
		  .AutoHeight()
		[
			SNew(SSeparator)
			.Thickness(5.0f)
		]
		+ SVerticalBox::Slot()
		  .Padding(FMargin{5.0f})
		  .AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.ContentPadding(FMargin{5.0f})
				// .OnClicked_Raw(this, &SProjectCleanerTabScanSettings::OnBtnResetExcludeSettingsClick)
				.ButtonColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.Blue"))
				[
					SNew(STextBlock)
					.Justification(ETextJustify::Center)
					.ToolTipText(FText::FromString(TEXT("Clears all fields in exclude settings")))
					.ColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.White"))
					.ShadowOffset(FVector2D{1.5f, 1.5f})
					.ShadowColorAndOpacity(FLinearColor::Black)
					.Font(FProjectCleanerStyles::GetFont("Bold", 10))
					.Text(FText::FromString(TEXT("Reset Exclude Settings")))
				]
			]
		]
		+ SVerticalBox::Slot()
		  .Padding(FMargin{5.0f})
		  .FillHeight(1.0f)
		[
			SNew(SBox)
			[
				SNew(SScrollBox)
				.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
				.AnimateWheelScrolling(true)
				.AllowOverscroll(EAllowOverscroll::No)
				+ SScrollBox::Slot()
				[
					ExcludeSettingsProperty
				]
			]
		]
	];
}

void SProjectCleanerTabScanSettings::UpdateData()
{
	if (!SubsystemPtr) return;

	AssetsTotalNum = SubsystemPtr->GetAssetsAll().Num();
	AssetsTotalSize = SubsystemPtr->GetAssetsTotalSize(SubsystemPtr->GetAssetsAll());
	AssetsIndirectNum = SubsystemPtr->GetAssetsIndirect().Num();
	AssetsIndirectSize = SubsystemPtr->GetAssetsTotalSize(SubsystemPtr->GetAssetsIndirect());
	AssetsExcludedNum = SubsystemPtr->GetAssetsExcluded().Num();
	AssetsExcludedSize = SubsystemPtr->GetAssetsTotalSize(SubsystemPtr->GetAssetsExcluded());
	AssetsUnusedNum = SubsystemPtr->GetAssetsUnused().Num();
	AssetsUnusedSize = SubsystemPtr->GetAssetsTotalSize(SubsystemPtr->GetAssetsUnused());
	FoldersTotalNum = SubsystemPtr->GetFoldersTotal().Num();
	FoldersEmptyNum = SubsystemPtr->GetFoldersEmpty().Num();
	FilesCorruptedNum = SubsystemPtr->GetFilesCorrupted().Num();
	FilesCorruptedSize = SubsystemPtr->GetFilesTotalSize(SubsystemPtr->GetFilesCorrupted());
	FilesNonEngineNum = SubsystemPtr->GetFilesNonEngine().Num();
	FilesNonEngineSize = SubsystemPtr->GetFilesTotalSize(SubsystemPtr->GetFilesNonEngine());
}

FReply SProjectCleanerTabScanSettings::OnBtnScanProjectClick() const
{
	if (!SubsystemPtr) return FReply::Handled();

	SubsystemPtr->ProjectScan();

	return FReply::Handled();
}

FText SProjectCleanerTabScanSettings::GetTextAssetsTotal() const
{
	return FText::FromString(FString::Printf(TEXT("Assets Total - %d (%s)"), AssetsTotalNum, *FText::AsMemory(AssetsTotalSize).ToString()));
}

FText SProjectCleanerTabScanSettings::GetTextAssetsIndirect() const
{
	return FText::FromString(FString::Printf(TEXT("Assets Indirect - %d (%s)"), AssetsIndirectNum, *FText::AsMemory(AssetsIndirectSize).ToString()));
}

FText SProjectCleanerTabScanSettings::GetTextAssetsExcluded() const
{
	return FText::FromString(FString::Printf(TEXT("Assets Excluded - %d (%s)"), AssetsExcludedNum, *FText::AsMemory(AssetsExcludedSize).ToString()));
}

FText SProjectCleanerTabScanSettings::GetTextAssetsUnused() const
{
	return FText::FromString(FString::Printf(TEXT("Assets Unused - %d (%s)"), AssetsUnusedNum, *FText::AsMemory(AssetsUnusedSize).ToString()));
}

FText SProjectCleanerTabScanSettings::GetTextFoldersTotal() const
{
	return FText::FromString(FString::Printf(TEXT("Folders Total - %d"), FoldersTotalNum));
}

FText SProjectCleanerTabScanSettings::GetTextFoldersEmpty() const
{
	return FText::FromString(FString::Printf(TEXT("Folders Empty - %d"), FoldersEmptyNum));
}

FText SProjectCleanerTabScanSettings::GetTextFilesCorrupted() const
{
	return FText::FromString(FString::Printf(TEXT("Files Corrupted - %d (%s)"), FilesCorruptedNum, *FText::AsMemory(FilesCorruptedSize).ToString()));
}

FText SProjectCleanerTabScanSettings::GetTextFilesNonEngine() const
{
	return FText::FromString(FString::Printf(TEXT("Files NonEngine - %d (%s)"), FilesNonEngineNum, *FText::AsMemory(FilesNonEngineSize).ToString()));
}

FSlateColor SProjectCleanerTabScanSettings::GetTextColorAssetsUnused() const
{
	return AssetsUnusedNum > 0 ? FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.Red") : FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.White");
}

FSlateColor SProjectCleanerTabScanSettings::GetTextColorAssetsExcluded() const
{
	return AssetsExcludedNum > 0 ? FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.Violet") : FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.White");
}

FSlateColor SProjectCleanerTabScanSettings::GetTextColorFoldersEmpty() const
{
	return FoldersEmptyNum > 0 ? FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.Red") : FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.White");
}
