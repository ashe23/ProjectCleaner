// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.h"
#include "ProjectCleanerSubsystem.generated.h"

class FAssetRegistryModule;

UCLASS(Config=EditorPerProjectUserSettings)
class UProjectCleanerSubsystem final : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	UProjectCleanerSubsystem();

	UPROPERTY(EditAnywhere, Config)
	bool bAutoCleanEmptyFolders = true;

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	void ProjectScan();
	void CheckEditorState();

	void NotifyMainTabActivated();
	void NotifyMainTabClosed();

	EProjectCleanerEditorState GetEditorState() const;
	EProjectCleanerScanState GetScanState() const;
	EProjectCleanerScanDataState GetScanDataState() const;
private:
	void AssetRegistryDelegatesRegister();
	void AssetRegistryDelegatesUnregister();

	EProjectCleanerEditorState EditorState = EProjectCleanerEditorState::Idle;
	EProjectCleanerScanState ScanState = EProjectCleanerScanState::Idle;
	EProjectCleanerScanDataState ScanDataState = EProjectCleanerScanDataState::None;

	FAssetRegistryModule* ModuleAssetRegistry;
	FDelegateHandle DelegateHandleAssetAdded;
	FDelegateHandle DelegateHandleAssetRemoved;
	FDelegateHandle DelegateHandleAssetRenamed;
	FDelegateHandle DelegateHandleAssetUpdated;
	FDelegateHandle DelegateHandlePathAdded;
	FDelegateHandle DelegateHandlePathRemoved;
};
