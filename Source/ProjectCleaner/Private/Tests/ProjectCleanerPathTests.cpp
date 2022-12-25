// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleaner.h"
#include "Misc/AutomationTest.h"
#include "Core/ProjectCleanerPath.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProjectCleanerPathInvalidTests, "Plugins.ProjectCleaner.Core.Paths", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProjectCleanerPathValidTests, "Plugins.ProjectCleaner.Core.Paths", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FProjectCleanerPathInvalidTests::RunTest(const FString& Parameters)
{
	const FProjectCleanerPath PathEmpty{TEXT("")};

	// following invalid paths, must all return empty string and return invalid state
	TArray<FString> TestCases;
	TestCases.Add(TEXT(""));
	TestCases.Add(TEXT("/"));
	TestCases.Add(TEXT("//"));
	TestCases.Add(TEXT("//"));
	TestCases.Add(TEXT("\\"));
	TestCases.Add(TEXT("//\\"));
	TestCases.Add(TEXT("."));
	TestCases.Add(TEXT("..."));
	TestCases.Add(TEXT("..."));
	TestCases.Add(TEXT("Game"));
	TestCases.Add(TEXT("Content"));

	for (const auto& TestCase : TestCases)
	{
		const FProjectCleanerPath Path{TestCase};
		if (Path.IsValid())
		{
			UE_LOG(LogProjectCleaner, Error, TEXT("Expected IsValid return false for %s, but got true"), *TestCase);
			return false;
		}
	}

	return true;
}

bool FProjectCleanerPathValidTests::RunTest(const FString& Parameters)
{
	struct FTestCase
	{
		FString Input;
		FString ExpectedPathAbsolute;
		FString ExpectedPathRelative;
	};

	const FString PathProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Content"));

	TArray<FTestCase> TestCases;

	// construction from relative paths
	TestCases.Add(FTestCase{TEXT("/Game"), PathProjectDir, TEXT("/Game")});
	TestCases.Add(FTestCase{TEXT("/Game/"), PathProjectDir, TEXT("/Game")});
	TestCases.Add(FTestCase{TEXT("/Game//"), PathProjectDir, TEXT("/Game")});
	TestCases.Add(FTestCase{TEXT("/Game/Folder"), PathProjectDir / TEXT("Folder"), TEXT("/Game/Folder")});
	TestCases.Add(FTestCase{TEXT("/Game/Folder/"), PathProjectDir / TEXT("Folder"), TEXT("/Game/Folder")});
	TestCases.Add(FTestCase{TEXT("/Game/Folder/AnotherFolder"), PathProjectDir / TEXT("Folder/AnotherFolder"), TEXT("/Game/Folder/AnotherFolder")});
	TestCases.Add(FTestCase{TEXT("/Game/Folder/AnotherFolder/"), PathProjectDir / TEXT("Folder/AnotherFolder"), TEXT("/Game/Folder/AnotherFolder")});
	TestCases.Add(FTestCase{TEXT("/Game/Folder/AnotherFolder//"), PathProjectDir / TEXT("Folder/AnotherFolder"), TEXT("/Game/Folder/AnotherFolder")});
	TestCases.Add(FTestCase{TEXT("/Game/Folder//AnotherFolder//"), PathProjectDir / TEXT("Folder/AnotherFolder"), TEXT("/Game/Folder/AnotherFolder")});
	TestCases.Add(FTestCase{TEXT("/Game//Folder//AnotherFolder//"), PathProjectDir / TEXT("Folder/AnotherFolder"), TEXT("/Game/Folder/AnotherFolder")});
	TestCases.Add(FTestCase{TEXT("//Game//Folder//AnotherFolder//"), PathProjectDir / TEXT("Folder/AnotherFolder"), TEXT("/Game/Folder/AnotherFolder")});

	// construction from absolute paths
	TestCases.Add(FTestCase{PathProjectDir, PathProjectDir, TEXT("/Game")});
	TestCases.Add(FTestCase{PathProjectDir / TEXT("/"), PathProjectDir, TEXT("/Game")});
	TestCases.Add(FTestCase{PathProjectDir, PathProjectDir, TEXT("/Game")});
	TestCases.Add(FTestCase{PathProjectDir / TEXT("Folder"), PathProjectDir / TEXT("Folder"), TEXT("/Game/Folder")});
	TestCases.Add(FTestCase{PathProjectDir / TEXT("Folder/"), PathProjectDir / TEXT("Folder"), TEXT("/Game/Folder")});
	TestCases.Add(FTestCase{PathProjectDir / TEXT("Folder/AnotherFolder"), PathProjectDir / TEXT("Folder/AnotherFolder"), TEXT("/Game/Folder/AnotherFolder")});
	TestCases.Add(FTestCase{PathProjectDir / TEXT("Folder/AnotherFolder/"), PathProjectDir / TEXT("Folder/AnotherFolder"), TEXT("/Game/Folder/AnotherFolder")});
	TestCases.Add(FTestCase{PathProjectDir / TEXT("Folder/AnotherFolder//"), PathProjectDir / TEXT("Folder/AnotherFolder"), TEXT("/Game/Folder/AnotherFolder")});
	TestCases.Add(FTestCase{PathProjectDir / TEXT("Folder//AnotherFolder//"), PathProjectDir / TEXT("Folder/AnotherFolder"), TEXT("/Game/Folder/AnotherFolder")});
	TestCases.Add(FTestCase{PathProjectDir / TEXT("/Folder//AnotherFolder//"), PathProjectDir / TEXT("Folder/AnotherFolder"), TEXT("/Game/Folder/AnotherFolder")});
	TestCases.Add(FTestCase{PathProjectDir / TEXT("//Folder//AnotherFolder//"), PathProjectDir / TEXT("Folder/AnotherFolder"), TEXT("/Game/Folder/AnotherFolder")});

	for (const auto& TestCase : TestCases)
	{
		const FProjectCleanerPath Path{TestCase.Input};
		if (Path.IsValid() == false)
		{
			UE_LOG(LogProjectCleaner, Error, TEXT("Expected IsValid return true for %s, but got false"), *TestCase.Input);
			return false;
		}

		if (Path.IsDirectory() == false)
		{
			UE_LOG(LogProjectCleaner, Error, TEXT("Expected IsDirectory return true for %s, but got false"), *TestCase.Input);
			return false;
		}

		if (Path.IsFile() == true)
		{
			UE_LOG(LogProjectCleaner, Error, TEXT("Expected IsFile return false for %s, but got true"), *TestCase.Input);
			return false;
		}

		if (Path.GetPathAbs().Equals(TestCase.ExpectedPathAbsolute) == false)
		{
			UE_LOG(LogProjectCleaner, Error, TEXT("Expected GetPathAbs to be equal %s, but got %s"), *TestCase.ExpectedPathAbsolute, *Path.GetPathAbs());
			return false;
		}

		if (Path.GetPathRel().Equals(TestCase.ExpectedPathRelative) == false)
		{
			UE_LOG(LogProjectCleaner, Error, TEXT("Expected GetPathRel to be equal %s, but got %s"), *TestCase.ExpectedPathRelative, *Path.GetPathRel());
			return false;
		}
	}

	return true;
}
