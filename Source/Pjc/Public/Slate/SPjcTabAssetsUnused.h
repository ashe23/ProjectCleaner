// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

enum class EPjcAssetCategory : uint8;
class SPjcTreeView;
class SPjcStatAssets;

class SPjcTabAssetsUnused final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcTabAssetsUnused) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	
protected:
	void OnScanProjectAssets();
	void OnCleanProject();
	bool CanCleanProject();
	
private:
	TSharedRef<SWidget> CreateToolbar() const;

	TSharedPtr<FUICommandList> Cmds;
	TSharedPtr<SPjcStatAssets> StatAssetsPtr;
	TSharedPtr<SPjcTreeView> TreeViewPtr;
	TMap<EPjcAssetCategory, TSet<FAssetData>> AssetsCategoryMapping;
};
