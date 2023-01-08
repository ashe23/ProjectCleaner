// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/Tabs/SProjectCleanerTabScanSettings.h"
#include "ProjectCleanerStyles.h"
#include "ProjectCleanerSubsystem.h"
#include "Libs/ProjectCleanerLibAsset.h"
#include "Settings/ProjectCleanerSettings.h"
// Engine Headers
#include "Libs/ProjectCleanerLibFile.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"

void SProjectCleanerTabScanSettings::Construct(const FArguments& InArgs)
{
	// if (!GEditor) return;
	// SubsystemPtr = GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>();
	// if (!SubsystemPtr) return;

	Settings = GetMutableDefault<UProjectCleanerSettings>();
	if (!Settings) return;

	// SubsystemPtr->OnProjectScanned().AddRaw(this, &SProjectCleanerTabScanSettings::OnProjectScanned);

	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bShowOptions = true;
	DetailsViewArgs.bAllowFavoriteSystem = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.ViewIdentifier = "ProjectCleanerSettings";

	const auto SettingsProperty = PropertyEditor.CreateDetailView(DetailsViewArgs);
	SettingsProperty->SetObject(Settings);

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
				.IsEnabled_Raw(this, &SProjectCleanerTabScanSettings::BtnScanProjectEnabled)
				.ButtonColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.Blue"))
				[
					SNew(STextBlock)
					.Justification(ETextJustify::Center)
					.ToolTipText(FText::FromString(TEXT("Scan the project based on specified settings.")))
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
				.OnClicked_Raw(this, &SProjectCleanerTabScanSettings::OnBtnCleanProjectClick)
				.IsEnabled_Raw(this, &SProjectCleanerTabScanSettings::BtnCleanProjectEnabled)
				.ButtonColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.Red"))
				[
					SNew(STextBlock)
					.Justification(ETextJustify::Center)
					.ToolTipText(FText::FromString(TEXT("Clean project based on specified CleanupMethod.")))
					.ColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.White"))
					.ShadowOffset(FVector2D{1.5f, 1.5f})
					.ShadowColorAndOpacity(FLinearColor::Black)
					.Font(FProjectCleanerStyles::GetFont("Bold", 10))
					.Text(FText::FromString(TEXT("Clean Project")))
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
				.Text_Raw(this, &SProjectCleanerTabScanSettings::GetTextAssetsUsed)
			]
			+ SVerticalBox::Slot()
			  .AutoHeight()
			  .Padding(FMargin{20.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock)
				.Font(FProjectCleanerStyles::GetFont("Bold", 11))
				.Text_Raw(this, &SProjectCleanerTabScanSettings::GetTextAssetsPrimary)
			]
			+ SVerticalBox::Slot()
			  .AutoHeight()
			  .Padding(FMargin{20.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock)
				.Font(FProjectCleanerStyles::GetFont("Bold", 11))
				.Text_Raw(this, &SProjectCleanerTabScanSettings::GetTextAssetsIndirect)
			]
			+ SVerticalBox::Slot()
			  .AutoHeight()
			  .Padding(FMargin{20.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock)
				.Font(FProjectCleanerStyles::GetFont("Bold", 11))
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
				.ColorAndOpacity_Raw(this, &SProjectCleanerTabScanSettings::GetTextColorFilesCorrupted)
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
				.OnClicked_Raw(this, &SProjectCleanerTabScanSettings::OnBtnResetSettingsClick)
				.ButtonColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.Blue"))
				[
					SNew(STextBlock)
					.Justification(ETextJustify::Center)
					.ToolTipText(FText::FromString(TEXT("Reset Settings to their default state")))
					.ColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.White"))
					.ShadowOffset(FVector2D{1.5f, 1.5f})
					.ShadowColorAndOpacity(FLinearColor::Black)
					.Font(FProjectCleanerStyles::GetFont("Bold", 10))
					.Text(FText::FromString(TEXT("Reset Settings")))
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
					SettingsProperty
				]
			]
		]
	];
}

SProjectCleanerTabScanSettings::~SProjectCleanerTabScanSettings()
{
	// SubsystemPtr->OnProjectScanned().RemoveAll(this);
	// SubsystemPtr = nullptr;
}

void SProjectCleanerTabScanSettings::UpdateData()
{
	// if (!SubsystemPtr) return;

	// const FProjectCleanerScanData& ScanData = SubsystemPtr->GetScanData();

	// AssetsTotalNum = ScanData.AssetsAll.Num();
	// AssetsTotalSize = UProjectCleanerLibAsset::GetAssetsTotalSize(ScanData.AssetsAll);
	// AssetsPrimaryNum = ScanData.AssetsPrimary.Num();
	// AssetsPrimarySize = UProjectCleanerLibAsset::GetAssetsTotalSize(ScanData.AssetsPrimary);
	// AssetsIndirectNum = ScanData.AssetsIndirect.Num();
	// AssetsIndirectSize = UProjectCleanerLibAsset::GetAssetsTotalSize(ScanData.AssetsIndirect);
	// AssetsExcludedNum = ScanData.AssetsExcluded.Num();
	// AssetsExcludedSize = UProjectCleanerLibAsset::GetAssetsTotalSize(ScanData.AssetsExcluded);
	// AssetsUnusedNum = ScanData.AssetsUnused.Num();
	// AssetsUnusedSize = UProjectCleanerLibAsset::GetAssetsTotalSize(ScanData.AssetsUnused);
	// AssetsUsedNum = ScanData.AssetsUsed.Num();
	// AssetsUsedSize = UProjectCleanerLibAsset::GetAssetsTotalSize(ScanData.AssetsUsed);
	// FoldersTotalNum = ScanData.FoldersAll.Num();
	// FoldersEmptyNum = ScanData.FoldersEmpty.Num();
	// FilesCorruptedNum = ScanData.FilesCorrupted.Num();
	// FilesCorruptedSize = UProjectCleanerLibFile::GetFilesTotalSize(ScanData.FilesCorrupted);
	// FilesNonEngineNum = ScanData.FilesNonEngine.Num();
	// FilesNonEngineSize = UProjectCleanerLibFile::GetFilesTotalSize(ScanData.FilesNonEngine);
}

