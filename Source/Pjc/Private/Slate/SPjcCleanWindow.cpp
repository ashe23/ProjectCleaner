// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcCleanWindow.h"
#include "Slate/PjcTabs.h"
#include "Slate/PjcWidgets.h"
#include "Slate/SPjcScanBar.h"
#include "PjcFormat.h"
#include "PjcScanDraft.h"
#include "PjcShim.h"
#include "PjcStyles.h"
#include "PjcSubsystem.h"
#include "PjcUnused.h"
#include "Pjc.h"
// Engine Headers
#include "FileHelpers.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	// unity builds join files: names unique to this file
	constexpr int32 PjcCleanTopRows = 5;

	// Version control of the project: provider name and what it means for deleted files, empty when there is none
	bool PjcCleanVersionControl(FString& OutName, FString& OutDetail) {
		ISourceControlModule& SourceControl = ISourceControlModule::Get();
		if (SourceControl.IsEnabled() && SourceControl.GetProvider().IsAvailable()) {
			OutName = SourceControl.GetProvider().GetName().ToString();
			const bool bGit = OutName.Contains(TEXT("Git"));
			OutDetail = bGit ? TEXT("Deleted files can be restored from it")
							 : TEXT("Files are checked out and marked for delete; submit when the project still works");
			return true;
		}
		if (UPjcSubsystem::ProjectHasVersionControl()) {
			OutName = TEXT("found next to the project");
			OutDetail = TEXT("Deleted files can be restored from it");
			return true;
		}
		return false;
	}
}	// namespace

void SPjcCleanWindow::Construct(const FArguments& InArgs) {
	Window = InArgs._Window;
	Plan = FPjcCleanPlan::Make(UPjcSubsystem::GetScanResult());

	// clang-format off
	ChildSlot
	[
		SNew(SBorder).BorderImage(PjcShim::GetBrushPage()).Padding(0.0f)
		[
			SAssignNew(Body, SBox)
		]
	];
	// clang-format on
	Rebuild();
}

void SPjcCleanWindow::Open() {
	// clang-format off
	const TSharedRef<SWindow> CleanWindow = SNew(SWindow)
		.Title(FText::FromString(TEXT("Clean project")))
		.ClientSize(FVector2D {720.0f, 620.0f})
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.SizingRule(ESizingRule::UserSized);
	// clang-format on
	const TSharedRef<SPjcCleanWindow> Content = SNew(SPjcCleanWindow).Window(CleanWindow);
	CleanWindow->SetContent(Content);

	// the window stays while assets are deleted
	const TWeakPtr<SPjcCleanWindow> WeakContent = Content;
	CleanWindow->SetRequestDestroyWindowOverride(FRequestDestroyWindowOverride::CreateLambda([WeakContent](const TSharedRef<SWindow>& Closing) {
		const TSharedPtr<SPjcCleanWindow> Pinned = WeakContent.Pin();
		if (!Pinned.IsValid() || Pinned->Phase != EPhase::Run) {
			FSlateApplication::Get().RequestDestroyWindow(Closing);
		}
	}));

	PjcTabs::SetWindowOpen(true);
	FSlateApplication::Get().AddModalWindow(CleanWindow, FSlateApplication::Get().GetActiveTopLevelWindow());
	PjcTabs::SetWindowOpen(false);
}

void SPjcCleanWindow::Rebuild() {
	const TSharedRef<SWidget> Content = Phase == EPhase::Confirm ? MakeConfirm() : (Phase == EPhase::Run ? MakeRun() : MakeDone());
	Body->SetContent(Content);
}

