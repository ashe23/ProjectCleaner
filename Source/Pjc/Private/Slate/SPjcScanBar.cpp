// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcScanBar.h"
#include "PjcFormat.h"
#include "PjcShim.h"
#include "PjcSubsystem.h"
// Engine Headers
#include "FileHelpers.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void SPjcScanBar::Construct(const FArguments& InArgs) {
	ScanFinishedHandle = UPjcSubsystem::OnScanFinished().AddSP(this, &SPjcScanBar::OnScanFinished);
	OnScanFinished();

	// clang-format off
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.GroupBorder")))
		.Padding(FMargin {6.0f, 4.0f})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ContentPadding(FMargin {12.0f, 2.0f})
				.ToolTipText(FText::FromString(TEXT("Scans the project again")))
				.OnClicked(this, &SPjcScanBar::OnScanClicked)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("Scan")))
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			.Padding(FMargin {10.0f, 0.0f})
			[
				SNew(STextBlock)
				.Text(this, &SPjcScanBar::GetStatusText)
				.ToolTipText(this, &SPjcScanBar::GetStatusToolTip)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ContentPadding(FMargin {12.0f, 2.0f})
				.ToolTipText(FText::FromString(TEXT("Deletes every unused asset and empty folder in the project")))
				.IsEnabled(this, &SPjcScanBar::IsCleanEnabled)
				.OnClicked(this, &SPjcScanBar::OnCleanClicked)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("Clean project…")))
				]
			]
		]
	];
	// clang-format on
}

SPjcScanBar::~SPjcScanBar() {
	UPjcSubsystem::OnScanFinished().Remove(ScanFinishedHandle);
}

void SPjcScanBar::ScanProject() {
	if (UPjcSubsystem::GetModuleAssetRegistry().Get().IsLoadingAssets()) {
		UPjcSubsystem::ShowNotificationWithOutputLog(
			TEXT("Scan waits for the Asset Registry: it is still discovering assets. Scan again when it has finished."),
			SNotificationItem::CS_Fail,
			5.0f
		);
		return;
	}

	if (!FEditorFileUtils::SaveDirtyPackages(true, true, true, false, false, false)) {
		UPjcSubsystem::ShowNotificationWithOutputLog(
			TEXT("Scan needs every asset saved, the scan reads them from disk."), SNotificationItem::CS_Fail, 5.0f
		);
		return;
	}

	UPjcSubsystem::Scan(true);
}

void SPjcScanBar::CleanProject() {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	int64 UnusedBytes = 0;
	for (const auto& Asset : Result.Unused) {
		UnusedBytes += Result.GetSize(Asset.PackageName);
	}
	TArray<FString> FoldersEmpty;
	UPjcSubsystem::GetFoldersEmpty(FoldersEmpty);

	const FText Title = FText::FromString(TEXT("Clean Project"));
	const FText Message = FText::FromString(
		FString::Printf(
			TEXT("Delete %s unused asset(s) (%s) and %s empty folder(s)?\n\n")
				TEXT("Assets something still holds in memory are kept and listed in Output Log.\n\n%s"),
			*PjcFormat::Count(Result.Unused.Num()),
			*PjcFormat::Size(UnusedBytes),
			*PjcFormat::Count(FoldersEmpty.Num()),
			*UPjcSubsystem::GetDeleteWarning()
		)
	);

	const EAppReturnType::Type ReturnType = PjcShim::ShowDialog(Title, Message, EAppMsgType::YesNo);
	if (ReturnType != EAppReturnType::Yes) return;

	UPjcSubsystem::DeleteAssetsUnused(true, true);
	UPjcSubsystem::DeleteFoldersEmpty(true, true);

	ScanProject();
}

FReply SPjcScanBar::OnScanClicked() const {
	ScanProject();
	return FReply::Handled();
}

FReply SPjcScanBar::OnCleanClicked() const {
	CleanProject();
	return FReply::Handled();
}

FText SPjcScanBar::GetStatusText() const {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	if (!Result.IsValid()) {
		return FText::FromString(TEXT("Not scanned yet"));
	}

	const FTimespan Age = FDateTime::UtcNow() - Result.Time;
	const FString Scanned = Age.GetTotalMinutes() < 1.0 ? TEXT("scanned just now")
		: Age.GetTotalHours() < 1.0						? FString::Printf(TEXT("scanned %d min ago"), FMath::FloorToInt(Age.GetTotalMinutes()))
														: FString::Printf(TEXT("scanned %d h ago"), FMath::FloorToInt(Age.GetTotalHours()));

	if (NumUnused == 0) {
		return FText::FromString(FString::Printf(TEXT("No unused assets · %s assets · %s"), *PjcFormat::Count(NumAssets), *Scanned));
	}

	return FText::FromString(
		FString::Printf(
			TEXT("%s unused · %s · of %s assets · %s"),
			*PjcFormat::Count(NumUnused),
			*PjcFormat::Size(SizeUnused),
			*PjcFormat::Count(NumAssets),
			*Scanned
		)
	);
}

FText SPjcScanBar::GetStatusToolTip() const {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	if (!Result.IsValid()) {
		return FText::FromString(TEXT("Press Scan to find unused assets"));
	}

	return FText::FromString(
		FString::Printf(TEXT("Last scan: %s, took %.1f s"), *Result.Time.ToString(TEXT("%Y-%m-%d %H:%M:%S UTC")), Result.Seconds)
	);
}

bool SPjcScanBar::IsCleanEnabled() const {
	return NumUnused > 0;
}

void SPjcScanBar::OnScanFinished() {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	NumAssets = Result.Assets.Num();
	NumUnused = Result.Unused.Num();
	SizeUnused = 0;
	for (const auto& Asset : Result.Unused) {
		SizeUnused += Result.GetSize(Asset.PackageName);
	}
}
