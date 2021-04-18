#pragma once

// Engine Headers
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SProjectCleanerCorruptedFilesUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerCorruptedFilesUI) {}
		SLATE_ARGUMENT(TArray<FAssetData>, CorruptedFiles);
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	void RefreshUIContent();
	void SetCorruptedFiles(const TArray<FAssetData>& NewCorruptedFiles);
private:
	TArray<FAssetData> CorruptedFiles;
	TSharedRef<SWidget> WidgetRef = SNullWidget::NullWidget;
};
