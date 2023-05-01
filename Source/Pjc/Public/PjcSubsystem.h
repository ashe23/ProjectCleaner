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

	void ToggleShowFoldersEmpty();
	void ToggleShowFoldersExcluded();
	void ToggleScanFoldersDev();
	void ToggleCleanAssetsUnused();
	void ToggleCleanFoldersEmpty();

	bool CanShowFoldersEmpty() const;
	bool CanShowFoldersExcluded() const;
	bool CanScanFoldersDev() const;
	bool CanCleanAssetsUnused() const;
	bool CanCleanFoldersEmpty() const;

private:
	UPROPERTY(Config)
	bool bShowFoldersEmpty = true;

	UPROPERTY(Config)
	bool bShowFoldersExcluded = true;

	UPROPERTY(Config)
	bool bScanFoldersDev = false;

	UPROPERTY(Config)
	bool bCleanAssetsUnused = true;

	UPROPERTY(Config)
	bool bCleanFoldersEmpty = true;

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
