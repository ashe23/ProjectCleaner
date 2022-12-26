// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerExcludeSettings.generated.h"

UCLASS(Config=EditorPerProjectUserSettings)
class UProjectCleanerExcludeSettings final : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetContainerName() const override;
	virtual FName GetCategoryName() const override;
	virtual FName GetSectionName() const override;
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Config, Category="Exclude Settings", meta=(ContentDir))
	TArray<FDirectoryPath> ExcludedFolders;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Config, Category="Exclude Settings")
	TArray<TSoftClassPtr<UObject>> ExcludedClasses;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Config, Category="Exclude Settings")
	TArray<TSoftObjectPtr<UObject>> ExcludedAssets;

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
