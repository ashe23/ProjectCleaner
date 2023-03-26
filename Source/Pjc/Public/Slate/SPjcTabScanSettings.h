// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

enum class EPjcScanResult : uint8;
enum class EPjcCleanupMethod : uint8;
struct FPjcScanData;

class SPjcTabScanSettings final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcTabScanSettings) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	void OnProjectScan(const EPjcScanResult ScanResult, const FString&);
	void UpdateData();

	FReply OnBtnScanProjectClick() const;
	FReply OnBtnCleanProjectClick() const;

	bool BtnCleanProjectEnabled() const;

	FText GetStatAssetsTotal() const;
	FText GetStatAssetsUsed() const;
	// FText GetStatAssetsUsedDeps() const;
	FText GetStatAssetsUnused() const;
	FText GetStatAssetsExcluded() const;
	FText GetStatAssetsIndirect() const;
	FText GetStatAssetsPrimary() const;
	FText GetStatAssetsEditor() const;
	FText GetStatAssetsExtReferenced() const;
	// FText GetStatFilesTotal() const;
	FText GetStatFilesNonEngine() const;
	FText GetStatFilesCorrupted() const;
	// FText GetStatFoldersTotal() const;
	FText GetStatFoldersEmpty() const;
	FText GetCleanupText(const EPjcCleanupMethod CleanupMethod) const;

	int32 NumAssetsTotal = 0;
	int32 NumAssetsUsed = 0;
	// int32 NumAssetsUsedDeps = 0;
	int32 NumAssetsUnused = 0;
	int32 NumAssetsExcluded = 0;
	int32 NumAssetsIndirect = 0;
	int32 NumAssetsPrimary = 0;
	int32 NumAssetsEditor = 0;
	int32 NumAssetsExtReferenced = 0;
	// int32 NumFilesTotal = 0;
	int32 NumFilesNonEngine = 0;
	int32 NumFilesCorrupted = 0;
	// int32 NumFoldersTotal = 0;
	int32 NumFoldersEmpty = 0;

	int64 SizeAssetsTotal = 0;
	int64 SizeAssetsUsed = 0;
	// int64 SizeAssetsUsedDeps = 0;
	int64 SizeAssetsUnused = 0;
	int64 SizeAssetsExcluded = 0;
	int64 SizeAssetsIndirect = 0;
	int64 SizeAssetsPrimary = 0;
	int64 SizeAssetsEditor = 0;
	int64 SizeAssetsExtReferenced = 0;
	// int64 SizeFilesTotal = 0;
	int64 SizeFilesNonEngine = 0;
	int64 SizeFilesCorrupted = 0;

	FNumberFormattingOptions NumberFormattingOptions;
};
