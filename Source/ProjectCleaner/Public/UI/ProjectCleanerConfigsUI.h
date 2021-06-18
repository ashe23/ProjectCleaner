// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "ProjectCleanerConfigsUI.generated.h"

UCLASS(Transient)
class UCleanerConfigs : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(DisplayName = "Scan Developer Contents Folders", EditAnywhere, Category = "CleanerConfigs")
	bool bScanDeveloperContents = false;

	UPROPERTY(DisplayName = "Remove Empty Folders After Assets Deleted", EditAnywhere, Category = "CleanerConfigs")
	bool bAutomaticallyDeleteEmptyFolders = true;

	UPROPERTY(DisplayName = "Deletion Chunk Limit", EditAnywhere, Category = "CleanerConfigs", meta = (ClampMin="20", ClampMax="1000", UIMin = "20", UIMax = "1000", ToolTip = "To prevent engine from freezing when deleting a lot of assets, we delete them by chunks.Here you can specify chunk size.Default is 20"))
	int32 DeleteChunkLimit = 20;
};

class SProjectCleanerConfigsUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerConfigsUI) {}
		SLATE_ARGUMENT(UCleanerConfigs*, CleanerConfigs)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetCleanerConfigs(UCleanerConfigs* Configs);
private:
	void InitUI();
	TSharedPtr<IDetailsView> ConfigsProperty;
	UCleanerConfigs* CleanerConfigs = nullptr;
};