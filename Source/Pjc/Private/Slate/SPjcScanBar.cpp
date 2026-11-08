// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcScanBar.h"
#include "Slate/SPjcCleanWindow.h"
#include "Slate/SPjcScanWindow.h"
#include "PjcFormat.h"
#include "PjcScanDraft.h"
#include "PjcStyles.h"
#include "PjcShim.h"
#include "PjcSubsystem.h"
// Engine Headers
#include "FileHelpers.h"
#include "Misc/PackageName.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void SPjcScanBar::Construct(const FArguments& InArgs) {
	// clang-format off
	ChildSlot
	[
		SNew(SBorder).BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.GroupBorder"))).Padding(FMargin {6.0f, 4.0f})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ContentPadding(FMargin {12.0f, 2.0f})
				.ToolTipText(FText::FromString(TEXT("Scans the project again")))
				.Visibility(this, &SPjcScanBar::GetScanVisibility)
				.OnClicked(this, &SPjcScanBar::OnScanClicked)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("Scan")))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonStyle(&PjcShim::GetPrimaryButtonStyle())
				.ContentPadding(FMargin {12.0f, 2.0f})
				.ToolTipText_Lambda([]() {
					const TCHAR* Settings = TEXT("Applies the changed scan settings and scans again");
					return FText::FromString(FPjcScanDraft::IsChanged() ? Settings : TEXT("Scans the changed project again"));
				})
				.Visibility(this, &SPjcScanBar::GetRescanVisibility)
				.OnClicked(this, &SPjcScanBar::OnRescanClicked)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("Rescan")))
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(FMargin {10.0f, 0.0f})
			[
				SNew(STextBlock)
				.Text(this, &SPjcScanBar::GetStatusText)
				.ColorAndOpacity(this, &SPjcScanBar::GetStatusColor)
				.ToolTipText(this, &SPjcScanBar::GetStatusToolTip)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ContentPadding(FMargin {12.0f, 2.0f})
				.ToolTipText(this, &SPjcScanBar::GetCleanToolTip)
				.IsEnabled(this, &SPjcScanBar::IsCleanEnabled)
				.OnClicked(this, &SPjcScanBar::OnCleanClicked)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("Clean project\u2026")))
				]
			]
		]
	];
	// clang-format on
}

SPjcScanBar::~SPjcScanBar() {}

bool SPjcScanBar::NeedsRescan() {
	return FPjcScanDraft::IsChanged() || (UPjcSubsystem::GetScanResult().IsValid() && UPjcSubsystem::GetChangesSinceScan().Num() > 0);
}

void SPjcScanBar::ScanProject() {
	if (UPjcSubsystem::GetModuleAssetRegistry().Get().IsLoadingAssets()) {
		const TCHAR* Discovering = TEXT("Scan waits for the Asset Registry: it is still discovering assets. Scan again when it has finished.");
		UPjcSubsystem::ShowNotificationWithOutputLog(Discovering, SNotificationItem::CS_Fail, 5.0f);
		return;
	}

	if (!FEditorFileUtils::SaveDirtyPackages(true, true, true, false, false, false)) {
		const TCHAR* Unsaved = TEXT("Scan needs every asset saved, the scan reads them from disk.");
		UPjcSubsystem::ShowNotificationWithOutputLog(Unsaved, SNotificationItem::CS_Fail, 5.0f);
		return;
	}

	const double StartTime = FPlatformTime::Seconds();
	if (!SPjcScanWindow::Run()) {
		UPjcSubsystem::ShowNotification(TEXT("Scan canceled \u00B7 results are from the last scan"), SNotificationItem::CS_None, 3.0f);
		return;
	}
	const FString Assets = PjcFormat::Count(UPjcSubsystem::GetScanResult().Assets.Num());
	const FString Message = FString::Printf(TEXT("Scanned %s assets in %.1f s"), *Assets, FPlatformTime::Seconds() - StartTime);
	UPjcSubsystem::ShowNotification(Message, SNotificationItem::CS_Success, 3.0f);
}

void SPjcScanBar::CleanProject() {
	SPjcCleanWindow::Open();
}