TSharedRef<SWidget> SPjcCleanWindow::MakeNumbers(const TArray<TTuple<FString, FString, FString>>& Numbers, const int32 Highlight) {
	const TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < Numbers.Num(); ++Index) {
		const FSlateColor Red {PjcWidgets::GetColor("ProjectCleaner.Color.Red")};
		const FSlateColor NumColor = Index == Highlight ? Red : FSlateColor::UseForeground();
		// clang-format off
		Row->AddSlot().FillWidth(1.0f)
		[
			SNew(SBox).Padding(FMargin {12.0f, 8.0f}).ToolTip(PjcWidgets::MakeToolTip(Numbers[Index].Get<1>(), Numbers[Index].Get<2>()))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(FText::FromString(Numbers[Index].Get<0>())).Font(PjcWidgets::GetFontMono(16)).ColorAndOpacity(NumColor)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(FText::FromString(Numbers[Index].Get<1>())).ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			]
		];
		// clang-format on
	}
	return PjcWidgets::MakeCard(Row, FMargin {1.0f});
}

TSharedRef<SWidget> SPjcCleanWindow::MakeTopRows(const FString& Title, const TArray<TPair<FString, FString>>& Rows, const int32 NumMore) {
	const TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
	// clang-format off
	List->AddSlot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 4.0f})
	[
		PjcWidgets::MakeCardTitle(Title)
	];
	// clang-format on
	for (const auto& Row : Rows) {
		// clang-format off
		List->AddSlot().AutoHeight().Padding(FMargin {0.0f, 1.0f})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(STextBlock).Text(FText::FromString(Row.Key)).Clipping(EWidgetClipping::ClipToBounds)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin {10.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock).Text(FText::FromString(Row.Value)).Font(PjcWidgets::GetFontMono(9)).ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		];
		// clang-format on
	}
	if (NumMore > 0) {
		// clang-format off
		List->AddSlot().AutoHeight()
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("and %s more"), *PjcFormat::Count(NumMore))))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		];
		// clang-format on
	}
	return List;
}

TSharedRef<SWidget> SPjcCleanWindow::MakeNote(const FString& Text, const bool bWarn) {
	const FLinearColor Edge = bWarn ? PjcWidgets::GetColor("ProjectCleaner.Color.Review") : PjcWidgets::GetColor("ProjectCleaner.Color.Line");
	// clang-format off
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(PjcWidgets::GetColor("ProjectCleaner.Color.Panel"))
		.Padding(0.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(3.0f)
				[
					SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(Edge)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(FMargin {10.0f, 6.0f})
			[
				SNew(STextBlock).Text(FText::FromString(Text)).AutoWrapText(true)
			]
		];
	// clang-format on
}

TSharedRef<SWidget> SPjcCleanWindow::MakeCheck(const ECheck Kind, const FString& Title, const FString& Detail, const TSharedRef<SWidget>& Extra) {
	const TCHAR* OtherIcon = Kind == ECheck::Info ? TEXT("i") : TEXT("\u2013");
	const TCHAR* Icon = Kind == ECheck::Ok ? TEXT("\u2713") : (Kind == ECheck::Warn ? TEXT("\u26A0") : OtherIcon);
	const FSlateColor OkColor {PjcWidgets::GetColor("ProjectCleaner.Color.Ok")};
	const FSlateColor WarnColor {PjcWidgets::GetColor("ProjectCleaner.Color.Review")};
	const FSlateColor OtherColor = Kind == ECheck::Warn ? WarnColor : FSlateColor::UseSubduedForeground();
	const FSlateColor IconColor = Kind == ECheck::Ok ? OkColor : OtherColor;
	const FString DetailText = Detail.IsEmpty() ? FString {} : TEXT(" \u00B7 ") + Detail;
	// clang-format off
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(PjcWidgets::GetColor("ProjectCleaner.Color.Recess"))
		.Padding(FMargin {10.0f, 5.0f})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 8.0f, 0.0f})
			[
				SNew(SBox).WidthOverride(14.0f).HAlign(HAlign_Center)
				[
					SNew(STextBlock).Text(FText::FromString(Icon)).ColorAndOpacity(IconColor)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(STextBlock).Text(FText::FromString(Title))
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(STextBlock).Text(FText::FromString(DetailText)).ColorAndOpacity(FSlateColor::UseSubduedForeground()).Clipping(EWidgetClipping::ClipToBounds)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {8.0f, 0.0f, 0.0f, 0.0f})
			[
				Extra
			]
		];
	// clang-format on
}

