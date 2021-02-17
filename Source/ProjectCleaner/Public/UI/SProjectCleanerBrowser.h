// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "IDetailsView.h"
#include "Widgets/SCompoundWidget.h"
#include "Engine/EngineTypes.h"
#include "Widgets/SBoxPanel.h"
#include "SProjectCleanerBrowser.generated.h"


UCLASS()
class UDirectoryFilterSettings : public UObject
{

	GENERATED_BODY()
public:
	UPROPERTY(DisplayName = "Directory", EditAnywhere, Category = "ExcludeThisDirectories", meta = (ContentDir))
	TArray<FDirectoryPath> DirectoryPaths;
};

/**
 * 
 */
class SProjectCleanerBrowser : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerBrowser) {}
		SLATE_ARGUMENT(UDirectoryFilterSettings*, DirectoryFilterSettings)
	SLATE_END_ARGS()

	typedef TSharedPtr<FString> FComboItemType;

	void Construct(const FArguments& InArgs);

	TSharedPtr<IDetailsView> DirectoryFilterProperty;
	UDirectoryFilterSettings* DirectoryFilterSettings;

};
