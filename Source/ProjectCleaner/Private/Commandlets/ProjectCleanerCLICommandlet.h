// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "ProjectCleanerCLICommandlet.generated.h"

/**
 * Commandlet for cleaning project via CLI
 */
UCLASS()
class PROJECTCLEANER_API UProjectCleanerCLICommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	UProjectCleanerCLICommandlet();
	virtual int32 Main(const FString& Params) override;

private:
	void ParseCommandLinesArguments(const FString& Params);
	bool IsArgumentsValid() const;
	void ShowArgumentsInLog();
	bool ProjectHasRedirectors() const;

	bool bArgumentsValid = false;
	bool bCheckOnly = false;
	bool bScanDeveloperContents;
	bool bAutomaticallyDeleteEmptyFolders;

	TArray<FString> ExcludedAssets;
	TArray<FString> ExcludedPaths;
	TArray<FString> ExcludedClasses;
};