FString SPjcCleanWindow::GetBlockReason() const {
	TArray<UPackage*> Dirty;
	FEditorFileUtils::GetDirtyContentPackages(Dirty);
	FString VcsName;
	FString VcsDetail;

	if (UPjcSubsystem::GetModuleAssetRegistry().Get().IsLoadingAssets()) return TEXT("Wait for the asset registry");
	if (SPjcScanBar::NeedsRescan()) return TEXT("Rescan first: results are from before the change");
	if (Dirty.Num() > 0) return TEXT("Save and scan first");
	if (!PjcCleanVersionControl(VcsName, VcsDetail) && !bBackup) return TEXT("Confirm you have a backup");
	if (Plan.Assets.Num() == 0) return TEXT("Nothing to delete");
	return FString {};
}

TSharedRef<SWidget> SPjcCleanWindow::MakeConfirm() {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	Sizes = Result.Sizes;

	// numbers: value, what, explanation
	const auto Number = [](const FString& Value, const TCHAR* What, const TCHAR* ToolTip) {
		return MakeTuple(Value, FString {What}, FString {ToolTip});
	};
	const TArray<TTuple<FString, FString, FString>> Numbers {
	  Number(PjcFormat::Count(Plan.Assets.Num()), TEXT("unused assets"), TEXT("Unused assets of the last scan: nothing used references them")),
	  Number(PjcFormat::Size(Plan.Size), TEXT("freed"), TEXT("Size on disk of the deleted assets")),
	  Number(PjcFormat::Count(Plan.NumFoldersEmpty), TEXT("empty folders"), TEXT("Folders the delete leaves empty, and folders empty now")),
	  Number(PjcFormat::Count(Plan.NumRedirectors), TEXT("redirectors"), TEXT("Redirectors nothing uses that point to deleted assets go with them")),
	};

	// where and what
	TArray<TPair<FString, FString>> FolderRows;
	for (int32 Index = 0; Index < Plan.Folders.Num() && Index < PjcCleanTopRows; ++Index) {
		const auto& Folder = Plan.Folders[Index];
		const FString Total = FString::Printf(TEXT("%s \u00B7 %s"), *PjcFormat::Size(Folder.Value.Size), *PjcFormat::Count(Folder.Value.Num));
		FolderRows.Emplace(Folder.Key.Mid(1), Total);
	}
	TArray<TPair<FString, FString>> ClassRows;
	for (int32 Index = 0; Index < Plan.Classes.Num() && Index < PjcCleanTopRows; ++Index) {
		const auto& Class = Plan.Classes[Index];
		FString ClassName = Class.Key.ToString();
		int32 Dot = INDEX_NONE;
		if (ClassName.FindLastChar(TEXT('.'), Dot)) {
			ClassName.RightChopInline(Dot + 1);
		}
		const FString Total = FString::Printf(TEXT("%s \u00B7 %s"), *PjcFormat::Size(Class.Value.Size), *PjcFormat::Count(Class.Value.Num));
		ClassRows.Emplace(ClassName, Total);
	}
	const int32 MoreFolders = FMath::Max(0, Plan.Folders.Num() - PjcCleanTopRows);
	const int32 MoreClasses = FMath::Max(0, Plan.Classes.Num() - PjcCleanTopRows);

	// checks
	const TSharedRef<SVerticalBox> Checks = SNew(SVerticalBox);
	const auto AddCheck = [&Checks](const TSharedRef<SWidget>& Check) {
		// clang-format off
		Checks->AddSlot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 1.0f})
		[
			Check
		];
		// clang-format on
	};
	const TSharedRef<SWidget> NoExtra = SNullWidget::NullWidget;
	const auto MakeButton = [](const FString& Label, TFunction<void()> OnClick) {
		// clang-format off
		return StaticCastSharedRef<SWidget>(
			SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
			.ButtonStyle(&PjcShim::GetButtonStyle427(false))
			.ForegroundColor(FSlateColor::UseForeground())
#endif
			.ContentPadding(FMargin {8.0f, 0.0f})
			.OnClicked_Lambda([OnClick]() {
				OnClick();
				return FReply::Handled();
			})
			[
				SNew(STextBlock).Text(FText::FromString(Label))
			]);
		// clang-format on
	};

	const auto OnScanAgain = [this]() {
		ScanAgain();
	};
	const auto OnReview = [this]() {
		Close();
		PjcTabs::ShowSet(PjcTabs::GetMain(), EPjcAssetSet::Review);
	};
	if (SPjcScanBar::NeedsRescan()) {
		const bool bSettings = FPjcScanDraft::IsChanged();
		const TCHAR* What = bSettings ? TEXT("Scan settings changed since the last scan") : TEXT("Project changed since the last scan");
		const FString Count = PjcFormat::Count(UPjcSubsystem::GetChangesSinceScan().Num()) + TEXT(" changes \u00B7 these numbers are old");
		const FString Detail = bSettings ? TEXT("these numbers are old") : Count;
		AddCheck(MakeCheck(ECheck::Warn, What, Detail, MakeButton(TEXT("Rescan"), OnScanAgain)));
	}
	if (Plan.NumReview > 0) {
		const FString ReviewTitle = FString::Printf(TEXT("%s assets to review"), *PjcFormat::Count(Plan.NumReview));
		const TCHAR* ReviewDetail = TEXT("Kept: the scan is not sure about them");
		AddCheck(MakeCheck(ECheck::Info, ReviewTitle, ReviewDetail, MakeButton(TEXT("Review"), OnReview)));
	}
	TArray<UPackage*> Dirty;
	FEditorFileUtils::GetDirtyContentPackages(Dirty);
	if (Dirty.Num() > 0) {
		const FString DirtyTitle = FString::Printf(TEXT("%s unsaved assets"), *PjcFormat::Count(Dirty.Num()));
		const TCHAR* DirtyDetail = TEXT("The scan reads assets from disk: save them and scan again");
		AddCheck(MakeCheck(ECheck::Warn, DirtyTitle, DirtyDetail, MakeButton(TEXT("Save all and scan"), OnScanAgain)));
	}
	else {
		AddCheck(MakeCheck(ECheck::Ok, TEXT("All assets saved"), FString {}, NoExtra));
	}
	FString VcsName;
	FString VcsDetail;
	if (PjcCleanVersionControl(VcsName, VcsDetail)) {
		AddCheck(MakeCheck(ECheck::Ok, TEXT("Version control: ") + VcsName, VcsDetail, NoExtra));
	}
	else {
		// clang-format off
		const TSharedRef<SWidget> Backup = SNew(SCheckBox)
			.Style(&PjcShim::GetCheckBoxStyle())
			.IsChecked_Lambda([this]() { return bBackup ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this](const ECheckBoxState State) { bBackup = State == ECheckBoxState::Checked; })
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("I have a backup of this project")))
			];
		// clang-format on
		AddCheck(MakeCheck(ECheck::Warn, TEXT("No version control"), TEXT("Deleting cannot be undone"), Backup));
	}
	const FPjcCookCheck& Cook = Result.CookCheck;
	if (!Cook.IsFound()) {
		AddCheck(MakeCheck(ECheck::None, TEXT("Cook check"), TEXT("No cook found \u00B7 optional"), NoExtra));
	}
	else if (Cook.IsCookAll()) {
		AddCheck(MakeCheck(ECheck::None, TEXT("Cook check"), TEXT("The last cook took nearly all assets \u00B7 not compared"), NoExtra));
	}
	else if (Cook.NumPackaged > 0) {
		const FString Packaged = PjcFormat::Count(Cook.NumPackaged) + TEXT(" unused assets the last cook packaged are in Review and kept");
		AddCheck(MakeCheck(ECheck::Ok, TEXT("Cook check"), Packaged, NoExtra));
	}
	else {
		const FString NotChecked
			= FString::Printf(TEXT(" \u00B7 %s changed after it: cook again to check them"), *PjcFormat::Count(Cook.NumNotChecked));
		const FString NoneCooked = TEXT("The last cook packaged none of them") + (Cook.NumNotChecked > 0 ? NotChecked : FString {});
		AddCheck(MakeCheck(ECheck::Ok, TEXT("Cook check"), NoneCooked, NoExtra));
	}
	if (UPjcSubsystem::GetModuleAssetRegistry().Get().IsLoadingAssets()) {
		AddCheck(MakeCheck(ECheck::Warn, TEXT("Asset registry"), TEXT("Still discovering assets"), NoExtra));
	}
	else {
		AddCheck(MakeCheck(ECheck::Ok, TEXT("Asset registry ready"), TEXT("Every asset was found before the scan"), NoExtra));
	}
	const TCHAR* EditorsDetail = TEXT("Editors that show an asset in the list are closed before deleting");
	AddCheck(MakeCheck(ECheck::Info, TEXT("Asset editors close first"), EditorsDetail, NoExtra));

	const FString Kept = TEXT("Kept: ") + PjcFormat::Count(Plan.NumExcluded) + TEXT(" excluded \u00B7 every used, editor only and Review asset");
	const FString DeleteLabel = FString::Printf(TEXT("Delete %s assets"), *PjcFormat::Count(Plan.Assets.Num()));

	// clang-format off
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot().Padding(FMargin {16.0f, 12.0f, 16.0f, 8.0f})
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					MakeNumbers(Numbers, 0)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 12.0f})
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(FMargin {0.0f, 0.0f, 14.0f, 0.0f})
					[
						MakeTopRows(TEXT("Folders"), FolderRows, MoreFolders)
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[
						MakeTopRows(TEXT("Classes"), ClassRows, MoreClasses)
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 12.0f})
				[
					MakeNote(Kept, false)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 4.0f})
				[
					PjcWidgets::MakeCardTitle(TEXT("Checks"))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					Checks
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBorder).BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.DarkGroupBorder"))).Padding(FMargin {16.0f, 8.0f})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([this]() {
						const FString Reason = GetBlockReason();
						return FText::FromString(Reason.IsEmpty() ? TEXT("Each asset is checked again right before it is deleted") : Reason);
					})
					.ColorAndOpacity_Lambda([this]() {
						return GetBlockReason().IsEmpty() ? FSlateColor::UseSubduedForeground() : FSlateColor {PjcWidgets::GetColor("ProjectCleaner.Color.Review")};
					})
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin {8.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
					.ButtonStyle(&PjcShim::GetButtonStyle427(false))
					.ForegroundColor(FSlateColor::UseForeground())
#endif
					.ContentPadding(FMargin {12.0f, 2.0f})
					.OnClicked_Lambda([this]() {
						Close();
						return FReply::Handled();
					})
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("Cancel")))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin {8.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
					.ButtonStyle(&PjcShim::GetButtonStyle427(true))
					.ForegroundColor(FSlateColor::UseForeground())
#endif
					.ButtonColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.UnusedFill"))
					.ContentPadding(FMargin {12.0f, 2.0f})
					.IsEnabled_Lambda([this]() { return GetBlockReason().IsEmpty(); })
					.OnClicked_Lambda([this]() {
						Run();
						return FReply::Handled();
					})
					[
						SNew(STextBlock).Text(FText::FromString(DeleteLabel)).ColorAndOpacity(FLinearColor::White)
					]
				]
			]
		];
	// clang-format on
}

