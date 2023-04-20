// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/AssetBrowser/SPjcContentBrowser.h"

void SPjcContentBrowser::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(STextBlock).Text(FText::FromString(TEXT("Content Browser View")))
	];
}
