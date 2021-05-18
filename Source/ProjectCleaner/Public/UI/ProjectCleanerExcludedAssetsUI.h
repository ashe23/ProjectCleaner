#pragma once

// Engine Headers
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SProjectCleanerExcludedAssetsUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerExcludedAssetsUI) {}
		// SLATE_ARGUMENT(TArray<FAssetData>, UnusedAssets)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
private:
	void RefreshUIContent();
	TSharedRef<SWidget> WidgetRef = SNullWidget::NullWidget;
};
