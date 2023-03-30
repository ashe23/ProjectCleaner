// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcExcludeSettings.generated.h"

UCLASS(Config = EditorPerProjectUserSettings)
class UPjcExcludeSettings final : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category="ExcludeSettings", meta=(ContentDir, ToolTip="Consider assets in specified paths as used. Always Recursive"))
	TArray<FDirectoryPath> ExcludedPaths;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category="ExcludeSettings", meta=(ToolTip="Consider assets of specified classes as used"))
	TArray<TSoftClassPtr<UObject>> ExcludedClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category="ExcludeSettings", meta=(ToolTip="Consider specifed assets as used"))
	TArray<TSoftObjectPtr<UObject>> ExcludedAssets;

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
