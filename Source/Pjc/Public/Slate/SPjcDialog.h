// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SWindow;

// Part of a dialog: a heading, a note with a colored edge, and a list of rows (name, value)
struct FPjcDialogPart
{
	FString Heading;
	FString Note;
	// Style color of the note edge, e.g. ProjectCleaner.Color.Yellow
	FName NoteColor = TEXT("ProjectCleaner.Color.Line");
	TArray<TPair<FString, FString>> Rows;
	// optional checkbox; CheckValue must outlive the dialog
	FString CheckLabel;
	bool* CheckValue = nullptr;
};

// Modal dialog of the tools: parts, then Cancel and a confirm button (or only OK)
class SPjcDialog final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcDialog) {}
	SLATE_ARGUMENT(TArray<FPjcDialogPart>, Parts)
	// Confirm button text; empty shows only OK
	SLATE_ARGUMENT(FString, ConfirmText)
	// Confirm button color: ProjectCleaner.Color.UnusedFill for deleting, Accent otherwise
	SLATE_ARGUMENT(FName, ConfirmColor)
	SLATE_ARGUMENT(TSharedPtr<SWindow>, Window)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Shows the dialog modal, true when confirmed (or OK)
	static bool Show(
		const FString& Title,
		const TArray<FPjcDialogPart>& Parts,
		const FString& ConfirmText,
		const FName ConfirmColor = TEXT("ProjectCleaner.Color.Accent")
	);

	// Note that version control can restore what is deleted or resaved, or a warning that nothing can
	static FPjcDialogPart MakeVersionControlNote();

  private:

	TWeakPtr<SWindow> Window;
	bool bConfirmed = false;
};
