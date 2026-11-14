// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SBox;
class SWindow;

// A line of a section: the name and where it leads, a line under it and what to do; on the right a note, a state pill and a
// button
struct FPjcFixRow
{
	FString Name;
	// After an arrow, red when that asset is missing
	FString Target;
	bool bTargetMissing = false;
	// A yellow dot after the name
	bool bUnsaved = false;
	FString Detail;
	// Color of the detail line, none: muted
	FName DetailColor;
	FString Hint;
	FString Side;
	FString Pill;
	FName PillColor;
	FString Button;
	TFunction<void()> OnButton;
	// The window closes first, then OnButton runs
	bool bButtonCloses = false;
};

struct FPjcFixSection
{
	FString Title;
	FName Color;
	FString Note;
	TArray<FPjcFixRow> Rows;
};

struct FPjcFixStat
{
	int32 Num = 0;
	FString Label;
	FName Color;
};

struct FPjcFixOption
{
	FString Label;
	FString Description;
	// Must outlive the window
	bool* Value = nullptr;
};

struct FPjcFixChoice
{
	FString Title;
	FString Description;
	bool bDanger = false;
	// Under the picked choice, e.g. an asset picker
	TSharedPtr<SWidget> Extra;
};

// What a window shows. It is built again after an option or a choice changes
struct FPjcFixView
{
	FString Title;
	FString Lead;
	TArray<FPjcFixStat> Stats;
	TArray<FPjcFixSection> Sections;
	TArray<FPjcFixOption> Options;
	TArray<FPjcFixChoice> Choices;
	// Index of the picked choice, must outlive the window
	int32* Choice = nullptr;
	FString WarningTitle;
	FString Warning;
	FString Tip;
	FString Confirm;
	bool bConfirmEnabled = true;
	bool bConfirmDanger = false;
	bool bCancel = true;
};

// Modal window of the redirector fixes (proto/issues, Redirector windows): a header with counts, sections of rows, options or
// choices, and a footer with the version control and the buttons
class SPjcFixWindow final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcFixWindow) {}
	SLATE_ARGUMENT(TFunction<FPjcFixView()>, Build)
	SLATE_ARGUMENT(TWeakPtr<SWindow>, Window)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	// Shows the window Build makes until it closes; true when confirmed. A row button that closes it runs after that
	static bool Show(const FString& WindowTitle, TFunction<FPjcFixView()> Build);
	// The open window is built again on its next tick, e.g. after an asset was picked in it
	static void RefreshOpen();

  private:

	void Rebuild();
	void Close(const bool bConfirm);
	TSharedRef<SWidget> MakeContent(const FPjcFixView& View);
	TSharedRef<SWidget> MakeSection(const FPjcFixSection& Section);
	TSharedRef<SWidget> MakeRow(const FPjcFixRow& Row);
	TSharedRef<SWidget> MakeFooter(const FPjcFixView& View);

	TFunction<FPjcFixView()> Build;
	TWeakPtr<SWindow> Window;
	TSharedPtr<SBox> Content;
	TFunction<void()> AfterClose;
	bool bConfirmed = false;
	bool bRebuild = false;
};
