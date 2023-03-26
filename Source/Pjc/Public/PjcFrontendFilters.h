// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once
//
// #include "CoreMinimal.h"
// #include "FrontendFilterBase.h"
// #include "PjcDelegates.h"
//
// class FPjcFilterAssetsPrimary final : public FFrontendFilter
// {
// public:
// 	explicit FPjcFilterAssetsPrimary(TSharedPtr<FFrontendFilterCategory> InCategory);
// 	virtual FString GetName() const override;
// 	virtual FText GetDisplayName() const override;
// 	virtual FText GetToolTipText() const override;
// 	virtual FLinearColor GetColor() const override;
// 	virtual void ActiveStateChanged(bool bActive) override;
// 	virtual bool PassesFilter(const FContentBrowserItem& InItem) const override;
// 	FPjcDelegateFilterChanged& OnFilterChange();
// private:
// 	TSet<FAssetData> Assets;
// 	FPjcDelegateFilterChanged DelegateFilterChanged;
// };
//
// class FPjcFilterAssetsExcluded final : public FFrontendFilter
// {
// public:
// 	explicit FPjcFilterAssetsExcluded(TSharedPtr<FFrontendFilterCategory> InCategory);
// 	virtual FString GetName() const override;
// 	virtual FText GetDisplayName() const override;
// 	virtual FText GetToolTipText() const override;
// 	virtual FLinearColor GetColor() const override;
// 	virtual void ActiveStateChanged(bool bActive) override;
// 	virtual bool PassesFilter(const FContentBrowserItem& InItem) const override;
// 	FPjcDelegateFilterChanged& OnFilterChange();
// private:
// 	TSet<FAssetData> Assets;
// 	FPjcDelegateFilterChanged DelegateFilterChanged;
// };
//
// class FPjcFilterAssetsIndirect final : public FFrontendFilter
// {
// public:
// 	explicit FPjcFilterAssetsIndirect(TSharedPtr<FFrontendFilterCategory> InCategory);
// 	virtual FString GetName() const override;
// 	virtual FText GetDisplayName() const override;
// 	virtual FText GetToolTipText() const override;
// 	virtual FLinearColor GetColor() const override;
// 	virtual void ActiveStateChanged(bool bActive) override;
// 	virtual bool PassesFilter(const FContentBrowserItem& InItem) const override;
// 	FPjcDelegateFilterChanged& OnFilterChange();
// private:
// 	TSet<FAssetData> Assets;
// 	FPjcDelegateFilterChanged DelegateFilterChanged;
// };
//
// class FPjcFilterAssetsExtReferenced final : public FFrontendFilter
// {
// public:
// 	explicit FPjcFilterAssetsExtReferenced(TSharedPtr<FFrontendFilterCategory> InCategory);
// 	virtual FString GetName() const override;
// 	virtual FText GetDisplayName() const override;
// 	virtual FText GetToolTipText() const override;
// 	virtual FLinearColor GetColor() const override;
// 	virtual void ActiveStateChanged(bool bActive) override;
// 	virtual bool PassesFilter(const FContentBrowserItem& InItem) const override;
// 	FPjcDelegateFilterChanged& OnFilterChange();
// private:
// 	TSet<FAssetData> Assets;
// 	FPjcDelegateFilterChanged DelegateFilterChanged;
// };
//
// class FPjcFilterAssetsUsed final : public FFrontendFilter
// {
// public:
// 	explicit FPjcFilterAssetsUsed(TSharedPtr<FFrontendFilterCategory> InCategory);
// 	virtual FString GetName() const override;
// 	virtual FText GetDisplayName() const override;
// 	virtual FText GetToolTipText() const override;
// 	virtual FLinearColor GetColor() const override;
// 	virtual void ActiveStateChanged(bool bActive) override;
// 	virtual bool PassesFilter(const FContentBrowserItem& InItem) const override;
// 	FPjcDelegateFilterChanged& OnFilterChange();
// private:
// 	TSet<FAssetData> Assets;
// 	FPjcDelegateFilterChanged DelegateFilterChanged;
// };