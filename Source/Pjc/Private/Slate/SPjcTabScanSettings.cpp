// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabScanSettings.h"
#include "PjcSubsystem.h"
#include "PjcTypes.h"
#include "PjcStyles.h"
#include "PjcConstants.h"
#include "PjcEditorSettings.h"
// Engine Headers
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SScrollBox.h"

void SPjcStatItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable)
{
	StatItem = InArgs._StatItem;

	SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments(), InTable);
}

TSharedRef<SWidget> SPjcStatItem::GenerateWidgetForColumn(const FName& InColumnName)
{
	if (InColumnName.IsEqual(TEXT("Category")))
	{
		const FLinearColor Color = StatItem->Num > 0 || StatItem->Size > 0 ? StatItem->TextColor : FLinearColor::White;
		return
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(StatItem->Padding)
			[
				SNew(STextBlock)
				.Text(FText::FromString(StatItem->Category))
				.Font(FPjcStyles::GetFont("Bold", 12))
				.ColorAndOpacity(Color)
				.ToolTipText(FText::FromString(StatItem->ToolTip))
			];
	}

	if (InColumnName.IsEqual(TEXT("Num")))
	{
		FNumberFormattingOptions NumberFormattingOptions;
		NumberFormattingOptions.SetUseGrouping(true);
		NumberFormattingOptions.SetMinimumFractionalDigits(0);

		const FString StrNum = StatItem->Num >= 0 ? FText::AsNumber(StatItem->Num, &NumberFormattingOptions).ToString() : TEXT("");
		const FLinearColor Color = StatItem->Num > 0 ? StatItem->TextColor : FLinearColor::White;

		return
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin{10.0f, 0.0f})
			[
				SNew(STextBlock)
				.Text(FText::FromString(StrNum))
				.ColorAndOpacity(Color)
			];
	}

	if (InColumnName.IsEqual(TEXT("Size")))
	{
		const FString StrSize = StatItem->Size >= 0 ? FText::AsMemory(StatItem->Size, IEC).ToString() : TEXT("");
		const FLinearColor Color = StatItem->Size > 0 ? StatItem->TextColor : FLinearColor::White;

		return
			SNew(STextBlock)
			.Text(FText::FromString(StrSize))
			.ColorAndOpacity(Color);
	}

	return SNew(STextBlock);
}

