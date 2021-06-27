// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UExcludeOptions;

/**
 * @brief Shows Exclude Options UI
 */
class SProjectCleanerExcludeOptionsUI : public SCompoundWidget
{
public:
	
	SLATE_BEGIN_ARGS(SProjectCleanerExcludeOptionsUI) {}
		SLATE_ARGUMENT(UExcludeOptions*, ExcludeOptions)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
private:

	/** Data **/
	TSharedPtr<IDetailsView> ExcludeOptionsProperty;
	UExcludeOptions* ExcludeOptions = nullptr;
};
