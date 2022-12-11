// // Copyright Ashot Barkhudaryan. All Rights Reserved.
//
// #pragma once
//
// #include "CoreMinimal.h"
// #include "ProjectCleanerDelegates.h"
// #include "ProjectCleanerExcludeSettings.generated.h"
//
// UCLASS(Transient, Config=EditorPerProjectUserSettings)
// class UProjectCleanerExcludeSettings final : public UObject
// {
// 	GENERATED_BODY()
//
// public:
// 	UPROPERTY(BlueprintReadWrite, EditAnywhere, Config, Category="Exclude Settings", DisplayName="Excluded Folders",
// 		meta=(ContentDir, ToolTip="Exclude assets contained within these folders from scanning."))
// 	TArray<FDirectoryPath> ExcludedFolders;
//
// 	UPROPERTY(BlueprintReadWrite, EditAnywhere, Config, Category="Exclude Settings", meta=(ToolTip="Exclude assets of specific classes from scanning."))
// 	TArray<TSoftClassPtr<UObject>> ExcludedClasses;
//
// 	UPROPERTY(BlueprintReadWrite, EditAnywhere, Config, Category="Exclude Settings", meta=(ToolTip="List of excluded assets."))
// 	TArray<TSoftObjectPtr<UObject>> ExcludedAssets;
//
// 	FProjectCleanerDelegateExcludeSettingsChanged& OnChange();
//
// protected:
// #if WITH_EDITOR
// 	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
// #endif
//
// private:
// 	FProjectCleanerDelegateExcludeSettingsChanged DelegateExcludeSettingChanged;
// };
