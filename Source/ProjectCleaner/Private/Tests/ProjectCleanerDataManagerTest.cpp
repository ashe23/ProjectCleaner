// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "CoreTypes.h"
#include "ObjectTools.h"
#include "Misc/AutomationTest.h"
#include "Core/ProjectCleanerDataManager.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Core/ProjectCleanerUtility.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Factories/MaterialFactoryNew.h"
#include "Tests/AutomationEditorCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProjectCleanerDataManagerTest, "ProjectCleaner.DataManager", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace ProjectCleanerTestHelper
{
	static const FString AutomationRootFolder_Abs = FPaths::ProjectContentDir() + TEXT("AutomationTests/");
	static const FName AutomationRootFolder_Rel = TEXT("/Game/AutomationTests");
	void Init()
	{
		// creating separate automation folder for testing
		FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*AutomationRootFolder_Abs);
	}

	void Cleanup()
	{
		// cleanup
		FPlatformFileManager::Get().GetPlatformFile().DeleteDirectoryRecursively(*AutomationRootFolder_Abs);
	}

	void CreateCorruptedAssets()
	{
		TArray<FString> ExpectedCorruptedFiles;
		ExpectedCorruptedFiles.Add(TEXT("TestUassetFile.uasset"));
		ExpectedCorruptedFiles.Add(TEXT("TestUmapFile.umap"));
		ExpectedCorruptedFiles.Add(TEXT("TestFolder/TestUassetFile.umap"));
		ExpectedCorruptedFiles.Add(TEXT("TestFolder/TestUmapFile.umap"));
		
		for (const auto& File : ExpectedCorruptedFiles)
		{
			const FString Path = AutomationRootFolder_Abs + File;
			FFileHelper::SaveStringToFile(TEXT(""), *Path);
		}
	}

	void CreateNonEngineFiles()
	{
		TArray<FString> ExpectedNonEngineFiles;
		ExpectedNonEngineFiles.Add(TEXT("TestTxtFile.txt"));
		ExpectedNonEngineFiles.Add(TEXT("TestMp4File.mp4"));
		ExpectedNonEngineFiles.Add(TEXT(".gitignore"));
		ExpectedNonEngineFiles.Add(TEXT("filewithoutextension"));
		ExpectedNonEngineFiles.Add(TEXT("config.ini"));
		ExpectedNonEngineFiles.Add(TEXT("TestFolder/config.ini"));
		ExpectedNonEngineFiles.Add(TEXT("TestFolder/TestTxtFile.txt"));

		for (const auto& File : ExpectedNonEngineFiles)
		{
			const FString Path = AutomationRootFolder_Abs + File;
			FFileHelper::SaveStringToFile(TEXT(""), *Path);
		}
	}

	template<typename TAssetClass, typename TFactoryClass>
	static void CreateAsset(const FString& AssetName, const FString& PackagePath, UClass* AssetClass)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
		
		const FString PackageName = PackagePath + TEXT("/") + AssetName;
		const EObjectFlags Flags = RF_Public | RF_Standalone;
		
		const auto Package = CreatePackage(*PackageName);
		if (!Package) return;
		
		UFactory* FactoryInst = NewObject<TFactoryClass>();
		if (!FactoryInst) return;
		
		const auto CreatedAsset = FactoryInst->FactoryCreateNew(AssetClass, Package, *AssetName, Flags, nullptr, GWarn);
		if (CreatedAsset)
		{
			AssetRegistryModule.AssetCreated(CreatedAsset);
			Package->SetDirtyFlag(true);
			// UPackage::SavePackage(Package, nullptr, RF_Standalone, *FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension()), GError, nullptr, false, true, SAVE_NoError);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Cant create asset"));
		}
	}
};
	
bool FProjectCleanerDataManagerTest::RunTest(const FString& Parameters)
{
	// =========================
	// GetAllAssetsByPath Tests
	// =========================
	
	ProjectCleanerTestHelper::Init();

	TArray<FAssetData> AllAssets;
	ProjectCleanerDataManagerV2::GetAllAssetsByPath(ProjectCleanerTestHelper::AutomationRootFolder_Rel, AllAssets);
	TestEqual("[GetAllAssetsByPath] All assets must ", AllAssets.Num(), 0);

	// Creating test assets
	ProjectCleanerTestHelper::CreateAsset<UMaterial, UMaterialFactoryNew>(
		TEXT("M_TestMaterial"),
		ProjectCleanerTestHelper::AutomationRootFolder_Rel.ToString(),
		UMaterial::StaticClass()
	);

	AllAssets.Reset();
	ProjectCleanerDataManagerV2::GetAllAssetsByPath(ProjectCleanerTestHelper::AutomationRootFolder_Rel, AllAssets);
	TestEqual("[GetAllAssetsByPath] All assets must ", AllAssets.Num(), 1);

	ProjectCleanerUtility::DeleteAssets(AllAssets);
	// Testing with lots of assets
	for (int32 i = 0; i < 100; ++i)
	{
		ProjectCleanerTestHelper::CreateAsset<UMaterial, UMaterialFactoryNew>(
		FString::Printf(TEXT("M_TestMaterial_%d"), i),
		ProjectCleanerTestHelper::AutomationRootFolder_Rel.ToString(),
			UMaterial::StaticClass()
		);
	}
	
	AllAssets.Reset();
	ProjectCleanerDataManagerV2::GetAllAssetsByPath(ProjectCleanerTestHelper::AutomationRootFolder_Rel, AllAssets);
	TestEqual("[GetAllAssetsByPath] All assets must ", AllAssets.Num(), 100);
	ProjectCleanerUtility::DeleteAssets(AllAssets);

	ProjectCleanerTestHelper::Cleanup();


	// ===========================
	// GetInvalidFilesByPath Tests
	// ===========================

	ProjectCleanerTestHelper::Init();

	ProjectCleanerDataManagerV2::GetAllAssetsByPath(ProjectCleanerTestHelper::AutomationRootFolder_Rel, AllAssets);
	TSet<FName> CorruptedAssets;
	TSet<FName> NonEngineFiles;
	ProjectCleanerDataManagerV2::GetInvalidFilesByPath(ProjectCleanerTestHelper::AutomationRootFolder_Abs, AllAssets, CorruptedAssets, NonEngineFiles);

	TestEqual("[GetInvalidFilesByPath] Corrupted assets must ", CorruptedAssets.Num(), 0);
	TestEqual("[GetInvalidFilesByPath] NonEngineFiles must ", NonEngineFiles.Num(), 0);

	ProjectCleanerTestHelper::CreateCorruptedAssets();
	ProjectCleanerTestHelper::CreateNonEngineFiles();
	ProjectCleanerDataManagerV2::GetInvalidFilesByPath(ProjectCleanerTestHelper::AutomationRootFolder_Abs, AllAssets, CorruptedAssets, NonEngineFiles);

	TestEqual("[GetInvalidFilesByPath] Corrupted assets must ", CorruptedAssets.Num(), 4);
	TestEqual("[GetInvalidFilesByPath] NonEngineFiles must ", NonEngineFiles.Num(), 7);
	
	ProjectCleanerTestHelper::Cleanup();
	
	return true;
}
#endif