// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcEditorAssetExcludeSettings.generated.h"

UCLASS(Config = EditorPerProjectUserSettings)
class UPjcEditorAssetExcludeSettings : public UObject
{
	GENERATED_BODY()

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category="AssetExcludeSettings", meta=(ContentDir, ToolTip="Consider assets in specified path as used."))
	TArray<FDirectoryPath> ExcludedPaths;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category="AssetExcludeSettings", meta=(ToolTip="Consider specified assets as used."))
	TArray<TSoftObjectPtr<UObject>> ExcludedAssets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category="AssetExcludeSettings", meta=(ShowTreeView, ToolTip="Consider assets of specified classes as used."))
	TArray<TSoftClassPtr<UObject>> ExcludedClasses;
};
