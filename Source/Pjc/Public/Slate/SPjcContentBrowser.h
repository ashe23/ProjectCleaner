// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

enum class EPjcAssetCategory : uint8;

class SPjcContentBrowser : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPjcContentBrowser) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void UpdateData(TMap<EPjcAssetCategory, TSet<FAssetData>>& AssetsCategoryMapping);
};
