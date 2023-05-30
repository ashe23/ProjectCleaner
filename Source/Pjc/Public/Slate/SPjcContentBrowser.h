// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ContentBrowserDelegates.h"
#include "Widgets/SCompoundWidget.h"

class SPjcTreeView;
class UPjcSubsystem;
enum class EPjcAssetCategory : uint8;

class SPjcContentBrowser final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcContentBrowser) {}
		SLATE_ARGUMENT(TSharedPtr<SPjcTreeView>, TreeViewPtr)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SPjcContentBrowser() override;
	void UpdateView();

protected:
	TSharedRef<SWidget> CreateToolbar() const;
	void CreateContentBrowser();
	bool AnyFilterActive() const;
private:
	bool bFilterAssetsUsedActive = false;
	bool bFilterAssetsPrimaryActive = false;
	bool bFilterAssetsEditorActive = false;
	bool bFilterAssetsIndirectActive = false;
	bool bFilterAssetsExcludedActive = false;
	bool bFilterAssetsExtReferencedActive = false;
	bool bFilterAssetsCircularActive = false;
	bool bFilterAssetsUnusedActive = true;

	FARFilter Filter;
	UPjcSubsystem* SubsystemPtr = nullptr;
	TSharedPtr<SPjcTreeView> TreeViewPtr;
	TSharedPtr<FUICommandList> Cmds;
	TSharedPtr<SWidget> ContentBrowserPtr;
	FSetARFilterDelegate DelegateFilter;
	FRefreshAssetViewDelegate DelegateRefreshView;
	FGetCurrentSelectionDelegate DelegateSelection;
};