void SPjcTabScanSettings::Construct(const FArguments& InArgs)
{
	SubsystemPtr = GEditor->GetEditorSubsystem<UPjcSubsystem>();
	check(SubsystemPtr && TEXT("SubsystemPtr is nullptr"));

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
	SettingsProperty->SetObject(GetMutableDefault<UPjcEditorSettings>());

	SubsystemPtr->OnProjectScan().AddRaw(this, &SPjcTabScanSettings::StatsUpdate);
	StatsUpdate(SubsystemPtr->GetLastScanResult());

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
				+ SVerticalBox::Slot().Padding(FMargin{5.0f}).AutoHeight().HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Font(FPjcStyles::GetFont("Bold", 15))
					.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Green"))
					.Text(FText::FromString(TEXT("Project Content Statistics")))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					StatView.ToSharedRef()
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

SPjcTabScanSettings::~SPjcTabScanSettings()
{
	if (SubsystemPtr)
	{
		SubsystemPtr->OnProjectScan().RemoveAll(this);
	}
}

FReply SPjcTabScanSettings::OnBtnScanProjectClick() const
{
	return FReply::Handled();
}

FReply SPjcTabScanSettings::OnBtnCleanProjectClick() const
{
	// const FText Title = FText::FromString(TEXT("Confirm project cleaning"));
	// const FText Msg = GetCleanupText(GetDefault<UPjcSettings>()->CleanupMethod);
	// const EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo, Msg, &Title);
	//
	// if (Result == EAppReturnType::No || Result == EAppReturnType::Cancel)
	// {
	// 	return FReply::Handled();
	// }
	//
	// GEditor->GetEditorSubsystem<UPjcSubsystem>()->ProjectClean();

	return FReply::Handled();
}

bool SPjcTabScanSettings::BtnCleanProjectEnabled() const
{
	return false;
}

void SPjcTabScanSettings::StatsUpdate(const FPjcScanResult& InScanResult)
{
	if (!StatView.IsValid())
	{
		SAssignNew(StatView, SListView<TSharedPtr<FPjcStatItem>>)
		.ListItemsSource(&StatItems)
		.OnGenerateRow_Raw(this, &SPjcTabScanSettings::OnStatsGenerateRow)
		.SelectionMode(ESelectionMode::None)
		.HeaderRow(GetStatsHeaderRow());
	}

	const FMargin MarginFirstLvl{5.0f, 0.0f};
	const FMargin MarginSecondLvl{15.0f, 0.0f, 0.0f, 0.0f};
	const FMargin MarginThirdLvl{30.0f, 0.0f, 0.0f, 0.0f};
	const FMargin MarginFourthLvl{45.0f, 0.0f, 0.0f, 0.0f};

	StatItems.Empty();

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				InScanResult.ScanStats.SizeFilesTotal,
				InScanResult.ScanStats.NumFilesTotal,
				TEXT("Files Total"),
				TEXT("Total number of files inside Content folder. This is actual size of your project."),
				MarginFirstLvl
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				InScanResult.ScanStats.SizeFilesAsset,
				InScanResult.ScanStats.NumFilesAsset,
				TEXT("Assets Total"),
				TEXT("Total number of assets in project"),
				MarginSecondLvl
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				InScanResult.ScanStats.SizeAssetsUsed,
				InScanResult.ScanStats.NumAssetsUsed,
				TEXT("Used"),
				TEXT("Total number of used assets in project"),
				MarginThirdLvl
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				InScanResult.ScanStats.SizeAssetsPrimary,
				InScanResult.ScanStats.NumAssetsPrimary,
				TEXT("Primary"),
				TEXT("Total number of primary assets in project"),
				MarginFourthLvl
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				InScanResult.ScanStats.SizeAssetsIndirect,
				InScanResult.ScanStats.NumAssetsIndirect,
				TEXT("Indirect"),
				TEXT("Total number of indirect assets in project. Assets that used in source code or config files"),
				MarginFourthLvl
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				InScanResult.ScanStats.SizeAssetsEditor,
				InScanResult.ScanStats.NumAssetsEditor,
				TEXT("Editor"),
				TEXT("Total number of editor specific assets in project. Assets like EditorUtilityBlueprint, EditorUtilityWidgets or EditorTutorial"),
				MarginFourthLvl
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				InScanResult.ScanStats.SizeAssetsExcluded,
				InScanResult.ScanStats.NumAssetsExcluded,
				TEXT("Excluded"),
				TEXT("Total number of exluded assets by settings"),
				MarginFourthLvl,
				FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Yellow").GetSpecifiedColor()
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				InScanResult.ScanStats.SizeAssetsExtReferenced,
				InScanResult.ScanStats.NumAssetsExtReferenced,
				TEXT("ExtReferenced"),
				TEXT("Total number of assets that have external referencers outside Content folder"),
				MarginFourthLvl
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				InScanResult.ScanStats.SizeAssetsUnused,
				InScanResult.ScanStats.NumAssetsUnused,
				TEXT("Unused"),
				TEXT("Total number of unused assets in project"),
				MarginThirdLvl,
				FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Red").GetSpecifiedColor()
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				InScanResult.ScanStats.SizeFilesCorrupted,
				InScanResult.ScanStats.NumFilesCorrupted,
				TEXT("Corrupted"),
				TEXT("Files that have .umap or .uasset extension, but are not loaded by AssetRegistry"),
				MarginThirdLvl,
				FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Red").GetSpecifiedColor()
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				InScanResult.ScanStats.SizeFilesNonAsset,
				InScanResult.ScanStats.NumFilesNonAsset,
				TEXT("Non Asset Files"),
				TEXT("Files that dont have .umap or .uasset extension, but are inside Content folder"),
				MarginSecondLvl
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				PjcConstants::NoSize,
				InScanResult.ScanStats.NumFoldersTotal,
				TEXT("Folders Total"),
				TEXT("Total number of folders inside Content folder"),
				MarginFirstLvl
			}
		)
	);

	StatItems.Emplace(
		MakeShareable(
			new FPjcStatItem{
				PjcConstants::NoSize,
				InScanResult.ScanStats.NumFoldersEmpty,
				TEXT("Empty"),
				TEXT("Total number of empty folders inside Content folder. Engine generated paths like Developers, Collections will be ignored if they are empty"),
				MarginSecondLvl
			}
		)
	);

	StatView->RebuildList();
}

TSharedRef<SHeaderRow> SPjcTabScanSettings::GetStatsHeaderRow() const
{
	return
		SNew(SHeaderRow)
		+ SHeaderRow::Column("Category")
		  .FillWidth(0.6f)
		  .HAlignCell(HAlign_Left)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(PjcConstants::HeaderRowMargin)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Category")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", PjcConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column("Num")
		  .FillWidth(0.2f)
		  .HAlignCell(HAlign_Center)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(PjcConstants::HeaderRowMargin)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Num")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", PjcConstants::HeaderRowFontSize))
		]
		+ SHeaderRow::Column("Size")
		  .FillWidth(0.2f)
		  .HAlignCell(HAlign_Center)
		  .VAlignCell(VAlign_Center)
		  .HAlignHeader(HAlign_Center)
		  .HeaderContentPadding(PjcConstants::HeaderRowMargin)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Size")))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Font(FPjcStyles::GetFont("Light", PjcConstants::HeaderRowFontSize))
		];
}

TSharedRef<ITableRow> SPjcTabScanSettings::OnStatsGenerateRow(TSharedPtr<FPjcStatItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SPjcStatItem, OwnerTable).StatItem(Item);
}

// FText SPjcTabScanSettings::GetCleanupText(const EPjcCleanupMethod CleanupMethod) const
// {
// 	switch (CleanupMethod)
// 	{
// 		case EPjcCleanupMethod::None: return FText::FromString(TEXT(""));
// 		case EPjcCleanupMethod::Full: return FText::FromString(TEXT("Are you sure you want to delete all unused assets and empty folders in project?"));
// 		case EPjcCleanupMethod::UnusedAssetsOnly: return FText::FromString(TEXT("Are you sure you want to delete all unused assets in project?"));
// 		case EPjcCleanupMethod::EmptyFoldersOnly: return FText::FromString(TEXT("Are you sure you want to delete all empty folders in project?"));
// 		default:
// 			return FText::FromString(TEXT(""));
// 	}
// }
