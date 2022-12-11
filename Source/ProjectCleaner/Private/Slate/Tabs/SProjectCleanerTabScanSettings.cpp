// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/Tabs/SProjectCleanerTabScanSettings.h"
#include "ProjectCleanerStyles.h"
#include "ProjectCleanerLibrary.h"
#include "Settings/ProjectCleanerExcludeSettings.h"
// Engine Headers
#include "ProjectCleanerConstants.h"
#include "ProjectCleanerSubsystem.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"

void SProjectCleanerTabScanSettings::Construct(const FArguments& InArgs)
{
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
				// .IsEnabled_Raw(this, &SProjectCleanerTabScanSettings::BtnScanProjectEnabled)
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
				.OnClicked_Raw(this, &SProjectCleanerTabScanSettings::OnBtnCleanProjectClick)
				.IsEnabled_Raw(this, &SProjectCleanerTabScanSettings::BtnCleanProjectEnabled)
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
				.OnClicked_Raw(this, &SProjectCleanerTabScanSettings::OnBtnDeleteEmptyFoldersClick)
				.IsEnabled_Raw(this, &SProjectCleanerTabScanSettings::BtnCleanEmptyFoldersEnabled)
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
			SNew(SHorizontalBox)
			.Visibility_Raw(this, &SProjectCleanerTabScanSettings::GetBtnScanProjectStatusVisibility)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBox)
				.WidthOverride(16.0f)
				.HeightOverride(16.0f)
				[
					SNew(SImage)
					.Image(FProjectCleanerStyles::Get().GetBrush(TEXT("ProjectCleaner.IconWarning16")))
				]
			]
			+ SHorizontalBox::Slot()
			  .AutoWidth()
			  .Padding(FMargin{5.0f, 2.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock)
				.Justification(ETextJustify::Left)
				.ColorAndOpacity(FProjectCleanerStyles::Get().GetColor("ProjectCleaner.Color.Yellow"))
				.Text_Raw(this, &SProjectCleanerTabScanSettings::GetBtnScanProjectToolTipText)
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
				.Text(this, &SProjectCleanerTabScanSettings::GetStatsTextAssetsTotal)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Font(FProjectCleanerStyles::GetFont("Bold", 13))
				.Text(this, &SProjectCleanerTabScanSettings::GetStatsTextAssetsIndirect)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Font(FProjectCleanerStyles::GetFont("Bold", 13))
				.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Yellow"))
				.Text(this, &SProjectCleanerTabScanSettings::GetStatsTextAssetsExcluded)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Red"))
				.Font(FProjectCleanerStyles::GetFont("Bold", 13))
				.Text(this, &SProjectCleanerTabScanSettings::GetStatsTextAssetsUnused)
			]
			+ SVerticalBox::Slot()
			  .AutoHeight()
			  .Padding(FMargin{0.0f, 10.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock)
				.Font(FProjectCleanerStyles::GetFont("Bold", 13))
				.ColorAndOpacity(FProjectCleanerStyles::Get().GetSlateColor("ProjectCleaner.Color.Red"))
				.Text(this, &SProjectCleanerTabScanSettings::GetStatsTextFoldersEmpty)
			]
			+ SVerticalBox::Slot()
			  .AutoHeight()
			  .Padding(FMargin{0.0f, 10.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock)
				.Font(FProjectCleanerStyles::GetFont("Bold", 13))
				.Text(this, &SProjectCleanerTabScanSettings::GetStatsTextFilesCorrupted)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Font(FProjectCleanerStyles::GetFont("Bold", 13))
				.Text(this, &SProjectCleanerTabScanSettings::GetStatsTextFilesNonEngine)
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
				.OnClicked_Raw(this, &SProjectCleanerTabScanSettings::OnBtnResetExcludeSettingsClick)
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

bool SProjectCleanerTabScanSettings::BtnCleanProjectEnabled() const
{
	if (!GEditor) return false;

	return GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>()->GetScanDataState() == EProjectCleanerScanDataState::Actual;
}

bool SProjectCleanerTabScanSettings::BtnCleanEmptyFoldersEnabled() const
{
	if (!GEditor) return false;

	return GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>()->GetScanDataState() == EProjectCleanerScanDataState::Actual;
}

FReply SProjectCleanerTabScanSettings::OnBtnScanProjectClick() const
{
	if (!GEditor) return FReply::Handled();

	GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>()->ProjectScan();

	return FReply::Handled();
}

FReply SProjectCleanerTabScanSettings::OnBtnCleanProjectClick() const
{
	// const auto ConfirmationStatus = UProjectCleanerLibrary::ConfirmationWindowShow(
	// 	FText::FromString(ProjectCleanerConstants::MsgConfirmCleanProjectTitle),
	// 	FText::FromString(ProjectCleanerConstants::MsgConfirmCleanProject)
	// );
	// if (UProjectCleanerLibrary::ConfirmationWindowCancelled(ConfirmationStatus))
	// {
	// 	return FReply::Handled();
	// }

	// Scanner->CleanProject();

	return FReply::Handled();
}

FReply SProjectCleanerTabScanSettings::OnBtnDeleteEmptyFoldersClick() const
{
	// const auto ConfirmationStatus = UProjectCleanerLibrary::ConfirmationWindowShow(
	// 	FText::FromString(ProjectCleanerConstants::MsgConfirmDeleteEmptyFoldersTitle),
	// 	FText::FromString(ProjectCleanerConstants::MsgConfirmDeleteEmptyFolders)
	// );
	// if (UProjectCleanerLibrary::ConfirmationWindowCancelled(ConfirmationStatus))
	// {
	// 	return FReply::Handled();
	// }

	// Scanner->DeleteEmptyFolders();

	return FReply::Handled();
}

FReply SProjectCleanerTabScanSettings::OnBtnResetExcludeSettingsClick() const
{
	const auto ExcludeSettings = GetMutableDefault<UProjectCleanerExcludeSettings>();
	if (!ExcludeSettings) return FReply::Handled();

	ExcludeSettings->ExcludedAssets.Empty();
	ExcludeSettings->ExcludedClasses.Empty();
	ExcludeSettings->ExcludedFolders.Empty();
	ExcludeSettings->PostEditChange();

	return FReply::Handled();
}

EVisibility SProjectCleanerTabScanSettings::GetBtnScanProjectStatusVisibility() const
{
	if (!GEditor) return EVisibility::Collapsed;

	return GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>()->GetScanDataState() == EProjectCleanerScanDataState::Actual ? EVisibility::Collapsed : EVisibility::Visible;
}

FText SProjectCleanerTabScanSettings::GetBtnScanProjectToolTipText() const
{
	if (!GEditor) return FText::FromString(TEXT(""));

	const EProjectCleanerScanDataState DataState = GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>()->GetScanDataState();

	if (DataState == EProjectCleanerScanDataState::None)
	{
		return FText::FromString(TEXT("Project Scan Required."));
	}

	if (DataState == EProjectCleanerScanDataState::ObsoleteByAssetRegistry)
	{
		return FText::FromString(TEXT("Project Scan Required. Asset Registry has been updated."));
	}

	if (DataState == EProjectCleanerScanDataState::ObsoleteBySettings)
	{
		return FText::FromString(TEXT("Project Scan Required. Exclude Settings has been updated."));
	}

	return FText::FromString(TEXT(""));
}

FText SProjectCleanerTabScanSettings::GetStatsTextAssetsTotal() const
{
	return FText::FromString(TEXT(""));
	// if (!Scanner.IsValid()) return FText::FromString(TEXT(""));
	//
	// const int32 AssetsTotalNum = Scanner->GetAssetTotalNum(UProjectCleanerLibrary::PathGetContentFolder(true));
	// const int64 AssetsTotalSize = Scanner->GetSizeTotal(UProjectCleanerLibrary::PathGetContentFolder(true));
	//
	// return FText::FromString(FString::Printf(TEXT("Assets Total - %d (%s)"), AssetsTotalNum, *FText::AsMemory(AssetsTotalSize).ToString()));
}

FText SProjectCleanerTabScanSettings::GetStatsTextAssetsIndirect() const
{
	return FText::FromString(TEXT(""));
	// if (!Scanner.IsValid()) return FText::FromString(TEXT(""));
	//
	// const auto IndirectAssets = Scanner->GetAssetsIndirect();
	// const int64 TotalSize = UProjectCleanerLibrary::AssetsGetTotalSize(IndirectAssets);
	//
	// return FText::FromString(FString::Printf(TEXT("Assets Indirect - %d (%s)"), IndirectAssets.Num(), *FText::AsMemory(TotalSize).ToString()));
}

FText SProjectCleanerTabScanSettings::GetStatsTextAssetsExcluded() const
{
	return FText::FromString(TEXT(""));
	// if (!Scanner.IsValid()) return FText::FromString(TEXT(""));
	//
	// const auto ExcludedAssets = Scanner->GetAssetsExcluded();
	// const int64 TotalSize = UProjectCleanerLibrary::AssetsGetTotalSize(ExcludedAssets);
	//
	// return FText::FromString(FString::Printf(TEXT("Assets Excluded - %d (%s)"), ExcludedAssets.Num(), *FText::AsMemory(TotalSize).ToString()));
}

FText SProjectCleanerTabScanSettings::GetStatsTextFilesNonEngine() const
{
	return FText::FromString(TEXT(""));
	// if (!Scanner.IsValid()) return FText::FromString(TEXT(""));
	//
	// const auto FilesNonEngine = Scanner->GetFilesNonEngine();
	// const int64 TotalSize = UProjectCleanerLibrary::FilesGetTotalSize(FilesNonEngine.Array());
	//
	// return FText::FromString(FString::Printf(TEXT("Files NonEngine - %d (%s)"), FilesNonEngine.Num(), *FText::AsMemory(TotalSize).ToString()));
}

FText SProjectCleanerTabScanSettings::GetStatsTextFilesCorrupted() const
{
	return FText::FromString(TEXT(""));
	// if (!Scanner.IsValid()) return FText::FromString(TEXT(""));
	//
	// const auto FilesCorrupted = Scanner->GetFilesCorrupted();
	// const int64 TotalSize = UProjectCleanerLibrary::FilesGetTotalSize(FilesCorrupted.Array());
	//
	// return FText::FromString(FString::Printf(TEXT("Files Corrupted - %d (%s)"), FilesCorrupted.Num(), *FText::AsMemory(TotalSize).ToString()));
}

FText SProjectCleanerTabScanSettings::GetStatsTextFoldersEmpty() const
{
	return FText::FromString(TEXT(""));
	// if (!Scanner.IsValid()) return FText::FromString(TEXT(""));
	//
	// return FText::FromString(FString::Printf(TEXT("Folders Empty - %d"), Scanner->GetFoldersEmpty().Num()));
}

FText SProjectCleanerTabScanSettings::GetStatsTextAssetsUnused() const
{
	return FText::FromString(TEXT(""));
	// if (!Scanner.IsValid()) return FText::FromString(TEXT(""));
	//
	// const int32 AssetsTotalNum = Scanner->GetAssetUnusedNum(UProjectCleanerLibrary::PathGetContentFolder(true));
	// const int64 AssetsTotalSize = Scanner->GetSizeUnused(UProjectCleanerLibrary::PathGetContentFolder(true));
	//
	// return FText::FromString(FString::Printf(TEXT("Assets Unused - %d (%s)"), AssetsTotalNum, *FText::AsMemory(AssetsTotalSize).ToString()));
}
