// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.h"
#include "ProjectCleanerSubsystem.generated.h"

UCLASS(Config=EditorPerProjectUserSettings)
class UProjectCleanerSubsystem final : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns all non engine files inside Content folder"))
	static void GetFilesNonEngine(TSet<FString>& FilesNonEngine);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns all corrupted files inside Content folder"))
	static void GetFilesCorrupted(TSet<FString>& FilesCorrupted);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns all indirectly used assets inside Content folder"))
	static void GetAssetsIndirect(TArray<FProjectCleanerIndirectAsset>& AssetsIndirect);
};
