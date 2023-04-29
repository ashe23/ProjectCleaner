// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcTypes.generated.h"

UCLASS(Config = EditorPerProjectUserSettings)
class UPjcEditorAssetExcludeSettings : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Config, Category="AssetExcludeSettings", meta=(ContentDir, ToolTip="Consider assets in specified paths as used"))
	TArray<FDirectoryPath> ExcludedPaths;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Config, Category="AssetExcludeSettings", meta=(ShowTreeView, ToolTip="Consider assets of specified classes as used"))
	TArray<TSoftClassPtr<UObject>> ExcludedClasses;

	UPROPERTY(Config)
	TArray<FName> ExcludedAssets;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override
	{
		Super::PostEditChangeProperty(PropertyChangedEvent);

		SaveConfig();
	}
#endif
};


struct FPjcStatItem
{
	FText Name;
	FText Num;
	FText Size;
	FText TooltipName;
	FText ToolTipNum;
	FText ToolTipSize;
	FLinearColor TextColor{FLinearColor::White};
	FMargin NamePadding{FMargin{0.0f}};
};
