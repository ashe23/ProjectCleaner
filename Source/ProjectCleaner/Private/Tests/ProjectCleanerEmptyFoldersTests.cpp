#include "CoreTypes.h"
#include "Misc/AutomationTest.h"
#include "ProjectCleanerHelper.h"
#include "ProjectCleanerUtility.h"


#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProjectClanerEmptyFolders, "ProjectCleaner.EmptyFolders", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FProjectClanerEmptyFolders::RunTest(const FString& Parameters)
{
	{
		TSet<FName> EmptyFolders;
		ProjectCleanerHelper::GetEmptyFolders(EmptyFolders);
		ProjectCleanerUtility::DeleteEmptyFolders(EmptyFolders);

		TestEqual(TEXT("Empty folders must be zero after deletion"), 0, EmptyFolders.Num());
	}


	return true;
}

#endif 