void SPjcCleanWindow::ScanAgain() {
	// the window stays; the scan saves unsaved assets first (it asks)
	if (FPjcScanDraft::IsChanged()) {
		FPjcScanDraft::Get().ApplyToSettings();
	}
	SPjcScanBar::ScanProject();
	Plan = FPjcCleanPlan::Make(UPjcSubsystem::GetScanResult());
	Rebuild();
}

TSharedRef<SWidget> SPjcCleanWindow::MakeRun() {
	const TSharedRef<SVerticalBox> StepList = SNew(SVerticalBox);
	for (int32 Index = 0; Index < Steps.Num(); ++Index) {
		// clang-format off
		StepList->AddSlot().AutoHeight().Padding(FMargin {0.0f, 2.0f})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin {0.0f, 0.0f, 8.0f, 0.0f})
			[
				SNew(SBox).WidthOverride(14.0f).HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([this, Index]() { return FText::FromString(Index < StepIndex ? TEXT("\u2713") : (Index == StepIndex ? TEXT("\u203A") : TEXT("\u00B7"))); })
					.ColorAndOpacity_Lambda([this, Index]() {
						return Index < StepIndex ? FSlateColor {PjcWidgets::GetColor("ProjectCleaner.Color.Ok")} : FSlateColor::UseSubduedForeground();
					})
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Steps[Index]))
				.ColorAndOpacity_Lambda([this, Index]() { return Index <= StepIndex ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground(); })
			]
		];
		// clang-format on
	}

	// clang-format off
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(FMargin {16.0f, 12.0f})
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				StepList
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 12.0f, 0.0f, 4.0f})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SProgressBar)
#if ENGINE_MAJOR_VERSION < 5
					.Style(&PjcShim::GetProgressStyle427())
