// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "PjcCommandlet.generated.h"

struct FAssetData;

/**
 * UnrealEditor-Cmd.exe <uproject> -run=Pjc [switches]
 *
 *   -scan_only               print stats only
 *   -full_cleanup            all delete_* operations below
 *   -delete_assets_unused
 *   -delete_folders_empty
 *   -delete_files_external
 *   -delete_files_corrupted
 *   -fix_redirectors         fix redirectors first instead of stopping
 *   -list                    print found items, not only counts
 *
 * Returns 0 on success, 1 when project has redirectors or something failed to delete.
 */
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
	FCleanupStats StatsCollect(const bool bPrintLists);
	void StatsPrint(const FCleanupStats& Stats);
	void ListPrint(const FString& Title, const TArray<FAssetData>& Assets);
	void ListPrint(const FString& Title, const TArray<FString>& Items);

	bool bScanOnly = false;
	bool bFullCleanup = false;
	bool bDeleteAssetsUnused = false;
	bool bDeleteFoldersEmpty = false;
	bool bDeleteFilesExternal = false;
	bool bDeleteFilesCorrupted = false;
	bool bFixRedirectors = false;
	bool bList = false;
};
