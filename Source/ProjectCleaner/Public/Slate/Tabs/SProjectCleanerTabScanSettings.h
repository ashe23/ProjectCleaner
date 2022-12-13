// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SProjectCleanerTabScanSettings final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerTabScanSettings)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

protected:
	void UpdateData();
	
	FText GetTextAssetsTotal() const;
	FText GetTextAssetsIndirect() const;
	FText GetTextAssetsExcluded() const;
	FText GetTextAssetsUnused() const;
	FText GetTextFoldersTotal() const;
	FText GetTextFoldersEmpty() const;
	FText GetTextFilesCorrupted() const;
	FText GetTextFilesNonEngine() const;

	FSlateColor GetTextColorAssetsUnused() const;
	FSlateColor GetTextColorAssetsExcluded() const;
	FSlateColor GetTextColorFoldersEmpty() const;

private:
	int32 AssetsTotalNum = 0;
	int32 AssetsIndirectNum = 0;
	int32 AssetsExcludedNum = 0;
	int32 AssetsUnusedNum = 0;
	int32 FoldersTotalNum = 0;
	int32 FoldersEmptyNum = 0;
	int32 FilesCorruptedNum = 0;
	int32 FilesNonEngineNum = 0;

	int64 AssetsTotalSize = 0;
	int64 AssetsIndirectSize = 0;
	int64 AssetsExcludedSize = 0;
	int64 AssetsUnusedSize = 0;
	int64 FilesCorruptedSize = 0;
	int64 FilesNonEngineSize = 0;
};
