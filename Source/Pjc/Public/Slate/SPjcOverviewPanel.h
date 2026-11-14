// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcOverview.h"
#include "Slate/PjcWidgets.h"
#include "Widgets/SCompoundWidget.h"

class FTabManager;
class SBox;
class SToolTip;
struct FPjcCookInfo;
enum class EPjcAssetSet : uint8;

DECLARE_DELEGATE_TwoParams(FPjcOnShowSet, const EPjcAssetSet /* Set */, const FString& /* Reason */);
DECLARE_DELEGATE_RetVal_TwoParams(bool, FPjcIsSetShown, const EPjcAssetSet /* Set */, const FString& /* Reason */);

/**
 * What the scan found for the whole project, compact, for the side column of Assets: the assets by status (a row
 * opens its list), the issues found (none, no card) and what each platform's last cook shipped.
 */
class SPjcOverviewPanel final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcOverviewPanel) {}
	// Tabs the issues link to (none in tests)
	SLATE_ARGUMENT(TSharedPtr<FTabManager>, TabManager)
	// Shows a list of Assets, only the assets of one reason when given
	SLATE_EVENT(FPjcOnShowSet, OnShowSet)
	// Whether Assets shows that list now: its row is marked
	SLATE_EVENT(FPjcIsSetShown, IsSetShown)
	// Shows the assets the last delete could not remove
	SLATE_EVENT(FSimpleDelegate, OnShowDeleteFailed)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SPjcOverviewPanel() override;

	// Name, explanation and color of a usage reason as the tools show it (Assets uses it for its Used list)
	static void GetReasonInfo(const EPjcUsageReason Reason, FString& OutName, FString& OutToolTip, FLinearColor& OutColor);

  private:

	// One cooked platform: when, and what it shipped of the scan (unset when its asset registry could not be read)
	struct FCook
	{
		FString Platform;
		FString File;
		FDateTime Time;
		TOptional<FPjcOverview::FShipped> Shipped;
	};

	void OnScanFinished();
	void Rebuild();

	TSharedRef<SWidget> MakeStatus();
	// Why used: a small row per reason under Used, Dependencies and Other last
	void AddReasonRows(const TSharedRef<SVerticalBox>& Rows);
	// Empty when nothing was found
	TSharedRef<SWidget> MakeIssues();
	TSharedRef<SWidget> MakeShipped();
	TSharedRef<SWidget> MakeCookRow(const int32 Index);
	TSharedRef<SWidget> MakeCookNumbers(const FCook& Cook) const;

	// Card with a title (an optional icon before it) and a value on the right, its rows under them
	static TSharedRef<SWidget> MakeCard(
		const FString& Title,
		const TSharedRef<SWidget>& Value,
		const TSharedRef<SWidget>& Content,
		const FSlateBrush* Icon = nullptr,
		const TOptional<FLinearColor>& Accent = {}
	);
	// Row: swatch, name, number, size, share; a button when OnClick is bound, marked while IsOn is true. Size is SizeWidth
	// wide (0: as wide as its text)
	static TSharedRef<SWidget> MakeRow(
		const TOptional<FLinearColor>& Swatch,
		const FString& Name,
		const FString& Number,
		const FSlateColor& NumberColor,
		const FString& Size,
		const FString& Share,
		const TSharedPtr<SToolTip>& ToolTip,
		const FSimpleDelegate& OnClick = FSimpleDelegate {},
		const TAttribute<bool>& IsOn = false,
		const float SizeWidth = 64.0f
	);
	// "Nov 5, 22:18" in local time, "Nov 5, 2026, 22:18" with the year
	static FString FormatCookTime(const FDateTime& UtcTime, const bool bWithYear);
	// "9 days ago"
	static FString FormatAgo(const FDateTime& UtcTime);

	FDelegateHandle ScanFinishedHandle;
	TWeakPtr<FTabManager> TabManager;
	FPjcOnShowSet OnShowSet;
	FPjcIsSetShown IsSetShown;
	FSimpleDelegate OnShowDeleteFailed;

	TSharedPtr<SBox> Body;
	FPjcOverview Overview;
	// Newest first; the one shown below the list
	TArray<FCook> Cooks;
	int32 SelectedCook = 0;
};
