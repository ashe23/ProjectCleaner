// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabScanSettings.h"
#include "PjcSubsystem.h"
#include "PjcSettings.h"
#include "PjcStyles.h"
// Engine Headers
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SScrollBox.h"

void SPjcTabScanSettings::Construct(const FArguments& InArgs)
{
	GEditor->GetEditorSubsystem<UPjcSubsystem>()->OnProjectScan().AddRaw(this, &SPjcTabScanSettings::OnProjectScan);

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
	SettingsProperty->SetObject(GetMutableDefault<UPjcSettings>());

	NumberFormattingOptions.SetUseGrouping(true);
	NumberFormattingOptions.SetMinimumFractionalDigits(0);

	UpdateData();

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().Padding(FMargin{5.0f})
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin{5.0f})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{0.0f, 0.0f, 5.0f, 0.0f})
				[
					SNew(SButton)
					.ContentPadding(FMargin{5.0f})
					.OnClicked_Raw(this, &SPjcTabScanSettings::OnBtnScanProjectClick)
					.ButtonColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Blue"))
					[
						SNew(STextBlock)
						.Justification(ETextJustify::Center)
						.ToolTipText(FText::FromString(TEXT("Scan project for unused assets, empty folders and other files")))
						.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.White"))
						.ShadowOffset(FVector2D{1.5f, 1.5f})
						.ShadowColorAndOpacity(FLinearColor::Black)
						.Font(FPjcStyles::GetFont("Bold", 10))
						.Text(FText::FromString(TEXT("Scan Project")))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{0.0f, 0.0f, 5.0f, 0.0f})
				[
					SNew(SButton)
					.ContentPadding(FMargin{5.0f})
					.OnClicked_Raw(this, &SPjcTabScanSettings::OnBtnCleanProjectClick)
					.IsEnabled_Raw(this, &SPjcTabScanSettings::BtnCleanProjectEnabled)
					.ButtonColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Red"))
					[
						SNew(STextBlock)
						.Justification(ETextJustify::Center)
						.ToolTipText(FText::FromString(TEXT("Clean project based on specified CleanupMethod.")))
						.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.White"))
						.ShadowOffset(FVector2D{1.5f, 1.5f})
						.ShadowColorAndOpacity(FLinearColor::Black)
						.Font(FPjcStyles::GetFont("Bold", 10))
						.Text(FText::FromString(TEXT("Clean Project")))
					]
				]
			]
			+ SVerticalBox::Slot().Padding(FMargin{5.0f}).AutoHeight()
			[
				SNew(SSeparator)
				.Thickness(5.0f)
			]
			+ SVerticalBox::Slot().Padding(FMargin{5.0f}).AutoHeight()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				[
					SNew(STextBlock)
					.Font(FPjcStyles::GetFont("Bold", 13))
					.Text_Raw(this, &SPjcTabScanSettings::GetStatAssetsTotal)
				]
				+ SVerticalBox::Slot()
				[
					SNew(STextBlock)
					.Font(FPjcStyles::GetFont("Bold", 13))
					.Text_Raw(this, &SPjcTabScanSettings::GetStatAssetsUnused)
				]
				+ SVerticalBox::Slot()
				[
					SNew(STextBlock)
					.Font(FPjcStyles::GetFont("Bold", 13))
					.Text_Raw(this, &SPjcTabScanSettings::GetStatAssetsUsed)
				]
				+ SVerticalBox::Slot().Padding(FMargin{15.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(STextBlock)
					.Font(FPjcStyles::GetFont("Bold", 11))
					.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Gray"))
					.Text_Raw(this, &SPjcTabScanSettings::GetStatAssetsExcluded)
				]
				+ SVerticalBox::Slot().Padding(FMargin{15.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(STextBlock)
					.Font(FPjcStyles::GetFont("Bold", 11))
					.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Gray"))
					.Text_Raw(this, &SPjcTabScanSettings::GetStatAssetsIndirect)
				]
				+ SVerticalBox::Slot().Padding(FMargin{15.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(STextBlock)
					.Font(FPjcStyles::GetFont("Bold", 11))
					.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Gray"))
					.Text_Raw(this, &SPjcTabScanSettings::GetStatAssetsPrimary)
				]
				+ SVerticalBox::Slot().Padding(FMargin{15.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(STextBlock)
					.Font(FPjcStyles::GetFont("Bold", 11))
					.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Gray"))
					.Text_Raw(this, &SPjcTabScanSettings::GetStatAssetsEditor)
				]
				+ SVerticalBox::Slot().Padding(FMargin{15.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(STextBlock)
					.Font(FPjcStyles::GetFont("Bold", 11))
					.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Gray"))
					.Text_Raw(this, &SPjcTabScanSettings::GetStatAssetsExtReferenced)
				]
				+ SVerticalBox::Slot().Padding(FMargin{0.0f, 10.0f, 0.0f, 0.0f})
				[
					SNew(STextBlock)
					.Font(FPjcStyles::GetFont("Bold", 13))
					.Text_Raw(this, &SPjcTabScanSettings::GetStatFilesNonEngine)
				]
				+ SVerticalBox::Slot()
				[
					SNew(STextBlock)
					.Font(FPjcStyles::GetFont("Bold", 13))
					.Text_Raw(this, &SPjcTabScanSettings::GetStatFilesCorrupted)
				]
				+ SVerticalBox::Slot().Padding(FMargin{0.0f, 10.0f, 0.0f, 0.0f})
				[
					SNew(STextBlock)
					.Font(FPjcStyles::GetFont("Bold", 13))
					.Text_Raw(this, &SPjcTabScanSettings::GetStatFoldersEmpty)
				]
			]
			+ SVerticalBox::Slot().Padding(FMargin{5.0f}).AutoHeight()
			[
				SNew(SSeparator)
				.Thickness(5.0f)
			]
			+ SVerticalBox::Slot().Padding(FMargin{5.0f}).FillHeight(1.0f)
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
	];
}

void SPjcTabScanSettings::OnProjectScan(const EPjcScanResult ScanResult, const FString&)
{
	if (ScanResult == EPjcScanResult::Success)
	{
		UpdateData();
	}
}

FReply SPjcTabScanSettings::OnBtnScanProjectClick() const
{
	GEditor->GetEditorSubsystem<UPjcSubsystem>()->ProjectScan();

	return FReply::Handled();
}

FReply SPjcTabScanSettings::OnBtnCleanProjectClick() const
{
	const FText Title = FText::FromString(TEXT("Confirm project cleaning"));
	const FText Msg = GetCleanupText(GetDefault<UPjcSettings>()->CleanupMethod);
	const EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo, Msg, &Title);

	if (Result == EAppReturnType::No || Result == EAppReturnType::Cancel)
	{
		return FReply::Handled();
	}

	GEditor->GetEditorSubsystem<UPjcSubsystem>()->ProjectClean();

	return FReply::Handled();
}

bool SPjcTabScanSettings::BtnCleanProjectEnabled() const
{
	return NumAssetsUnused > 0 || NumFoldersEmpty > 0;
}

void SPjcTabScanSettings::UpdateData()
{
	const UPjcSubsystem* SubsystemPtr = GEditor->GetEditorSubsystem<UPjcSubsystem>();

	NumAssetsTotal = SubsystemPtr->GetNumAssetsAll();
	NumAssetsUsed = SubsystemPtr->GetNumAssetsUsed();
	NumAssetsUnused = SubsystemPtr->GetNumAssetsUnused();
	NumAssetsExcluded = SubsystemPtr->GetNumAssetsExcluded();
	NumAssetsIndirect = SubsystemPtr->GetNumAssetsIndirect();
	NumAssetsPrimary = SubsystemPtr->GetNumAssetsPrimary();
	NumAssetsEditor = SubsystemPtr->GetNumAssetsEditor();
	NumAssetsExtReferenced = SubsystemPtr->GetNumAssetsExtReferenced();
	NumFilesNonEngine = SubsystemPtr->GetNumFilesNonEngine();
	NumFilesCorrupted = SubsystemPtr->GetNumFilesNonCorrupted();
	NumFoldersEmpty = SubsystemPtr->GetNumFolderEmpty();

	SizeAssetsTotal = SubsystemPtr->GetSizeAssetsAll();
	SizeAssetsUsed = SubsystemPtr->GetSizeAssetsUsed();
	SizeAssetsUnused = SubsystemPtr->GetSizeAssetsUnused();
	SizeAssetsExcluded = SubsystemPtr->GetSizeAssetsExcluded();
	SizeAssetsIndirect = SubsystemPtr->GetSizeAssetsIndirect();
	SizeAssetsPrimary = SubsystemPtr->GetSizeAssetsPrimary();
	SizeAssetsEditor = SubsystemPtr->GetSizeAssetsEditor();
	SizeAssetsExtReferenced = SubsystemPtr->GetSizeAssetsExtReferenced();
	SizeFilesNonEngine = SubsystemPtr->GetSizeFilesNonEngine();
	SizeFilesCorrupted = SubsystemPtr->GetSizeFilesCorrupted();
}

FText SPjcTabScanSettings::GetStatAssetsTotal() const
{
	const FString StrSize = FText::AsMemory(SizeAssetsTotal, IEC).ToString();
	const FString StrNum = FText::AsNumber(NumAssetsTotal, &NumberFormattingOptions).ToString();

	return FText::FromString(FString::Printf(TEXT("Assets Total - %s ( %s )"), *StrNum, *StrSize));
}

FText SPjcTabScanSettings::GetStatAssetsUsed() const
{
	const FString StrSize = FText::AsMemory(SizeAssetsUsed, IEC).ToString();
	const FString StrNum = FText::AsNumber(NumAssetsUsed, &NumberFormattingOptions).ToString();

	return FText::FromString(FString::Printf(TEXT("Assets Used - %s ( %s )"), *StrNum, *StrSize));
}

FText SPjcTabScanSettings::GetStatAssetsUnused() const
{
	const FString StrSize = FText::AsMemory(SizeAssetsUnused, IEC).ToString();
	const FString StrNum = FText::AsNumber(NumAssetsUnused, &NumberFormattingOptions).ToString();

	return FText::FromString(FString::Printf(TEXT("Assets Unused - %s ( %s )"), *StrNum, *StrSize));
}

FText SPjcTabScanSettings::GetStatAssetsExcluded() const
{
	const FString StrSize = FText::AsMemory(SizeAssetsExcluded, IEC).ToString();
	const FString StrNum = FText::AsNumber(NumAssetsExcluded, &NumberFormattingOptions).ToString();

	return FText::FromString(FString::Printf(TEXT("Excluded - %s ( %s )"), *StrNum, *StrSize));
}

FText SPjcTabScanSettings::GetStatAssetsIndirect() const
{
	const FString StrSize = FText::AsMemory(SizeAssetsIndirect, IEC).ToString();
	const FString StrNum = FText::AsNumber(NumAssetsIndirect, &NumberFormattingOptions).ToString();

	return FText::FromString(FString::Printf(TEXT("Indirect - %s ( %s )"), *StrNum, *StrSize));
}

FText SPjcTabScanSettings::GetStatAssetsPrimary() const
{
	const FString StrSize = FText::AsMemory(SizeAssetsPrimary, IEC).ToString();
	const FString StrNum = FText::AsNumber(NumAssetsPrimary, &NumberFormattingOptions).ToString();

	return FText::FromString(FString::Printf(TEXT("Primary - %s ( %s )"), *StrNum, *StrSize));
}

FText SPjcTabScanSettings::GetStatAssetsEditor() const
{
	const FString StrSize = FText::AsMemory(SizeAssetsEditor, IEC).ToString();
	const FString StrNum = FText::AsNumber(NumAssetsEditor, &NumberFormattingOptions).ToString();

	return FText::FromString(FString::Printf(TEXT("Editor - %s ( %s )"), *StrNum, *StrSize));
}

FText SPjcTabScanSettings::GetStatAssetsExtReferenced() const
{
	const FString StrSize = FText::AsMemory(SizeAssetsExtReferenced, IEC).ToString();
	const FString StrNum = FText::AsNumber(NumAssetsExtReferenced, &NumberFormattingOptions).ToString();

	return FText::FromString(FString::Printf(TEXT("ExtReferenced - %s ( %s )"), *StrNum, *StrSize));
}

FText SPjcTabScanSettings::GetStatFilesNonEngine() const
{
	const FString StrSize = FText::AsMemory(SizeFilesNonEngine, IEC).ToString();
	const FString StrNum = FText::AsNumber(NumFilesNonEngine, &NumberFormattingOptions).ToString();

	return FText::FromString(FString::Printf(TEXT("Files NonEngine - %s ( %s )"), *StrNum, *StrSize));
}

FText SPjcTabScanSettings::GetStatFilesCorrupted() const
{
	const FString StrSize = FText::AsMemory(SizeFilesCorrupted, IEC).ToString();
	const FString StrNum = FText::AsNumber(NumFilesCorrupted, &NumberFormattingOptions).ToString();

	return FText::FromString(FString::Printf(TEXT("Files Corrupted - %s ( %s )"), *StrNum, *StrSize));
}

FText SPjcTabScanSettings::GetStatFoldersEmpty() const
{
	const FString StrNum = FText::AsNumber(NumFoldersEmpty, &NumberFormattingOptions).ToString();

	return FText::FromString(FString::Printf(TEXT("Folders Empty - %s"), *StrNum));
}

FText SPjcTabScanSettings::GetCleanupText(const EPjcCleanupMethod CleanupMethod) const
{
	switch (CleanupMethod)
	{
		case EPjcCleanupMethod::None: return FText::FromString(TEXT(""));
		case EPjcCleanupMethod::Full: return FText::FromString(TEXT("Are you sure you want to delete all unused assets and empty folders in project?"));
		case EPjcCleanupMethod::UnusedAssetsOnly: return FText::FromString(TEXT("Are you sure you want to delete all unused assets in project?"));
		case EPjcCleanupMethod::EmptyFoldersOnly: return FText::FromString(TEXT("Are you sure you want to delete all empty folders in project?"));
		default:
			return FText::FromString(TEXT(""));
	}
}
