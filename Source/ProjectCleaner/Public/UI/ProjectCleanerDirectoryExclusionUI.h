// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "ProjectCleanerDirectoryExclusionUI.generated.h"

UCLASS(Transient)
class UExcludeDirectoriesFilterSettings : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(DisplayName = "Directory", EditAnywhere, Category = "ExcludeThisDirectories", meta = (ContentDir))
	TArray<FDirectoryPath> Paths;
};

/**
 * @brief Shows Exclude Directories UI
 */
class SProjectCleanerDirectoryExclusionUI : public SCompoundWidget
{
public:
	
	SLATE_BEGIN_ARGS(SProjectCleanerDirectoryExclusionUI) {}
		SLATE_ARGUMENT(UExcludeDirectoriesFilterSettings*, ExcludeDirectoriesFilterSettings)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
private:

	/** Data **/
	TSharedPtr<IDetailsView> ExcludeDirectoriesFilterSettingsProperty;
	UExcludeDirectoriesFilterSettings* ExcludeDirectoriesFilterSettings = nullptr;
};
