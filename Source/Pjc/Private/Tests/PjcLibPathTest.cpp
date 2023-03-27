// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Libs/PjcLibPath.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPjcLibPathDefaults,
	"Plugins.ProjectCleaner.Libs.Path.DefaultPaths",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPjcLibPathNormalize,
	"Plugins.ProjectCleaner.Libs.Path.Normalize",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPjcLibPathToAbsolute,
	"Plugins.ProjectCleaner.Libs.Path.ToAbsolute",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPjcLibPathToAssetPath,
	"Plugins.ProjectCleaner.Libs.Path.ToAssetPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPjcLibPathToObjectPath,
	"Plugins.ProjectCleaner.Libs.Path.ToObjectPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPjcLibPathGetFilePath,
	"Plugins.ProjectCleaner.Libs.Path.GetFilePath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPjcLibPathGetPathName,
	"Plugins.ProjectCleaner.Libs.Path.GetPathName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPjcLibPathGetFileExtension,
	"Plugins.ProjectCleaner.Libs.Path.GetFileExtension",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FPjcLibPathDefaults::RunTest(const FString& Parameters)
{
	// default paths must not end with slash and must start with drive letter

	TArray<FString> TestCases;
	TestCases.Emplace(FPjcLibPath::ProjectDir());
	TestCases.Emplace(FPjcLibPath::ContentDir());
	TestCases.Emplace(FPjcLibPath::SourceDir());
	TestCases.Emplace(FPjcLibPath::ConfigDir());
	TestCases.Emplace(FPjcLibPath::PluginsDir());
	TestCases.Emplace(FPjcLibPath::SavedDir());
	TestCases.Emplace(FPjcLibPath::DevelopersDir());
	TestCases.Emplace(FPjcLibPath::CollectionsDir());
	TestCases.Emplace(FPjcLibPath::CurrentUserDevelopersDir());
	TestCases.Emplace(FPjcLibPath::CurrentUserCollectionsDir());

	for (const auto& TestCase : TestCases)
	{
		TestFalse(FString::Printf(TEXT("DefaultPaths - Input: \"%s\" must not be empty"), *TestCase), TestCase.IsEmpty());
		TestFalse(FString::Printf(TEXT("DefaultPaths - Input: \"%s\" must not contain any trailing slash"), *TestCase), TestCase.EndsWith(TEXT("/")) || TestCase.EndsWith(TEXT("\\")));
		TestFalse(FString::Printf(TEXT("DefaultPaths - Input: \"%s\" must be absolute"), *TestCase), !(TestCase.Len() > 2 && TestCase[1] == ':'));
		TestFalse(FString::Printf(TEXT("DefaultPaths - Input: \"%s\" must be collapsed"), *TestCase), TestCase.Contains(TEXT("..")) || TestCase.Contains(TEXT(".")));
	}

	return true;
}

