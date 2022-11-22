// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "ProjectCleanerWindowMain.generated.h"

UCLASS(Transient)
class UProjectCleanerStatsListItem : public UObject
{
	GENERATED_BODY()
public:
	FString Title;
	FString Used;
	FString Unused;
	FString Excluded;
	FString Indirect;
};

class SProjectCleanerStatsListItem : public SMultiColumnTableRow<TWeakObjectPtr<UProjectCleanerStatsListItem>>
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerStatsListItem)
		{
		}

		SLATE_ARGUMENT(TWeakObjectPtr<UProjectCleanerStatsListItem>, ListItem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		ListItem = InArgs._ListItem;

		SMultiColumnTableRow<TWeakObjectPtr<UProjectCleanerStatsListItem>>::Construct(
			SMultiColumnTableRow<TWeakObjectPtr<UProjectCleanerStatsListItem>>::FArguments()
			.Padding(FMargin{0.0f, 5.0f, 0.0f, 0.0f}),
			InOwnerTableView
		);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override
	{
		if (InColumnName.IsEqual(TEXT("Title")))
		{
			return SNew(STextBlock).Text(FText::FromString(ListItem->Title));
		}

		if (InColumnName.IsEqual(TEXT("Used")))
		{
			return SNew(STextBlock).Text(FText::FromString(ListItem->Used));
		}

		if (InColumnName.IsEqual(TEXT("Unused")))
		{
			return SNew(STextBlock).Text(FText::FromString(ListItem->Unused));
		}

		if (InColumnName.IsEqual(TEXT("Excluded")))
		{
			return SNew(STextBlock).Text(FText::FromString(ListItem->Excluded));
		}

		if (InColumnName.IsEqual(TEXT("Indirect")))
		{
			return SNew(STextBlock).Text(FText::FromString(ListItem->Indirect));
		}

		return SNew(STextBlock).Text(FText::FromString(""));
	}
private:
	TWeakObjectPtr<UProjectCleanerStatsListItem> ListItem;
};

class SProjectCleanerWindowMain : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerWindowMain)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
private:
	FText GetNumAllAssets() const;
	FText GetNumUsedAssets() const;
	FText GetNumUnusedAssets() const;
	FText GetNumExcludedAssets() const;
	FText GetNumIndirectAssets() const;
	FText GetNumAllFolders() const;
	FText GetNumEmptyFolders() const;
	FText GetNumCorruptedAssets() const;
	FText GetNumExternalFiles() const;
	FText GetSizeTotal() const;
	FText GetSizeUsed() const;
	FText GetSizeUnused() const;
	FText GetSizeExcluded() const;
	FText GetSizeIndirect() const;
	static void OnNavigateWiki();
	static bool IsWidgetEnabled();
	static int32 GetWidgetIndex();

	TArray<TWeakObjectPtr<UProjectCleanerStatsListItem>> ListItems;
	TSharedPtr<SListView<TWeakObjectPtr<UProjectCleanerStatsListItem>>> ListView;
};
