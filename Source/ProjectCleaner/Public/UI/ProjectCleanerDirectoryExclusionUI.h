#pragma once

// Engine Headers
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "ProjectCleanerDirectoryExclusionUI.generated.h"

UCLASS()
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
	TSharedPtr<IDetailsView> ExcludeDirectoriesFilterSettingsProperty;
	UExcludeDirectoriesFilterSettings* ExcludeDirectoriesFilterSettings = nullptr;
};
