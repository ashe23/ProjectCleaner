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
	static bool IsUnderFolder(const FString& InSearchFolderPath, const FString& InRootFolderPath);
	static bool IsUnderFolders(const FString& InSearchFolderPath, const TSet<FString>& Folders);
	static bool FileHasEngineExtension(const FString& Extension);
	static bool FileIsCorrupted(const FString& InFilePathAbs);
	static bool FileContainsIndirectAssets(const FString& FileContent);
	static int64 FilesGetTotalSize(const TArray<FString>& Files);
	static FString Normalize(const FString& InPath);
};
