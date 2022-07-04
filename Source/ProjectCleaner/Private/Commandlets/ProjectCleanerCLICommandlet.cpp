// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerCLICommandlet.h"
#include "Misc/ScopedSlowTask.h"
#include "Kismet/KismetStringLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Core/ProjectCleanerDataManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogProjectCleanerCLI, Display, All);

UProjectCleanerCLICommandlet::UProjectCleanerCLICommandlet()
{
	IsServer = false;
}

int32 UProjectCleanerCLICommandlet::Main(const FString& Params)
{
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("===================================="));
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("=======  ProjectCleaner CLI  ======="));
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("===================================="));

	ParseCommandLinesArguments(Params);

	if (ProjectHasRedirectors())
	{
		UE_LOG(LogProjectCleanerCLI, Error, TEXT("Project contains redirectors. Please fix up them before procceding."));
		UE_LOG(LogProjectCleanerCLI, Error, TEXT("Run 'UE4Editor-Cmd.exe <GameName or uproject> -run=ResavePackages -fixupredirects -autocheckout -projectonly -unattended'"));
		return 1;
	}
	
	if (IsArgumentsValid())
	{
		FProjectCleanerDataManager CleanerDataManager;
		CleanerDataManager.SetSilentMode(true);
		CleanerDataManager.SetUserExcludedAssets(ExcludedAssets);
		CleanerDataManager.SetExcludePaths(ExcludedPaths);
		CleanerDataManager.SetExcludeClasses(ExcludedClasses);
		CleanerDataManager.SetScanDeveloperContents(bScanDeveloperContents);
		CleanerDataManager.AnalyzeProject();
		
		UE_LOG(LogProjectCleanerCLI, Display, TEXT("===================================="));
		UE_LOG(LogProjectCleanerCLI, Display, TEXT("========= Statistics    ============"));
		UE_LOG(LogProjectCleanerCLI, Display, TEXT("===================================="));
		CleanerDataManager.PrintInfo();
		UE_LOG(LogProjectCleanerCLI, Display, TEXT("===================================="));
		UE_LOG(LogProjectCleanerCLI, Display, TEXT(""));

		if (bCheckOnly)
		{
			return 0;
		}
		
		UE_LOG(LogProjectCleanerCLI, Display, TEXT("Deleted: %d assets"), CleanerDataManager.DeleteAllUnusedAssets());
		if (bAutomaticallyDeleteEmptyFolders)
		{
			UE_LOG(LogProjectCleanerCLI, Display, TEXT("Deleted: %d empty folders"), CleanerDataManager.DeleteEmptyFolders());
		}
		UE_LOG(LogProjectCleanerCLI, Display, TEXT("===================================="));
		UE_LOG(LogProjectCleanerCLI, Display, TEXT("=========  After Cleanup    ========"));
		UE_LOG(LogProjectCleanerCLI, Display, TEXT("===================================="));
		CleanerDataManager.PrintInfo();
		UE_LOG(LogProjectCleanerCLI, Display, TEXT("===================================="));
		
	}
	
	return 0;
}

