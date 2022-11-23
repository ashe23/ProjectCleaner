// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.generated.h"

UCLASS(Transient, Config=EditorPerProjectUserSettings)
class UProjectCleanerScanSettings : public UObject
{
	GENERATED_BODY()
public:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override
	{
		Super::PostEditChangeProperty(PropertyChangedEvent);

		SaveConfig();
	}

	UPROPERTY(DisplayName = "Scan Developer Content", EditAnywhere, Config, Category = "ScanSettings", meta = (ToolTip = "Scan assets in 'Developers' folder. By Default false"))
	bool bScanDeveloperContents = false;

	UPROPERTY(DisplayName = "Delete Empty Folders After Assets Deleted", EditAnywhere, Config, Category = "ScanSettings")
	bool bAutomaticallyDeleteEmptyFolders = true;

	UPROPERTY(DisplayName = "Paths", EditAnywhere, Config, Category = "ScanSettings|ExcludeOptions", meta = (ContentDir))
	TArray<FDirectoryPath> Paths;

	UPROPERTY(DisplayName = "Classes", EditAnywhere, Config, Category = "ScanSettings|ExcludeOptions")
	TArray<UClass*> Classes;
};
