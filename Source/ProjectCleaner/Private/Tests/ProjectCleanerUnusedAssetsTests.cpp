// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "CoreTypes.h"
#include "Misc/AutomationTest.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/MaterialFactoryNew.h"
#include "ObjectTools.h"
#include "Core/ProjectCleanerUtility.h"
#include "StructsContainer.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProjectCleanerUnusedAssetsTests, "ProjectCleaner.UnusedAssets", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)


// Handle creation and deletion of assets
struct FCleanerAssetHelper
{
	static void CreateTestAssets()
	{
		
	}
	static void Cleanup()
	{
		
	}
	
	template<typename TAssetClass, typename TFactoryClass>
	static void CreateAsset(const FString& AssetName, const FString& PackagePath, UClass* AssetClass, UClass* FactoryClass)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
		
		const FString PackageName = PackagePath + TEXT("/") + AssetName;
		const EObjectFlags Flags = RF_Public | RF_Standalone;
		
		UPackage* Package = CreatePackage(*PackageName);
		if (!Package) return;
		
		UFactory* FactoryInst = NewObject<TFactoryClass>();
		if (!FactoryInst) return;
		
		UObject* CreatedAsset = FactoryInst->FactoryCreateNew(AssetClass, Package, *AssetName, Flags, nullptr, GWarn);
		if (CreatedAsset)
		{
			AssetRegistryModule.AssetCreated(CreatedAsset);
			Package->MarkPackageDirty();
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Cant create asset"));
		}
	}
};

bool FProjectCleanerUnusedAssetsTests::RunTest(const FString& Parameters)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	const FString ProjectContentDir{"/Game"};
	auto ExcludeOptions = GetMutableDefault<UExcludeOptions>();
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	TArray<FAssetData> UnusedAssets;

	// all TEST CASES
	// Test Format
	// CreateTestAssets()
	// QueryAndTest()
	// Cleanup()
	
	/**
		1)Unused Assets must not contain any primary asset type
		2)Unused Assets must not contain any dependent assets of primary asset types (Example: using material in level, material must not be in list)
		3)Unused Assets must not contain any assets that are used indirectly (In source code or in config files)
		4)Unused Assets must not contain any assets from MegascansPluginFolder(if its active)
		5)Unused Assets must not contain any assets from DeveloperContents if 'Scan Developer Contents' checkbox is disabled
			5.1) if some assets got dependant assets that are under DeveloperContents folder, then
				5.1.1) Automatically turn 'Show DevelopersContents' flag to true
				5.1.2) Show notification to user about it
		6)Unused Assets must not contain any assets from excluded paths
		7)Unused Assets must not contain any assets from excluded classes
		8)Unused Assets must not contain any assets that are excluded by user
	**/
	
	// TEST CASE 1 all assets are under / Game folder, all filters disabled, no excluded assets, no external dependencies, no megascans plugin
	FCleanerAssetHelper::CreateAsset<UMaterial, UMaterialFactoryNew>(
		TEXT("M_TestMaterial1"),
		ProjectContentDir,
		UMaterial::StaticClass(),
		UMaterialFactoryNew::StaticClass()
	);

	ProjectCleanerUtility::GetUnusedAssets(UnusedAssets);
	TestEqual(TEXT("UnusedAssets num must"), UnusedAssets.Num(), 1);
	
	UnusedAssets.Reset();
	
	FCleanerAssetHelper::CreateAsset<UMaterial, UMaterialFactoryNew>(
		TEXT("M_TestMaterial2"),
		ProjectContentDir + TEXT("/TestFolder"),
		UMaterial::StaticClass(),
		UMaterialFactoryNew::StaticClass()
	);
	
	ProjectCleanerUtility::GetUnusedAssets(UnusedAssets);
	TestEqual(TEXT("UnusedAssets num must"), UnusedAssets.Num(), 2);

	// cleanup
	ObjectTools::DeleteAssets(UnusedAssets, false);
	PlatformFile.DeleteDirectory(*(FPaths::ProjectContentDir() + TEXT("TestFolder")));
	AssetRegistryModule.Get().RemovePath(ProjectContentDir + TEXT("/TestFolder"));

	// TEST CASE 2 all assets are under /Game folder, filter by class enabled, no excluded assets, no external deps, no megascans plugin
	UnusedAssets.Empty();
	ExcludeOptions->Classes.Add(UMaterial::StaticClass());
	FCleanerAssetHelper::CreateAsset<UMaterial, UMaterialFactoryNew>(
		TEXT("M_TestMaterial1"),
		ProjectContentDir,
		UMaterial::StaticClass(),
		UMaterialFactoryNew::StaticClass()
	);
	ProjectCleanerUtility::GetUnusedAssets(UnusedAssets);
	TestEqual(TEXT("UnusedAssets num must"), UnusedAssets.Num(), 0);

	// cleanup
	ObjectTools::DeleteAssets(UnusedAssets, false);
	
	return true;
}

#endif