void UProjectCleanerCLICommandlet::ParseCommandLinesArguments(const FString& Params)
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> Parameters;
	ParseCommandLine(*Params, Tokens, Switches, Parameters);

	// CLI - arguments
	// -Check
	// -ScanDevContent
	// -DeleteEmptyFolders
	// -ExcludeAssets= /Game/Blueprint/aaa.uasset
	// -ExcludeAssetsInPath = /Game/Blueprint/
	// -ExcludeAssetWithClass= UBlueprint,UMaterial

	// if no argument given then we set default scenario
	// -Check - false
	// -ScanDevContent - false
	// -DeleteEmptyFolders - true
	// -ExcludeAssets - empty
	// -ExcludeAssetsInPath - empty 
	// -ExcludeAssetWithClass - empty
	if (Switches.Num() == 0 && Parameters.Num() == 1 && Tokens.Num() == 0) // Parameters contain -run=ProjectCleanerCLI - argument only
	{
		bArgumentsValid = true;
		bCheckOnly = false;
		bScanDeveloperContents = false;
		bAutomaticallyDeleteEmptyFolders = true;
		ExcludedAssets.Empty();
		ExcludedPaths.Empty();
		ExcludedClasses.Empty();

		ShowArgumentsInLog();
		
		return;
	}
	
	for (const auto& Switch : Switches)
	{
		if (Switch.Equals(TEXT("Check"), ESearchCase::IgnoreCase))
		{
			bCheckOnly = true;
		}

		if (Switch.Equals(TEXT("ScanDevContent"), ESearchCase::IgnoreCase))
		{
			bScanDeveloperContents = true;
		}

		if (Switch.Equals(TEXT("DeleteEmptyFolders"), ESearchCase::IgnoreCase))
		{
			bAutomaticallyDeleteEmptyFolders = true;
		}
	}
	
	FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	TSet<FString> InvalidObjectPaths;
	TSet<FString> InvalidPaths;
	TSet<FString> InvalidClasses;
	
	for (const auto& Param : Parameters)
	{
		if (Param.Key.Equals(TEXT("ExcludeAssets"), ESearchCase::IgnoreCase))
		{
			// parsing string arguments to array
			TArray<FString> ParsedArray;
			Param.Value.ParseIntoArray(ParsedArray, TEXT(","), true);

			for (const auto& ObjectPath : ParsedArray)
			{
				// checking if given asset ObjectPath are valid
				const FAssetData AssetData = AssetRegistry.Get().GetAssetByObjectPath(FName{*ObjectPath});
				if (AssetData.IsValid())
				{
					ExcludedAssets.AddUnique(ObjectPath);
				}
				else
				{
					InvalidObjectPaths.Add(ObjectPath);
				}
			}
		}

		if (Param.Key.Equals(TEXT("ExcludeAssetsInPath"), ESearchCase::IgnoreCase))
		{
			// parsing string arguments to array
			TArray<FString> ParsedArray;
			Param.Value.ParseIntoArray(ParsedArray, TEXT(","), true);

			for (const auto& Path : ParsedArray)
			{
				if (AssetRegistry.Get().PathExists(Path))
				{
					ExcludedPaths.AddUnique(Path);
				}
				else
				{
					InvalidPaths.Add(Path);
				}
			}
		}

		if (Param.Key.Equals(TEXT("ExcludeAssetsWithClass"), ESearchCase::IgnoreCase))
		{
			TArray<FString> ParsedArray;
			Param.Value.ParseIntoArray(ParsedArray, TEXT(","), true);

			for (const auto& ClassName : ParsedArray)
			{
				TArray<FAssetData> Assets;
				AssetRegistry.Get().GetAssetsByClass(FName{*ClassName}, Assets, true);

				if (Assets.Num() > 0)
				{
					ExcludedClasses.AddUnique(ClassName);
				}
				else
				{
					InvalidClasses.Add(ClassName);
				}
			}
		}
	}

	if (InvalidObjectPaths.Num() > 0 || InvalidPaths.Num() > 0 || InvalidClasses.Num() > 0)
	{
		UE_LOG(LogProjectCleanerCLI, Display, TEXT(""));
		UE_LOG(LogProjectCleanerCLI, Display, TEXT(""));
		UE_LOG(LogProjectCleanerCLI, Display, TEXT("=========================================="));
		UE_LOG(LogProjectCleanerCLI, Display, TEXT("        Invalid Arguments Detected !      "));
		UE_LOG(LogProjectCleanerCLI, Display, TEXT("=========================================="));
		
		bArgumentsValid = false;
		for (const auto& InvalidObjectPath : InvalidObjectPaths)
		{
			UE_LOG(LogProjectCleanerCLI, Error, TEXT("%s - Invalid ObjectPath"), *InvalidObjectPath);
		}

		for (const auto& InvalidPath : InvalidPaths)
		{
			UE_LOG(LogProjectCleanerCLI, Error, TEXT("%s - Invalid path.Does not exists in AssetRegistry"), *InvalidPath);
		}

		for (const auto& InvalidClass : InvalidClasses)
		{
			UE_LOG(LogProjectCleanerCLI, Error, TEXT("%s - Invalid path.Does not exists in AssetRegistry"), *InvalidClass);
		}

		UE_LOG(LogProjectCleanerCLI, Display, TEXT(""));
		UE_LOG(LogProjectCleanerCLI, Display, TEXT(""));
		UE_LOG(LogProjectCleanerCLI, Warning, TEXT("Tip: ObjectPaths must be of format /Game/Materials/NewMaterial.NewMaterial"));
		UE_LOG(LogProjectCleanerCLI, Warning, TEXT("Tip: Paths must be of format /Game/Materials"));
		UE_LOG(LogProjectCleanerCLI, Warning, TEXT("Tip: Classes must be of format Material, Texture2D, Blueprint, ParticleSystem etc."));
	}
	else
	{
		bArgumentsValid = true;
	}

	ShowArgumentsInLog();
}

bool UProjectCleanerCLICommandlet::IsArgumentsValid() const
{
	return bArgumentsValid;
}

void UProjectCleanerCLICommandlet::ShowArgumentsInLog()
{
	UE_LOG(LogProjectCleanerCLI, Display, TEXT(""));
	UE_LOG(LogProjectCleanerCLI, Display, TEXT(""));
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("CLI arguments - %s"), IsArgumentsValid() ? TEXT("OK") : TEXT("Invalid"));
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("	Check [Just show information, no actions performed] - %s"), bCheckOnly ? TEXT("True") : TEXT("False"));
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("	ScanDevContent [Scan Developers Folder] - %s"), bScanDeveloperContents ? TEXT("True") : TEXT("False"));
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("	DeleteEmptyFolders [Automatically delete all empty folders after assets deleted] - %s"), bAutomaticallyDeleteEmptyFolders ? TEXT("True") : TEXT("False"));
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("	ExcludeAssets [Assets paths to exclude from scanning] - %s"), ExcludedAssets.Num() > 0 ? *UKismetStringLibrary::JoinStringArray(ExcludedAssets, TEXT(",")) : TEXT("[]"));
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("	ExcludeAssetsInPath [Paths to exclude from scanning] - %s"), ExcludedPaths.Num() > 0 ? *UKismetStringLibrary::JoinStringArray(ExcludedPaths, TEXT(",")) : TEXT("[]"));
	UE_LOG(LogProjectCleanerCLI, Display, TEXT("	ExcludeAssetsWithClass [Asset Classes to exclude from scanning] - %s"), ExcludedClasses.Num() > 0 ? *UKismetStringLibrary::JoinStringArray(ExcludedClasses, TEXT(",")) : TEXT("[]"));
}

bool UProjectCleanerCLICommandlet::ProjectHasRedirectors() const
{
	const auto AssetRegistry = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
 	TArray<FAssetData> RedirectAssets;
 	AssetRegistry->Get().GetAssetsByClass(UObjectRedirector::StaticClass()->GetFName(), RedirectAssets);

	return RedirectAssets.Num() > 0;
}
