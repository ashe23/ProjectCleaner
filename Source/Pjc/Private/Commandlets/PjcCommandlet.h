// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "PjcCommandlet.generated.h"

UCLASS()
class UPjcCommandlet : public UCommandlet
{
	GENERATED_BODY()

	struct FCleanupStats
	{
		int32 NumAssetsAll = 0;
		int32 NumAssetsUsed = 0;
		int32 NumAssetsUnused = 0;
		int32 NumFoldersEmpty = 0;
		int32 NumFilesExternal = 0;
		int32 NumFilesCorrupted = 0;
	};

public:
	UPjcCommandlet();
	virtual int32 Main(const FString& Params) override;

private:
	void ParseCommandLinesArguments(const FString& Params);
	void StatsPrint(const FCleanupStats& Stats);

	bool bScanOnly = false;
	bool bFullCleanup = false;
	bool bDeleteAssetsUnused = false;
	bool bDeleteFoldersEmpty = false;
	bool bDeleteFilesExternal = false;
	bool bDeleteFilesCorrupted = false;
};
