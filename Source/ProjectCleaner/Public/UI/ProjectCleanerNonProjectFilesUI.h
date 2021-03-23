#pragma once

// Engine Headers
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SProjectCleanerNonProjectFilesUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerNonProjectFilesUI) {}
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
};
