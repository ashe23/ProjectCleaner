// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcClean.h"
#include "Widgets/SCompoundWidget.h"

class SBox;
class SWindow;

/**
 * Clean project window, three steps in one modal window: what gets deleted with the checks before it, the progress
 * batch by batch (Stop ends after the running batch), and the result with what was not deleted and why.
 */
class SPjcCleanWindow final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcCleanWindow) {}
	SLATE_ARGUMENT(TSharedPtr<SWindow>, Window)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Shows the window as a modal window over the active one
	static void Open();

  private:

	enum class EPhase : uint8
	{
		Confirm,
		Run,
		Done,
	};

	// One line of the checks: icon kind, title, detail, and a widget on the right (button, check box)
	enum class ECheck : uint8
	{
		Ok,
		Warn,
		Info,
		None,
	};

	void Rebuild();
	TSharedRef<SWidget> MakeConfirm();
	TSharedRef<SWidget> MakeRun();
	TSharedRef<SWidget> MakeDone();
	static TSharedRef<SWidget> MakeCheck(const ECheck Kind, const FString& Title, const FString& Detail, const TSharedRef<SWidget>& Extra);
	static TSharedRef<SWidget> MakeNumbers(const TArray<TTuple<FString, FString, FString>>& Numbers, const int32 Highlight);
	static TSharedRef<SWidget> MakeTopRows(const FString& Title, const TArray<TPair<FString, FString>>& Rows, const int32 NumMore);
	static TSharedRef<SWidget> MakeNote(const FString& Text, const bool bWarn);

	// what stops the delete in the confirm step, empty when nothing does
	FString GetBlockReason() const;
	void ScanAgain();
	void Run();
	void SetStep(const int32 Step, const FString& Current);
	static void PumpUi();
	void Close();

	TWeakPtr<SWindow> Window;
	TSharedPtr<SBox> Body;
	EPhase Phase = EPhase::Confirm;
	FPjcCleanPlan Plan;
	TMap<FName, int64> Sizes;
	bool bBackup = false;

	// run
	TArray<FString> Steps;
	int32 StepIndex = 0;
	float Percent = 0.0f;
	FString CurrentText;
	bool bStopRequested = false;

	// result
	int32 NumDeleted = 0;
	int64 SizeFreed = 0;
	int32 NumFoldersRemoved = 0;
	int32 NumRedirectorsRemoved = 0;
	int32 NumLeft = 0;
	bool bStopped = false;
	TArray<FString> NotDeleted;
	TArray<FName> Skipped;
	TArray<FName> Broken;
};
