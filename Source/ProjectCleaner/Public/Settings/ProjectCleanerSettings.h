// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.h"
#include "ProjectCleanerSettings.generated.h"

UCLASS(Config = EditorPerProjectUserSettings)
class UProjectCleanerSettings final : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetContainerName() const override;
	virtual FName GetCategoryName() const override;
	virtual FName GetSectionName() const override;
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;

	UPROPERTY(EditAnywhere, Config, Category="General")
	EProjectCleanerCleanupMethod CleanupMethod = EProjectCleanerCleanupMethod::Full;

	UPROPERTY(EditAnywhere, Config, Category="ScanSettings", meta=(ContentDir))
	TArray<FDirectoryPath> ScanPaths;

	UPROPERTY(EditAnywhere, Config, Category="ScanSettings")
	TArray<TSoftClassPtr<UObject>> ScanClasses;

	UPROPERTY(EditAnywhere, Config, Category="ExcludeSettings", meta=(ContentDir))
	TArray<FDirectoryPath> ExcludePaths;

	UPROPERTY(EditAnywhere, Config, Category="ExcludeSettings")
	TArray<TSoftClassPtr<UObject>> ExcludeClasses;

	UPROPERTY(EditAnywhere, Config, Category="ExcludeSettings")
	TArray<TSoftObjectPtr<UObject>> ExcludeAssets;

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
