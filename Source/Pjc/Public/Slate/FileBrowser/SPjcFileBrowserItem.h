// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

enum class EPjcBrowserItemFileType
{
	None,
	External,
	Corrupted
};

struct FPjcFileBrowserItem
{
	int64 FileSize = 0;
	EPjcBrowserItemFileType FileType = EPjcBrowserItemFileType::None;
	FString FileName;
	FString FilePath;
	FString FileExtension;
};


class SPjcFileBrowserItem final : public SMultiColumnTableRow<TSharedPtr<FPjcFileBrowserItem>>
{
public:
	SLATE_BEGIN_ARGS(SPjcFileBrowserItem) {}
		SLATE_ARGUMENT(TSharedPtr<FPjcFileBrowserItem>, Item)
		SLATE_ARGUMENT(FString, HighlightText)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable);
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;

private:
	FString HighlightText;
	TSharedPtr<FPjcFileBrowserItem> Item;
};
