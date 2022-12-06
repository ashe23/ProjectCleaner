// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleaner.h"
#include "Misc/AutomationTest.h"
#include "ProjectCleanerLibrary.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FProjectCleanerLibraryPathConvertToAbsTest,
	"Plugins.ProjectCleaner.Library.PathConvertToAbs",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter
)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FProjectCleanerLibraryPathConvertToRelTest,
	"Plugins.ProjectCleaner.Library.PathConvertToRel",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter
)

struct FProjectCleanerTestCase
{
	FString Input;
	FString Expected;
};

bool FProjectCleanerLibraryPathConvertToAbsTest::RunTest(const FString& Parameters)
{
	const FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());

	TArray<FProjectCleanerTestCase> TestCases;
	TestCases.Add(FProjectCleanerTestCase{TEXT(""), TEXT("")});
	TestCases.Add(FProjectCleanerTestCase{TEXT("aaa"), TEXT("")});
	TestCases.Add(FProjectCleanerTestCase{TEXT("/Game"), ProjectDir / TEXT("Content")});
	TestCases.Add(FProjectCleanerTestCase{TEXT("/Game/"), ProjectDir / TEXT("Content")});
	TestCases.Add(FProjectCleanerTestCase{TEXT("/Game//"), ProjectDir / TEXT("Content")});
	TestCases.Add(FProjectCleanerTestCase{TEXT("//Game/"), ProjectDir / TEXT("Content")});
	TestCases.Add(FProjectCleanerTestCase{TEXT("/Game/MyFolder"), ProjectDir / TEXT("Content/MyFolder")});
	TestCases.Add(FProjectCleanerTestCase{TEXT("/Game/MyFolder/"), ProjectDir / TEXT("Content/MyFolder")});
	TestCases.Add(FProjectCleanerTestCase{TEXT("/Game/MyFolder//"), ProjectDir / TEXT("Content/MyFolder")});
	TestCases.Add(FProjectCleanerTestCase{TEXT("/Game/StarterContent"), ProjectDir / TEXT("Content/StarterContent")});
	TestCases.Add(FProjectCleanerTestCase{TEXT("/Game/StarterContent/material.uasset"), ProjectDir / TEXT("Content/StarterContent/material.uasset")});
	TestCases.Add(FProjectCleanerTestCase{TEXT("/Game/StarterContent//material.uasset"), ProjectDir / TEXT("Content/StarterContent/material.uasset")});
	TestCases.Add(FProjectCleanerTestCase{TEXT("/Game//StarterContent//material.uasset"), ProjectDir / TEXT("Content/StarterContent/material.uasset")});

	for (const auto& TestCase : TestCases)
	{
		const FString Value = UProjectCleanerLibrary::PathConvertToAbs(TestCase.Input);
		if (!TestCase.Expected.Equals(Value))
		{
			UE_LOG(LogProjectCleaner, Error, TEXT("Expected %s, got %s, input %s"), *TestCase.Expected, *Value, *TestCase.Input);
			return false;
		}
	}

	return true;
}

bool FProjectCleanerLibraryPathConvertToRelTest::RunTest(const FString& Parameters)
{
	const FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());

	TArray<FProjectCleanerTestCase> TestCases;
	TestCases.Add(FProjectCleanerTestCase{TEXT(""), TEXT("")});
	TestCases.Add(FProjectCleanerTestCase{TEXT("aaa"), TEXT("")});
	TestCases.Add(FProjectCleanerTestCase{ProjectDir / TEXT("Content"), TEXT("/Game")});
	TestCases.Add(FProjectCleanerTestCase{ProjectDir / TEXT("Content/"), TEXT("/Game")});
	TestCases.Add(FProjectCleanerTestCase{ProjectDir / TEXT("Content//"), TEXT("/Game")});
	TestCases.Add(FProjectCleanerTestCase{ProjectDir / TEXT("Content/MyFolder"), TEXT("/Game/MyFolder")});
	TestCases.Add(FProjectCleanerTestCase{ProjectDir / TEXT("Content/MyFolder/"), TEXT("/Game/MyFolder")});
	TestCases.Add(FProjectCleanerTestCase{ProjectDir / TEXT("Content/MyFolder//"), TEXT("/Game/MyFolder")});
	TestCases.Add(FProjectCleanerTestCase{ProjectDir / TEXT("Content/MyFolder///"), TEXT("/Game/MyFolder")});
	TestCases.Add(FProjectCleanerTestCase{ProjectDir / TEXT("Content//MyFolder"), TEXT("/Game/MyFolder")});
	TestCases.Add(FProjectCleanerTestCase{ProjectDir / TEXT("Content//MyFolder/"), TEXT("/Game/MyFolder")});
	TestCases.Add(FProjectCleanerTestCase{ProjectDir / TEXT("Content//MyFolder//"), TEXT("/Game/MyFolder")});
	TestCases.Add(FProjectCleanerTestCase{ProjectDir / TEXT("Content/StarterContent/Materials/"), TEXT("/Game/StarterContent/Materials")});
	TestCases.Add(FProjectCleanerTestCase{ProjectDir / TEXT("Content/StarterContent//Materials//"), TEXT("/Game/StarterContent/Materials")});
	TestCases.Add(FProjectCleanerTestCase{ProjectDir / TEXT("Content/StarterContent/material.uasset"), TEXT("/Game/StarterContent/material.uasset")});
	TestCases.Add(FProjectCleanerTestCase{ProjectDir / TEXT("Content/StarterContent//material.uasset"), TEXT("/Game/StarterContent/material.uasset")});
	TestCases.Add(FProjectCleanerTestCase{ProjectDir / TEXT("Content/StarterContent//material.uasset"), TEXT("/Game/StarterContent/material.uasset")});

	for (const auto& TestCase : TestCases)
	{
		const FString Value = UProjectCleanerLibrary::PathConvertToRel(TestCase.Input);
		if (!TestCase.Expected.Equals(Value))
		{
			UE_LOG(LogProjectCleaner, Error, TEXT("Expected %s, got %s, input %s"), *TestCase.Expected, *Value, *TestCase.Input);
			return false;
		}
	}

	return true;
}
