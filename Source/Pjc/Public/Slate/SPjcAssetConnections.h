// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

struct FPjcUsageItem;
class SBox;

DECLARE_DELEGATE_OneParam(FPjcOnConnectionAsset, const FName&);

/**
 * One asset with its connections: on the left usage chain (asset used for its own reason first) and assets that use it,
 * in the middle the asset, on the right assets it uses. Curves link them, clicking an asset moves there.
 */
class SPjcAssetConnections final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcAssetConnections) {}
	SLATE_EVENT(FPjcOnConnectionAsset, OnInspect)
	SLATE_EVENT(FPjcOnConnectionAsset, OnOpen)
	SLATE_EVENT(FPjcOnConnectionAsset, OnBrowse)
	SLATE_EVENT(FPjcOnConnectionAsset, OnShowInMap)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Chain goes from asset used for its own reason to given asset, empty for unused ones. Invalid item shows hint */
	void SetAsset(
		const TSharedPtr<FPjcUsageItem>& InItem,
		const TArray<TSharedPtr<FPjcUsageItem>>& InChain,
		const TArray<TSharedPtr<FPjcUsageItem>>& InUsedBy,
		const TArray<TSharedPtr<FPjcUsageItem>>& InUses,
		const FText& InWhy,
		const FString& InOwned
	);

  private:

	struct FLink
	{
		TSharedPtr<SWidget> From;
		TSharedPtr<SWidget> To;
		/** link drawn only while both ends are inside these */
		TSharedPtr<SWidget> FromClip;
		TSharedPtr<SWidget> ToClip;
		FLinearColor Color;
		float Thickness;
		bool bVertical;
	};

	void Rebuild();
	void GetLinks(TArray<FLink>& OutLinks) const;
	/** Runs on next tick, clicked card is rebuilt away by it */
	void Defer(const FPjcOnConnectionAsset& Delegate, const FName& Package);
	TSharedRef<SWidget> MakeCard(const TSharedPtr<FPjcUsageItem>& InItem, const bool bHighlight);
	TSharedRef<SWidget> MakeCenter();
	TSharedRef<SWidget> MakeColumn(const FText& Title, const TSharedRef<SWidget>& Content);
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FPjcUsageItem> InItem, const TSharedRef<STableViewBase>& OwnerTable, const bool bUsedBy);

	FPjcOnConnectionAsset OnInspect;
	FPjcOnConnectionAsset OnOpen;
	FPjcOnConnectionAsset OnBrowse;
	FPjcOnConnectionAsset OnShowInMap;

	TSharedPtr<FPjcUsageItem> Item;
	TArray<TSharedPtr<FPjcUsageItem>> Chain;
	TArray<TSharedPtr<FPjcUsageItem>> UsedBy;
	TArray<TSharedPtr<FPjcUsageItem>> Uses;
	FText Why;
	FString Owned;

	TSharedPtr<SBox> Content;
	TSharedPtr<SWidget> CenterCard;
	TArray<TSharedPtr<SWidget>> ChainCards;
	TSharedPtr<SWidget> ChainBox;
	TSharedPtr<SListView<TSharedPtr<FPjcUsageItem>>> UsedByList;
	TSharedPtr<SListView<TSharedPtr<FPjcUsageItem>>> UsesList;
	/** cards of rows on screen, rows scrolled away release theirs */
	TMap<const FPjcUsageItem*, TWeakPtr<SWidget>> UsedByCards;
	TMap<const FPjcUsageItem*, TWeakPtr<SWidget>> UsesCards;

	friend class SPjcConnectionLinks;
};
