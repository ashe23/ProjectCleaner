// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "CoreTypes.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProjectCleanerRelationalMapTests, "ProjectCleaner.RelationalMap", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FProjectCleanerRelationalMapTests::RunTest(const FString& Parameters)
{
	return true;
}

#endif