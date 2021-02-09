// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "IDetailsView.h"
#include "SCompoundWidget.h"
#include "SlateBasics.h"
#include "Engine/EngineTypes.h"
#include "SProjectCleanerBrowser.generated.h"


UCLASS()
class UDirectoryFilterSettings : public UObject
{

	GENERATED_BODY()
public:
	UPROPERTY(DisplayName = "Dest Path", EditAnywhere, Category = "DirectoryFilterSettings", meta = (ContentDir))
	TArray<FDirectoryPath> DirectoryFilterPath;
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

	// TSharedRef<SWidget> MakeWidgetForOption(FComboItemType InOption);
	// void OnSelectionChanged(FComboItemType NewValue, ESelectInfo::Type);
	// FText GetCurrentItemLabel() const;

	TSharedPtr<IDetailsView> DirFilterSettings;
	UDirectoryFilterSettings* DirectoryFilterSettings;
	
	// FComboItemType CurrentItem;
	// TArray<FComboItemType> Options;

	// TArray<FString> ExcludeDirectoryList;
};
