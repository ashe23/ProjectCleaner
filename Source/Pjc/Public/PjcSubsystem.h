// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "PjcDelegates.h"
#include "PjcTypes.h"
#include "PjcSubsystem.generated.h"

class UPjcSettings;
class IAssetRegistry;
class FAssetToolsModule;

UCLASS(Config=EditorPerProjectUserSettings, meta=(ToolTip="ProjectCleanerSubsystem"))
class UPjcSubsystem final : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	UPROPERTY(BlueprintReadOnly, Config, Category="ProjectCleaner")
	bool bShowPathsEmpty = true;

	UPROPERTY(BlueprintReadOnly, Config, Category="ProjectCleaner")
	bool bShowPathsExcluded = true;

	UPROPERTY(BlueprintReadOnly, Config, Category="ProjectCleaner")
	bool bShowPathsEngineGenerated = true;

	UPROPERTY(BlueprintReadOnly, Config, Category="ProjectCleaner")
	bool bShowPathsUnusedOnly = false;
	
	void ProjectScan();
	void ProjectClean() const;

	UFUNCTION(BlueprintCallable, Category="Pjc")
	void Test(const FName& Path);

	EPjcScannerState GetScannerState() const;

	FPjcDelegateOnProjectScan& OnProjectScan();

private:
	bool CanScanProject(FString& ErrMsg) const;

	EPjcScannerState ScannerState = EPjcScannerState::Idle;

	FPjcDelegateOnProjectScan DelegateOnProjectScan;

	IAssetRegistry* ModuleAssetRegistry = nullptr;
	const FAssetToolsModule* ModuleAssetTools = nullptr;
};
