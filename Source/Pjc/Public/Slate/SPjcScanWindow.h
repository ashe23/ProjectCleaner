// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcSubsystem.h"
#include "Widgets/SCompoundWidget.h"

class SWindow;

/**
 * Scan window: a centered modal window while the project is scanned. Steps with what each found, the item being read,
 * elapsed time and percent, Cancel (the last result stays). It closes by itself when the scan is done.
 */
class SPjcScanWindow final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcScanWindow) {}
	SLATE_ARGUMENT(TSharedPtr<SWindow>, Window)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Scans in the window, true when the scan finished (false when canceled or the registry was not ready)
	static bool Run();

  private:

	EActiveTimerReturnType StartScan(double CurrentTime, float DeltaTime);
	float GetPercent() const;
	static void PumpUi();

	TWeakPtr<SWindow> Window;
	FPjcScanProgress Progress;
	bool bScanned = false;
	bool bFinished = false;
	double StartTime = 0.0;
	int32 StepIndex = 0;
	FString CurrentText;
	TArray<FString> Found;
};
