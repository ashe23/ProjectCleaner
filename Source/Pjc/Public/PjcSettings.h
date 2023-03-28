// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcTypes.h"
#include "PjcSettings.generated.h"

UCLASS(Config = EditorPerProjectUserSettings)
class UPjcSettings final : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetContainerName() const override;
	virtual FName GetCategoryName() const override;
	virtual FName GetSectionName() const override;
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;

	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category="General", meta=(ToolTip="Project Cleanup Method"))
	// EPjcCleanupMethod CleanupMethod = EPjcCleanupMethod::Full;
	
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
