// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FPjcCorruptedAssetItem;

class SPjcTabAssetsCorrupted final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcTabAssetsCorrupted) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	int32 NumFilesTotal = 0;
	TSharedPtr<FUICommandList> Cmds;
	TArray<TSharedPtr<FPjcCorruptedAssetItem>> ItemsAll;
	TSharedPtr<SListView<TSharedPtr<FPjcCorruptedAssetItem>>> ListView;

	EColumnSortMode::Type ColumnSortModeFilePath = EColumnSortMode::None;
	EColumnSortMode::Type ColumnSortModeFileName = EColumnSortMode::None;
	EColumnSortMode::Type ColumnSortModeFileExt = EColumnSortMode::None;
	EColumnSortMode::Type ColumnSortModeFileSize = EColumnSortMode::None;
};
