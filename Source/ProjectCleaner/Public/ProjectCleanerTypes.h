// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.generated.h"

UCLASS(Transient, Config=EditorPerProjectUserSettings)
class UProjectCleanerScanSettings final : public UObject
{
	GENERATED_BODY()
public:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override
	{
		Super::PostEditChangeProperty(PropertyChangedEvent);

		SaveConfig();
	}

	UPROPERTY(DisplayName="Scan Developer Content", EditAnywhere, Config, Category="ScanSettings", meta=(ToolTip="Scan assets in 'Developers' folder. By Default false"))
	bool bScanDeveloperContents = false;

	UPROPERTY(EditAnywhere, Config, Category = "ScanSettings", meta=(ToolTip="Automatically delete empty folders after deleting unused assets"))
	bool bAutomaticallyDeleteEmptyFolders = true;

	UPROPERTY(DisplayName="Paths", EditAnywhere, Config, Category="ScanSettings|ExcludeOptions", meta=(ContentDir, ToolTip="Exclude assets and their dependencies inside specified paths"))
	TArray<FDirectoryPath> Paths;

	UPROPERTY(DisplayName="Classes", EditAnywhere, Config, Category="ScanSettings|ExcludeOptions", meta=(ToolTip="Exclude assets and their dependencies of specified classes"))
	TArray<UClass*> Classes;
};

UCLASS(Transient)
class UProjectCleanerStatListItem final : public UObject
{
	GENERATED_BODY()
public:
	FString Name;
	FString Category;
	FString Count;
	FString Size;
	FLinearColor Color = FLinearColor::White;
};

UCLASS(Transient)
class UProjectCleanerStatTreeItem : public UObject
{
	GENERATED_BODY()
	
public:
	UProjectCleanerStatTreeItem(){}

	FString Path;
	FString Size;
};
