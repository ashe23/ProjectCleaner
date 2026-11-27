// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Slate/SPjcTable.h"
#include "Widgets/SCompoundWidget.h"

class SWindow;

// Part of a dialog: a heading, a note with a colored edge, a list of rows (name, value), and a table
struct FPjcDialogPart
{
	FString Heading;
	FString Note;
	// Style color of the note edge, e.g. ProjectCleaner.Color.Yellow
	FName NoteColor = TEXT("ProjectCleaner.Color.Line");
	TArray<TPair<FString, FString>> Rows;
	FPjcTable Table;
};

// Modal dialog of the tools: parts, then Cancel and a confirm button (or only OK)
class SPjcDialog final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcDialog) : _Width(0.0f) {}
	SLATE_ARGUMENT(TArray<FPjcDialogPart>, Parts)
	// Confirm button text; empty shows only OK
	SLATE_ARGUMENT(FString, ConfirmText)
	// Confirm button color: ProjectCleaner.Color.UnusedFill for deleting, Accent otherwise
	SLATE_ARGUMENT(FName, ConfirmColor)
	SLATE_ARGUMENT(TSharedPtr<SWindow>, Window)
	// Width of a dialog as high as its content, 0 fills the window
	SLATE_ARGUMENT(float, Width)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Shows the dialog modal, true when confirmed (or OK). With a table the window is as high as its content
	static bool Show(
		const FString& Title,
		const TArray<FPjcDialogPart>& Parts,
		const FString& ConfirmText,
		const FName ConfirmColor = TEXT("ProjectCleaner.Color.Accent"),
		const float Width = 560.0f
	);

	// Note that version control can restore what is deleted or resaved, or a warning that nothing can
	static FPjcDialogPart MakeVersionControlNote();

  private:

	TWeakPtr<SWindow> Window;
	bool bConfirmed = false;
};
