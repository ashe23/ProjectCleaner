// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcCookInfo.h"
#include "PjcOverview.h"
#include "Slate/PjcWidgets.h"
#include "Widgets/SCompoundWidget.h"

class FTabManager;
class SBox;
class SHorizontalBox;
struct FSlateColor;
class SToolTip;

// Overview tool: what the shared scan found for the whole project
class SPjcTabOverview final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcTabOverview) {}
	// Tabs the Overview links to (none in tests)
	SLATE_ARGUMENT(TSharedPtr<FTabManager>, TabManager)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SPjcTabOverview() override;

	// Name, explanation and color of a usage reason as the tools show it (Assets uses it for its Used list)
	static void GetReasonInfo(const EPjcUsageReason Reason, FString& OutName, FString& OutToolTip, FLinearColor& OutColor);

  private:

	void OnScanFinished();
	void Rebuild();
	void OpenTab(const FName TabId) const;
	// Shows the asset in Asset Usage
	void Inspect(const FName Package) const;
	// Shows the folder in Assets Unused
	void ShowFolder(const FString Path) const;

	TSharedRef<SWidget> MakeSummary();
	TSharedRef<SWidget> MakeIssues();
	TSharedRef<SWidget> MakeReasons() const;
	TSharedRef<SWidget> MakeClasses() const;
	TSharedRef<SWidget> MakeUnusedFolders();
	TSharedRef<SWidget> MakeLargestUnused();
	TSharedRef<SWidget> MakeShippedCard() const;

	// One number of a number card: label (with its color), count, the line under it, a link right of the label
	static TSharedRef<SWidget> MakeNumberColumn(
		const FString& Label,
		const TOptional<FLinearColor>& Swatch,
		const int32 Num,
		const FString& Sub,
		const FSlateColor& NumColor,
		const FString& ToolTip,
		const TSharedRef<SWidget>& Extra
	);
	// Adds a number to a number card, a line between numbers
	static void AddNumberColumn(const TSharedRef<SHorizontalBox>& Columns, const TSharedRef<SWidget>& Column);
	// Card with a title, a hint on the right and its rows
	static TSharedRef<SWidget> MakeSection(const FString& Title, const FString& Hint, const TSharedRef<SWidget>& Content);
	// Two cards side by side, as high as the higher one
	static TSharedRef<SWidget> MakeColumns(const TSharedRef<SWidget>& Left, const TSharedRef<SWidget>& Right);
	// Name and value on one line, a bar under them; a button when OnClick is bound
	static TSharedRef<SWidget> MakeRow(
		const TSharedRef<SWidget>& Name,
		const TSharedRef<SWidget>& Value,
		const TArray<PjcWidgets::FBarSegment>& Segments,
		const double BarTotal,
		const TSharedPtr<SToolTip>& ToolTip,
		const FSimpleDelegate& OnClick = FSimpleDelegate {}
	);
	// UE5 class names are paths (/Script/Engine.Texture2D): the name after the dot
	static FString GetClassShortName(const FName ClassName);

	FDelegateHandle ScanFinishedHandle;
	TWeakPtr<FTabManager> TabManager;
	TSharedPtr<SBox> Body;
	FPjcOverview Overview;

	// Last cook, without its package list
	FPjcCookInfo Cook;
	TOptional<FPjcOverview::FShipped> Shipped;
};
