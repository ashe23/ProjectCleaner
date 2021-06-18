#include "CoreTypes.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "ProjectCleanerHelper.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProjectCleanerPathsTests, "ProjectCleaner.PathConversions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FProjectCleanerPathsTests::RunTest(const FString& Parameters)
{
	const FString ProjectContentDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
	const FString AssetAbsPath = ProjectContentDir + TEXT("Material");
	const FString AssetInternalPath = FString{ TEXT("/Game/Material") };

	FString TestPathInternal = ProjectCleanerHelper::ConvertAbsolutePathToInternal(AssetAbsPath);
	TestEqual(TEXT("Path Converted from Absolute to Internal must"), TestPathInternal, AssetInternalPath);


	FString TestPathAbsolute = ProjectCleanerHelper::ConvertInternalToAbsolutePath(AssetInternalPath);
	TestEqual(TEXT("Path Converted from Internal to Absolute must"), TestPathAbsolute, AssetAbsPath);

	return true;
}

#endif