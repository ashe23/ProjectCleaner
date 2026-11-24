// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcCleanWindow.h"
#include "Slate/SPjcTabIssues.h"
#include "Slate/PjcTabs.h"
#include "Slate/PjcWidgets.h"
#include "Slate/SPjcScanBar.h"
#include "Slate/SPjcTable.h"
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
#include "Slate/PjcActions.h"
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
	// Version control of the project: provider name and what it means for deleted files, empty when there is none
	bool PjcCleanVersionControl(FString& OutName, FString& OutDetail) {
		ISourceControlModule& SourceControl = ISourceControlModule::Get();
		if (SourceControl.IsEnabled() && SourceControl.GetProvider().IsAvailable()) {
			OutName = SourceControl.GetProvider().GetName().ToString();
			const bool bGit = OutName.Contains(TEXT("Git"));
			OutDetail = bGit ? TEXT("Deleted files can be restored from it")
							 : TEXT("Files are checked out and marked for delete. Submit when the project still works");
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
	if (InArgs._Result.IsSet()) {
		CleanResult = InArgs._Result.GetValue();
		Phase = EPhase::Done;
	}

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
	PjcWidgets::ShowModal(CleanWindow);
	PjcTabs::SetWindowOpen(false);

	if (Content->bOpenLogOnClose) {
		FGlobalTabmanager::Get()->TryInvokeTab(FTabId {FName {TEXT("OutputLog")}});
	}
}

void SPjcCleanWindow::Rebuild() {
	const TSharedRef<SWidget> Content = Phase == EPhase::Confirm ? MakeConfirm() : (Phase == EPhase::Run ? MakeRun() : MakeDone());
	Body->SetContent(Content);
}

TSharedRef<SWidget> SPjcCleanWindow::MakeNumbers(const TArray<TTuple<FString, FString, FString>>& Numbers) {
	TArray<PjcWidgets::FStat> Stats;
	for (const auto& Number : Numbers) {
		PjcWidgets::FStat& Stat = Stats.AddDefaulted_GetRef();
		Stat.Value = Number.Get<0>();
		Stat.Label = Number.Get<1>();
		Stat.ToolTip = Number.Get<2>();
	}
	// what the delete removes, in red
	if (Stats.Num() > 0) Stats[0].Color = TEXT("ProjectCleaner.Color.Red");
	return PjcWidgets::MakeStats(Stats);
}

TSharedRef<SWidget>
SPjcCleanWindow::MakeTotals(const FString& Title, const TArray<TPair<FString, FPjcOverview::FTotal>>& Totals, const int64 Size, const bool bFolders) {
	const auto Column = [](const TCHAR* Label, const float FixedWidth, const bool bNumber, const bool bBar) {
		FPjcTableColumn Out;
		Out.Label = Label;
		Out.FixedWidth = FixedWidth;
		Out.bNumber = bNumber;
		Out.bBar = bBar;
		return Out;
	};
	FPjcTable Table;
	Table.Columns = {
	  Column(TEXT("Name"), 0.0f, false, false),
	  Column(TEXT(""), 44.0f, false, true),
	  Column(TEXT("Size"), 72.0f, true, false),
	  Column(TEXT("Assets"), 56.0f, true, false),
	};
	Table.NumVisible = 6;
	Table.bFixedHeight = true;
	for (const auto& Total : Totals) {
		FPjcTableRow& Row = Table.Rows.AddDefaulted_GetRef();
		FString Name = Total.Key;
		if (bFolders) {
			// Game/ muted, then the folder in it
			Name.RemoveFromStart(TEXT("/"));
			int32 Slash = INDEX_NONE;
			if (Name.FindChar(TEXT('/'), Slash)) {
				Row.Prefix = Name.Left(Slash + 1);
				Name.RightChopInline(Slash + 1);
			}
		}
		Row.Cells = {Name, FString {}, PjcFormat::Size(Total.Value.Size), PjcFormat::Count(Total.Value.Num)};
		Row.Share = Size > 0 ? static_cast<float>(static_cast<double>(Total.Value.Size) / static_cast<double>(Size)) : 0.0f;
		Row.ToolTipTitle = Total.Key;
		Row.ToolTip = FString::Printf(
			TEXT("%s \u00B7 %s \u00B7 %s of space freed"),
			*FPjcTable::Plural(Total.Value.Num, TEXT("asset")),
			*PjcFormat::Size(Total.Value.Size),
			*PjcFormat::Percent(Total.Value.Size, Size)
		);
	}

	// clang-format off
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 5.0f})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				PjcWidgets::MakeCardTitle(Title)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock)
				.Text(FText::FromString(PjcFormat::Count(Totals.Num())))
				.Font(PjcWidgets::GetFontMono(8))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SPjcTable).Table(Table)
		];
	// clang-format on
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
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SBox)
					.ToolTip(Detail.IsEmpty() ? TSharedPtr<SToolTip> {} : TSharedPtr<SToolTip> {PjcWidgets::MakeToolTip(Title, Detail)})
					[
						SNew(SPjcFitText)
						.Text(DetailText)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
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

	if (UPjcSubsystem::GetModuleAssetRegistry().Get().IsLoadingAssets()) return TEXT("Wait for Asset Registry");
	if (SPjcScanBar::NeedsRescan()) return TEXT("Rescan first. Results are from before the change");
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
	  Number(PjcFormat::Count(Plan.Assets.Num()), TEXT("Unused assets"), TEXT("Unused assets from last scan. No used asset references them")),
	  Number(PjcFormat::Size(Plan.Size), TEXT("Space freed"), TEXT("Size on disk of deleted assets")),
	  Number(PjcFormat::Count(Plan.NumFoldersEmpty), TEXT("Empty folders"), TEXT("Folders that are empty now or after the delete")),
	  Number(
		  PjcFormat::Count(Plan.NumRedirectors), TEXT("Redirectors"), TEXT("Redirectors to deleted assets that nothing uses. They are deleted too")
	  ),
	};

	// where and what: every folder and class
	TArray<TPair<FString, FPjcOverview::FTotal>> Classes;
	for (const auto& Class : Plan.Classes) {
		FString ClassName = Class.Key.ToString();
		int32 Dot = INDEX_NONE;
		if (ClassName.FindLastChar(TEXT('.'), Dot)) {
			ClassName.RightChopInline(Dot + 1);
		}
		Classes.Emplace(ClassName, Class.Value);
	}

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
		PjcTabs::ShowIssues(PjcTabs::GetMain(), static_cast<uint8>(EPjcIssueCategory::Review));
	};
	if (SPjcScanBar::NeedsRescan()) {
		const bool bSettings = FPjcScanDraft::IsChanged();
		const TCHAR* What = bSettings ? TEXT("Scan settings changed since last scan") : TEXT("Project changed since last scan");
		const FString Count = PjcFormat::Count(UPjcSubsystem::GetChangesSinceScan().Num()) + TEXT(" changes \u00B7 these numbers are old");
		const FString Detail = bSettings ? TEXT("these numbers are old") : Count;
		AddCheck(MakeCheck(ECheck::Warn, What, Detail, MakeButton(TEXT("Rescan"), OnScanAgain)));
	}
	if (Plan.NumReview > 0) {
		const FString ReviewTitle = FString::Printf(TEXT("%s assets to review"), *PjcFormat::Count(Plan.NumReview));
		const TCHAR* ReviewDetail = TEXT("Kept because scan is not sure about them");
		AddCheck(MakeCheck(ECheck::Info, ReviewTitle, ReviewDetail, MakeButton(TEXT("Review"), OnReview)));
	}
	TArray<UPackage*> Dirty;
	FEditorFileUtils::GetDirtyContentPackages(Dirty);
	if (Dirty.Num() > 0) {
		const FString DirtyTitle = FString::Printf(TEXT("%s unsaved assets"), *PjcFormat::Count(Dirty.Num()));
		const TCHAR* DirtyDetail = TEXT("Scan reads assets from disk. Save them and scan again");
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
		AddCheck(MakeCheck(ECheck::None, TEXT("Cook check"), TEXT("Last cook includes almost all assets \u00B7 not compared"), NoExtra));
	}
	else if (Cook.NumPackaged > 0) {
		const FString Packaged = PjcFormat::Count(Cook.NumPackaged) + TEXT(" unused assets are in last cook. Clean project keeps them in Review");
		AddCheck(MakeCheck(ECheck::Ok, TEXT("Cook check"), Packaged, NoExtra));
	}
	else {
		const FString NotChecked
			= FString::Printf(TEXT(" \u00B7 %s changed after it. Cook again to check them"), *PjcFormat::Count(Cook.NumNotChecked));
		const FString NoneCooked = TEXT("Last cook includes none of them") + (Cook.NumNotChecked > 0 ? NotChecked : FString {});
		AddCheck(MakeCheck(ECheck::Ok, TEXT("Cook check"), NoneCooked, NoExtra));
	}
	if (UPjcSubsystem::GetModuleAssetRegistry().Get().IsLoadingAssets()) {
		AddCheck(MakeCheck(ECheck::Warn, TEXT("Asset Registry"), TEXT("Still discovering assets"), NoExtra));
	}
	else {
		AddCheck(MakeCheck(ECheck::Ok, TEXT("Asset Registry ready"), TEXT("All assets were found before scan"), NoExtra));
	}
	{
		// clang-format off
		const TSharedRef<SWidget> Force = SNew(SCheckBox)
			.Style(&PjcShim::GetCheckBoxStyle())
			.ToolTip(PjcWidgets::MakeToolTip(TEXT("Force delete"), PjcActions::GetForceDeleteWarning()))
			.IsChecked_Lambda([this]() { return bForceDelete ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this](const ECheckBoxState State) { bForceDelete = State == ECheckBoxState::Checked; })
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Force delete")))
			];
		// clang-format on
		const TCHAR* ForceDetail = TEXT("Something in memory holds them \u00B7 the engine's Force Delete \u00B7 cannot be undone");
		AddCheck(MakeCheck(ECheck::Info, TEXT("Assets that fail to delete"), ForceDetail, Force));
	}
	const TCHAR* EditorsDetail = TEXT("All open asset editors close before the delete");
	AddCheck(MakeCheck(ECheck::Info, TEXT("Asset editors close first"), EditorsDetail, NoExtra));
	// redirectors never block Clean: the scan follows them, so nothing used is deleted through one
	if (Plan.NumRedirectorsLeft > 0) {
		const auto OnRedirectors = [this]() {
			Close();
			PjcTabs::ShowIssues(PjcTabs::GetMain(), static_cast<uint8>(EPjcIssueCategory::Redirectors));
		};
		const FString StayTitle = FString::Printf(TEXT("%s redirectors stay"), *PjcFormat::Count(Plan.NumRedirectorsLeft));
		const TCHAR* StayDetail = TEXT("Nothing used is deleted through them. Fix them in Issues when your team is ready");
		AddCheck(MakeCheck(ECheck::Info, StayTitle, StayDetail, MakeButton(TEXT("Issues"), OnRedirectors)));
	}

	const FString Kept = TEXT("Kept: ") + PjcFormat::Count(Plan.NumExcluded) + TEXT(" excluded \u00B7 all used, editor only and Review assets");
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
					MakeNumbers(Numbers)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 12.0f})
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(FMargin {0.0f, 0.0f, 14.0f, 0.0f})
					[
						MakeTotals(TEXT("Folders"), Plan.Folders, Plan.Size, true)
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[
						MakeTotals(TEXT("Classes"), Classes, Plan.Size, false)
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
						return FText::FromString(Reason.IsEmpty() ? TEXT("Each asset is checked again before it is deleted") : Reason);
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
		const auto IsDone = [this, Index]() {
			return Index < StepIndex;
		};
		const auto IsRunning = [this, Index]() {
			return Index == StepIndex;
		};
		// clang-format off
		StepList->AddSlot().AutoHeight().Padding(FMargin {0.0f, 2.0f})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 8.0f, 0.0f})
			[
				PjcWidgets::MakeStepMark(IsDone, IsRunning)
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
				MakeNote(TEXT("The editor deletes in batches. If something still holds an asset, it is kept and listed at the end."), false)
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
					.ToolTip(PjcWidgets::MakeToolTip(FText::FromString(TEXT("Stops after the current batch. Deleted assets stay deleted."))))
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
	  TEXT("Checking that assets are still unused"),
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
	CleanResult.Skipped.Reset();
	for (const auto& Asset : Plan.Assets) {
		if (UnusedPackages.Contains(Asset.PackageName)) {
			ToDelete.Add(Asset);
		}
		else {
			CleanResult.Skipped.Add(Asset.PackageName);
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
	CleanResult.bStopped = Progress.bStopped;
	CleanResult.NumForced = 0;
	CleanResult.NumInMemory = 0;
	if (bForceDelete && !CleanResult.bStopped) {
		TArray<FName> Held;
		UPjcSubsystem::GetLastCleanupKept().GetKeys(Held);
		if (Held.Num() > 0) {
			SetStep(1, FString::Printf(TEXT("Force deleting %s"), *PjcFormat::Count(Held.Num())));
			CleanResult.NumForced = PjcActions::ForceDelete(Held, false, false, &CleanResult.NumInMemory);
		}
	}
	Percent = 0.85f;

	TArray<FName> Deleted;
	CleanResult.NumDeleted = 0;
	CleanResult.SizeFreed = 0;
	for (const auto& Asset : ToDelete) {
		if (FPackageName::DoesPackageExist(Asset.PackageName.ToString())) continue;

		Deleted.Add(Asset.PackageName);
		++CleanResult.NumDeleted;
		CleanResult.SizeFreed += Sizes.FindRef(Asset.PackageName);
	}
	CleanResult.NumLeft = CleanResult.bStopped ? ToDelete.Num() - Progress.NumTried : 0;
	CleanResult.NumRedirectorsRemoved = 0;
	for (const auto& Redirector : RedirectorsBefore) {
		CleanResult.NumRedirectorsRemoved += FPackageName::DoesPackageExist(Redirector) ? 0 : 1;
	}

	// empty folders, not after a stop: the rest is still there
	CleanResult.NumFoldersRemoved = 0;
	if (!CleanResult.bStopped) {
		SetStep(2, FString {});
		TArray<FString> FoldersBefore;
		UPjcSubsystem::GetFoldersEmpty(FoldersBefore);
		UPjcSubsystem::DeleteFoldersEmpty(false, false);
		for (const auto& Folder : FoldersBefore) {
			CleanResult.NumFoldersRemoved += IFileManager::Get().DirectoryExists(*Folder) ? 0 : 1;
		}
	}
	Percent = 0.9f;

	SetStep(3, FString {});
	UPjcSubsystem::Scan(false);
	Percent = 1.0f;

	// what was not deleted, and what lost a reference
	CleanResult.NotDeleted.Reset();
	for (const auto& Pair : UPjcSubsystem::GetLastCleanupKept()) {
		CleanResult.NotDeleted.Add(FString::Printf(TEXT("%s \u00B7 %s"), *FPackageName::GetShortName(Pair.Key), *Pair.Value));
	}
	FPjcCleanPlan::GetBrokenReferencers(Deleted, CleanResult.Broken);
	for (const FName Referencer : CleanResult.Broken) {
		UE_LOG(LogProjectCleaner, Warning, TEXT("Clean project: %s references a deleted asset"), *Referencer.ToString());
	}

	Phase = EPhase::Done;
	Rebuild();
}

TSharedRef<SWidget> SPjcCleanWindow::MakeDone() {
	const TArray<TTuple<FString, FString, FString>> Numbers {
	  MakeTuple(PjcFormat::Count(CleanResult.NumDeleted), FString {TEXT("Assets deleted")}, FString {TEXT("Unused assets removed from disk")}),
	  MakeTuple(PjcFormat::Size(CleanResult.SizeFreed), FString {TEXT("Space freed")}, FString {TEXT("Size on disk of deleted assets")}),
	  MakeTuple(PjcFormat::Count(CleanResult.NumFoldersRemoved), FString {TEXT("Folders removed")}, FString {TEXT("Empty folders deleted")}),
	  MakeTuple(
		  PjcFormat::Count(CleanResult.NumRedirectorsRemoved), FString {TEXT("Redirectors removed")}, FString {TEXT("Redirectors to deleted assets")}
	  ),
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
	if (CleanResult.bStopped) {
		const FString Left = PjcFormat::Count(CleanResult.NumLeft);
		AddNote(TEXT("Stopped after a batch. ") + Left + TEXT(" unused assets are left. Run Clean project again to continue."), true);
	}
	if (CleanResult.Skipped.Num() > 0) {
		TArray<FString> Names;
		for (int32 Index = 0; Index < CleanResult.Skipped.Num() && Index < 8; ++Index) {
			Names.Add(FPackageName::GetShortName(CleanResult.Skipped[Index]));
		}
		const FString SkippedText = FString::Printf(TEXT("%s skipped because they are used now: "), *PjcFormat::Count(CleanResult.Skipped.Num()));
		AddNote(SkippedText + FString::Join(Names, TEXT(", ")), true);
	}
	if (CleanResult.NumForced > 0) {
		AddNote(FString::Printf(TEXT("%s force deleted."), *PjcFormat::Count(CleanResult.NumForced)), false);
	}
	if (CleanResult.NumInMemory > 0) {
		const FString InMemory
			= FString::Printf(TEXT("%s deleted from disk, but the editor still holds them. "), *PjcFormat::Count(CleanResult.NumInMemory));
		AddNote(InMemory + TEXT("Restart the editor to remove them from the lists."), true);
	}
	if (CleanResult.NotDeleted.Num() > 0) {
		const FString Lines
			= FString::Join(TArray<FString> {CleanResult.NotDeleted.GetData(), FMath::Min(CleanResult.NotDeleted.Num(), 8)}, TEXT("\n"));
		const FString Held
			= FString::Printf(TEXT("%s failed to delete. Each line shows what still holds it. "), *PjcFormat::Count(CleanResult.NotDeleted.Num()));
		const TCHAR* Next = TEXT("They stay in Assets. Force delete removes them anyway. Use it only when you are sure nothing uses them.\n");
		AddNote(Held + Next + Lines, true);
	}
	if (CleanResult.Broken.Num() > 0) {
		TArray<FString> Names;
		for (int32 Index = 0; Index < CleanResult.Broken.Num() && Index < 8; ++Index) {
			Names.Add(FPackageName::GetShortName(CleanResult.Broken[Index]));
		}
		const FString BrokenText
			= FString::Printf(TEXT("%s assets reference a deleted asset (Output Log): "), *PjcFormat::Count(CleanResult.Broken.Num()));
		AddNote(BrokenText + FString::Join(Names, TEXT(", ")), true);
	}
	else {
		AddNote(TEXT("References check: no asset references a deleted one."), false);
	}
	FString VcsName;
	FString VcsDetail;
	if (PjcCleanVersionControl(VcsName, VcsDetail)) {
		AddNote(TEXT("Version control (") + VcsName + TEXT("): deleted files show as removed. Commit when the project still works."), false);
	}
	else {
		AddNote(TEXT("No version control. Keep your backup until you are sure the project works."), true);
	}
	AddNote(TEXT("Next: open the main maps and play them before you commit."), false);

	const bool bHeld = CleanResult.NotDeleted.Num() > 0;
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
					MakeNumbers(Numbers)
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
					.ButtonStyle(&FPjcStyles::Get().GetWidgetStyle<FButtonStyle>("ProjectCleaner.Button.DangerQuiet"))
#if ENGINE_MAJOR_VERSION < 5
					.ForegroundColor(FSlateColor::UseForeground())
#endif
					.Visibility(bHeld ? EVisibility::Visible : EVisibility::Collapsed)
					.ContentPadding(FMargin {12.0f, 2.0f})
					.ToolTip(PjcWidgets::MakeToolTip(TEXT("Force delete"), PjcActions::GetForceDeleteWarning()))
					.OnClicked_Lambda([this]() {
						TArray<FName> Held;
						UPjcSubsystem::GetLastCleanupKept().GetKeys(Held);
						int32 InMemory = 0;
						const int32 Gone = PjcActions::ForceDelete(Held, true, false, &InMemory);
						if (Gone == 0) return FReply::Handled();

						// the result again, with what is gone now
						CleanResult.NumForced += Gone;
						CleanResult.NumInMemory += InMemory;
						CleanResult.NumDeleted += Gone;
						UPjcSubsystem::Scan(false);
						CleanResult.NotDeleted.Reset();
						for (const auto& Pair : UPjcSubsystem::GetLastCleanupKept()) {
							CleanResult.NotDeleted.Add(FString::Printf(TEXT("%s \u00B7 %s"), *FPackageName::GetShortName(Pair.Key), *Pair.Value));
						}
						Rebuild();
						return FReply::Handled();
					})
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("Force delete\u2026"))).ColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.DangerText"))
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
					.ToolTipText(FText::FromString(TEXT("Closes this window and opens the Output Log.")))
					.OnClicked_Lambda([this]() {
						bOpenLogOnClose = true;
						Close();
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
