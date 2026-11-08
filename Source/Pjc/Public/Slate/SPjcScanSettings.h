// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcScanDraft.h"
#include "PjcTypes.h"
#include "Widgets/SCompoundWidget.h"

class SBox;
class SToolTip;

/**
 * Scan settings panel shown by every tool: one line per setting (name, number from the last scan, control), what it
 * does in its tooltip. Edits the shared draft (FPjcScanDraft), Rescan in the scan bar applies it.
 */
class SPjcScanSettings final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcScanSettings) {}
	SLATE_EVENT(FSimpleDelegate, OnClose)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SPjcScanSettings() override;

  private:

	using FDraftFlag = bool FPjcScanDraft::*;
	using FDraftList = TArray<FString> FPjcScanDraft::*;

	void Rebuild();
	void RequestRebuild();
	void OnScanFinished();

	TSharedRef<SWidget> MakeHeader();
	TSharedRef<SWidget> MakeCategory(const FName Id, const FString& Title, const FString& ToolTip, const TSharedRef<SWidget>& Content) const;
	TSharedRef<SWidget> MakeRow(
		const FString& Label,
		const FString& Value,
		const TSharedRef<SWidget>& Control,
		const TSharedRef<SToolTip>& ToolTip,
		const bool bChanged,
		const bool bDim,
		const float Indent = 0.0f
	) const;
	TSharedRef<SWidget>
	MakeFixedRow(const FString& Label, const int32 Num, const FString& What, const FString& Description, const FString& Extra = FString {}) const;
	TSharedRef<SWidget> MakeSwitchRow(const FString& Label, FDraftFlag Flag, const int32 Num, const FString& Description, const FString& More);
	TSharedRef<SWidget> MakeListRows(
		const FName Id,
		const FString& Label,
		FDraftList List,
		const FString& Placeholder,
		const FString& Description,
		const float Indent = 0.0f
	);

	FSimpleDelegate OnClose;
	TSharedPtr<SBox> Body;
	FDelegateHandle ScanFinishedHandle;
	FDelegateHandle DraftEditedHandle;

	bool bRebuildPending = false;

	// List whose text field is open
	FName AddingList;

	// numbers from the last scan
	TMap<EPjcUsageReason, int32> NumRoots;
	int32 NumLocalized = 0;
	int32 NumFolderLabelOnly = 0;
	int32 NumFolderLabelUnused = 0;
	int64 SizeFolderLabelUnused = 0;
	int32 NumProjectPlugins = 0;
};
