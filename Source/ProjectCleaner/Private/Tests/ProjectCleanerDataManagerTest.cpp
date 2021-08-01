// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "CoreTypes.h"
#include "ObjectTools.h"
#include "Misc/AutomationTest.h"
#include "Core/ProjectCleanerDataManager.h"
#include "Core/ProjectCleanerUtility.h"
#include "StructsContainer.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "AssetRegistry/AssetRegistryModule.h"
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
		FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*(AutomationRootFolder_Abs + TEXT("Collections/")));
		FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*(AutomationRootFolder_Abs + TEXT("Developers/")));
		FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*(AutomationRootFolder_Abs + TEXT("Developers/") + FPaths::GameUserDeveloperFolderName() + TEXT("/")));
		FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*(AutomationRootFolder_Abs + TEXT("Developers/") + FPaths::GameUserDeveloperFolderName() + TEXT("/Collections/")));
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

	bool ContainsForbiddenFolders(const TSet<FName>& EmptyFolders)
	{
		const FString CollectionsFolder = AutomationRootFolder_Abs + TEXT("Collections/");
		const FString DevelopersFolder = AutomationRootFolder_Abs + TEXT("Developers/");
		const FString UserDir = DevelopersFolder + FPaths::GameUserDeveloperFolderName() + TEXT("/");
		const FString UserCollectionsDir = UserDir + TEXT("Collections/");
		
		return
		EmptyFolders.Contains(FName{*CollectionsFolder}) ||
		EmptyFolders.Contains(FName{*DevelopersFolder}) ||
		EmptyFolders.Contains(FName{*UserDir}) ||
		EmptyFolders.Contains(FName{*UserCollectionsDir});
	}
};
	
