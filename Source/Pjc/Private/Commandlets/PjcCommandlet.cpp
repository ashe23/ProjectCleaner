// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Commandlets/PjcCommandlet.h"

#include "PjcSubsystem.h"
#include "PjcTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogProjectCleanerCLI, Display, All);

UPjcCommandlet::UPjcCommandlet()
{
	IsServer = false;
	LogToConsole = false;
	ShowErrorCount = false;
	ShowProgress = false;
}

int32 UPjcCommandlet::Main(const FString& Params)
{
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("===================================="));
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("=======  ProjectCleaner CLI  ======="));
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("===================================="));

	ParseCommandLinesArguments(Params);

	// cli arguments
	//- scan_only
	//- delete_assets_unused
	//- delete_folders_empty
	//- delete_files_external
	//- delete_files_corrupted

	if (UPjcSubsystem::ProjectContainsRedirectors())
	{
		UE_LOG(LogProjectCleanerCLI, Warning, TEXT("Project contains redirectors that must be fixed first."));
		UE_LOG(LogProjectCleanerCLI, Warning, TEXT("Please run following command before doing any cleaning operations."));
		UE_LOG(LogProjectCleanerCLI, Warning, TEXT("	UE4Editor.exe <GameName or uproject> -run=ResavePackages -fixupredirects -autocheckout -projectonly -unattended"));

		return 0;
	}


	TArray<FAssetData> AssetsAll;
	TArray<FAssetData> AssetsUsed;
	TArray<FAssetData> AssetsUnused;
	TArray<FString> FoldersEmpty;
	TArray<FString> FilesExternal;
	TArray<FString> FilesCorrupted;

	UPjcSubsystem::GetAssetsAll(AssetsAll);
	UPjcSubsystem::GetAssetsUsed(AssetsUsed);
	UPjcSubsystem::GetAssetsUnused(AssetsUnused);
	UPjcSubsystem::GetFoldersEmpty(FoldersEmpty);
	UPjcSubsystem::GetFilesExternalFiltered(FilesExternal);
	UPjcSubsystem::GetFilesCorrupted(FilesCorrupted);

	const FCleanupStats StatsBefore{
		AssetsAll.Num(),
		AssetsUsed.Num(),
		AssetsUnused.Num(),
		FoldersEmpty.Num(),
		FilesExternal.Num(),
		FilesCorrupted.Num()
	};


	UE_LOG(LogProjectCleanerCLI, Display, TEXT("======================="));
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("=====  Scan Info   ===="));
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("======================="));
	StatsPrint(StatsBefore);

	if (bScanOnly) return 0;

	if (bDeleteAssetsUnused)
	{
		UPjcSubsystem::DeleteAssetsUnused();
	}

	if (bDeleteFilesExternal)
	{
		UPjcSubsystem::DeleteFilesExternal();
	}

	if (bDeleteFilesCorrupted)
	{
		UPjcSubsystem::DeleteFilesCorrupted();
	}

	if (bDeleteFoldersEmpty)
	{
		UPjcSubsystem::DeleteFoldersEmpty();
	}

	if (bDeleteAssetsUnused || bDeleteFilesExternal || bDeleteFilesCorrupted || bDeleteFoldersEmpty)
	{
		UPjcSubsystem::GetAssetsAll(AssetsAll);
		UPjcSubsystem::GetAssetsUsed(AssetsUsed);
		UPjcSubsystem::GetAssetsUnused(AssetsUnused);
		UPjcSubsystem::GetFoldersEmpty(FoldersEmpty);
		UPjcSubsystem::GetFilesExternalFiltered(FilesExternal);
		UPjcSubsystem::GetFilesCorrupted(FilesCorrupted);

		const FCleanupStats StatsAfter{
			AssetsAll.Num(),
			AssetsUsed.Num(),
			AssetsUnused.Num(),
			FoldersEmpty.Num(),
			FilesExternal.Num(),
			FilesCorrupted.Num()
		};

		UE_LOG(LogProjectCleanerCLI, Display, TEXT("======================="));
		UE_LOG(LogProjectCleanerCLI, Display, TEXT("====  Cleanup Info  ==="));
		UE_LOG(LogProjectCleanerCLI, Display, TEXT("======================="));
		StatsPrint(StatsAfter);
	}

	return 0;
}

void UPjcCommandlet::ParseCommandLinesArguments(const FString& Params)
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> Parameters;
	ParseCommandLine(*Params, Tokens, Switches, Parameters);

	for (const auto& Switch : Switches)
	{
		if (Switch.Equals(TEXT("scan_only")))
		{
			bScanOnly = true;
			break;
		}

		if (Switch.Equals(TEXT("delete_assets_unused")))
		{
			bDeleteAssetsUnused = true;
		}

		if (Switch.Equals(TEXT("delete_folders_empty")))
		{
			bDeleteFoldersEmpty = true;
		}

		if (Switch.Equals(TEXT("delete_files_external")))
		{
			bDeleteFilesExternal = true;
		}

		if (Switch.Equals(TEXT("delete_files_corrupted")))
		{
			bDeleteFilesCorrupted = true;
		}
	}
}

void UPjcCommandlet::StatsPrint(const FCleanupStats& Stats)
{
	const float UnusedPercent = Stats.NumAssetsAll == 0 ? 0.0f : static_cast<float>(Stats.NumAssetsUnused) / Stats.NumAssetsAll * 100.0f;

	UE_LOG(LogProjectCleanerCLI, Display, TEXT("Assets All - %d"), Stats.NumAssetsAll);
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("Assets Used - %d"), Stats.NumAssetsUsed);
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("Assets Unused - %d"), Stats.NumAssetsUnused);
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("Unused %% - %.1f"), UnusedPercent);
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("================="));
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("Files External - %d"), Stats.NumFilesExternal);
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("Files Corrupted - %d"), Stats.NumFilesCorrupted);
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("Folders Empty - %d"), Stats.NumFoldersEmpty);
}