FReply SPjcScanBar::OnScanClicked() const {
	ScanProject();
	return FReply::Handled();
}

FReply SPjcScanBar::OnRescanClicked() const {
	if (FPjcScanDraft::IsChanged()) {
		FPjcScanDraft::Get().ApplyToSettings();
	}
	ScanProject();
	return FReply::Handled();
}

EVisibility SPjcScanBar::GetScanVisibility() const {
	return NeedsRescan() ? EVisibility::Collapsed : EVisibility::Visible;
}

EVisibility SPjcScanBar::GetRescanVisibility() const {
	return NeedsRescan() ? EVisibility::Visible : EVisibility::Collapsed;
}

FSlateColor SPjcScanBar::GetStatusColor() const {
	return NeedsRescan() ? FSlateColor {FPjcStyles::Get().GetColor("ProjectCleaner.Color.Yellow")} : FSlateColor::UseSubduedForeground();
}

FReply SPjcScanBar::OnCleanClicked() const {
	CleanProject();
	return FReply::Handled();
}

FText SPjcScanBar::GetStatusText() const {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	if (FPjcScanDraft::IsChanged()) {
		return FText::FromString(TEXT("Settings changed \u00B7 results are from the last scan"));
	}
	if (!Result.IsValid()) {
		return FText::FromString(TEXT("Not scanned yet"));
	}
	if (UPjcSubsystem::GetChangesSinceScan().Num() > 0) {
		return FText::FromString(TEXT("Project changed \u00B7 results are from the last scan"));
	}

	const FTimespan Age = FDateTime::UtcNow() - Result.Time;
	const int32 Minutes = FMath::FloorToInt(Age.GetTotalMinutes());
	const FString Ago = Minutes < 60 ? FString::Printf(TEXT("%d min ago"), Minutes) : FString::Printf(TEXT("%d h ago"), Minutes / 60);
	return FText::FromString(Minutes < 1 ? FString {TEXT("Scanned just now")} : TEXT("Scanned ") + Ago);
}

FText SPjcScanBar::GetStatusToolTip() const {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	if (!Result.IsValid()) {
		return FText::FromString(TEXT("Scan finds which assets are used, unused or to review"));
	}

	const FString When = FString::Printf(TEXT("Last scan %s, took %.1f s"), *Result.Time.ToString(TEXT("%Y-%m-%d %H:%M UTC")), Result.Seconds);
	const FPjcProjectChanges& Changes = UPjcSubsystem::GetChangesSinceScan();
	if (FPjcScanDraft::IsChanged() || Changes.Num() == 0) {
		return FText::FromString(When);
	}

	// what changed since, the newest first
	TArray<FString> Lines;
	const auto AddCount = [&Lines](const int32 Num, const TCHAR* What) {
		if (Num > 0) {
			Lines.Add(FString::Printf(TEXT("%s %s"), *PjcFormat::Count(Num), What));
		}
	};
	AddCount(Changes.NumAdded, TEXT("added"));
	AddCount(Changes.NumRemoved, TEXT("removed"));
	AddCount(Changes.NumRenamed, TEXT("renamed"));
	AddCount(Changes.NumSaved, TEXT("saved"));
	const FString Counts = FString::Join(Lines, TEXT(" \u00B7 "));
	TArray<FString> Names;
	for (int32 Index = Changes.Recent.Num() - 1; Index >= 0; --Index) {
		Names.Add(FPackageName::GetShortName(Changes.Recent[Index]));
	}
	return FText::FromString(When + TEXT("\nSince: ") + Counts + TEXT("\n") + FString::Join(Names, TEXT(", ")));
}

bool SPjcScanBar::IsCleanEnabled() const {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	return Result.IsValid() && !NeedsRescan() && Result.Unused.Num() > 0;
}

FText SPjcScanBar::GetCleanToolTip() const {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	if (!Result.IsValid()) {
		return FText::FromString(TEXT("Scan first"));
	}
	if (NeedsRescan()) {
		return FText::FromString(TEXT("Rescan first: results are from before the change"));
	}
	return FText::FromString(TEXT("Deletes every unused asset and empty folder in the project"));
}
