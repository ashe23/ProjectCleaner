// // Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.
//
// #include "CoreTypes.h"
// #include "Misc/AutomationTest.h"
// #include "Misc/FileHelper.h"
// #include "Core/ProjectCleanerDataManager.h"
// #include "AssetRegistry/AssetRegistryModule.h"
// #include "Factories/MaterialFactoryNew.h"
// #include "UObject/Package.h"
// #include "Core/ProjectCleanerUtility.h"
//
// #if WITH_DEV_AUTOMATION_TESTS
//
// IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProjectCleanerDataManagerEmptyFoldersTests, "ProjectCleaner.EmptyFolders", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
// IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProjectCleanerDataManagerInvalidFilesTests, "ProjectCleaner.InvalidFiles", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
// IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProjectCleanerDataManagerUnusedAssetsTests, "ProjectCleaner.UnusedAssets", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
//
// DEFINE_LATENT_AUTOMATION_COMMAND(FCreateAssetCommand);
// bool FCreateAssetCommand::Update()
// {
// 	
// 	return true;
// }
//
// namespace ProjectCleanerTestsHelper
// {
// 	static const FString TestRootFolder = FPaths::ProjectContentDir() + TEXT("AutomatationTests/");
// 	static const FString CollectionsFolder = TestRootFolder + TEXT("/Collections/");
// 	static const FString DevelopersFolder = TestRootFolder + TEXT("/Developers/");
// 	static const FString UserDir = DevelopersFolder + FPaths::GameUserDeveloperFolderName() + TEXT("/");
// 	static const FString UserCollectionsDir = UserDir + TEXT("Collections/");
// 	static const FString SourceDir = TestRootFolder + TEXT("Source/");
// 	static const FString ConfigDir = TestRootFolder + TEXT("Config/");
// 	static const FString PluginsDir = TestRootFolder + TEXT("Plugins/");
// 	
// 	bool ContainsForbiddenFolders(const TSet<FName>& EmptyFolders)
// 	{
// 		return
// 		EmptyFolders.Contains(FName{*CollectionsFolder}) ||
// 		EmptyFolders.Contains(FName{*DevelopersFolder}) ||
// 		EmptyFolders.Contains(FName{*UserDir}) ||
// 		EmptyFolders.Contains(FName{*UserCollectionsDir});
// 	}
// }
//
// // Empty folders
// bool FProjectCleanerDataManagerEmptyFoldersTests::RunTest(const FString& Parameters)
// {
// 	using namespace ProjectCleanerTestsHelper;
// 	
// 	// Creating separate folder for testing
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*TestRootFolder);
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*CollectionsFolder);
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*DevelopersFolder);
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*UserDir);
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*UserCollectionsDir);
// 	//
// 	// ProjectCleanerDataManager DataManager;
// 	// DataManager.EnableTestMode();
//
// 	// ====================================================
// 	// Tests when Scanning of Developer contents enabled
// 	// ====================================================
// 	// DataManager.GetCleanerConfigs()->bScanDeveloperContents = true;
//  //
// 	//
// 	// // #TESTCASE 1 [Empty folders][No Folders]
// 	// DataManager.Update();
// 	// TestEqual(TEXT("[Empty folders][No Folders] count"), DataManager.EmptyFolders.Num(), 0);
//  //
// 	// // #TESTCASE 2 [Empty folders][Single empty folder]
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*(TestRootFolder + TEXT("/ef1")));
// 	// DataManager.Update();
// 	// TestEqual(TEXT("Empty folders count"), DataManager.EmptyFolders.Num(), 1);
// 	// TestFalse(TEXT("Empty folders must not contain forbidden folders"), ContainsForbiddenFolders(DataManager.EmptyFolders));
// 	// FPlatformFileManager::Get().GetPlatformFile().DeleteDirectory(*(TestRootFolder + TEXT("/ef1")));
// 	//
// 	// // #TESTCASE 3 [Empty folders][Multiple single empty folders]
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*(TestRootFolder + TEXT("/ef1")));
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*(TestRootFolder + TEXT("/ef2")));
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*(TestRootFolder + TEXT("/ef3")));
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*(TestRootFolder + TEXT("/ef4")));
// 	// DataManager.Update();
// 	// TestEqual(TEXT("[Empty folders][Multiple single empty folders] count"), DataManager.EmptyFolders.Num(), 4);
// 	// TestFalse(TEXT("Empty folders must not contain forbidden folders"), ContainsForbiddenFolders(DataManager.EmptyFolders));
// 	// FPlatformFileManager::Get().GetPlatformFile().DeleteDirectory(*(TestRootFolder + TEXT("/ef1")));
// 	// FPlatformFileManager::Get().GetPlatformFile().DeleteDirectory(*(TestRootFolder + TEXT("/ef2")));
// 	// FPlatformFileManager::Get().GetPlatformFile().DeleteDirectory(*(TestRootFolder + TEXT("/ef3")));
// 	// FPlatformFileManager::Get().GetPlatformFile().DeleteDirectory(*(TestRootFolder + TEXT("/ef4")));
// 	//
// 	// // #TESTCASE 4 [Empty folders][Single Empty folders with nested empty folders]
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(TestRootFolder + TEXT("/ef1/sef1/ssef1/sssef1"))));
// 	// DataManager.Update();
// 	// TestEqual(TEXT("[Empty folders][Single Empty folders with nested empty folders] count"), DataManager.EmptyFolders.Num(), 4);
// 	// TestFalse(TEXT("Empty folders must not contain forbidden folders"), ContainsForbiddenFolders(DataManager.EmptyFolders));
// 	// FPlatformFileManager::Get().GetPlatformFile().DeleteDirectoryRecursively(*(TestRootFolder + TEXT("/ef1")));
// 	//
// 	// // #TESTCASE 5 [Empty folders][Multiple Empty folders with nested empty folders]
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(TestRootFolder + TEXT("/ef1/sef1/ssef1/sssef1"))));
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(TestRootFolder + TEXT("/ef2/sef2/ssef2/sssef2"))));
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(TestRootFolder + TEXT("/ef3/sef3/"))));
// 	// DataManager.Update();
// 	// TestEqual(TEXT("[Empty folders][Multiple Empty folders with nested empty folders] count"), DataManager.EmptyFolders.Num(), 10);
// 	// TestFalse(TEXT("Empty folders must not contain forbidden folders"), ContainsForbiddenFolders(DataManager.EmptyFolders));
// 	// FPlatformFileManager::Get().GetPlatformFile().DeleteDirectoryRecursively(*(TestRootFolder + TEXT("/ef1")));
// 	// FPlatformFileManager::Get().GetPlatformFile().DeleteDirectoryRecursively(*(TestRootFolder + TEXT("/ef2")));
// 	// FPlatformFileManager::Get().GetPlatformFile().DeleteDirectoryRecursively(*(TestRootFolder + TEXT("/ef3")));
// 	//
// 	// // #TESTCASE 6 [Empty folders][Multiple Empty folders with nested empty folders more complex case]
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(TestRootFolder + TEXT("/ef1/sef1/ssef1/sssef1"))));
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(TestRootFolder + TEXT("/ef2/sef2"))));
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(TestRootFolder + TEXT("/ef3/sef3//ssef3"))));
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(TestRootFolder + TEXT("/ef4"))));
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(TestRootFolder + TEXT("/ef5/sef5/ssef5/sssef5/ssssef5"))));
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(TestRootFolder + TEXT("/ef6"))));
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(TestRootFolder + TEXT("/ef7"))));
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(TestRootFolder + TEXT("/ef8"))));
// 	// DataManager.Update();
// 	// TestEqual(TEXT("[Empty folders][Multiple Empty folders with nested empty folders more complex case] count"), DataManager.EmptyFolders.Num(), 18);
// 	// TestFalse(TEXT("Empty folders must not contain forbidden folders"), ContainsForbiddenFolders(DataManager.EmptyFolders));
// 	// FPlatformFileManager::Get().GetPlatformFile().DeleteDirectoryRecursively(*(TestRootFolder + TEXT("/ef1")));
// 	// FPlatformFileManager::Get().GetPlatformFile().DeleteDirectoryRecursively(*(TestRootFolder + TEXT("/ef2")));
// 	// FPlatformFileManager::Get().GetPlatformFile().DeleteDirectoryRecursively(*(TestRootFolder + TEXT("/ef3")));
// 	// FPlatformFileManager::Get().GetPlatformFile().DeleteDirectoryRecursively(*(TestRootFolder + TEXT("/ef4")));
// 	// FPlatformFileManager::Get().GetPlatformFile().DeleteDirectoryRecursively(*(TestRootFolder + TEXT("/ef5")));
// 	// FPlatformFileManager::Get().GetPlatformFile().DeleteDirectoryRecursively(*(TestRootFolder + TEXT("/ef6")));
// 	// FPlatformFileManager::Get().GetPlatformFile().DeleteDirectoryRecursively(*(TestRootFolder + TEXT("/ef7")));
// 	// FPlatformFileManager::Get().GetPlatformFile().DeleteDirectoryRecursively(*(TestRootFolder + TEXT("/ef8")));
// 	//
// 	// // #TESTCASE 7 [Empty folders][Multiple Empty folders with nested empty folders, nested level 30]
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(TestRootFolder + TEXT("/ef1/ef1/ef1/ef1/ef1/ef1/ef1/ef1/ef1/ef1//ef1/ef1/ef1/ef1/ef1//ef1/ef1/ef1/ef1/ef1//ef1/ef1/ef1/ef1/ef1//ef1/ef1/ef1/ef1/ef1/"))));
// 	// DataManager.Update();
//  //    TestEqual(TEXT("[Empty folders][Multiple Empty folders with nested empty folders, nested level 30] count"), DataManager.EmptyFolders.Num(), 30);
// 	// TestFalse(TEXT("Empty folders must not contain forbidden folders"), ContainsForbiddenFolders(DataManager.EmptyFolders));
//  //    FPlatformFileManager::Get().GetPlatformFile().DeleteDirectoryRecursively(*(TestRootFolder + TEXT("/ef1")));
// 	//
//  //
// 	// // ====================================================
// 	// // Tests when Scanning of Developer contents disabled
// 	// // ====================================================
// 	// DataManager.CleanerConfigs->bScanDeveloperContents = false;
//  //
// 	// // #TESTCASE 1 [Empty folders][Single Folder in developer folder]
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectory((*(DevelopersFolder + TEXT("/ef1"))));
// 	// DataManager.Update();
// 	// TestEqual(TEXT("[Empty folders][Single Folders][Dev folder disabled] count"), DataManager.EmptyFolders.Num(), 0);
// 	// FPlatformFileManager::Get().GetPlatformFile().DeleteDirectory(*(DevelopersFolder + TEXT("/ef1")));
// 	//
// 	// // #TESTCASE 2 [Empty folders][Single Folder with Nested Empty folders in developer folder]
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(DevelopersFolder + TEXT("/ef1/ef2/ef3"))));
// 	// DataManager.Update();
// 	// TestEqual(TEXT("[Empty folders][Single Folder with Nested Empty folders in developer folder] count"), DataManager.EmptyFolders.Num(), 0);
// 	// FPlatformFileManager::Get().GetPlatformFile().DeleteDirectoryRecursively(*(DevelopersFolder + TEXT("/ef1")));
// 	//
// 	// // #TESTCASE 3 [Empty folders][Single Folder with Nested Empty folders in developer folder and content folder]
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectory((*(DevelopersFolder + TEXT("/ef1"))));
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectory((*(TestRootFolder + TEXT("/ef2"))));
// 	// DataManager.Update();
// 	// TestEqual(TEXT("[Empty folders][Single Folder with Nested Empty folders in developer folder] count"), DataManager.EmptyFolders.Num(), 1);
// 	// FPlatformFileManager::Get().GetPlatformFile().DeleteDirectoryRecursively(*(DevelopersFolder + TEXT("/ef1")));
// 	// FPlatformFileManager::Get().GetPlatformFile().DeleteDirectoryRecursively((*(TestRootFolder + TEXT("/ef2"))));
//  //
// 	// // #TESTCASE 4 [Empty folders][Multiple Folders with Nested Empty folders in developer folder and content folder]
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(DevelopersFolder + TEXT("/ef1/ef2/ef3"))));
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(DevelopersFolder + TEXT("/ef1/ef4"))));
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(DevelopersFolder + TEXT("/ef1/ef5/ef6"))));
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(DevelopersFolder + TEXT("/ef7/ef8"))));
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(TestRootFolder + TEXT("/ef9/ef10"))));
// 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree((*(TestRootFolder + TEXT("/ef11/ef12/ef13"))));
// 	// DataManager.Update();
// 	// TestEqual(TEXT("[Empty folders][Multiple Folders with Nested Empty folders in developer folder and content folder] count"), DataManager.EmptyFolders.Num(), 5);
//  //
// 	//
// 	// FPlatformFileManager::Get().GetPlatformFile().DeleteDirectoryRecursively(*TestRootFolder);
// 	return true;
// }
//
// // Non Engine Files
// // bool FProjectCleanerDataManagerInvalidFilesTests::RunTest(const FString& Parameters)
// // {
// // 	using namespace ProjectCleanerTestsHelper;
// //
// // 	FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*(TestRootFolder));
// // 	
// // 	// creating empty uasset and umap files that must be detected
// // 	TArray<FString> ExpectedCorruptedFiles;
// // 	ExpectedCorruptedFiles.Add(TEXT("TestUassetFile.uasset"));
// // 	ExpectedCorruptedFiles.Add(TEXT("TestUmapFile.umap"));
// // 	ExpectedCorruptedFiles.Add(TEXT("TestFolder/TestUassetFile.umap"));
// // 	ExpectedCorruptedFiles.Add(TEXT("TestFolder/TestUmapFile.umap"));
// // 	
// // 	// also creating NonEngine files
// // 	TArray<FString> ExpectedNonEngineFiles;
// // 	ExpectedNonEngineFiles.Add(TEXT("TestTxtFile.txt"));
// // 	ExpectedNonEngineFiles.Add(TEXT("TestMp4File.mp4"));
// // 	ExpectedNonEngineFiles.Add(TEXT(".gitignore"));
// // 	ExpectedNonEngineFiles.Add(TEXT("filewithoutextension"));
// // 	ExpectedNonEngineFiles.Add(TEXT("config.ini"));
// // 	ExpectedNonEngineFiles.Add(TEXT("TestFolder/config.ini"));
// // 	ExpectedNonEngineFiles.Add(TEXT("TestFolder/TestTxtFile.txt"));
// // 	
// // 	for (const auto& File : ExpectedCorruptedFiles)
// // 	{
// // 		const FString Path = TestRootFolder + TEXT("/") + File;
// // 		FFileHelper::SaveStringToFile(TEXT(""), *Path);
// // 	}
// // 	
// // 	for (const auto& File : ExpectedNonEngineFiles)
// // 	{
// // 		const FString Path = TestRootFolder + TEXT("/") + File;
// // 		FFileHelper::SaveStringToFile(TEXT(""), *Path);
// // 	}
// // 	
// // 	ProjectCleanerDataManager DataManager;
// // 	DataManager.SetRootFolder(TestRootFolder);
// // 	DataManager.Update();
// // 	
// // 	TestEqual(TEXT("Corrupted files num must"), DataManager.CorruptedAssets.Num(), 4);
// // 	TestEqual(TEXT("NonEngine files num must"), DataManager.NonEngineFiles.Num(), 7);
// // 	
// // 	// cleanup
// // 	FPlatformFileManager::Get().GetPlatformFile().DeleteDirectoryRecursively(*TestRootFolder);
// //
// // 	return true;
// // }
//
//
// // /**
// // * Latent command to create an asset
// // */
// // DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FCreateNewAssetCommand, TSharedPtr<ProjectCleanerTestsHelper::FCreateAssetInfo>, AssetInfo);
// // bool FCreateNewAssetCommand::Update()
// // {
// // 	AssetInfo->CreateAsset();
// // 	return true;
// // }
// // 	
// // /**
// // * Latent command to save an asset
// // */
// // DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FSaveNewAssetCommand, TSharedPtr<ProjectCleanerTestsHelper::FCreateAssetInfo>, AssetInfo);
// // bool FSaveNewAssetCommand::Update()
// // {
// // 	AssetInfo->SaveNewAsset();
// // 	return true;
// // }
// //
// // // Unused assets
// // bool FProjectCleanerDataManagerUnusedAssetsTests::RunTest(const FString& Parameters)
// // {
// // 	// todo: ashe23
// // 	// using namespace ProjectCleanerTestsHelper;
// // 	//
// // 	// FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*(TestRootFolder));
// // 	//
// // 	// ProjectCleanerDataManager DataManager;
// // 	// DataManager.SetRootFolder(TestRootFolder);
// // 	// DataManager.Update();
// // 	//
// // 	// FString RelativeRootPath = DataManager.RelativeRootFolder.ToString();
// // 	// RelativeRootPath.RemoveFromEnd(TEXT("/"));
// // 	//
// // 	// ProjectCleanerTestsHelper::CreateAsset<UMaterial, UMaterialFactoryNew>(
// // 	// 	TEXT("M_TestMaterial1"),
// // 	// 	RelativeRootPath,
// // 	// 	UMaterial::StaticClass(),
// // 	// 	UMaterialFactoryNew::StaticClass()
// // 	// );
// // 	// TestEqual(TEXT("Unused assets num must"), DataManager.UnusedAssets.Num(), 1);
// // 	//
// // 	// FPlatformFileManager::Get().GetPlatformFile().DeleteDirectoryRecursively(*(TestRootFolder));
// // 	return true;
// // }
//
// #endif