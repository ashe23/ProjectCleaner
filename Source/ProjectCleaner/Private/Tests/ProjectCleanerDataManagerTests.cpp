// // Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.
//
// #include "CoreTypes.h"
// #include "Misc/AutomationTest.h"
// #include "Core/ProjectCleanerDataManager.h"
//
// #if WITH_DEV_AUTOMATION_TESTS
//
// IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProjectCleanerDataManagerTests, "ProjectCleaner.ProjectCleanerDataManager", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
//
// bool FProjectCleanerDataManagerTests::RunTest(const FString& Parameters)
// {
// 	const FString TestRootFolder = ProjectCleanerDataManager::ProjectContentDir + TEXT("AutomatationProjectCleanerRoot");
// 	// todo:ashe23 create root folder
// 	FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*TestRootFolder);
// 	
// 	ProjectCleanerDataManager DataManager;
// 	// DataManager.SetRootFolder(TestRootFolder);
// 	DataManager.Update();
// 	
// 	// #TESTCASE 1 [Empty folders Test][Single Folder]
// 	TestEqual(TEXT("Empty folders count"), DataManager.EmptyFolders.Num(), 0);
// 	TestEqual(TEXT("All assets count"), DataManager.AllAssets.Num(), 0);
// 	TestEqual(TEXT("All used assets count"), DataManager.UsedAssets.Num(), 0);
//
// 	FPlatformFileManager::Get().GetPlatformFile().DeleteDirectory(*TestRootFolder);
// 	
// 	return true;
// }
//
// #endif