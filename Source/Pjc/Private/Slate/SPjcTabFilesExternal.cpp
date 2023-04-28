// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabFilesExternal.h"

void SPjcTabFilesExternal::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(STextBlock).Text(FText::FromString(TEXT("Tab Files External")))
	];
}