#endif
					.Percent_Lambda([this]() { return TOptional<float> {Percent}; })
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {10.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(SBox).WidthOverride(40.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("%d%%"), FMath::FloorToInt(Percent * 100.0f))); })
						.Font(PjcWidgets::GetFontMono(9))
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(CurrentText); }).ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 12.0f, 0.0f, 0.0f})
			[
				MakeNote(TEXT("The editor deletes in batches. Assets something still holds are kept and listed at the end."), false)
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBorder).BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.DarkGroupBorder"))).Padding(FMargin {16.0f, 8.0f})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { return FText::FromString(bStopRequested ? TEXT("Stopping after this batch\u2026") : TEXT("")); })
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
					.ButtonStyle(&PjcShim::GetButtonStyle427(false))
					.ForegroundColor(FSlateColor::UseForeground())
#endif
					.ContentPadding(FMargin {12.0f, 2.0f})
					.ToolTipText(FText::FromString(TEXT("Stops after the batch that is running. What is deleted stays deleted.")))
					.IsEnabled_Lambda([this]() { return !bStopRequested; })
					.OnClicked_Lambda([this]() {
						bStopRequested = true;
						return FReply::Handled();
					})
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("Stop")))
					]
				]
			]
		];
	// clang-format on
}

void SPjcCleanWindow::SetStep(const int32 Step, const FString& Current) {
	StepIndex = Step;
	CurrentText = Current;
	PumpUi();
}