bool FPjcLibPathNormalize::RunTest(const FString& Parameters)
{
	// contracts
	// 1. must start with slash or Disk drive letter (example C:/)
	// 2. must not contain any duplicate slashes
	// 3. must not end with trailing slash
	// 4. all consecutive or mixed separators must be collapsed
	// 5. must return empty string if path is outside project directory

	// for any invalid path must TOptional

	TArray<TPair<FString, FString>> TestCases = {

		// empty cases
		TPair<FString, FString>(TEXT(""), TEXT("")),
		TPair<FString, FString>(TEXT("C:/"), TEXT("")),
		TPair<FString, FString>(TEXT("C://"), TEXT("")),
		TPair<FString, FString>(TEXT("C:\\"), TEXT("")),
		TPair<FString, FString>(TEXT(":\\"), TEXT("")),
		TPair<FString, FString>(TEXT(":/"), TEXT("")),
		TPair<FString, FString>(TEXT(":"), TEXT("")),
		TPair<FString, FString>(TEXT("."), TEXT("")),
		TPair<FString, FString>(TEXT("Some/Invalid/PAath"), TEXT("")),
		TPair<FString, FString>(TEXT("/Game//..//"), TEXT("")),
		TPair<FString, FString>(FPaths::ConvertRelativePathToFull(FPaths::EngineDir()), TEXT("")),
		TPair<FString, FString>(FPaths::ConvertRelativePathToFull(FPaths::EngineContentDir()), TEXT("")),

		// absolute paths
		TPair<FString, FString>(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()), FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Content"))),
		TPair<FString, FString>(FPaths::ConvertRelativePathToFull(FPaths::ProjectPluginsDir()), FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Plugins"))),
		TPair<FString, FString>(FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir()), FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Config"))),
		TPair<FString, FString>(FPaths::ConvertRelativePathToFull(FPaths::GameSourceDir()), FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Source"))),
		TPair<FString, FString>(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("MyFolder/")), FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("MyFolder"))),
		TPair<FString, FString>(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("//MyFolder/")),
		                        FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("MyFolder"))),
		TPair<FString, FString>(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("//MyFolder/Test")),
		                        FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("MyFolder/Test"))),
		TPair<FString, FString>(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("//MyFolder/Test/")),
		                        FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("MyFolder/Test"))),
		TPair<FString, FString>(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("//MyFolder/Test//")),
		                        FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("MyFolder/Test"))),
		TPair<FString, FString>(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("//MyFolder/./Test//")),
		                        FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("MyFolder/Test"))),
		TPair<FString, FString>(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("//MyFolder//..//Test//")),
		                        FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Test"))),

		// relative asset paths
		TPair<FString, FString>(TEXT("/Game"), TEXT("/Game")),
		TPair<FString, FString>(TEXT("/Game/"), TEXT("/Game")),
		TPair<FString, FString>(TEXT("/Game//"), TEXT("/Game")),
		TPair<FString, FString>(TEXT("/Game/MyFolder"), TEXT("/Game/MyFolder")),
		TPair<FString, FString>(TEXT("/Game/MyFolder/"), TEXT("/Game/MyFolder")),
		TPair<FString, FString>(TEXT("/Game/MyFolder//"), TEXT("/Game/MyFolder")),
		TPair<FString, FString>(TEXT("/Game//MyFolder/"), TEXT("/Game/MyFolder")),
		TPair<FString, FString>(TEXT("/Game//MyFolder/"), TEXT("/Game/MyFolder")),
		TPair<FString, FString>(TEXT("//Game//MyFolder//"), TEXT("/Game/MyFolder")),

	};

	// Run the test cases
	for (const TPair<FString, FString>& TestCase : TestCases)
	{
		const FString Input = TestCase.Key;
		const FString Expected = TestCase.Value;
		const FString Actual = FPjcLibPath::Normalize(Input);

		TestEqual(FString::Printf(TEXT("PathNormalize - Input: \"%s\""), *Input), Actual, Expected);
	}

	return true;
}

bool FPjcLibPathToAbsolute::RunTest(const FString& Parameters)
{
	// contracts
	// 1. must start with drive letter
	// 2. must be under project dir
	// 3. must be collapsed
	// 4. must not end with trailing slash

	TArray<TPair<FString, FString>> TestCases
	{
		// empty cases
		TPair<FString, FString>{TEXT(""), TEXT("")},
		TPair<FString, FString>{TEXT("C:"), TEXT("")},
		TPair<FString, FString>{TEXT("C:/"), TEXT("")},
		TPair<FString, FString>{TEXT("C:\\"), TEXT("")},
		TPair<FString, FString>{TEXT("C://"), TEXT("")},


		TPair<FString, FString>{FPjcLibPath::ProjectDir() / TEXT("/"), FPjcLibPath::ProjectDir()},
		TPair<FString, FString>{FPjcLibPath::ProjectDir() / TEXT("\\"), FPjcLibPath::ProjectDir()},
		TPair<FString, FString>{FPjcLibPath::ContentDir() / TEXT("./Folder/"), FPjcLibPath::ContentDir() / TEXT("Folder")},
		TPair<FString, FString>{FPjcLibPath::ContentDir() / TEXT("my_file.txt"), FPjcLibPath::ContentDir() / TEXT("my_file.txt")},
		TPair<FString, FString>{TEXT("/Game"), FPjcLibPath::ContentDir()},
		TPair<FString, FString>{TEXT("/Game/"), FPjcLibPath::ContentDir()},
		TPair<FString, FString>{TEXT("/Game//"), FPjcLibPath::ContentDir()},
		TPair<FString, FString>{TEXT("//Game//"), FPjcLibPath::ContentDir()},
	};

	for (const TPair<FString, FString>& TestCase : TestCases)
	{
		const FString Input = TestCase.Key;
		const FString Expected = TestCase.Value;
		const FString Actual = FPjcLibPath::ToAbsolute(Input);

		TestEqual(FString::Printf(TEXT("ToAbsolute - Input: \"%s\""), *Input), Actual, Expected);
	}

	return true;
}

