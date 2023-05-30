// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SPjcTreeView;
class SPjcStatAssets;
class SPjcContentBrowser;
enum class EPjcAssetCategory : uint8;

class SPjcTabAssetsUnused final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcTabAssetsUnused) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedRef<SWidget> CreateToolbar() const;
	void UpdateView();

	bool bCanCleanProject = false;
	TSharedPtr<FUICommandList> Cmds;
	TSharedPtr<SPjcTreeView> TreeViewPtr;
	TSharedPtr<SPjcStatAssets> StatAssetsPtr;
	TSharedPtr<SPjcContentBrowser> ContentBrowserPtr;
};
