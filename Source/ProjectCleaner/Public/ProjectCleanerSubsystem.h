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

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="ScanSettings")
	bool bAutoCleanEmptyFolders = true;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="ScanSettings")
	bool bScanDevFolder = false;

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	void ScanProject();
	void ToggleConfigCleanEmptyFolder();
	void ToggleConfigScanDevFolder();

private:
	void ContainersEmpty();
	void ContainersReset();
	void ContainersShrink();

	void FixupRedirectors() const;

	TArray<FAssetData> AssetsAll;
	TArray<FAssetData> AssetsIndirect;
	TArray<FAssetData> AssetsExcluded;
	TArray<FAssetData> AssetsUsed;
	TArray<FAssetData> AssetsUnused;

	TSet<FString> FilesNonEngine;
	TSet<FString> FilesCorrupted;

	TSet<FString> FoldersAll;
	TSet<FString> FoldersEmpty;


	FAssetRegistryModule* ModuleAssetRegistry;
	FAssetToolsModule* ModuleAssetTools;
};
