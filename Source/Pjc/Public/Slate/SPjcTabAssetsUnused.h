// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SPjcContentBrowser;
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
	void OnTreeViewSelectionChanged(const TSet<FString>& InSelectedPaths);

private:
	TSharedRef<SWidget> CreateToolbar() const;

	bool bCanCleanProject = false;
	TSharedPtr<FUICommandList> Cmds;
	TSharedPtr<SPjcTreeView> TreeViewPtr;
	TSharedPtr<SPjcStatAssets> StatAssetsPtr;
	TSharedPtr<SPjcContentBrowser> ContentBrowserPtr;
};
