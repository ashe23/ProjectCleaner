// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProjectCleanerLibPath.generated.h"

UCLASS()
class UProjectCleanerLibPath final : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static FString FolderContent(const EProjectCleanerPathType PathType);
	static FString FolderDevelopers(const EProjectCleanerPathType PathType);
	static FString FolderDeveloper(const EProjectCleanerPathType PathType);
	static FString FolderCollections(const EProjectCleanerPathType PathType);
	static FString FolderDeveloperCollections(const EProjectCleanerPathType PathType);
	static FString FolderMsPresets(const EProjectCleanerPathType PathType);
	static FString Convert(const FString& InPath, const EProjectCleanerPathType ToPathType);
};
