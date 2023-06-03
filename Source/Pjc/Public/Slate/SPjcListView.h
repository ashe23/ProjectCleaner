// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

template <typename ItemType>
class SPjcListItemView : public SMultiColumnTableRow<ItemType>
{
public:
	SLATE_BEGIN_ARGS(SPjcListItemView) {}
		SLATE_ARGUMENT(ItemType, Item)
	SLATE_END_ARGS()

private:
	ItemType Item;
};