bool FPjcLibPathToAssetPath::RunTest(const FString& Parameters)
{
	// contracts
	// 1. must always start with /Game
	// 2. must not end with slash

	TArray<TPair<FString, FString>> TestCases
	{
		TPair<FString, FString>{TEXT(""), TEXT("")},
		TPair<FString, FString>{TEXT("/Game"), TEXT("/Game")},
		TPair<FString, FString>{TEXT("/Game/"), TEXT("/Game")},
		TPair<FString, FString>{TEXT("/Game//"), TEXT("/Game")},
		TPair<FString, FString>{TEXT("//Game//"), TEXT("/Game")},
		TPair<FString, FString>{TEXT("//Game//MyFolder"), TEXT("/Game/MyFolder")},
		TPair<FString, FString>{TEXT("//Game//MyFolder/"), TEXT("/Game/MyFolder")},
		TPair<FString, FString>{FPjcLibPath::ContentDir(), TEXT("/Game")},
		TPair<FString, FString>{FPjcLibPath::ContentDir() / TEXT("TestFolder"), TEXT("/Game/TestFolder")},
		TPair<FString, FString>{FPjcLibPath::ContentDir() / TEXT("/TestFolder"), TEXT("/Game/TestFolder")},
		TPair<FString, FString>{FPjcLibPath::ContentDir() / TEXT("/TestFolder/"), TEXT("/Game/TestFolder")},
		TPair<FString, FString>{FPjcLibPath::ContentDir() / TEXT("/TestFolder//"), TEXT("/Game/TestFolder")},
		TPair<FString, FString>{FPjcLibPath::ContentDir() / TEXT("TestFolder/NestedFolder"), TEXT("/Game/TestFolder/NestedFolder")},
		TPair<FString, FString>{FPjcLibPath::ContentDir() / TEXT("TestFolder//NestedFolder"), TEXT("/Game/TestFolder/NestedFolder")},
		TPair<FString, FString>{FPjcLibPath::ContentDir() / TEXT("TestFolder//NestedFolder/"), TEXT("/Game/TestFolder/NestedFolder")},
		TPair<FString, FString>{FPjcLibPath::ContentDir() / TEXT("TestFolder//NestedFolder\\"), TEXT("/Game/TestFolder/NestedFolder")},
	};

	for (const TPair<FString, FString>& TestCase : TestCases)
	{
		const FString Input = TestCase.Key;
		const FString Expected = TestCase.Value;
		const FString Actual = FPjcLibPath::ToAssetPath(Input);

		TestEqual(FString::Printf(TEXT("ToAssetPath - Input: \"%s\""), *Input), Actual, Expected);
	}

	return true;
}

bool FPjcLibPathToObjectPath::RunTest(const FString& Parameters)
{
	// contracts
	// 1. must always start with /Game
	// 2. must end with {asset_name}.{asset_name}
	// 3. must not have trailing slash

	TArray<TPair<FString, FName>> TestCases
	{
		TPair<FString, FName>{TEXT(""), TEXT("")},
		TPair<FString, FName>{TEXT("C:/"), TEXT("")},
		TPair<FString, FName>{TEXT("C:\\"), TEXT("")},
		TPair<FString, FName>{TEXT("C://"), TEXT("")},
		TPair<FString, FName>{FPjcLibPath::ContentDir(), TEXT("")},
		TPair<FString, FName>{TEXT("/Game/MyAsset.MyAsset"), TEXT("/Game/MyAsset.MyAsset")},
		TPair<FString, FName>{TEXT("/Game/Materials/M_Mat_Master.M_Mat_Master"), TEXT("/Game/Materials/M_Mat_Master.M_Mat_Master")},
		TPair<FString, FName>{TEXT("Material'/Game/StarterContent/Materials/M_ColorGrid_LowSpec.M_ColorGrid_LowSpec'"), TEXT("/Game/StarterContent/Materials/M_ColorGrid_LowSpec.M_ColorGrid_LowSpec")},
		TPair<FString, FName>{
			TEXT("Blueprint'/Game/ParagonLtBelica/Characters/Heroes/Belica/LtBelicaPlayerCharacter.LtBelicaPlayerCharacter'"),
			TEXT("/Game/ParagonLtBelica/Characters/Heroes/Belica/LtBelicaPlayerCharacter.LtBelicaPlayerCharacter")
		},
		TPair<FString, FName>{
			TEXT("/Game/ParagonLtBelica/Characters/Heroes/Belica/LtBelicaPlayerCharacter"),
			TEXT("")
		},

	};

	for (const TPair<FString, FName>& TestCase : TestCases)
	{
		const FString Input = TestCase.Key;
		const FName Expected = TestCase.Value;
		const FName Actual = FPjcLibPath::ToObjectPath(Input);

		TestEqual(FString::Printf(TEXT("ToObjectPath - Input: \"%s\""), *Input), Actual, Expected);
	}

	return true;
}

