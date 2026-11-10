// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SVerticalBox;

// One class in the bar
struct FPjcClassBarItem
{
	FName ClassName;
	FString Label;
	FLinearColor Color = FLinearColor::Gray;
	int32 Num = 0;
	FString ToolTip;
};

/**
 * Class filter of a list as a bar of toggles next to it: every class with its color and count. A click turns one class
 * on or off, no modifier key. A class in its color is listed, a grey one is not; with nothing picked every class is
 * listed. "All classes" on top: a click turns every class on, the next one turns every class off.
 */
class SPjcClassBar final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcClassBar) {}
	SLATE_EVENT(FSimpleDelegate, OnChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Classes of the list, in the order to show. Picks of classes that are gone are dropped
	void SetItems(const TArray<FPjcClassBarItem>& InItems);
	// Whether assets of the class are listed
	bool IsShown(const FName ClassName) const;
	// Every class on again, without telling the owner
	void Reset();
	// A click on a class, or on "All classes" for None
	void Toggle(const FName ClassName);

	bool IsAllOn() const {
		return !bNone && Picked.Num() == 0;
	}
	bool IsNone() const {
		return bNone;
	}
	const TSet<FName>& GetPicked() const {
		return Picked;
	}

  private:

	TSharedRef<SWidget> MakeRow(const FPjcClassBarItem& Item);

	FSimpleDelegate OnChanged;
	TArray<FPjcClassBarItem> Items;
	TSharedPtr<SVerticalBox> Rows;
	// Classes turned on; empty with bNone false: every class is on
	TSet<FName> Picked;
	// Every class turned off
	bool bNone = false;
};
