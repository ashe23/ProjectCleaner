// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/Tabs/SPjcTabAssetsInspection.h"

void SPjcTabAssetsInspection::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(STextBlock).Text(FText::FromString(TEXT("Assets Inspection")))
	];
}
