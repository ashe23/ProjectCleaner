// // Copyright Ashot Barkhudaryan. All Rights Reserved.
//
// #pragma once
//
// #include "CoreMinimal.h"
// #include "PjcTypes.h"
// #include "PjcEditorSettings.generated.h"
//
// UCLASS(Config = EditorPerProjectUserSettings)
// class UPjcEditorSettings : public UDeveloperSettings
// {
// 	GENERATED_BODY()
//
// public:
// 	virtual FName GetContainerName() const override;
// 	virtual FName GetCategoryName() const override;
// 	virtual FName GetSectionName() const override;
// 	virtual FText GetSectionText() const override;
// 	virtual FText GetSectionDescription() const override;
//
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category="General")
// 	EPjcCleanupMethod CleanupMethod = EPjcCleanupMethod::Full;
//
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category="AssetExclusionSettings", meta=(ContentDir, ToolTip="Consider assets in specified paths as used. Recursive."))
// 	TArray<FDirectoryPath> ExcludedAssetPaths;
//
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category="AssetExclusionSettings", meta=(ToolTip="Consider assets of specified classes as used."))
// 	TArray<TSoftClassPtr<UObject>> ExcludedAssetClasses;
//
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category="AssetExclusionSettings", meta=(ToolTip="Consider specified assets as used."))
// 	TArray<TSoftObjectPtr<UObject>> ExcludedAssets;
//
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category="ExternalFilesExclusionSettings", meta=(RelativeToGameDir))
// 	TArray<FDirectoryPath> ExcludedExternalFilePaths;
//
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category="ExternalFilesExclusionSettings")
// 	TArray<FFilePath> ExcludedExternalFiles;
//
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category="ExternalFilesExclusionSettings")
// 	TArray<FString> ExcludedExternalFileExtensions;
//
// protected:
// #if WITH_EDITOR
// 	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
// #endif
// };
