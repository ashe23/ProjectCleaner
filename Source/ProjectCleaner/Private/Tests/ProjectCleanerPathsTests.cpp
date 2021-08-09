// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "CoreTypes.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Core/ProjectCleanerUtility.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProjectCleanerPathsTests, "ProjectCleaner.PathConversions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FProjectCleanerPathsTests::RunTest(const FString& Parameters)
{
	const FString ProjectContentDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
	const FString AssetAbsPath = ProjectContentDir + TEXT("Material");
	const FString AssetInternalPath = FString{ TEXT("/Game/Material") };

	const FString TestPathInternal = ProjectCleanerUtility::ConvertAbsolutePathToInternal(AssetAbsPath);
	const FString TestPathAbsolute = ProjectCleanerUtility::ConvertInternalToAbsolutePath(AssetInternalPath);

	TestEqual(TEXT("Path Converted from Absolute to Internal must"), TestPathInternal, AssetInternalPath);
	TestEqual(TEXT("Path Converted from Internal to Absolute must"), TestPathAbsolute, AssetAbsPath);
	
	return true;
}

#endif