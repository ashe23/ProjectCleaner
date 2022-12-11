// // Copyright Ashot Barkhudaryan. All Rights Reserved.
//
// #include "ProjectCleaner.h"
// #include "Misc/AutomationTest.h"
// #include "ProjectCleanerLibrary.h"
//
// IMPLEMENT_SIMPLE_AUTOMATION_TEST(
// 	FProjectCleanerLibraryPathConvertToAbsTest,
// 	"Plugins.ProjectCleaner.Library.PathConvertToAbs",
// 	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter
// )
//
// IMPLEMENT_SIMPLE_AUTOMATION_TEST(
// 	FProjectCleanerLibraryPathConvertToRelTest,
// 	"Plugins.ProjectCleaner.Library.PathConvertToRel",
// 	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter
// )
//
// IMPLEMENT_SIMPLE_AUTOMATION_TEST(
// 	FProjectCleanerLibraryPathIsUnderFolder,
// 	"Plugins.ProjectCleaner.Library.PathIsUnderFolder",
// 	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter
// )
//
//
// bool FProjectCleanerLibraryPathConvertToAbsTest::RunTest(const FString& Parameters)
// {
// 	struct FTestCase
// 	{
// 		FString Input;
// 		FString Expected;
// 	};
//
// 	const FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
//
// 	TArray<FTestCase> TestCases;
// 	TestCases.Add(FTestCase{TEXT(""), TEXT("")});
// 	TestCases.Add(FTestCase{TEXT("aaa"), TEXT("")});
// 	TestCases.Add(FTestCase{TEXT("/Game"), ProjectDir / TEXT("Content")});
// 	TestCases.Add(FTestCase{TEXT("/Game/"), ProjectDir / TEXT("Content")});
// 	TestCases.Add(FTestCase{TEXT("/Game//"), ProjectDir / TEXT("Content")});
// 	TestCases.Add(FTestCase{TEXT("//Game/"), ProjectDir / TEXT("Content")});
// 	TestCases.Add(FTestCase{TEXT("/Game/MyFolder"), ProjectDir / TEXT("Content/MyFolder")});
// 	TestCases.Add(FTestCase{TEXT("/Game/MyFolder/"), ProjectDir / TEXT("Content/MyFolder")});
// 	TestCases.Add(FTestCase{TEXT("/Game/MyFolder//"), ProjectDir / TEXT("Content/MyFolder")});
// 	TestCases.Add(FTestCase{TEXT("/Game/StarterContent"), ProjectDir / TEXT("Content/StarterContent")});
// 	TestCases.Add(FTestCase{TEXT("/Game/StarterContent/material.uasset"), ProjectDir / TEXT("Content/StarterContent/material.uasset")});
// 	TestCases.Add(FTestCase{TEXT("/Game/StarterContent//material.uasset"), ProjectDir / TEXT("Content/StarterContent/material.uasset")});
// 	TestCases.Add(FTestCase{TEXT("/Game//StarterContent//material.uasset"), ProjectDir / TEXT("Content/StarterContent/material.uasset")});
//
// 	for (const auto& TestCase : TestCases)
// 	{
// 		const FString Value = UProjectCleanerLibrary::PathConvertToAbs(TestCase.Input);
// 		if (!TestCase.Expected.Equals(Value))
// 		{
// 			UE_LOG(LogProjectCleaner, Error, TEXT("Expected %s, got %s, input %s"), *TestCase.Expected, *Value, *TestCase.Input);
// 			return false;
// 		}
// 	}
//
// 	return true;
// }
//
// bool FProjectCleanerLibraryPathConvertToRelTest::RunTest(const FString& Parameters)
// {
// 	struct FTestCase
// 	{
// 		FString Input;
// 		FString Expected;
// 	};
//
// 	const FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
//
// 	TArray<FTestCase> TestCases;
// 	TestCases.Add(FTestCase{TEXT(""), TEXT("")});
// 	TestCases.Add(FTestCase{TEXT("aaa"), TEXT("")});
// 	TestCases.Add(FTestCase{ProjectDir / TEXT("Content"), TEXT("/Game")});
// 	TestCases.Add(FTestCase{ProjectDir / TEXT("Content/"), TEXT("/Game")});
// 	TestCases.Add(FTestCase{ProjectDir / TEXT("Content//"), TEXT("/Game")});
// 	TestCases.Add(FTestCase{ProjectDir / TEXT("Content/MyFolder"), TEXT("/Game/MyFolder")});
// 	TestCases.Add(FTestCase{ProjectDir / TEXT("Content/MyFolder/"), TEXT("/Game/MyFolder")});
// 	TestCases.Add(FTestCase{ProjectDir / TEXT("Content/MyFolder//"), TEXT("/Game/MyFolder")});
// 	TestCases.Add(FTestCase{ProjectDir / TEXT("Content/MyFolder///"), TEXT("/Game/MyFolder")});
// 	TestCases.Add(FTestCase{ProjectDir / TEXT("Content//MyFolder"), TEXT("/Game/MyFolder")});
// 	TestCases.Add(FTestCase{ProjectDir / TEXT("Content//MyFolder/"), TEXT("/Game/MyFolder")});
// 	TestCases.Add(FTestCase{ProjectDir / TEXT("Content//MyFolder//"), TEXT("/Game/MyFolder")});
// 	TestCases.Add(FTestCase{ProjectDir / TEXT("Content/StarterContent/Materials/"), TEXT("/Game/StarterContent/Materials")});
// 	TestCases.Add(FTestCase{ProjectDir / TEXT("Content/StarterContent//Materials//"), TEXT("/Game/StarterContent/Materials")});
// 	TestCases.Add(FTestCase{ProjectDir / TEXT("Content/StarterContent/material.uasset"), TEXT("/Game/StarterContent/material.uasset")});
// 	TestCases.Add(FTestCase{ProjectDir / TEXT("Content/StarterContent//material.uasset"), TEXT("/Game/StarterContent/material.uasset")});
// 	TestCases.Add(FTestCase{ProjectDir / TEXT("Content/StarterContent//material.uasset"), TEXT("/Game/StarterContent/material.uasset")});
//
// 	for (const auto& TestCase : TestCases)
// 	{
// 		const FString Value = UProjectCleanerLibrary::PathConvertToRel(TestCase.Input);
// 		if (!TestCase.Expected.Equals(Value))
// 		{
// 			UE_LOG(LogProjectCleaner, Error, TEXT("Expected %s, got %s, input %s"), *TestCase.Expected, *Value, *TestCase.Input);
// 			return false;
// 		}
// 	}
//
// 	return true;
// }
//
// bool FProjectCleanerLibraryPathIsUnderFolder::RunTest(const FString& Parameters)
// {
// 	struct FTestCase
// 	{
// 		FString SearchFolder;
// 		FString RootFolder;
// 		bool bExpected;
// 	};
//
// 	TArray<FTestCase> TestCases;
// 	TestCases.Add(FTestCase{TEXT(""), TEXT(""), false});
// 	TestCases.Add(FTestCase{TEXT("/Game/StarterContent"), TEXT("/Game"), true});
// 	TestCases.Add(FTestCase{TEXT("/Game/StarterContent/"), TEXT("/Game"), true});
// 	TestCases.Add(FTestCase{TEXT("/Game/StarterContent"), TEXT("/Game/StarterContent"), true});
// 	TestCases.Add(FTestCase{TEXT("/Game/StarterContent"), TEXT("/Game/StarterContent/MyFolder"), false});
// 	TestCases.Add(FTestCase{TEXT("/Game/StarterContent/MyFolder/Materials"), TEXT("/Game/StarterContent/MyFolder"), true});
// 	TestCases.Add(FTestCase{UProjectCleanerLibrary::PathGetDeveloperFolder(true), UProjectCleanerLibrary::PathGetContentFolder(true), true});
// 	TestCases.Add(FTestCase{UProjectCleanerLibrary::PathGetContentFolder(true), UProjectCleanerLibrary::PathGetDeveloperFolder(true), false});
//
// 	for (const auto& TestCase : TestCases)
// 	{
// 		if (UProjectCleanerLibrary::PathIsUnderFolder(TestCase.SearchFolder, TestCase.RootFolder) != TestCase.bExpected)
// 		{
// 			UE_LOG(LogProjectCleaner, Error, TEXT("Expected %s to be under folder %s, got false"), *TestCase.SearchFolder, *TestCase.RootFolder);
// 			return false;
// 		}
// 	}
//
// 	return true;
// }
