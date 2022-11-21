// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Libs/ProjectCleanerAssetLibrary.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProjectCleanerAssetLibraryTest, "Plugins.ProjectCleaner.Library.Asset", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FProjectCleanerAssetLibraryTest::RunTest(const FString& Parameters)
{
	return true;
}
