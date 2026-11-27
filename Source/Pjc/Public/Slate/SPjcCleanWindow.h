// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "PjcClean.h"
#include "Widgets/SCompoundWidget.h"

class SBox;
class SWindow;

// What Clean project did, for its result step
struct FPjcCleanResult
{
	int32 NumDeleted = 0;
	int64 SizeFreed = 0;
	int32 NumFoldersRemoved = 0;
	int32 NumRedirectorsRemoved = 0;
	// stopped after a batch, with the unused assets left
	bool bStopped = false;
	int32 NumLeft = 0;
	int32 NumForced = 0;
	// force deleted from disk, the editor still holds them
	int32 NumInMemory = 0;
	// name and what holds it
	TArray<FString> NotDeleted;
	// name and why revision control keeps it
	TArray<FString> Blocked;
	// used by now, not deleted
	TArray<FName> Skipped;
	// still on disk, they reference a deleted asset
	TArray<FName> Broken;
};

/**
 * Clean project window, three steps in one modal window: what gets deleted with the checks before it, the progress
 * batch by batch (Stop ends after the running batch), and the result with what was not deleted and why.
 */
class SPjcCleanWindow final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcCleanWindow) {}
	SLATE_ARGUMENT(TSharedPtr<SWindow>, Window)
	// Opens on the result step with this result
	SLATE_ARGUMENT(TOptional<FPjcCleanResult>, Result)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Shows the window as a modal window over the active one
	// @param bLastResult opens on the result of the last Clean project of this editor session, when there is one
	static void Open(const bool bLastResult = false);
	// Numbers of the last Clean project of this editor session
	// @return false when there was none
	static bool GetLastResult(int32& OutDeleted, int32& OutNotDeleted);

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
	// Numbers (value, label, tooltip): the first two large, the first in red, the other two as lines on the right
	static TSharedRef<SWidget> MakeNumbers(const TArray<TTuple<FString, FString, FString>>& Numbers);
	// Folders or classes the delete frees space in: name, share, size, assets
	static TSharedRef<SWidget>
	MakeTotals(const FString& Title, const TArray<TPair<FString, FPjcOverview::FTotal>>& Totals, const int64 Size, const bool bFolders);
	static TSharedRef<SWidget> MakeNote(const FString& Text, const bool bWarn);

	// what stops the delete in the confirm step, empty when nothing does
	FString GetBlockReason() const;
	void ScanAgain();
	void Run();
	void SetStep(const int32 Step, const FString& Current);
	static void PumpUi();
	void Close();
	void FillNotDeleted();

	TWeakPtr<SWindow> Window;
	TSharedPtr<SBox> Body;
	EPhase Phase = EPhase::Confirm;
	FPjcCleanPlan Plan;
	TMap<FName, int64> Sizes;
	bool bBackup = false;
	// a modal window blocks the editor, so the log opens after it closes
	bool bOpenLogOnClose = false;

	// run
	TArray<FString> Steps;
	int32 StepIndex = 0;
	float Percent = 0.0f;
	FString CurrentText;
	bool bStopRequested = false;

	FPjcCleanResult CleanResult;
};
