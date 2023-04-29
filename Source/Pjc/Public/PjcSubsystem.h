// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "PjcSubsystem.generated.h"

UCLASS(Config=EditorPerProjectUserSettings, meta=(ToolTip="ProjectCleanerSubsystem"))
class UPjcSubsystem final : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void ToggleShowFoldersDev();
	void ToggleShowFoldersEmpty();
	void ToggleShowFoldersExcluded();

	bool CanShowFoldersDev() const;
	bool CanShowFoldersEmpty() const;
	bool CanShowFoldersExcluded() const;

private:
	UPROPERTY(Config)
	bool bShowFoldersDev = false;

	UPROPERTY(Config)
	bool bShowFoldersEmpty = true;

	UPROPERTY(Config)
	bool bShowFoldersExcluded = true;

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
