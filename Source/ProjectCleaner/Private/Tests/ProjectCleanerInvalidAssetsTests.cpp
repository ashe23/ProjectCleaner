// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "CoreTypes.h"
#include "Misc/AutomationTest.h"
#include "Core/ProjectCleanerUtility.h"
#include "Misc/FileHelper.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProjectCleanerInvalidFilesTests, "ProjectCleaner.InvalidFiles", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FProjectCleanerInvalidFilesTests::RunTest(const FString& Parameters)
{
	const FString ProjectContentDir = FPaths::ProjectContentDir();
	
	// creating empty uasset and umap files that must be detected
	TArray<FString> ExpectedCorruptedFiles;
	ExpectedCorruptedFiles.Add(TEXT("TestUassetFile.uasset"));
	ExpectedCorruptedFiles.Add(TEXT("TestUmapFile.umap"));
	ExpectedCorruptedFiles.Add(TEXT("TestFolder/TestUassetFile.umap"));
	ExpectedCorruptedFiles.Add(TEXT("TestFolder/TestUmapFile.umap"));

	// also creating NonEngine files
	TArray<FString> ExpectedNonEngineFiles;
	ExpectedNonEngineFiles.Add(TEXT("TestTxtFile.txt"));
	ExpectedNonEngineFiles.Add(TEXT("TestMp4File.mp4"));
	ExpectedNonEngineFiles.Add(TEXT(".gitignore"));
	ExpectedNonEngineFiles.Add(TEXT("filewithoutextension"));
	ExpectedNonEngineFiles.Add(TEXT("config.ini"));
	ExpectedNonEngineFiles.Add(TEXT("TestFolder/config.ini"));
	ExpectedNonEngineFiles.Add(TEXT("TestFolder/TestTxtFile.txt"));

	for (const auto& File : ExpectedCorruptedFiles)
	{
		FFileHelper::SaveStringToFile(TEXT(""), *(ProjectContentDir + File));
	}

	for (const auto& File : ExpectedNonEngineFiles)
	{
		FFileHelper::SaveStringToFile(TEXT(""), *(ProjectContentDir + File));
	}
	
	// finding Invalid files using ProjectCleaner API
	TSet<FString> CorruptedFiles;
	TSet<FString> NonEngineFiles;
	ProjectCleanerUtility::GetInvalidFiles(CorruptedFiles, NonEngineFiles);

	TestEqual(TEXT("Corrupted files num must"), CorruptedFiles.Num(), 4);
	TestEqual(TEXT("NonEngine files num must"), NonEngineFiles.Num(), 7);

	// cleanup
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	
	for (const auto& File : ExpectedCorruptedFiles)
	{
		PlatformFile.DeleteFile(*(ProjectContentDir + File));
	}
	
	for (const auto& File : ExpectedNonEngineFiles)
	{
		PlatformFile.DeleteFile(*(ProjectContentDir + File));
	}

	PlatformFile.DeleteDirectory(*(ProjectContentDir + TEXT("TestFolder")));
	
	return true;
}

#endif