void SPjcCleanWindow::PumpUi() {
	// the delete runs here: the window is drawn and Stop is read between batches
	if (FSlateApplication::IsInitialized()) {
		FSlateApplication::Get().PumpMessages();
		FSlateApplication::Get().Tick();
	}
}

void SPjcCleanWindow::Run() {
	Phase = EPhase::Run;
	Steps = {
	  TEXT("Checking they are still unused"),
	  TEXT("Closing asset editors and deleting assets"),
	  TEXT("Deleting empty folders"),
	  TEXT("Scanning again"),
	};
	Percent = 0.0f;
	Rebuild();

	// every asset is checked again: one that is used by now is skipped and named
	SetStep(0, FString::Printf(TEXT("%s assets"), *PjcFormat::Count(Plan.Assets.Num())));
	TArray<FAssetData> UnusedNow;
	UPjcSubsystem::GetAssetsUnused(UnusedNow, false);
	TSet<FName> UnusedPackages;
	for (const auto& Asset : UnusedNow) {
		UnusedPackages.Add(Asset.PackageName);
	}
	TArray<FAssetData> ToDelete;
	Skipped.Reset();
	for (const auto& Asset : Plan.Assets) {
		if (UnusedPackages.Contains(Asset.PackageName)) {
			ToDelete.Add(Asset);
		}
		else {
			Skipped.Add(Asset.PackageName);
		}
	}
	Percent = 0.05f;

	// batches; Stop ends after the running one
	SetStep(1, TEXT("Closing asset editors\u2026"));
	FPjcDeleteProgress Progress;
	FPjcDeleteProgress* ProgressPtr = &Progress;
	Progress.OnBatch = [this, ProgressPtr](const int32 NumTried, const int32 NumTotal, const FString& Current) {
		Percent = 0.05f + 0.8f * static_cast<float>(NumTried) / static_cast<float>(FMath::Max(1, NumTotal));
		SetStep(1, FString::Printf(TEXT("%s / %s \u00B7 %s"), *PjcFormat::Count(NumTried), *PjcFormat::Count(NumTotal), *Current));
		ProgressPtr->bStopRequested = bStopRequested;
	};
	TArray<FString> RedirectorsBefore;
	for (const auto& Redirector : UPjcSubsystem::GetScanResult().Redirectors) {
		RedirectorsBefore.Add(Redirector.PackageName.ToString());
	}
	UPjcSubsystem::DeleteAssets(ToDelete, false, false, &Progress);
	bStopped = Progress.bStopped;
	Percent = 0.85f;

	TArray<FName> Deleted;
	NumDeleted = 0;
	SizeFreed = 0;
	for (const auto& Asset : ToDelete) {
		if (FPackageName::DoesPackageExist(Asset.PackageName.ToString())) continue;

		Deleted.Add(Asset.PackageName);
		++NumDeleted;
		SizeFreed += Sizes.FindRef(Asset.PackageName);
	}
	NumLeft = bStopped ? ToDelete.Num() - Progress.NumTried : 0;
	NumRedirectorsRemoved = 0;
	for (const auto& Redirector : RedirectorsBefore) {
		NumRedirectorsRemoved += FPackageName::DoesPackageExist(Redirector) ? 0 : 1;
	}

	// empty folders, not after a stop: the rest is still there
	NumFoldersRemoved = 0;
	if (!bStopped) {
		SetStep(2, FString {});
		TArray<FString> FoldersBefore;
		UPjcSubsystem::GetFoldersEmpty(FoldersBefore);
		UPjcSubsystem::DeleteFoldersEmpty(false, false);
		for (const auto& Folder : FoldersBefore) {
			NumFoldersRemoved += IFileManager::Get().DirectoryExists(*Folder) ? 0 : 1;
		}
	}
	Percent = 0.9f;

	SetStep(3, FString {});
	UPjcSubsystem::Scan(false);
	Percent = 1.0f;

	// what was not deleted, and what lost a reference
	NotDeleted.Reset();
	for (const auto& Pair : UPjcSubsystem::GetLastCleanupKept()) {
		NotDeleted.Add(FString::Printf(TEXT("%s \u00B7 %s"), *FPackageName::GetShortName(Pair.Key), *Pair.Value));
	}
	FPjcCleanPlan::GetBrokenReferencers(Deleted, Broken);
	for (const FName Referencer : Broken) {
		UE_LOG(LogProjectCleaner, Warning, TEXT("Clean project: %s references a deleted asset"), *Referencer.ToString());
	}

	Phase = EPhase::Done;
	Rebuild();
}

