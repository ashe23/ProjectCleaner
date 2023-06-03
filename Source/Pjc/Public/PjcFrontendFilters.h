// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FrontendFilterBase.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FPjcDelegateFilterChanged, const bool bActive);

class FPjcFilterAssetsUsed final : public FFrontendFilter
{
public:
	explicit FPjcFilterAssetsUsed(TSharedPtr<FFrontendFilterCategory> InCategory);
	virtual FString GetName() const override;
	virtual FText GetDisplayName() const override;
	virtual FText GetToolTipText() const override;
	virtual FLinearColor GetColor() const override;
	virtual void ActiveStateChanged(bool bActive) override;
	virtual bool PassesFilter(const FContentBrowserItem& InItem) const override;
	void UpdateData();

	FPjcDelegateFilterChanged& OnFilterChanged();

private:
	FPjcDelegateFilterChanged DelegateFilterChanged;
	TSet<FAssetData> Assets;
};

class FPjcFilterAssetsPrimary final : public FFrontendFilter
{
public:
	explicit FPjcFilterAssetsPrimary(TSharedPtr<FFrontendFilterCategory> InCategory);
	virtual FString GetName() const override;
	virtual FText GetDisplayName() const override;
	virtual FText GetToolTipText() const override;
	virtual FLinearColor GetColor() const override;
	virtual void ActiveStateChanged(bool bActive) override;
	virtual bool PassesFilter(const FContentBrowserItem& InItem) const override;
	void UpdateData();

	FPjcDelegateFilterChanged& OnFilterChanged();

private:
	FPjcDelegateFilterChanged DelegateFilterChanged;
	TSet<FAssetData> Assets;
};

class FPjcFilterAssetsIndirect final : public FFrontendFilter
{
public:
	explicit FPjcFilterAssetsIndirect(TSharedPtr<FFrontendFilterCategory> InCategory);
	virtual FString GetName() const override;
	virtual FText GetDisplayName() const override;
	virtual FText GetToolTipText() const override;
	virtual FLinearColor GetColor() const override;
	virtual void ActiveStateChanged(bool bActive) override;
	virtual bool PassesFilter(const FContentBrowserItem& InItem) const override;
	void UpdateData();

	FPjcDelegateFilterChanged& OnFilterChanged();

private:
	FPjcDelegateFilterChanged DelegateFilterChanged;
	TSet<FAssetData> Assets;
};

class FPjcFilterAssetsCircular final : public FFrontendFilter
{
public:
	explicit FPjcFilterAssetsCircular(TSharedPtr<FFrontendFilterCategory> InCategory);
	virtual FString GetName() const override;
	virtual FText GetDisplayName() const override;
	virtual FText GetToolTipText() const override;
	virtual FLinearColor GetColor() const override;
	virtual void ActiveStateChanged(bool bActive) override;
	virtual bool PassesFilter(const FContentBrowserItem& InItem) const override;
	void UpdateData();

	FPjcDelegateFilterChanged& OnFilterChanged();

private:
	FPjcDelegateFilterChanged DelegateFilterChanged;
	TSet<FAssetData> Assets;
};

class FPjcFilterAssetsEditor final : public FFrontendFilter
{
public:
	explicit FPjcFilterAssetsEditor(TSharedPtr<FFrontendFilterCategory> InCategory);
	virtual FString GetName() const override;
	virtual FText GetDisplayName() const override;
	virtual FText GetToolTipText() const override;
	virtual FLinearColor GetColor() const override;
	virtual void ActiveStateChanged(bool bActive) override;
	virtual bool PassesFilter(const FContentBrowserItem& InItem) const override;
	void UpdateData();

	FPjcDelegateFilterChanged& OnFilterChanged();

private:
	FPjcDelegateFilterChanged DelegateFilterChanged;
	TSet<FAssetData> Assets;
};

class FPjcFilterAssetsExcluded final : public FFrontendFilter
{
public:
	explicit FPjcFilterAssetsExcluded(TSharedPtr<FFrontendFilterCategory> InCategory);
	virtual FString GetName() const override;
	virtual FText GetDisplayName() const override;
	virtual FText GetToolTipText() const override;
	virtual FLinearColor GetColor() const override;
	virtual void ActiveStateChanged(bool bActive) override;
	virtual bool PassesFilter(const FContentBrowserItem& InItem) const override;
	void UpdateData();

	FPjcDelegateFilterChanged& OnFilterChanged();

private:
	FPjcDelegateFilterChanged DelegateFilterChanged;
	TSet<FAssetData> Assets;
};

class FPjcFilterAssetsExtReferenced final : public FFrontendFilter
{
public:
	explicit FPjcFilterAssetsExtReferenced(TSharedPtr<FFrontendFilterCategory> InCategory);
	virtual FString GetName() const override;
	virtual FText GetDisplayName() const override;
	virtual FText GetToolTipText() const override;
	virtual FLinearColor GetColor() const override;
	virtual void ActiveStateChanged(bool bActive) override;
	virtual bool PassesFilter(const FContentBrowserItem& InItem) const override;
	void UpdateData();

	FPjcDelegateFilterChanged& OnFilterChanged();

private:
	FPjcDelegateFilterChanged DelegateFilterChanged;
	TSet<FAssetData> Assets;
};
