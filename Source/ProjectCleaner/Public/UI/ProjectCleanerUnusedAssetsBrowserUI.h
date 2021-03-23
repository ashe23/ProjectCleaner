#pragma once

// Engine Headers
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
// #include "ProjectCleanerUnusedAssetsBrowserUI.generated.h"

class SProjectCleanerUnusedAssetsBrowserUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerUnusedAssetsBrowserUI) {}
		SLATE_ARGUMENT(TArray<FAssetData*>, UnusedAssets)
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
private:
	TArray<FAssetData*> UnusedAssets;
};