TSharedRef<SWidget> SPjcCleanWindow::MakeDone() {
	const TArray<TTuple<FString, FString, FString>> Numbers {
	  MakeTuple(PjcFormat::Count(NumDeleted), FString {TEXT("assets deleted")}, FString {TEXT("Unused assets removed from disk")}),
	  MakeTuple(PjcFormat::Size(SizeFreed), FString {TEXT("freed")}, FString {TEXT("Size on disk of the deleted assets")}),
	  MakeTuple(PjcFormat::Count(NumFoldersRemoved), FString {TEXT("folders removed")}, FString {TEXT("Empty folders deleted")}),
	  MakeTuple(PjcFormat::Count(NumRedirectorsRemoved), FString {TEXT("redirectors removed")}, FString {TEXT("Redirectors to deleted assets")}),
	};

	const TSharedRef<SVerticalBox> Notes = SNew(SVerticalBox);
	const auto AddNote = [&Notes](const FString& Text, const bool bWarn) {
		// clang-format off
		Notes->AddSlot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 8.0f})
		[
			MakeNote(Text, bWarn)
		];
		// clang-format on
	};
	if (bStopped) {
		const FString Left = PjcFormat::Count(NumLeft);
		AddNote(TEXT("Stopped after a batch: ") + Left + TEXT(" unused assets are left. Clean project again to continue."), true);
	}
	if (Skipped.Num() > 0) {
		TArray<FString> Names;
		for (int32 Index = 0; Index < Skipped.Num() && Index < 8; ++Index) {
			Names.Add(FPackageName::GetShortName(Skipped[Index]));
		}
		const FString SkippedText = FString::Printf(TEXT("%s skipped, used by now: "), *PjcFormat::Count(Skipped.Num()));
		AddNote(SkippedText + FString::Join(Names, TEXT(", ")), true);
	}
	if (NotDeleted.Num() > 0) {
		const FString Lines = FString::Join(TArray<FString> {NotDeleted.GetData(), FMath::Min(NotDeleted.Num(), 8)}, TEXT("\n"));
		const FString Held = FString::Printf(TEXT("%s failed to delete, each with what still holds it. "), *PjcFormat::Count(NotDeleted.Num()));
		const TCHAR* Next = TEXT("They stay in Assets. To delete one anyway, delete it in the Content Browser and choose Force Delete, ")
			TEXT("only when you are sure nothing uses it.\n");
		AddNote(Held + Next + Lines, true);
	}
	if (Broken.Num() > 0) {
		TArray<FString> Names;
		for (int32 Index = 0; Index < Broken.Num() && Index < 8; ++Index) {
			Names.Add(FPackageName::GetShortName(Broken[Index]));
		}
		const FString BrokenText = FString::Printf(TEXT("%s assets reference a deleted asset (Output Log): "), *PjcFormat::Count(Broken.Num()));
		AddNote(BrokenText + FString::Join(Names, TEXT(", ")), true);
	}
	else {
		AddNote(TEXT("References check: no asset references a deleted one."), false);
	}
	FString VcsName;
	FString VcsDetail;
	if (PjcCleanVersionControl(VcsName, VcsDetail)) {
		AddNote(TEXT("Version control (") + VcsName + TEXT("): the deleted files show as removed. Commit when the project still works."), false);
	}
	else {
		AddNote(TEXT("No version control: keep your backup until the project still works."), true);
	}
	AddNote(TEXT("Next: open the main maps and play them before you commit."), false);

	const bool bHeld = NotDeleted.Num() > 0;
	// clang-format off
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot().Padding(FMargin {16.0f, 12.0f, 16.0f, 8.0f})
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 12.0f})
				[
					MakeNumbers(Numbers, -1)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					Notes
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBorder).BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.DarkGroupBorder"))).Padding(FMargin {16.0f, 8.0f})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin {8.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
					.ButtonStyle(&PjcShim::GetButtonStyle427(false))
					.ForegroundColor(FSlateColor::UseForeground())
