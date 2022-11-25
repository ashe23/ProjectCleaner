// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProjectCleanerLibrary.generated.h"

UCLASS(DisplayName="ProjectCleanerLibrary", meta=(ToolTip="Project Cleaner collection of helper functions"))
class UProjectCleanerLibrary final : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="ProjectCleaner", meta=(ToolTip="Returns all subdirectories for given directory. Non Recursive. Specify Exclude Directories that you want to exclude"))
	static void GetSubDirectories(const FString& RootDir, TSet<FString>& SubDirectories, TSet<FString>& ExcludeDirectories);
};
