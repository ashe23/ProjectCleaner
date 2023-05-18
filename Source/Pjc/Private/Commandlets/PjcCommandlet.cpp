// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Commandlets/PjcCommandlet.h"
#include "PjcTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogProjectCleanerCLI, Display, All);

UPjcCommandlet::UPjcCommandlet()
{
	IsServer = false;
}

int32 UPjcCommandlet::Main(const FString& Params)
{
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("===================================="));
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("=======  ProjectCleaner CLI  ======="));
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("===================================="));

	// ParseCommandLinesArguments(Params);

	// cli arguments
	// -check
	// -cleanup_method=full,unused_assets_only or empty_folders_only
	// -exclude_settings=editor or custom
	// -exclude_folders=[]
	// -exclude_classes=[]
	// -exclude_assets=[]

	const UPjcAssetExcludeSettings* AssetExcludeSettings = GetDefault<UPjcAssetExcludeSettings>();
	if (!AssetExcludeSettings) return 1;

	UE_LOG(LogProjectCleanerCLI, Display, TEXT("%d"), AssetExcludeSettings->ExcludedFolders.Num());

	return 0;
}

void UPjcCommandlet::ParseCommandLinesArguments(const FString& Params)
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> Parameters;
	ParseCommandLine(*Params, Tokens, Switches, Parameters);

	UE_LOG(LogProjectCleanerCLI, Warning, TEXT("Tokes:"));
	for (const auto& Token : Tokens)
	{
		UE_LOG(LogProjectCleanerCLI, Warning, TEXT("%s"), *Token);
	}

	UE_LOG(LogProjectCleanerCLI, Warning, TEXT("Switches:"));
	for (const auto& Switch : Switches)
	{
		UE_LOG(LogProjectCleanerCLI, Warning, TEXT("%s"), *Switch);
	}

	UE_LOG(LogProjectCleanerCLI, Warning, TEXT("Params:"));
	for (const auto& Param : Parameters)
	{
		UE_LOG(LogProjectCleanerCLI, Warning, TEXT("%s - %s"), *Param.Key, *Param.Value);
	}
}
