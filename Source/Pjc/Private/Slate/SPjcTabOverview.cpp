// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabOverview.h"
#include "Slate/SPjcScanBar.h"
#include "PjcFormat.h"
#include "PjcSettings.h"
#include "PjcSubsystem.h"
// Engine Headers
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void SPjcTabOverview::Construct(const FArguments& InArgs) {
	ScanFinishedHandle = UPjcSubsystem::OnScanFinished().AddSP(this, &SPjcTabOverview::OnScanFinished);
	OnScanFinished();

	// clang-format off
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SPjcScanBar)
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(FMargin {14.0f})
		[
			SNew(STextBlock).Text(this, &SPjcTabOverview::GetSummaryText)
		]
	];
	// clang-format on

	if (!UPjcSubsystem::GetScanResult().IsValid() && GetDefault<UPjcEditorSettings>()->bScanOnOpen) {
		SPjcScanBar::ScanProject();
	}
}

SPjcTabOverview::~SPjcTabOverview() {
	UPjcSubsystem::OnScanFinished().Remove(ScanFinishedHandle);
}

void SPjcTabOverview::OnScanFinished() {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	NumUsed = 0;
	NumEditorOnly = 0;
	NumUnused = 0;
	for (const auto& Pair : Result.Status) {
		NumUsed += Pair.Value == EPjcAssetStatus::Used ? 1 : 0;
		NumEditorOnly += Pair.Value == EPjcAssetStatus::EditorOnly ? 1 : 0;
		NumUnused += Pair.Value == EPjcAssetStatus::Unused ? 1 : 0;
	}
}

FText SPjcTabOverview::GetSummaryText() const {
	if (!UPjcSubsystem::GetScanResult().IsValid()) return FText::GetEmpty();

	return FText::FromString(
		FString::Printf(
			TEXT("Used %s · Editor only %s · Unused %s"), *PjcFormat::Count(NumUsed), *PjcFormat::Count(NumEditorOnly), *PjcFormat::Count(NumUnused)
		)
	);
}