bool FProjectCleanerDataManagerTest::RunTest(const FString& Parameters)
{
	using namespace ProjectCleanerTestHelper;
	
	// =========================
	// GetAllAssetsByPath Tests
	// =========================
	
	Init();

	TArray<FAssetData> AllAssets;
	ProjectCleanerDataManager::GetAllAssetsByPath(AutomationRootFolder_Rel, AllAssets);
	TestEqual("[GetAllAssetsByPath] All assets must ", AllAssets.Num(), 0);

	// Creating test assets
	CreateAsset<UMaterial, UMaterialFactoryNew>(
		TEXT("M_TestMaterial"),
		AutomationRootFolder_Rel.ToString(),
		UMaterial::StaticClass()
	);

	AllAssets.Reset();
	ProjectCleanerDataManager::GetAllAssetsByPath(AutomationRootFolder_Rel, AllAssets);
	TestEqual("[GetAllAssetsByPath] All assets must ", AllAssets.Num(), 1);

	ProjectCleanerUtility::DeleteAssets(AllAssets);
	// Testing with lots of assets
	for (int32 i = 0; i < 100; ++i)
	{
		CreateAsset<UMaterial, UMaterialFactoryNew>(
		FString::Printf(TEXT("M_TestMaterial_%d"), i),
		AutomationRootFolder_Rel.ToString(),
			UMaterial::StaticClass()
		);
	}
	
	AllAssets.Reset();
	ProjectCleanerDataManager::GetAllAssetsByPath(AutomationRootFolder_Rel, AllAssets);
	TestEqual("[GetAllAssetsByPath] All assets must ", AllAssets.Num(), 100);
	ProjectCleanerUtility::DeleteAssets(AllAssets);

	Cleanup();


	// ===========================
	// GetInvalidFilesByPath Tests
	// ===========================

	Init();

	ProjectCleanerDataManager::GetAllAssetsByPath(AutomationRootFolder_Rel, AllAssets);
	TSet<FName> CorruptedAssets;
	TSet<FName> NonEngineFiles;
	ProjectCleanerDataManager::GetInvalidFilesByPath(AutomationRootFolder_Abs, AllAssets, CorruptedAssets, NonEngineFiles);

	TestEqual("[GetInvalidFilesByPath] Corrupted assets must ", CorruptedAssets.Num(), 0);
	TestEqual("[GetInvalidFilesByPath] NonEngineFiles must ", NonEngineFiles.Num(), 0);

	CreateCorruptedAssets();
	CreateNonEngineFiles();
	ProjectCleanerDataManager::GetInvalidFilesByPath(AutomationRootFolder_Abs, AllAssets, CorruptedAssets, NonEngineFiles);

	TestEqual("[GetInvalidFilesByPath] Corrupted assets must ", CorruptedAssets.Num(), 4);
	TestEqual("[GetInvalidFilesByPath] NonEngineFiles must ", NonEngineFiles.Num(), 7);
	
	Cleanup();

	// =============================
	// GetIndirectAssetsByPath Tests
	// =============================
	
	const FString ConfigFileContent = TEXT(R"(
[/Script/EngineSettings.GameMapsSettings]
GameDefaultMap=/Game/Maps/NewMap.NewMap
EditorStartupMap=/Game/Maps/NewMap.NewMap

[/Script/HardwareTargeting.HardwareTargetingSettings]
TargetedHardwareClass=Desktop
AppliedTargetedHardwareClass=Desktop
DefaultGraphicsPerformance=Maximum
AppliedDefaultGraphicsPerformance=Maximum

[/Script/Engine.Engine]
+ActiveGameNameRedirects=(OldGameName="TP_Blank",NewGameName="/Script/PluginDev426")
+ActiveGameNameRedirects=(OldGameName="/Script/TP_Blank",NewGameName="/Script/PluginDev426")
+ActiveClassRedirects=(OldClassName="TP_BlankGameModeBase",NewClassName="PluginDev426GameModeBase")"
);

	const FString SourceFileContent = TEXT(R"(
		static ConstructorHelpers::FObjectFinder<UMaterial> Material(TEXT("Material'/Game/NewMaterial.NewMaterial'"));
		static ConstructorHelpers::FObjectFinder<UMaterial> Material(TEXT("/Game/NewMaterial1.NewMaterial1"));
		static ConstructorHelpers::FObjectFinder<UStaticMesh> Material(TEXT("StaticMesh'/Game/SM_Monkey.SM_Monkey'"));
		static ConstructorHelpers::FObjectFinder<UStaticMesh> Material(TEXT("/Game/SM_Monkey1.SM_Monkey1"));


		// Material'/Game/NewFolder/TestMat.TestMat'
	)");
	

	// Create Plugins Dir hierarchy
	const FString PluginSourceFileContent = TEXT(R"(
		static ConstructorHelpers::FObjectFinder<UMaterial> Material(TEXT("Material'/Game/NewMaterial23.NewMaterial23'"));
		static ConstructorHelpers::FObjectFinder<UStaticMesh> Material(TEXT("StaticMesh'/Game/SM_Monkey24.SM_Monkey24'"));
		// Material'/Game/NewFolder/TestMat25.TestMat25'
	)");
	const FString PluginConfigFileContent = TEXT(R"(
[FilterPlugin]
; This section lists additional files which will be packaged along with your plugin. Paths should be listed relative to the root plugin directory, and
; may include "...", "*", and "?" wildcards to match directories, files, and individual characters respectively.
;
; Examples:
;    /README.txt
;    /Extras/...
;    /Binaries/ThirdParty/*.dll
;    /Game/Blueprints/SomeBP.SomeBP
/Game/Test/SomeAsset.SomeAsset
)");

	TestTrue("[HasIndirectlyUsedAssets] must ", ProjectCleanerDataManager::HasIndirectlyUsedAssets(SourceFileContent));
	TestTrue("[HasIndirectlyUsedAssets] must ", ProjectCleanerDataManager::HasIndirectlyUsedAssets(ConfigFileContent));
	TestTrue("[HasIndirectlyUsedAssets] must ", ProjectCleanerDataManager::HasIndirectlyUsedAssets(PluginSourceFileContent));
	TestTrue("[HasIndirectlyUsedAssets] must ", ProjectCleanerDataManager::HasIndirectlyUsedAssets(PluginConfigFileContent));
	TestFalse("[HasIndirectlyUsedAssets] must ", ProjectCleanerDataManager::HasIndirectlyUsedAssets(TEXT("")));
	TestFalse("[HasIndirectlyUsedAssets] must ", ProjectCleanerDataManager::HasIndirectlyUsedAssets(TEXT("asdfqwer")));
	

	// ======================
	// GetEmptyFolders Tests
	// ======================
	
	Init();

	TSet<FName> EmptyFolders;

	// Tests when Scanning of Developer contents enabled
	bool bScanDevContent = true;
	ProjectCleanerDataManager::GetEmptyFolders(AutomationRootFolder_Abs,EmptyFolders, bScanDevContent);
	
	// #TESTCASE 1 [Empty folders][No Folders]
	TestEqual(TEXT("[Empty folders][No Folders] count"), EmptyFolders.Num(), 0);
 
	// #TESTCASE 2 [Empty folders][Single empty folder]
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*(AutomationRootFolder_Abs + TEXT("ef1/")));
	
	ProjectCleanerDataManager::GetEmptyFolders(AutomationRootFolder_Abs,EmptyFolders, bScanDevContent);
	
	TestEqual(TEXT("Empty folders count"), EmptyFolders.Num(), 1);
	TestFalse(TEXT("Empty folders must not contain forbidden folders"), ContainsForbiddenFolders(EmptyFolders));

	Cleanup();
	Init();
	
	// #TESTCASE 3 [Empty folders][Multiple single empty folders]
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*(AutomationRootFolder_Abs + TEXT("ef1/")));
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*(AutomationRootFolder_Abs + TEXT("ef2/")));
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*(AutomationRootFolder_Abs + TEXT("ef3/")));
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*(AutomationRootFolder_Abs + TEXT("ef4/")));
	
	ProjectCleanerDataManager::GetEmptyFolders(AutomationRootFolder_Abs,EmptyFolders, bScanDevContent);
	
	TestEqual(TEXT("[Empty folders][Multiple single empty folders] count"), EmptyFolders.Num(), 4);
	TestFalse(TEXT("Empty folders must not contain forbidden folders"), ContainsForbiddenFolders(EmptyFolders));

	Cleanup();
	Init();

	
	// #TESTCASE 4 [Empty folders][Single Empty folders with nested empty folders]
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(AutomationRootFolder_Abs + TEXT("ef1/sef1/ssef1/sssef1/"))));
	
	ProjectCleanerDataManager::GetEmptyFolders(AutomationRootFolder_Abs,EmptyFolders, bScanDevContent);
	
	TestEqual(TEXT("[Empty folders][Single Empty folders with nested empty folders] count"), EmptyFolders.Num(), 4);
	TestFalse(TEXT("Empty folders must not contain forbidden folders"), ContainsForbiddenFolders(EmptyFolders));
	
	Cleanup();
	Init();
	
	// #TESTCASE 5 [Empty folders][Multiple Empty folders with nested empty folders]
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(AutomationRootFolder_Abs + TEXT("ef1/sef1/ssef1/sssef1/"))));
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(AutomationRootFolder_Abs + TEXT("ef2/sef2/ssef2/sssef2/"))));
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(AutomationRootFolder_Abs + TEXT("ef3/sef3/"))));

	ProjectCleanerDataManager::GetEmptyFolders(AutomationRootFolder_Abs,EmptyFolders, bScanDevContent);
	
	TestEqual(TEXT("[Empty folders][Multiple Empty folders with nested empty folders] count"), EmptyFolders.Num(), 10);
	TestFalse(TEXT("Empty folders must not contain forbidden folders"), ContainsForbiddenFolders(EmptyFolders));

	Cleanup();
	Init();
	
	// #TESTCASE 6 [Empty folders][Multiple Empty folders with nested empty folders more complex case]
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(AutomationRootFolder_Abs + TEXT("ef1/sef1/ssef1/sssef1"))));
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(AutomationRootFolder_Abs + TEXT("ef2/sef2"))));
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(AutomationRootFolder_Abs + TEXT("ef3/sef3//ssef3"))));
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(AutomationRootFolder_Abs + TEXT("ef4"))));
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(AutomationRootFolder_Abs + TEXT("ef5/sef5/ssef5/sssef5/ssssef5"))));
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(AutomationRootFolder_Abs + TEXT("ef6"))));
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(AutomationRootFolder_Abs + TEXT("ef7"))));
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(AutomationRootFolder_Abs + TEXT("ef8"))));

	ProjectCleanerDataManager::GetEmptyFolders(AutomationRootFolder_Abs,EmptyFolders, bScanDevContent);
	
	TestEqual(TEXT("[Empty folders][Multiple Empty folders with nested empty folders more complex case] count"), EmptyFolders.Num(), 18);
	TestFalse(TEXT("Empty folders must not contain forbidden folders"), ContainsForbiddenFolders(EmptyFolders));

	Cleanup();
	Init();
	
	// #TESTCASE 7 [Empty folders][Multiple Empty folders with nested empty folders, nested level 30]
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(AutomationRootFolder_Abs + TEXT("ef1/ef1/ef1/ef1/ef1/ef1/ef1/ef1/ef1/ef1//ef1/ef1/ef1/ef1/ef1//ef1/ef1/ef1/ef1/ef1//ef1/ef1/ef1/ef1/ef1//ef1/ef1/ef1/ef1/ef1/"))));
	
	ProjectCleanerDataManager::GetEmptyFolders(AutomationRootFolder_Abs,EmptyFolders, bScanDevContent);
	
    TestEqual(TEXT("[Empty folders][Multiple Empty folders with nested empty folders, nested level 30] count"), EmptyFolders.Num(), 30);
	TestFalse(TEXT("Empty folders must not contain forbidden folders"), ContainsForbiddenFolders(EmptyFolders));

	Cleanup();
	Init();

	// ===================================================
	// Tests when Scanning of Developer contents disabled
	bScanDevContent = false;
 
	// #TESTCASE 1 [Empty folders][Single Folder in developer folder]
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectory((*(AutomationRootFolder_Abs + TEXT("Developers/ef1/"))));
	
	ProjectCleanerDataManager::GetEmptyFolders(AutomationRootFolder_Abs,EmptyFolders, bScanDevContent);

	TestEqual(TEXT("[Empty folders][Single Folders][Dev folder disabled] count"), EmptyFolders.Num(), 0);

	Cleanup();
	Init();
	
	// #TESTCASE 2 [Empty folders][Single Folder with Nested Empty folders in developer folder]
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(AutomationRootFolder_Abs + TEXT("Developers/ef1/ef2/ef3/"))));
	
	ProjectCleanerDataManager::GetEmptyFolders(AutomationRootFolder_Abs,EmptyFolders, bScanDevContent);
	
	TestEqual(TEXT("[Empty folders][Single Folder with Nested Empty folders in developer folder] count"), EmptyFolders.Num(), 0);

	Cleanup();
	Init();
	
	// #TESTCASE 3 [Empty folders][Single Folder with Nested Empty folders in developer folder and content folder]
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectory((*(AutomationRootFolder_Abs + TEXT("Developers/ef1/"))));
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectory((*(AutomationRootFolder_Abs + TEXT("ef2/"))));

	ProjectCleanerDataManager::GetEmptyFolders(AutomationRootFolder_Abs,EmptyFolders, bScanDevContent);
	
	TestEqual(TEXT("[Empty folders][Single Folder with Nested Empty folders in developer folder] count"), EmptyFolders.Num(), 1);

	Cleanup();
	Init();
 
	// #TESTCASE 4 [Empty folders][Multiple Folders with Nested Empty folders in developer folder and content folder]
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(AutomationRootFolder_Abs + TEXT("Developers/ef1/ef2/ef3/"))));
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(AutomationRootFolder_Abs + TEXT("Developers/ef1/ef4/"))));
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(AutomationRootFolder_Abs + TEXT("Developers/ef1/ef5/ef6/"))));
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(AutomationRootFolder_Abs + TEXT("Developers/ef7/ef8/"))));
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(AutomationRootFolder_Abs + TEXT("ef9/ef10/"))));
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(AutomationRootFolder_Abs + TEXT("ef11/ef12/ef13/"))));

	ProjectCleanerDataManager::GetEmptyFolders(AutomationRootFolder_Abs,EmptyFolders, bScanDevContent);
	
	TestEqual(TEXT("[Empty folders][Multiple Folders with Nested Empty folders in developer folder and content folder] count"), EmptyFolders.Num(), 5);
 
	Cleanup();
	
	return true;
}
#endif