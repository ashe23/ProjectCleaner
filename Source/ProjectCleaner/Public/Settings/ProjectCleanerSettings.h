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
	virtual FName GetSectionName() const override;
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;

	void ToggleAutoCleanEmptyFolders();
	void ToggleShowTreeViewLines();
	void ToggleShowTreeViewFoldersEmpty();
	void ToggleShowTreeViewFoldersExcluded();

	UPROPERTY(EditAnywhere, Config, Category="General", meta=(ToolTip="Automatically delete empty folders after deleting unused assets. By default, it is enabled."))
	bool bAutoCleanEmptyFolders = true;

	UPROPERTY(EditAnywhere, Config, Category="TreeView", meta=(ToolTip="Show TreeView organizer lines"), DisplayName="Show Lines")
	bool bShowTreeViewLines = true;

	UPROPERTY(EditAnywhere, Config, Category="TreeView", meta=(ToolTip="Show empty folders in tree view"), DisplayName="Show Folders Empty")
	bool bShowTreeViewFoldersEmpty = true;

	UPROPERTY(EditAnywhere, Config, Category="TreeView", meta=(ToolTip="Show excluded folders in tree view"), DisplayName="Show Folders Excluded")
	bool bShowTreeViewFoldersExcluded = true;

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
