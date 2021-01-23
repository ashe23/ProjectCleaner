// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * This class responsible for different file and directory operations in unreal engine context
 */
class PROJECTCLEANER_API ProjectCleanerUtility
{
public:
	static bool HasFiles(const FString& SearchPath);
	static bool HasDirectories(const FString& SearchPath);
	static bool GetAllEmptyDirectories(const FString& SearchPath, TArray<FString>& Directories, const bool bIsRootDirectory);
	static void GetChildrenDirectories(const FString& SearchPath, TArray<FString>& Output);
	static void RemoveDevsAndCollectionsDirectories(TArray<FString>& Directories);
};