bool FPjcLibPathGetFilePath::RunTest(const FString& Parameters)
{
	// contracts
	// 1. must return clean path without file name or extension
	// 2. must return empty string if path is not file

	TArray<TPair<FString, FString>> TestCases
	{
		TPair<FString, FString>{TEXT(""), TEXT("")},
		TPair<FString, FString>{TEXT("/Game"), TEXT("")},
		TPair<FString, FString>{FPjcLibPath::ContentDir() / TEXT("my_file.txt"), FPjcLibPath::ContentDir()},
		TPair<FString, FString>{FPjcLibPath::ContentDir() / TEXT("/my_file.txt"), FPjcLibPath::ContentDir()},
		TPair<FString, FString>{FPjcLibPath::ContentDir() / TEXT("Folder/my_file.txt"), FPjcLibPath::ContentDir() / TEXT("Folder")},
		TPair<FString, FString>{FPjcLibPath::ContentDir() / TEXT("Folder//my_file.txt"), FPjcLibPath::ContentDir() / TEXT("Folder")},
		TPair<FString, FString>{FPjcLibPath::ContentDir(), TEXT("")},
		TPair<FString, FString>{FPjcLibPath::SourceDir(), TEXT("")},
	};

	for (const TPair<FString, FString>& TestCase : TestCases)
	{
		const FString Input = TestCase.Key;
		const FString Expected = TestCase.Value;
		const FString Actual = FPjcLibPath::GetFilePath(Input);

		TestEqual(FString::Printf(TEXT("GetFilePath - Input: \"%s\""), *Input), Actual, Expected);
	}

	return true;
}

bool FPjcLibPathGetPathName::RunTest(const FString& Parameters)
{
	// contracts
	// 1. must return given path last leaf name

	TArray<TPair<FString, FString>> TestCases
	{
		TPair<FString, FString>{TEXT(""), TEXT("")},
		TPair<FString, FString>{TEXT("/Game"), TEXT("Game")},
		TPair<FString, FString>{FPjcLibPath::ContentDir() / TEXT("MyFolder"), TEXT("MyFolder")},
		TPair<FString, FString>{FPjcLibPath::ContentDir() / TEXT("//MyFolder"), TEXT("MyFolder")},
		TPair<FString, FString>{FPjcLibPath::ContentDir() / TEXT("MyFolder/my_file.txt"), TEXT("MyFolder")},
		TPair<FString, FString>{FPjcLibPath::ContentDir() / TEXT("MyFolder/my_file.txt"), TEXT("MyFolder")},
		TPair<FString, FString>{FPjcLibPath::ContentDir() / TEXT("Test/AnotherTest/my_file.txt"), TEXT("AnotherTest")},
	};

	for (const TPair<FString, FString>& TestCase : TestCases)
	{
		const FString Input = TestCase.Key;
		const FString Expected = TestCase.Value;
		const FString Actual = FPjcLibPath::GetPathName(Input);

		TestEqual(FString::Printf(TEXT("GetPathName - Input: \"%s\""), *Input), Actual, Expected);
	}

	return true;
}

bool FPjcLibPathGetFileExtension::RunTest(const FString& Parameters)
{
	TArray<TPair<FString, FString>> TestCases
	{
		TPair<FString, FString>{TEXT(""), TEXT("")},
		TPair<FString, FString>{FPjcLibPath::ContentDir() / TEXT("my_file.txt"), TEXT("txt")},
		TPair<FString, FString>{FPjcLibPath::ContentDir() / TEXT("Test/other.mp4"), TEXT("mp4")},
		TPair<FString, FString>{FPjcLibPath::ContentDir() / TEXT("Test/my_asset.uasset"), TEXT("uasset")},
		TPair<FString, FString>{TEXT("/SomeRandomPath/test.bin"), TEXT("bin")},
	};

	for (const TPair<FString, FString>& TestCase : TestCases)
	{
		const FString Input = TestCase.Key;
		const FString Expected = TestCase.Value;
		const FString Actual = FPjcLibPath::GetFileExtension(Input, false);

		TestEqual(FString::Printf(TEXT("GetFileExtension - Input: \"%s\""), *Input), Actual, Expected);
	}

	return true;
}
