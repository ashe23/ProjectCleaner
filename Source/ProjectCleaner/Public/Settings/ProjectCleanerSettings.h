// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerSettings.generated.h"

UCLASS(Config = EditorPerProjectUserSettings)
class UProjectCleanerSettings final : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetContainerName() const override;
	virtual FName GetCategoryName() const override;
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;

	UPROPERTY(EditAnywhere, Config, Category="General", meta=(ToolTip="Automatically delete empty folders after deleting unused assets. By default, it is enabled."))
	bool bAutoCleanEmptyFolders = true;

	UPROPERTY(EditAnywhere, Config, Category="TreeView", meta=(ToolTip="Show TreeView organizer lines"))
	bool bShowTreeViewLines = true;

	UPROPERTY(EditAnywhere, Config, Category="TreeView", meta=(ToolTip="Show empty folders in tree view"), DisplayName="Show Folders Empty")
	bool bShowTreeViewFoldersEmpty = true;

	UPROPERTY(EditAnywhere, Config, Category="TreeView", meta=(ToolTip="Show excluded folders in tree view"), DisplayName="Show Folders Excluded")
	bool bShowTreeViewFoldersExcluded = true;

	UPROPERTY(EditAnywhere, Config, Category="Exclude Settings", meta=(ContentDir))
	TArray<FDirectoryPath> ExcludedFolders;

	UPROPERTY(EditAnywhere, Config, Category="Exclude Settings")
	TArray<TSoftClassPtr<UObject>> ExcludedClasses;

	UPROPERTY(EditAnywhere, Config, Category="Exclude Settings")
	TArray<TSoftObjectPtr<UObject>> ExcludedAssets;
};