void SProjectCleanerTabScanSettings::OnProjectScanned()
{
	UpdateData();
}

FReply SProjectCleanerTabScanSettings::OnBtnScanProjectClick() const
{
	// if (!SubsystemPtr) return FReply::Handled();

	// SubsystemPtr->ProjectScan();

	return FReply::Handled();
}

FReply SProjectCleanerTabScanSettings::OnBtnCleanProjectClick() const
{
	if (!SubsystemPtr) return FReply::Handled();

	const FText Title = FText::FromString(TEXT("Confirm project cleaning"));
	FText Msg;
	if (Settings->CleanupMethod == EProjectCleanerCleanupMethod::Full)
	{
		Msg = FText::FromString(TEXT("Are you sure you want to delete all unused assets and empty folders in project?"));
	}

	if (Settings->CleanupMethod == EProjectCleanerCleanupMethod::UnusedAssetsOnly)
	{
		Msg = FText::FromString(TEXT("Are you sure you want to delete all unused assets in project?"));
	}

	if (Settings->CleanupMethod == EProjectCleanerCleanupMethod::EmptyFoldersOnly)
	{
		Msg = FText::FromString(TEXT("Are you sure you want to delete all empty folders in project?"));
	}

	const auto Result = FMessageDialog::Open(EAppMsgType::YesNo, Msg, &Title);
	if (Result == EAppReturnType::No || Result == EAppReturnType::Cancel)
	{
		return FReply::Handled();
	}

	// SubsystemPtr->ProjectClean(GetDefault<UProjectCleanerSettings>()->bAutoCleanEmptyFolders);

	return FReply::Handled();
}

bool SProjectCleanerTabScanSettings::BtnScanProjectEnabled() const
{
	return false;
	// return SubsystemPtr && SubsystemPtr->CanScanProject();
}

bool SProjectCleanerTabScanSettings::BtnCleanProjectEnabled() const
{
	return false;
	// return SubsystemPtr && SubsystemPtr->CanScanProject() && AssetsUnusedNum > 0 || FoldersEmptyNum > 0;
}

FReply SProjectCleanerTabScanSettings::OnBtnResetSettingsClick() const
{
	if (!SubsystemPtr) return FReply::Handled();
	if (!Settings) return FReply::Handled();

	Settings->CleanupMethod = EProjectCleanerCleanupMethod::Full;
	Settings->ScanPaths.Empty();
	Settings->ScanClasses.Empty();
	Settings->ExcludePaths.Empty();
	Settings->ExcludeAssets.Empty();
	Settings->ExcludeClasses.Empty();
	Settings->PostEditChange();

	// SubsystemPtr->ProjectScan();

	return FReply::Handled();
}

FText SProjectCleanerTabScanSettings::GetTextAssetsTotal() const
{
	return FText::FromString(FString::Printf(TEXT("Assets Total - %d (%s)"), AssetsTotalNum, *FText::AsMemory(AssetsTotalSize).ToString()));
}

FText SProjectCleanerTabScanSettings::GetTextAssetsUsed() const
{
	return FText::FromString(FString::Printf(TEXT("Assets Used - %d (%s)"), AssetsUsedNum, *FText::AsMemory(AssetsUsedSize).ToString()));
}

FText SProjectCleanerTabScanSettings::GetTextAssetsIndirect() const
{
	return FText::FromString(FString::Printf(TEXT("Indirect - %d (%s)"), AssetsIndirectNum, *FText::AsMemory(AssetsIndirectSize).ToString()));
}

FText SProjectCleanerTabScanSettings::GetTextAssetsExcluded() const
{
	return FText::FromString(FString::Printf(TEXT("Excluded - %d (%s)"), AssetsExcludedNum, *FText::AsMemory(AssetsExcludedSize).ToString()));
}

FText SProjectCleanerTabScanSettings::GetTextAssetsPrimary() const
{
	return FText::FromString(FString::Printf(TEXT("Primary - %d (%s)"), AssetsPrimaryNum, *FText::AsMemory(AssetsPrimarySize).ToString()));
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
	return AssetsExcludedNum > 0 ? FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.Yellow") : FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.White");
}

FSlateColor SProjectCleanerTabScanSettings::GetTextColorFoldersEmpty() const
{
	return FoldersEmptyNum > 0 ? FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.Red") : FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.White");
}

FSlateColor SProjectCleanerTabScanSettings::GetTextColorFilesCorrupted() const
{
	return FilesCorruptedNum > 0 ? FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.Red") : FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.White");
}