#endif
					.Visibility(bHeld ? EVisibility::Visible : EVisibility::Collapsed)
					.ContentPadding(FMargin {12.0f, 2.0f})
					.OnClicked_Lambda([this]() {
						Close();
						PjcTabs::ShowNotDeleted(PjcTabs::GetMain());
						return FReply::Handled();
					})
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("Show failed")))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin {8.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
					.ButtonStyle(&PjcShim::GetButtonStyle427(false))
					.ForegroundColor(FSlateColor::UseForeground())
#endif
					.ContentPadding(FMargin {12.0f, 2.0f})
					.OnClicked_Lambda([]() {
						FGlobalTabmanager::Get()->TryInvokeTab(FTabId {FName {TEXT("OutputLog")}});
						return FReply::Handled();
					})
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("Output Log")))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin {8.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
					.ForegroundColor(FSlateColor::UseForeground())
#endif
					.ButtonStyle(&PjcShim::GetPrimaryButtonStyle())
					.ContentPadding(FMargin {12.0f, 2.0f})
					.OnClicked_Lambda([this]() {
						Close();
						return FReply::Handled();
					})
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("Done")))
					]
				]
			]
		];
	// clang-format on
}

void SPjcCleanWindow::Close() {
	if (const TSharedPtr<SWindow> Pinned = Window.Pin()) {
		Pinned->RequestDestroyWindow();
	}
}
