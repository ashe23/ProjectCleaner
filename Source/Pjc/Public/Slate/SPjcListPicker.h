// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class SEditableTextBox;

// Something a Scan settings list can hold, with what it is
struct FPjcPickerItem
{
	// What the list stores: plugin name, class path, folder path, asset object path
	FString Value;
	// Shown name, the value when empty
	FString Label;
	// What it is, e.g. "120 assets", "Texture \u00B7 4.2 MiB"
	FString Meta;
	// Order before typing (biggest first)
	int64 Sort = 0;
	// Typed by the user, not found in the project
	bool bTyped = false;
};

DECLARE_DELEGATE_OneParam(FPjcOnPicked, const FString&);

/**
 * Picker of a Scan settings list: a search field and the matching items with what they are. Up and down move, Enter
 * adds, Esc closes; items already in the list are ticked. A value that is not found can be typed and added.
 */
class SPjcListPicker final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcListPicker) : _MaxBeforeTyping(0), _bAllowTyped(true), _bTypedIsPath(false) {}
	SLATE_ARGUMENT(TArray<FPjcPickerItem>, Items)
	// Values the list has
	SLATE_ARGUMENT(TArray<FString>, Added)
	SLATE_ARGUMENT(FString, Hint)
	// Search text to start with
	SLATE_ARGUMENT(FString, Query)
	// Items shown before typing, biggest first (0: every item)
	SLATE_ARGUMENT(int32, MaxBeforeTyping)
	SLATE_ARGUMENT(bool, bAllowTyped)
	// a typed value must start with / (folders, assets)
	SLATE_ARGUMENT(bool, bTypedIsPath)
	SLATE_EVENT(FPjcOnPicked, OnPicked)
	SLATE_EVENT(FSimpleDelegate, OnClose)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// The search field, to focus when the picker opens
	TSharedPtr<SEditableTextBox> GetSearch() const {
		return Search;
	}

	const FString& GetQuery() const {
		return Query;
	}

  private:

	using FItemPtr = TSharedPtr<FPjcPickerItem>;

	void Refresh();
	void Pick(const FItemPtr& Item);
	FReply OnSearchKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent);
	TSharedRef<ITableRow> OnGenerateRow(FItemPtr Item, const TSharedRef<STableViewBase>& Table);

	TArray<FPjcPickerItem> Items;
	TSet<FString> Added;
	FString Query;
	int32 MaxBeforeTyping = 0;
	bool bAllowTyped = true;
	bool bTypedIsPath = false;
	FPjcOnPicked OnPicked;
	FSimpleDelegate OnClose;

	TArray<FItemPtr> Rows;
	TSharedPtr<SListView<FItemPtr>> List;
	TSharedPtr<SEditableTextBox> Search;
	int32 Highlight = 0;
};
