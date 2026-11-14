// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Tests/PjcTestFixture.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "PjcGraphRouter.h"
#include "PjcInspector.h"
#include "PjcIssues.h"
#include "PjcLocations.h"
#include "PjcClean.h"
#include "Slate/SPjcScanSettings.h"
#include "PjcCookInfo.h"
#include "PjcOverview.h"
#include "PjcScanDraft.h"
#include "PjcSettings.h"
#include "PjcShim.h"
#include "PjcSubsystem.h"
#include "PjcTypes.h"
#include "PjcUnused.h"
#include "AssetRegistry/AssetRegistryState.h"
#include "Curves/CurveFloat.h"
#include "Engine/ObjectLibrary.h"
#include "Engine/AssetManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SPjcTabIssues.h"
#include "Slate/SPjcTabUnused.h"
#include "Slate/SPjcTabInspector.h"
#include "Slate/SPjcOverviewPanel.h"
#include "Slate/SPjcTabHelp.h"
#include "Pjc.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformOutputDevices.h"
#include "Misc/EngineVersion.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/RedirectCollector.h"
#include "Misc/ScopeExit.h"
#include "Serialization/MemoryWriter.h"
#include "UObject/ObjectRedirector.h"

namespace
{
	TSet<FString> ToObjectPaths(const TArray<FAssetData>& Assets) {
		TSet<FString> Paths;
		for (const auto& Asset : Assets) {
			Paths.Add(PjcShim::GetObjectPathString(Asset));
		}
		return Paths;
	}

	bool IsFixturePath(const FString& Path) {
		const TArray<FString> Roots {FPjcTestFixture::RootPath(), FPjcTestFixture::LocalizedRootPath(), FPjcTestFixture::PluginRootPath()};
		const bool bInRoot = Roots.ContainsByPredicate([&Path](const FString& Root) {
			return Path.StartsWith(Root + TEXT("/"));
		});
		// the splash asset lives in the project's Splash folder
		return bInRoot || Path.StartsWith(FPjcTestFixture::SplashPackage() + TEXT("."));
	}

	TSet<FString> FixtureAssets(const TArray<FAssetData>& Assets) {
		TSet<FString> Paths;
		for (const auto& Path : ToObjectPaths(Assets)) {
			if (IsFixturePath(Path)) {
				Paths.Add(Path);
			}
		}
		return Paths;
	}

	TSet<FString> FixtureFiles(const TArray<FString>& Files) {
		const FString Root = UPjcSubsystem::PathNormalize(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("PjcTests")));
		TSet<FString> Result;
		for (const auto& File : Files) {
			const FString Normalized = UPjcSubsystem::PathNormalize(File);
			if (Normalized.StartsWith(Root)) {
				Result.Add(Normalized);
			}
		}
		return Result;
	}

	TSet<FString> Normalized(const TArray<FString>& Files) {
		TSet<FString> Result;
		for (const auto& File : Files) {
			Result.Add(UPjcSubsystem::PathNormalize(File));
		}
		return Result;
	}

	TSet<FString> ObjectPaths(const FPjcTestFixture& Fixture, const TArray<FString>& Names) {
		TSet<FString> Result;
		for (const auto& Name : Names) {
			Result.Add(Fixture.ObjectPath(Name));
		}
		return Result;
	}

	void TestSetEqual(FAutomationTestBase& Test, const FString& What, const TSet<FString>& Actual, const TSet<FString>& Expected) {
		for (const auto& Item : Expected.Difference(Actual)) {
			Test.AddError(FString::Printf(TEXT("%s: missing %s"), *What, *Item));
		}
		for (const auto& Item : Actual.Difference(Expected)) {
			Test.AddError(FString::Printf(TEXT("%s: unexpected %s"), *What, *Item));
		}
	}

	// Changes scan settings for one test, restores them on destruction
	struct FPjcScopedScanSettings
	{
		UPjcScanSettings* Settings;
		bool bScanProjectPlugins;
		TArray<FString> IgnoredPlugins;
		bool bUnregisteredPrimaryUsed;
		bool bCinematicAssetsUsed;
		TArray<TSoftClassPtr<UObject>> UsedClasses;

		explicit FPjcScopedScanSettings(const bool bInScanProjectPlugins) : Settings(GetMutableDefault<UPjcScanSettings>()) {
			bScanProjectPlugins = Settings->bScanProjectPlugins;
			IgnoredPlugins = Settings->IgnoredPlugins;
			bUnregisteredPrimaryUsed = Settings->bUnregisteredPrimaryUsed;
			bCinematicAssetsUsed = Settings->bCinematicAssetsUsed;
			UsedClasses = Settings->UsedClasses;

			// known state, fixture expectations are written for defaults
			Settings->bScanProjectPlugins = bInScanProjectPlugins;
			Settings->IgnoredPlugins.Reset();
			Settings->bUnregisteredPrimaryUsed = true;
			Settings->bCinematicAssetsUsed = true;
			Settings->UsedClasses.Reset();
		}

		~FPjcScopedScanSettings() {
			Settings->bScanProjectPlugins = bScanProjectPlugins;
			Settings->IgnoredPlugins = IgnoredPlugins;
			Settings->bUnregisteredPrimaryUsed = bUnregisteredPrimaryUsed;
			Settings->bCinematicAssetsUsed = bCinematicAssetsUsed;
			Settings->UsedClasses = UsedClasses;
		}
	};

	FName PackageOf(const FPjcTestFixture& Fixture, const TCHAR* Name) {
		return FName {*FPackageName::ObjectPathToPackageName(Fixture.ObjectPath(Name))};
	}

	// BP_Placed is used only by the actor file of L_Feature, a level of the PjcFeature plugin. It counts as a dependency of
	// L_Feature when that level is scanned, as an outside reference when it is not; used either way. Scans the project
	// with plugins and again without, so the scan is as before for the rest of the test
	void TestActorFileReason(FAutomationTestBase& Test, const FPjcTestFixture& Fixture) {
		const FName Level = PackageOf(Fixture, TEXT("L_Feature"));
		const FName Placed = PackageOf(Fixture, TEXT("BP_Placed"));
		const auto Check = [&Test, Placed](const TCHAR* When, const EPjcUsageReason Reason, const FName Parent) {
			const FPjcAssetUsage* Usage = UPjcSubsystem::GetScanResult().Usage.Find(Placed);
			if (!Test.TestNotNull(*FString::Printf(TEXT("%s: blueprint scanned"), When), Usage)) return;

			Test.TestTrue(*FString::Printf(TEXT("%s: blueprint used"), When), Usage->bUsed);
			Test.TestEqual(*FString::Printf(TEXT("%s: blueprint reason"), When), static_cast<int32>(Usage->Reason), static_cast<int32>(Reason));
			Test.TestEqual(*FString::Printf(TEXT("%s: blueprint parent"), When), Usage->Parent, Parent);
		};

		// the level is in a plugin left out of the scan: an outside reference (UE4 reads actor files of scanned content only)
#if ENGINE_MAJOR_VERSION >= 5
		Check(TEXT("level not scanned"), EPjcUsageReason::EngineReferenced, NAME_None);
#else
		const FPjcAssetUsage* NotScanned = UPjcSubsystem::GetScanResult().Usage.Find(Placed);
		Test.TestTrue(TEXT("level not scanned: blueprint used"), NotScanned && NotScanned->bUsed);
#endif

		// an actor file of a map that does not exist belongs to no map: never relabeled
		const FString Missing = TEXT("/Game/") + PjcShim::GetExternalActorsFolderName() + TEXT("/Maps/PjcNoSuchMap/A/BC/PjcNoSuchActor");
		Test.TestTrue(TEXT("actor file of a deleted map has no owner"), UPjcSubsystem::GetExternalOwner(FName {*Missing}).IsNone());

		UPjcScanSettings* Settings = GetMutableDefault<UPjcScanSettings>();
		Settings->bScanProjectPlugins = true;
		if (Test.TestTrue(TEXT("scanned with plugins"), UPjcSubsystem::Scan(false))) {
#if ENGINE_MAJOR_VERSION >= 5
			Check(TEXT("level scanned"), EPjcUsageReason::Dependency, Level);
			const FPjcAssetUsage* Usage = UPjcSubsystem::GetScanResult().Usage.Find(Placed);
			Test.TestTrue(TEXT("placed in the level"), Usage && UPjcSubsystem::GetUsageReasonText(*Usage).StartsWith(TEXT("Placed in")));
#else
			const FPjcAssetUsage* Usage = UPjcSubsystem::GetScanResult().Usage.Find(Placed);
			Test.TestTrue(TEXT("level scanned: blueprint used"), Usage && Usage->bUsed);
#endif
		}
		Settings->bScanProjectPlugins = false;
		Test.TestTrue(TEXT("scanned without plugins again"), UPjcSubsystem::Scan(false));
	}

	// Inspector lists of L_Feature, a level that saves its actor (a BP_Placed) in its own file: the lists leave the file out
	// and go through it. Runs inside the one test that builds the Lyra layout: a second build of it in one editor session
	// crashes UE 4.27 (Find in Blueprints index) and UE 5.3 on Linux (World Partition keeps the actors of the first level)
	void TestInspectorActorFiles(FAutomationTestBase& Test, const FPjcTestFixture& Fixture) {
#if ENGINE_MAJOR_VERSION >= 5
		const FName Level = PackageOf(Fixture, TEXT("L_Feature"));
		const FName Placed = PackageOf(Fixture, TEXT("BP_Placed"));
		const auto IsActorFile = [](const FPjcInspectorLink& Link) {
			return FPjcInspectorGraph::IsExternal(Link.Package);
		};

		TArray<FPjcInspectorLink> Links;
		int32 Through = 0;
		FPjcInspectorGraph::GetLinks(Level, false, Links, Through);
		Test.TestTrue(TEXT("level uses: went through its actor file"), Through > 0);
		Test.TestFalse(TEXT("level uses: no actor file listed"), Links.ContainsByPredicate(IsActorFile));
		const FPjcInspectorLink* Used = Links.FindByPredicate([Placed](const FPjcInspectorLink& Link) {
			return Link.Package == Placed;
		});
		if (Test.TestNotNull(TEXT("level uses the blueprint of its actor"), Used)) {
			Test.TestTrue(TEXT("through the actor file"), Used->bThrough);
		}

		FPjcInspectorGraph::GetLinks(Placed, true, Links, Through);
		Test.TestTrue(TEXT("blueprint used by: went through the actor file"), Through > 0);
		Test.TestFalse(TEXT("blueprint used by: no actor file listed"), Links.ContainsByPredicate(IsActorFile));
		Test.TestTrue(TEXT("blueprint is used by the level"), Links.ContainsByPredicate([Level](const FPjcInspectorLink& Link) {
			return Link.Package == Level && Link.bThrough;
		}));

		FPjcInspectorGraph::GetLinks(Level, true, Links, Through);
		Test.TestFalse(TEXT("level used by: its own actor file is not listed"), Links.ContainsByPredicate(IsActorFile));
		Test.TestEqual(TEXT("level used by: its own actor file is not counted"), Through, 0);
		FPjcTestFixture::Case(Test, TEXT("inspector through actor files"));
#else
		FPjcTestFixture::Case(Test, TEXT("inspector through actor files"), TEXT("the UE 4 fixture level keeps its actor in the level file"));
#endif
	}

	TSet<FString> UnusedFixtureAssets() {
		TArray<FAssetData> Assets;
		UPjcSubsystem::GetAssetsUnused(Assets, false);
		return FixtureAssets(Assets);
	}

	FString RelativeToProject(const FString& Path) {
		FString Result = UPjcSubsystem::PathNormalize(Path);
		FPaths::MakePathRelativeTo(Result, *(UPjcSubsystem::PathNormalize(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir())) + TEXT("/")));
		return Result;
	}

	// Same keys as Scripts/pjc.py parses from commandlet output
	FString JsonStats(const int32 All, const int32 Used, const int32 Unused, const int32 External, const int32 Corrupted, const int32 FoldersEmpty) {
		TArray<FString> Items;
		Items.Add(FString::Printf(TEXT("\"assets_all\": %d"), All));
		Items.Add(FString::Printf(TEXT("\"assets_used\": %d"), Used));
		Items.Add(FString::Printf(TEXT("\"assets_unused\": %d"), Unused));
		Items.Add(FString::Printf(TEXT("\"files_external\": %d"), External));
		Items.Add(FString::Printf(TEXT("\"files_corrupted\": %d"), Corrupted));
		Items.Add(FString::Printf(TEXT("\"folders_empty\": %d"), FoldersEmpty));
		return FString::Printf(TEXT("{%s}"), *FString::Join(Items, TEXT(", ")));
	}

	FString JsonList(const TArray<FString>& Items) {
		TArray<FString> Quoted;
		for (const auto& Item : Items) {
			Quoted.Add(FString::Printf(TEXT("\"%s\""), *Item.Replace(TEXT("\\"), TEXT("/"))));
		}
		return FString::Printf(TEXT("[%s]"), *FString::Join(Quoted, TEXT(", ")));
	}
}	// namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPjcTestPaths, "ProjectCleaner.Paths", PJC_TEST_FLAGS)

bool FPjcTestPaths::RunTest(const FString& Parameters) {
	const FString ContentDir = UPjcSubsystem::PathNormalize(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()));

	TestEqual(TEXT("normalize empty"), UPjcSubsystem::PathNormalize(TEXT("")), FString {});
	TestEqual(TEXT("normalize trailing slash"), UPjcSubsystem::PathNormalize(TEXT("/Game/Foo/")), FString {TEXT("/Game/Foo")});
	TestEqual(TEXT("normalize duplicate slashes"), UPjcSubsystem::PathNormalize(TEXT("/Game//Foo///Bar")), FString {TEXT("/Game/Foo/Bar")});
	TestEqual(TEXT("normalize parent dirs"), UPjcSubsystem::PathNormalize(TEXT("/Game/Foo/../Bar")), FString {TEXT("/Game/Bar")});
	TestEqual(TEXT("normalize backslashes"), UPjcSubsystem::PathNormalize(ContentDir.Replace(TEXT("/"), TEXT("\\")) + TEXT("\\")), ContentDir);

	TestEqual(TEXT("absolute of /Game"), UPjcSubsystem::PathConvertToAbsolute(TEXT("/Game")), ContentDir);
	TestEqual(TEXT("absolute of /Game/Foo"), UPjcSubsystem::PathConvertToAbsolute(TEXT("/Game/Foo")), ContentDir / TEXT("Foo"));
	TestEqual(TEXT("relative of content dir"), UPjcSubsystem::PathConvertToRelative(ContentDir), FString {TEXT("/Game")});
	const FString RelativeSubdir = UPjcSubsystem::PathConvertToRelative(ContentDir / TEXT("Foo/Bar"));
	TestEqual(TEXT("relative of content subdir"), RelativeSubdir, FString {TEXT("/Game/Foo/Bar")});

	TestEqual(TEXT("object path"), UPjcSubsystem::PathConvertToObjectPath(TEXT("/Game/Foo/Bar.Bar")), FString {TEXT("/Game/Foo/Bar.Bar")});
	TestEqual(TEXT("object path of class"), UPjcSubsystem::PathConvertToObjectPath(TEXT("/Game/Foo/BP.BP_C")), FString {TEXT("/Game/Foo/BP.BP")});
	const FString ExportTextPath = UPjcSubsystem::PathConvertToObjectPath(TEXT("Blueprint'/Game/Foo/BP.BP'"));
	TestEqual(TEXT("object path of export text"), ExportTextPath, FString {TEXT("/Game/Foo/BP.BP")});
	TestEqual(TEXT("object path of package"), UPjcSubsystem::PathConvertToObjectPath(TEXT("/Game/Foo/Bar")), FString {});
	TestEqual(TEXT("object path outside project"), UPjcSubsystem::PathConvertToObjectPath(TEXT("/Engine/Foo/Bar.Bar")), FString {});

	TestTrue(TEXT("/Game is inside project"), UPjcSubsystem::PathIsInsideProject(TEXT("/Game")));
	TestTrue(TEXT("/Game/Foo is inside project"), UPjcSubsystem::PathIsInsideProject(TEXT("/Game/Foo")));
	TestFalse(TEXT("/GameData/Foo is not inside project"), UPjcSubsystem::PathIsInsideProject(TEXT("/GameData/Foo")));
	TestFalse(TEXT("/Engine/Foo is not inside project"), UPjcSubsystem::PathIsInsideProject(TEXT("/Engine/Foo")));
	TestFalse(TEXT("/Script/Engine is not inside project"), UPjcSubsystem::PathIsInsideProject(TEXT("/Script/Engine")));

#if ENGINE_MAJOR_VERSION >= 5
	TestTrue(TEXT("external actors folder"), UPjcSubsystem::FolderIsExternal(TEXT("/Game/__ExternalActors__/Maps/Main")));
	TestTrue(TEXT("external objects folder"), UPjcSubsystem::FolderIsExternal(TEXT("/Game/__ExternalObjects__/Maps/Main")));
	FPjcTestFixture::Case(*this, TEXT("external actor folders"));
#else
	FPjcTestFixture::Case(*this, TEXT("external actor folders"), TEXT("UE 4 has no external actors"));
#endif
	TestFalse(TEXT("regular folder is not external"), UPjcSubsystem::FolderIsExternal(TEXT("/Game/Maps")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPjcTestPrimaryTypes, "ProjectCleaner.Scan.PrimaryTypes", PJC_TEST_FLAGS)

bool FPjcTestPrimaryTypes::RunTest(const FString& Parameters) {
	if (!PjcShim::IsAssetManagerValid()) {
		AddError(TEXT("AssetManager is not initialized"));
		return false;
	}

	// logged so reports show what a project registers (Lyra like setups, see #58, #64)
	TArray<FPrimaryAssetTypeInfo> TypeInfos;
	UAssetManager::Get().GetPrimaryAssetTypeInfoList(TypeInfos);
	for (const auto& TypeInfo : TypeInfos) {
		const UClass* BaseClass = TypeInfo.AssetBaseClassLoaded;
		const FString BaseName = BaseClass ? BaseClass->GetPathName() : FString {TEXT("none")};
		AddInfo(FString::Printf(TEXT("primary type %s, base class %s"), *TypeInfo.PrimaryAssetType.ToString(), *BaseName));
	}

	TSet<FName> ClassNames;
	UPjcSubsystem::GetClassNamesPrimary(ClassNames);
	TestFalse(TEXT("UObject must never be primary base class"), ClassNames.Contains(PjcShim::GetClassName(UObject::StaticClass())));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPjcTestScanClassification, "ProjectCleaner.Scan.Classification", PJC_TEST_FLAGS)

bool FPjcTestScanClassification::RunTest(const FString& Parameters) {
	if (!FPjcTestFixture::IsTestHost()) {
		AddInfo(TEXT("Skipped, fixture tests run only in test host projects"));
		return true;
	}

	FPjcTestFixture Fixture;
	if (!Fixture.Build(*this)) return false;

	TArray<FAssetData> Assets;

	for (const bool bPlugins : {false, true}) {
		FPjcScopedScanSettings ScopedSettings {bPlugins};
		const FString Mode = bPlugins ? TEXT("plugins scanned") : TEXT("plugins not scanned");
		const TSet<FString> ExpectedUsed = ObjectPaths(Fixture, Fixture.UsedNames(bPlugins));
		const TSet<FString> ExpectedUnused = ObjectPaths(Fixture, Fixture.UnusedNames(bPlugins));

		UPjcSubsystem::GetAssetsAll(Assets);
		TestSetEqual(*this, Mode + TEXT(", all"), FixtureAssets(Assets), ExpectedUsed.Union(ExpectedUnused));

		UPjcSubsystem::GetAssetsUsed(Assets, false);
		TestSetEqual(*this, Mode + TEXT(", used"), FixtureAssets(Assets), ExpectedUsed);

		UPjcSubsystem::GetAssetsUnused(Assets, false);
		TestSetEqual(*this, Mode + TEXT(", unused"), FixtureAssets(Assets), ExpectedUnused);

		UPjcSubsystem::GetAssetsEngineReferenced(Assets, false);
		const TArray<FString> ExternallyReferenced = bPlugins ? TArray<FString> {} : TArray<FString> {TEXT("C_RefByPlugin")};
		TestSetEqual(*this, Mode + TEXT(", externally referenced"), FixtureAssets(Assets), ObjectPaths(Fixture, ExternallyReferenced));
	}

	FPjcScopedScanSettings ScopedSettings {false};

	UPjcSubsystem::GetAssetsPrimary(Assets, false);
	TestSetEqual(*this, TEXT("primary"), FixtureAssets(Assets), ObjectPaths(Fixture, Fixture.PrimaryNames()));

	UPjcSubsystem::GetAssetsEditor(Assets, false);
	TArray<FString> EditorNames {TEXT("EUB_Tool"), TEXT("BP_AnimModifier")};
	if (FPjcTestFixture::ValidatorBaseClass()) {
		EditorNames.Add(TEXT("BP_Validator"));
	}
	TestSetEqual(*this, TEXT("editor"), FixtureAssets(Assets), ObjectPaths(Fixture, EditorNames));

	UPjcSubsystem::GetAssetsExcluded(Assets, false);
	TestSetEqual(*this, TEXT("excluded"), FixtureAssets(Assets), ObjectPaths(Fixture, Fixture.ExcludedNames()));

	UPjcSubsystem::GetAssetsCircular(Assets, false);
	TestSetEqual(*this, TEXT("circular"), FixtureAssets(Assets), ObjectPaths(Fixture, {TEXT("OL_CircA"), TEXT("OL_CircB")}));

	TArray<FPjcAssetIndirectInfo> IndirectInfos;
	UPjcSubsystem::GetAssetsIndirect(Assets, IndirectInfos, false);
	const TArray<FString> IndirectNames {
	  TEXT("C_Indirect"), TEXT("C_IndirectPackage"), TEXT("C_IndirectFile"), TEXT("C_InPython"), TEXT("C_InPlatformConfig")
	};
	const TSet<FString> Indirect = ObjectPaths(Fixture, IndirectNames);
	TestSetEqual(*this, TEXT("indirect"), FixtureAssets(Assets), Indirect);

	const FPjcAssetIndirectInfo* IndirectInfo = IndirectInfos.FindByPredicate([&](const FPjcAssetIndirectInfo& Info) {
		return PjcShim::GetObjectPathString(Info.Asset).Equals(Fixture.ObjectPath(TEXT("C_Indirect")));
	});
	if (TestNotNull(TEXT("indirect info"), IndirectInfo)) {
		const FString InfoFile = UPjcSubsystem::PathNormalize(IndirectInfo->FilePath);
		const FString ConfigFile = UPjcSubsystem::PathNormalize(FPjcTestFixture::IndirectConfigFile());
		TestEqual(TEXT("indirect info file"), InfoFile, ConfigFile);
		TestEqual(TEXT("indirect info line"), IndirectInfo->FileNum, 2);
	}

	TArray<FString> Files;

	UPjcSubsystem::GetFoldersEmpty(Files);
	TestSetEqual(*this, TEXT("empty folders"), FixtureFiles(Files), Normalized(Fixture.EmptyFolders()));

	UPjcSubsystem::GetFilesExternalFiltered(Files, false);
	TestSetEqual(*this, TEXT("external files"), FixtureFiles(Files), Normalized({Fixture.ExternalFile()}));

	UPjcSubsystem::GetFilesExternalExcluded(Files, false);
	TestSetEqual(*this, TEXT("external excluded files"), FixtureFiles(Files), Normalized({Fixture.ExternalExcludedFile()}));

	UPjcSubsystem::GetFilesCorrupted(Files, false);
	TestSetEqual(*this, TEXT("corrupted files"), FixtureFiles(Files), Normalized({Fixture.CorruptedFile()}));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPjcTestScanCleanup, "ProjectCleaner.Scan.Cleanup", PJC_TEST_FLAGS)

bool FPjcTestScanCleanup::RunTest(const FString& Parameters) {
	if (!FPjcTestFixture::IsTestHost()) {
		AddInfo(TEXT("Skipped, fixture tests run only in test host projects"));
		return true;
	}

	// plugins scanned: deletion inside plugin content is covered too. Asset only a folder label that does not package
	// it references is deleted (engine refuses, label pass does it) and label is kept
	FPjcScopedScanSettings ScopedSettings {true};

	FPjcTestFixture Fixture;
	if (!Fixture.Build(*this)) return false;

	TestTrue(TEXT("DeleteAssetsUnused succeeded"), UPjcSubsystem::DeleteAssetsUnused(false, false));

	for (const auto& Name : Fixture.UnusedNames(true)) {
		TestFalse(FString::Printf(TEXT("unused %s deleted"), *Name), FPaths::FileExists(Fixture.PackageFile(Name)));
	}
	for (const auto& Name : Fixture.UsedNames(true)) {
		TestTrue(FString::Printf(TEXT("used %s kept"), *Name), FPaths::FileExists(Fixture.PackageFile(Name)));
	}

	TArray<FAssetData> Assets;
	UPjcSubsystem::GetAssetsUnused(Assets, false);
	TestSetEqual(*this, TEXT("unused after cleanup"), FixtureAssets(Assets), {});

	TestTrue(TEXT("DeleteFilesExternal succeeded"), UPjcSubsystem::DeleteFilesExternal(false, false));
	TestFalse(TEXT("external file deleted"), FPaths::FileExists(Fixture.ExternalFile()));
	TestTrue(TEXT("excluded external file kept"), FPaths::FileExists(Fixture.ExternalExcludedFile()));

	TestTrue(TEXT("DeleteFilesCorrupted succeeded"), UPjcSubsystem::DeleteFilesCorrupted(false, false));
	TestFalse(TEXT("corrupted file deleted"), FPaths::FileExists(Fixture.CorruptedFile()));

	TestTrue(TEXT("DeleteFoldersEmpty succeeded"), UPjcSubsystem::DeleteFoldersEmpty(false, false));
	for (const auto& Folder : Fixture.EmptyFolders()) {
		TestFalse(FString::Printf(TEXT("empty folder %s deleted"), *Folder), FPaths::DirectoryExists(Folder));
	}

	TArray<FString> Folders;
	UPjcSubsystem::GetFoldersEmpty(Folders);
	TestSetEqual(*this, TEXT("empty folders after cleanup"), FixtureFiles(Folders), {});

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPjcTestScanSettings, "ProjectCleaner.Scan.Settings", PJC_TEST_FLAGS)

bool FPjcTestScanSettings::RunTest(const FString& Parameters) {
	if (!FPjcTestFixture::IsTestHost()) {
		AddInfo(TEXT("Skipped, fixture tests run only in test host projects"));
		return true;
	}

	FPjcScopedScanSettings ScopedSettings {false};
	UPjcScanSettings* Settings = ScopedSettings.Settings;

	FPjcTestFixture Fixture;
	if (!Fixture.Build(*this)) return false;

	const TSet<FString> UnusedDefault = ObjectPaths(Fixture, Fixture.UnusedNames(false));
	TestSetEqual(*this, TEXT("defaults"), UnusedFixtureAssets(), UnusedDefault);

	Settings->bCinematicAssetsUsed = false;
	TestSetEqual(*this, TEXT("cinematics not used"), UnusedFixtureAssets(), UnusedDefault.Union(ObjectPaths(Fixture, {TEXT("LS_Cinematic")})));
	Settings->bCinematicAssetsUsed = true;

	Settings->UsedClasses.Add(UCurveFloat::StaticClass());
	TArray<FString> NotCurves;
	for (const auto& Name : Fixture.UnusedNames(false)) {
		if (!Name.StartsWith(TEXT("C_"))) {
			NotCurves.Add(Name);
		}
	}
	TestSetEqual(*this, TEXT("curves are used classes"), UnusedFixtureAssets(), ObjectPaths(Fixture, NotCurves));
	Settings->UsedClasses.Reset();

	Settings->bUnregisteredPrimaryUsed = false;
	const TSet<FString> WithOutside = UnusedDefault.Union(ObjectPaths(Fixture, {TEXT("BP_PrimaryOutside")}));
	TestSetEqual(*this, TEXT("unregistered primary classes not used"), UnusedFixtureAssets(), WithOutside);
	Settings->bUnregisteredPrimaryUsed = true;

	TArray<FAssetData> LabelOnly;
	UPjcSubsystem::GetAssetsFolderLabelOnly(LabelOnly, false);
	TestSetEqual(*this, TEXT("kept only by packaged folder labels"), FixtureAssets(LabelOnly), ObjectPaths(Fixture, {TEXT("C_Packaged")}));

	Settings->bScanProjectPlugins = true;
	Settings->IgnoredPlugins.Add(TEXT("PjcTestContent"));
	TestSetEqual(*this, TEXT("ignored plugin"), UnusedFixtureAssets(), UnusedDefault);
	TestFalse(TEXT("ignored plugin is not scan root"), UPjcSubsystem::GetProjectRootPaths().Contains(FPjcTestFixture::PluginRootPath()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPjcTestScanUsage, "ProjectCleaner.Scan.Usage", PJC_TEST_FLAGS)

bool FPjcTestScanUsage::RunTest(const FString& Parameters) {
	if (!FPjcTestFixture::IsTestHost()) {
		AddInfo(TEXT("Skipped, fixture tests run only in test host projects"));
		return true;
	}

	FPjcScopedScanSettings ScopedSettings {false};

	FPjcTestFixture Fixture;
	if (!Fixture.Build(*this)) return false;

	TArray<FAssetData> AssetsAll;
	TArray<FAssetData> AssetsUsed;
	TMap<FName, FPjcAssetUsage> Usage;
	UPjcSubsystem::GetAssetsAll(AssetsAll);
	UPjcSubsystem::GetAssetsUsed(AssetsUsed, false);
	UPjcSubsystem::GetAssetsUsage(Usage, false);

	// same verdict as used list, and every used asset leads to an asset used for its own reason
	TestEqual(TEXT("usage of every scanned asset"), Usage.Num(), AssetsAll.Num());
	const TSet<FAssetData> UsedSet {AssetsUsed};
	for (const auto& Asset : AssetsAll) {
		const FPjcAssetUsage* AssetUsage = Usage.Find(Asset.PackageName);
		if (!TestNotNull(*FString::Printf(TEXT("usage of %s"), *Asset.PackageName.ToString()), AssetUsage)) continue;

		TestEqual(*FString::Printf(TEXT("%s used"), *Asset.PackageName.ToString()), AssetUsage->bUsed, UsedSet.Contains(Asset));
		if (!AssetUsage->bUsed || !IsFixturePath(Asset.PackageName.ToString())) continue;

		const FPjcAssetUsage* Step = AssetUsage;
		int32 NumSteps = 0;
		while (Step && !Step->Parent.IsNone() && NumSteps < 32) {
			Step = Usage.Find(Step->Parent);
			++NumSteps;
		}
		const bool bRootReason = Step && Step->Reason != EPjcUsageReason::None && Step->Reason != EPjcUsageReason::Dependency;
		const bool bReachesRoot = bRootReason && Step->Parent.IsNone() && Step->Reason != EPjcUsageReason::FolderLabel;
		TestTrue(*FString::Printf(TEXT("%s chain reaches asset used for its own reason"), *Asset.PackageName.ToString()), bReachesRoot);
	}

	const auto TestReason = [&](const TCHAR* Name, const EPjcUsageReason Reason, const TCHAR* Parent, const TCHAR* DetailPart) {
		const FPjcAssetUsage* AssetUsage = Usage.Find(PackageOf(Fixture, Name));
		if (!TestNotNull(*FString::Printf(TEXT("usage of %s"), Name), AssetUsage)) return;

		TestEqual(*FString::Printf(TEXT("%s reason"), Name), static_cast<int32>(AssetUsage->Reason), static_cast<int32>(Reason));
		TestEqual(*FString::Printf(TEXT("%s parent"), Name), AssetUsage->Parent, Parent ? PackageOf(Fixture, Parent) : FName {NAME_None});
		if (DetailPart) {
			const FString DetailCheck = FString::Printf(TEXT("%s detail '%s' has '%s'"), Name, *AssetUsage->Detail, DetailPart);
			TestTrue(*DetailCheck, AssetUsage->Detail.Contains(DetailPart));
		}
	};

	TestReason(TEXT("PAL_Root"), EPjcUsageReason::Primary, nullptr, nullptr);
	TestReason(TEXT("OL_Chain"), EPjcUsageReason::Dependency, TEXT("PAL_Root"), nullptr);
	TestReason(TEXT("C_ChainLeaf"), EPjcUsageReason::Dependency, TEXT("OL_Chain"), nullptr);
	TestReason(TEXT("BP_Primary"), EPjcUsageReason::Primary, nullptr, nullptr);
	TestReason(TEXT("BP_PrimaryOutside"), EPjcUsageReason::PrimaryClass, nullptr, nullptr);
	TestReason(TEXT("C_Indirect"), EPjcUsageReason::Indirect, nullptr, TEXT("PjcTests.ini, line 2"));
	TestReason(TEXT("C_RefByPlugin"), EPjcUsageReason::EngineReferenced, nullptr, TEXT("/PjcTestContent/"));
	TestReason(TEXT("EUB_Tool"), EPjcUsageReason::Editor, nullptr, nullptr);
	TestReason(TEXT("BP_AnimModifier"), EPjcUsageReason::Editor, nullptr, nullptr);
	if (FPjcTestFixture::ValidatorBaseClass()) {
		TestReason(TEXT("BP_Validator"), EPjcUsageReason::Editor, nullptr, nullptr);
		FPjcTestFixture::Case(*this, TEXT("validator blueprint"));
	}
	else {
		FPjcTestFixture::Case(*this, TEXT("validator blueprint"), TEXT("DataValidation plugin not loaded"));
	}
	TestReason(TEXT("C_ByAsset"), EPjcUsageReason::Excluded, nullptr, TEXT("Excluded asset"));
	TestReason(TEXT("C_ByFolder"), EPjcUsageReason::Excluded, nullptr, TEXT("In excluded folder"));
	TestReason(TEXT("CLC_ByClass"), EPjcUsageReason::Excluded, nullptr, TEXT("Excluded class"));
	TestReason(TEXT("LS_Cinematic"), EPjcUsageReason::Cinematic, nullptr, TEXT("Cinematic"));
	TestReason(TEXT("C_PjcSplash"), EPjcUsageReason::Splash, nullptr, nullptr);
	TestReason(TEXT("C_Labeled"), EPjcUsageReason::None, nullptr, nullptr);
	TestReason(TEXT("C_Packaged"), EPjcUsageReason::FolderLabel, TEXT("PAL_Packaged"), nullptr);
	TestReason(TEXT("C_UnusedLeaf"), EPjcUsageReason::None, nullptr, nullptr);
	TestReason(TEXT("C_Indirect_fr"), EPjcUsageReason::Localized, TEXT("C_Indirect"), nullptr);
	TestReason(TEXT("C_AlwaysCooked"), EPjcUsageReason::ConfigFolder, nullptr, TEXT("/Game/PjcTests/AlwaysCook"));
	TestReason(TEXT("C_InPython"), EPjcUsageReason::Indirect, nullptr, TEXT("PjcTests.py"));
	TestReason(TEXT("PAL_Collection"), EPjcUsageReason::Primary, nullptr, nullptr);
	TestReason(TEXT("C_InCollection"), EPjcUsageReason::Dependency, TEXT("PAL_Collection"), TEXT("PjcTestCollection"));
	TestReason(TEXT("C_InPlatformConfig"), EPjcUsageReason::Indirect, nullptr, TEXT("PjcPlatformTests.ini"));
	TestReason(TEXT("C_RegistryData"), EPjcUsageReason::ConfigFolder, nullptr, TEXT("/Game/PjcTests/Registry"));
	TestReason(TEXT("C_Unused_fr"), EPjcUsageReason::None, nullptr, nullptr);
	if (Fixture.HasCues()) {
		TestReason(TEXT("GC_PjcTest_Fired"), EPjcUsageReason::GameplayCue, nullptr, TEXT("fired by BP_CueUser"));
		TestReason(TEXT("GC_PjcTest_Parent"), EPjcUsageReason::GameplayCue, nullptr, TEXT("GameplayCue.PjcTest.Parent.Child"));
		TestReason(TEXT("GC_PjcTest_Native"), EPjcUsageReason::GameplayCue, nullptr, TEXT("named in source or config"));
		TestReason(TEXT("GC_PjcTest_Unfired"), EPjcUsageReason::GameplayCueNotFired, nullptr, TEXT("GameplayCue.PjcTest.Unfired"));
		TestReason(TEXT("GC_PjcTest_ByUnused"), EPjcUsageReason::GameplayCueNotFired, nullptr, nullptr);
		TestReason(TEXT("GC_PjcTest_Outside"), EPjcUsageReason::GameplayCueNotFired, nullptr, TEXT("outside"));
		FPjcTestFixture::Case(*this, TEXT("GameplayCue reasons"));
	}
	else {
		FPjcTestFixture::Case(*this, TEXT("GameplayCue reasons"), TEXT("GameplayAbilities plugin not loaded"));
	}

	const FPjcAssetUsage* Packaged = Usage.Find(PackageOf(Fixture, TEXT("C_Packaged")));
	if (TestNotNull(TEXT("usage of C_Packaged"), Packaged)) {
		TestTrue(TEXT("C_Packaged kept only by folder label"), Packaged->bFolderLabelOnly && Packaged->bUsed);
	}
	const FPjcAssetUsage* Labeled = Usage.Find(PackageOf(Fixture, TEXT("C_Labeled")));
	if (TestNotNull(TEXT("usage of C_Labeled"), Labeled)) {
		TestFalse(TEXT("C_Labeled unused, its label does not package it"), Labeled->bUsed || Labeled->bFolderLabelOnly);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPjcTestScanResult, "ProjectCleaner.Scan.Result", PJC_TEST_FLAGS)

bool FPjcTestScanResult::RunTest(const FString& Parameters) {
	if (!FPjcTestFixture::IsTestHost()) {
		AddInfo(TEXT("Skipped, fixture tests run only in test host projects"));
		return true;
	}

	FPjcScopedScanSettings ScopedSettings {false};

	FPjcTestFixture Fixture;
	if (!Fixture.Build(*this)) return false;

	// unused redirector, like the engine leaves behind after a move (the fixture has saved ones only in its Lyra layout).
	// Kept in memory and taken out of the registry at the end, the engine deletes redirectors only when loaded from disk.
	const FString RedirectorPackageName = FPjcTestFixture::RootPath() / TEXT("Redirectors/R_Test");
	UObjectRedirector* Redirector = NewObject<UObjectRedirector>(CreatePackage(*RedirectorPackageName), TEXT("R_Test"), RF_Public | RF_Standalone);
	Redirector->DestinationObject = FSoftObjectPath {Fixture.ObjectPath(TEXT("C_UnusedLeaf"))}.ResolveObject();
	FAssetRegistryModule::AssetCreated(Redirector);
	// registry before 5.5 unregisters the redirection on delete but does not register it on create (ensure)
#if ENGINE_MAJOR_VERSION < 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0)
	GRedirectCollector.AddAssetPathRedirection(FName {*Redirector->GetPathName()}, FName {*Fixture.ObjectPath(TEXT("C_UnusedLeaf"))});
#elif ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 5
	GRedirectCollector.AddAssetPathRedirection(FSoftObjectPath {Redirector}, FSoftObjectPath {Fixture.ObjectPath(TEXT("C_UnusedLeaf"))});
#endif
	ON_SCOPE_EXIT {
		// garbage collection frees it
		FAssetRegistryModule::AssetDeleted(Redirector);
		Redirector->DestinationObject = nullptr;
		Redirector->ClearFlags(RF_Public | RF_Standalone);
	};

	int32 NumBroadcasts = 0;
	const FDelegateHandle Handle = UPjcSubsystem::OnScanFinished().AddLambda([&NumBroadcasts]() {
		++NumBroadcasts;
	});
	const bool bScanned = UPjcSubsystem::Scan(false);
	UPjcSubsystem::OnScanFinished().Remove(Handle);

	TestTrue(TEXT("scan made a result"), bScanned);
	TestEqual(TEXT("scan finished broadcast once"), NumBroadcasts, 1);

	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	TestTrue(TEXT("result is valid"), Result.IsValid());

	// every place a file names an asset, for the Source & config list of Assets: the fixture config names C_Indirect on line 2
	const TArray<FPjcSourceRef>* SourceRefs = Result.SourceRefs.Find(PackageOf(Fixture, TEXT("C_Indirect")));
	if (TestNotNull(TEXT("source places of C_Indirect"), SourceRefs)) {
		const bool bConfigLine = SourceRefs->ContainsByPredicate([](const FPjcSourceRef& Ref) {
			return Ref.File.EndsWith(TEXT("PjcTests.ini")) && Ref.Line == 2 && FPaths::IsRelative(Ref.File);
		});
		TestTrue(TEXT("C_Indirect named in PjcTests.ini on line 2, relative to the project"), bConfigLine);
	}

	// same answers as the separate queries
	TArray<FAssetData> AssetsAll;
	TArray<FAssetData> AssetsUsed;
	TArray<FAssetData> AssetsUnused;
	UPjcSubsystem::GetAssetsAll(AssetsAll);
	UPjcSubsystem::GetAssetsUsed(AssetsUsed, false);
	UPjcSubsystem::GetAssetsUnused(AssetsUnused, false);
	// redirectors are not content: own list, and out of every other list
	const TSet<FString> Redirectors = ToObjectPaths(Result.Redirectors);
	const auto WithoutRedirectors = [&Redirectors](const TArray<FAssetData>& Assets) {
		return ToObjectPaths(Assets).Difference(Redirectors);
	};
	const FName RedirectorPackage {*RedirectorPackageName};
	const auto IsRedirectorPackage = [&RedirectorPackage](const FAssetData& Asset) {
		return Asset.PackageName == RedirectorPackage;
	};
	TestTrue(TEXT("R_Test in redirectors"), Result.Redirectors.ContainsByPredicate(IsRedirectorPackage));
	TestFalse(TEXT("R_Test not in assets"), Result.Assets.ContainsByPredicate(IsRedirectorPackage));
	TestFalse(TEXT("R_Test not in unused"), Result.Unused.ContainsByPredicate(IsRedirectorPackage));
	TestFalse(TEXT("R_Test has no size"), Result.Sizes.Contains(RedirectorPackage));
	TestTrue(TEXT("R_Test keeps its usage"), Result.Usage.Contains(RedirectorPackage));
	TestFalse(TEXT("R_Test is no unused asset"), AssetsUnused.ContainsByPredicate(IsRedirectorPackage));
	TestFalse(TEXT("R_Test is no asset"), AssetsAll.ContainsByPredicate(IsRedirectorPackage));
	TestTrue(TEXT("redirectors are not in assets"), ToObjectPaths(Result.Assets).Intersect(Redirectors).Num() == 0);
	TestEqual(TEXT("result assets"), ToObjectPaths(Result.Assets).Num(), WithoutRedirectors(AssetsAll).Num());
	TestTrue(TEXT("result used assets"), ToObjectPaths(Result.Used).Difference(ToObjectPaths(AssetsUsed)).Num() == 0);
	TestTrue(TEXT("result unused assets"), ToObjectPaths(Result.Unused).Difference(ToObjectPaths(AssetsUnused)).Num() == 0);
	TestEqual(TEXT("result unused count"), Result.Unused.Num(), WithoutRedirectors(AssetsUnused).Num());

	// every scanned asset is used or unused, has usage and a size; chains keep their redirectors
	TestEqual(TEXT("used and unused cover every asset"), Result.Used.Num() + Result.Unused.Num(), Result.Assets.Num());
	TestEqual(TEXT("usage of every asset and redirector"), Result.Usage.Num(), Result.Assets.Num() + Result.Redirectors.Num());
	TestEqual(TEXT("size of every asset"), Result.Sizes.Num(), Result.Assets.Num());
	for (const auto& Asset : Result.Unused) {
		TestFalse(*FString::Printf(TEXT("%s unused in usage"), *Asset.PackageName.ToString()), Result.IsUsed(Asset.PackageName));
	}

	const FName UnusedLeaf = PackageOf(Fixture, TEXT("C_UnusedLeaf"));
	TestFalse(TEXT("C_UnusedLeaf unused"), Result.IsUsed(UnusedLeaf));
	TestTrue(TEXT("C_UnusedLeaf has a size"), Result.GetSize(UnusedLeaf) > 0);
	TestTrue(TEXT("PAL_Root used"), Result.IsUsed(PackageOf(Fixture, TEXT("PAL_Root"))));

	// status: game use, editor only use, unused
	TestEqual(TEXT("status of every asset"), Result.Status.Num(), Result.Assets.Num());
	int32 NumUnusedStatus = 0;
	for (const auto& Pair : Result.Status) {
		NumUnusedStatus += Pair.Value == EPjcAssetStatus::Unused ? 1 : 0;
	}
	TestEqual(TEXT("unused status of every unused asset"), NumUnusedStatus, Result.Unused.Num());
	const auto TestStatus = [&](const TCHAR* Name, const EPjcAssetStatus Status, const EPjcUsageReason ChainReason) {
		const FName Package = PackageOf(Fixture, Name);
		TestEqual(*FString::Printf(TEXT("%s status"), Name), static_cast<int32>(Result.GetStatus(Package)), static_cast<int32>(Status));
		const int32 ChainActual = static_cast<int32>(Result.GetChainReason(Package));
		TestEqual(*FString::Printf(TEXT("%s chain reason"), Name), ChainActual, static_cast<int32>(ChainReason));
	};
	// labels cook nothing themselves; without cook rules what they list or keep ships only through other references
	TestStatus(TEXT("PAL_Root"), EPjcAssetStatus::EditorOnly, EPjcUsageReason::Primary);
	TestStatus(TEXT("OL_Chain"), EPjcAssetStatus::EditorOnly, EPjcUsageReason::Primary);
	TestStatus(TEXT("C_ChainLeaf"), EPjcAssetStatus::EditorOnly, EPjcUsageReason::Primary);
	TestStatus(TEXT("C_Labeled"), EPjcAssetStatus::Unused, EPjcUsageReason::None);
	TestStatus(TEXT("C_PjcSplash"), EPjcAssetStatus::EditorOnly, EPjcUsageReason::Splash);
	TestStatus(TEXT("C_Packaged"), EPjcAssetStatus::Used, EPjcUsageReason::FolderLabel);
	TestStatus(TEXT("PAL_Packaged"), EPjcAssetStatus::Used, EPjcUsageReason::Primary);
	TestTrue(TEXT("C_Labeled listed by its folder label"), Result.FolderLabelUnused.Contains(PackageOf(Fixture, TEXT("C_Labeled"))));
	TestEqual(TEXT("unused assets folder labels list: C_Labeled, BP_Labeled"), Result.FolderLabelUnused.Num(), 2);
	TestStatus(TEXT("BP_Primary"), EPjcAssetStatus::Used, EPjcUsageReason::Primary);
	TestStatus(TEXT("C_Indirect"), EPjcAssetStatus::Used, EPjcUsageReason::Indirect);
	TestStatus(TEXT("C_Indirect_fr"), EPjcAssetStatus::Used, EPjcUsageReason::Localized);
	TestStatus(TEXT("C_AlwaysCooked"), EPjcAssetStatus::Used, EPjcUsageReason::ConfigFolder);
	TestStatus(TEXT("C_Unused_fr"), EPjcAssetStatus::Unused, EPjcUsageReason::None);
	TestStatus(TEXT("EUB_Tool"), EPjcAssetStatus::EditorOnly, EPjcUsageReason::Editor);
	TestStatus(TEXT("C_UnusedLeaf"), EPjcAssetStatus::Unused, EPjcUsageReason::None);
	if (Fixture.HasCues()) {
		TestStatus(TEXT("GC_PjcTest_Fired"), EPjcAssetStatus::Used, EPjcUsageReason::GameplayCue);
		for (const auto& Name : Fixture.ReviewNames()) {
			TestStatus(*Name, EPjcAssetStatus::Review, EPjcUsageReason::GameplayCueNotFired);
		}
		TestStatus(TEXT("BP_CueUnused"), EPjcAssetStatus::Unused, EPjcUsageReason::None);
	}

	// overview: every asset counted once by status, by reason and by class
	const FPjcOverview Overview = FPjcOverview::Make(Result);
	TestEqual(TEXT("overview all"), Overview.All.Num, Result.Assets.Num());
	const int32 NumByStatus = Overview.Used.Num + Overview.EditorOnly.Num + Overview.Review.Num + Overview.Unused.Num;
	TestEqual(TEXT("overview by status"), NumByStatus, Overview.All.Num);
	TestEqual(TEXT("overview unused"), Overview.Unused.Num, Result.Unused.Num());
	int32 NumByReason = 0;
	int64 SizeByReason = 0;
	const FPjcOverview::FReason* Unreferenced = nullptr;
	const FPjcOverview::FReason* Indirect = nullptr;
	for (const auto& Row : Overview.Reasons) {
		NumByReason += Row.Total.Num;
		SizeByReason += Row.Total.Size;
		Unreferenced = Row.Reason == EPjcUsageReason::None ? &Row : Unreferenced;
		Indirect = Row.Reason == EPjcUsageReason::Indirect ? &Row : Indirect;
	}
	// reasons tell why assets are used: unused and review ones are not counted
	TestEqual(TEXT("overview by reason"), NumByReason, Overview.Used.Num + Overview.EditorOnly.Num);
	TestEqual(TEXT("overview size by reason"), SizeByReason, Overview.Used.Size + Overview.EditorOnly.Size);
	TestTrue(TEXT("overview has no unused reason"), Unreferenced == nullptr);
	TestTrue(TEXT("overview source and config reason"), Indirect && Indirect->Total.Num > 0);
	int32 NumByClass = 0;
	for (int32 Index = 0; Index < Overview.Classes.Num(); ++Index) {
		const FPjcOverview::FClass& Class = Overview.Classes[Index];
		NumByClass += Class.Total.Num;
		const int64 SizeSum = Class.SizeUsed + Class.SizeEditorOnly + Class.SizeReview + Class.SizeUnused;
		TestEqual(*FString::Printf(TEXT("%s size by status"), *Class.Name.ToString()), SizeSum, Class.Total.Size);
		TestTrue(TEXT("classes biggest first"), Index == 0 || Overview.Classes[Index - 1].Total.Size >= Class.Total.Size);
	}
	TestEqual(TEXT("overview by class"), NumByClass, Overview.All.Num);
	const int32 MaxLargest = FMath::Min(Result.Unused.Num(), static_cast<int32>(FPjcOverview::MaxLargestUnused));
	TestEqual(TEXT("largest unused"), Overview.LargestUnused.Num(), MaxLargest);
	for (int32 Index = 1; Index < Overview.LargestUnused.Num(); ++Index) {
		TestTrue(TEXT("largest unused biggest first"), Overview.LargestUnused[Index - 1].Size >= Overview.LargestUnused[Index].Size);
	}
	TestTrue(TEXT("unused folders found"), Overview.UnusedFolders.Num() > 0);
	for (const auto& Folder : Overview.UnusedFolders) {
		TestTrue(*FString::Printf(TEXT("%s mostly unused"), *Folder.Path), Folder.Unused.Size * 2 >= Folder.All.Size && Folder.Unused.Num > 0);
		const bool bParentListed = Overview.UnusedFolders.ContainsByPredicate([&Folder](const FPjcOverview::FFolder& Other) {
			return Folder.Path.StartsWith(Other.Path + TEXT("/"));
		});
		TestFalse(*FString::Printf(TEXT("%s parent not listed"), *Folder.Path), bParentListed);
	}

	// issues: same answers as the separate queries
	TestSetEqual(*this, TEXT("result external files"), FixtureFiles(Result.FilesExternal), Normalized({Fixture.ExternalFile()}));
	TestSetEqual(*this, TEXT("result kept external files"), FixtureFiles(Result.FilesExternalKept), Normalized({Fixture.ExternalExcludedFile()}));
	TestSetEqual(*this, TEXT("result corrupted files"), FixtureFiles(Result.FilesCorrupted), Normalized({Fixture.CorruptedFile()}));
	TestSetEqual(*this, TEXT("result empty folders"), FixtureFiles(Result.FoldersEmpty), Normalized(Fixture.EmptyFolders()));

	// project changes after the scan: an asset added and removed outside Project Cleaner, a new scan starts over
	TestEqual(TEXT("no change right after the scan"), UPjcSubsystem::GetChangesSinceScan().Num(), 0);
	const FString ChangedPackageName = FPjcTestFixture::RootPath() / TEXT("Misc/C_Changed");
	UCurveFloat* Changed = NewObject<UCurveFloat>(CreatePackage(*ChangedPackageName), TEXT("C_Changed"), RF_Public | RF_Standalone);
	FAssetRegistryModule::AssetCreated(Changed);
	TestEqual(TEXT("added asset counted"), UPjcSubsystem::GetChangesSinceScan().NumAdded, 1);
	FAssetRegistryModule::AssetDeleted(Changed);
	Changed->ClearFlags(RF_Public | RF_Standalone);
	TestEqual(TEXT("removed asset counted"), UPjcSubsystem::GetChangesSinceScan().NumRemoved, 1);
	const bool bRecent = UPjcSubsystem::GetChangesSinceScan().Recent.Contains(FName {*ChangedPackageName});
	TestTrue(TEXT("changed package listed"), bRecent);
	TestTrue(TEXT("scanned again"), UPjcSubsystem::Scan(false));
	TestEqual(TEXT("no change after the new scan"), UPjcSubsystem::GetChangesSinceScan().Num(), 0);

	// what the Scan settings pickers offer
	TArray<FPjcPickerItem> Picked;
	SPjcScanSettings::GetPickerItems(FName {TEXT("ExcludedClasses")}, Picked);
	const FPjcPickerItem* CurveClass = Picked.FindByPredicate([](const FPjcPickerItem& Item) {
		return Item.Value == UCurveFloat::StaticClass()->GetPathName();
	});
	TestTrue(TEXT("picker classes with their assets"), CurveClass && CurveClass->Sort > 0 && CurveClass->Label == TEXT("CurveFloat"));
	SPjcScanSettings::GetPickerItems(FName {TEXT("ExcludedFolders")}, Picked);
	const bool bFolder = Picked.ContainsByPredicate([](const FPjcPickerItem& Item) {
		return Item.Value == FPjcTestFixture::RootPath() / TEXT("Unused");
	});
	TestTrue(TEXT("picker folders"), bFolder);
	SPjcScanSettings::GetPickerItems(FName {TEXT("ExcludedAssets")}, Picked);
	TestEqual(TEXT("picker assets"), Picked.Num(), UPjcSubsystem::GetScanResult().Assets.Num());

	// a window follows the scan step by step; a cancel keeps the last result
	TSet<int32> StepsFound;
	FPjcScanProgress Progress;
	Progress.OnFound = [&StepsFound](const EPjcScanStep Step, const FString& Found) {
		StepsFound.Add(static_cast<int32>(Step));
	};
	TestTrue(TEXT("scanned with progress"), UPjcSubsystem::Scan(false, &Progress));
	// every step up to Files (Cook check only with a cook)
	for (int32 Step = 0; Step <= static_cast<int32>(EPjcScanStep::Files); ++Step) {
		TestTrue(*FString::Printf(TEXT("step %d found"), Step), StepsFound.Contains(Step));
	}
	const FDateTime LastTime = UPjcSubsystem::GetScanResult().Time;
	FPjcScanProgress Cancel;
	Cancel.bCancelRequested = true;
	TestFalse(TEXT("canceled scan"), UPjcSubsystem::Scan(false, &Cancel));
	TestTrue(TEXT("cancel told"), Cancel.bCanceled);
	TestEqual(TEXT("last result stays"), UPjcSubsystem::GetScanResult().Time, LastTime);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPjcTestScanDraft, "ProjectCleaner.Scan.Draft", PJC_TEST_FLAGS)

bool FPjcTestScanDraft::RunTest(const FString& Parameters) {
	if (!FPjcTestFixture::IsTestHost()) {
		AddInfo(TEXT("Skipped, writes DefaultEditor.ini, runs only in test host projects"));
		return true;
	}

	FPjcScopedScanSettings ScopedSettings {false};
	FPjcScanDraft::Revert();
	TestFalse(TEXT("draft starts as the settings"), FPjcScanDraft::IsChanged());

	// not edited yet: follows the settings when they change elsewhere
	ScopedSettings.Settings->bUnregisteredPrimaryUsed = false;
	TestFalse(TEXT("draft follows settings until edited"), FPjcScanDraft::Get().bUnregisteredPrimaryUsed || FPjcScanDraft::IsChanged());
	ScopedSettings.Settings->bUnregisteredPrimaryUsed = true;

	// editing the draft leaves the settings alone until applied
	FPjcScanDraft::Get().bUnregisteredPrimaryUsed = false;
	FPjcScanDraft::NotifyEdited();
	TestTrue(TEXT("edited draft is changed"), FPjcScanDraft::IsChanged());
	TestTrue(TEXT("settings untouched by the draft"), ScopedSettings.Settings->bUnregisteredPrimaryUsed);
	FPjcScanDraft::Revert();
	TestFalse(TEXT("undo back to the settings"), FPjcScanDraft::IsChanged());
	TestTrue(TEXT("undo restores the value"), FPjcScanDraft::Get().bUnregisteredPrimaryUsed);

	FPjcScanDraft::Get().bCinematicAssetsUsed = false;
	FPjcScanDraft::Get().UsedClasses.Add(UCurveFloat::StaticClass()->GetPathName());
	FPjcScanDraft::NotifyEdited();
	FPjcScanDraft::Get().ApplyToSettings();
	TestFalse(TEXT("applied draft is not changed"), FPjcScanDraft::IsChanged());
	TestFalse(TEXT("applied to settings"), ScopedSettings.Settings->bCinematicAssetsUsed);
	if (TestEqual(TEXT("used classes applied"), ScopedSettings.Settings->UsedClasses.Num(), 1)) {
		TestEqual(TEXT("used class resolves"), ScopedSettings.Settings->UsedClasses[0].LoadSynchronous(), UCurveFloat::StaticClass());
	}

	TestTrue(TEXT("plugin defaults"), FPjcScanDraft {}.bUnregisteredPrimaryUsed && FPjcScanDraft {}.bCinematicAssetsUsed);

	// DefaultEditor.ini back to what it was
	ScopedSettings.Settings->bCinematicAssetsUsed = true;
	ScopedSettings.Settings->UsedClasses.Reset();
	PjcShim::UpdateDefaultConfigFile(ScopedSettings.Settings);
	FPjcScanDraft::Revert();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPjcTestExclusionsMoved, "ProjectCleaner.Settings.ExclusionsMoved", PJC_TEST_FLAGS)

bool FPjcTestExclusionsMoved::RunTest(const FString& Parameters) {
	if (!FPjcTestFixture::IsTestHost()) {
		AddInfo(TEXT("Skipped, writes DefaultEditor.ini, runs only in test host projects"));
		return true;
	}

	UPjcAssetExcludeSettings* Settings = GetMutableDefault<UPjcAssetExcludeSettings>();
	const TArray<FDirectoryPath> SavedFolders = Settings->ExcludedFolders;
	const TArray<TSoftClassPtr<UObject>> SavedClasses = Settings->ExcludedClasses;
	const TArray<TSoftObjectPtr<UObject>> SavedAssets = Settings->ExcludedAssets;
	Settings->ExcludedFolders = {FDirectoryPath {TEXT("/Game/PjcKept")}};
	Settings->ExcludedClasses.Reset();
	Settings->ExcludedAssets.Reset();

	// what versions before 3.1 saved per user
	const FString Section = UPjcAssetExcludeSettings::StaticClass()->GetPathName();
	GConfig->SetArray(*Section, TEXT("ExcludedFolders"), {TEXT("(Path=\"/Game/PjcMoved\")"), TEXT("(Path=\"/Game/PjcKept\")")}, GEditorPerProjectIni);
	GConfig->SetArray(*Section, TEXT("ExcludedClasses"), {TEXT("/Script/Engine.CurveFloat")}, GEditorPerProjectIni);
	GConfig->SetArray(*Section, TEXT("ExcludedAssets"), {TEXT("/Game/PjcMoved/C_Moved.C_Moved")}, GEditorPerProjectIni);

	TestEqual(TEXT("moved entries, the one already shared is not doubled"), UPjcSubsystem::MoveExclusionsToProject(), 3);
	TestEqual(TEXT("folders"), Settings->ExcludedFolders.Num(), 2);
	TestTrue(TEXT("moved folder"), Settings->ExcludedFolders.ContainsByPredicate([](const FDirectoryPath& Dir) {
		return Dir.Path == TEXT("/Game/PjcMoved");
	}));
	const FString MovedClass = Settings->ExcludedClasses.Num() == 1 ? Settings->ExcludedClasses[0].ToSoftObjectPath().ToString() : FString {};
	const FString MovedAsset = Settings->ExcludedAssets.Num() == 1 ? Settings->ExcludedAssets[0].ToSoftObjectPath().ToString() : FString {};
	TestEqual(TEXT("moved class"), MovedClass, FString {TEXT("/Script/Engine.CurveFloat")});
	TestEqual(TEXT("moved asset"), MovedAsset, FString {TEXT("/Game/PjcMoved/C_Moved.C_Moved")});

	TArray<FString> Left;
	GConfig->GetArray(*Section, TEXT("ExcludedFolders"), Left, GEditorPerProjectIni);
	TestEqual(TEXT("per user settings emptied"), Left.Num(), 0);
	TestEqual(TEXT("moves only once"), UPjcSubsystem::MoveExclusionsToProject(), 0);

	// file exclusions are shared too
	UPjcFileExcludeSettings* FileSettings = GetMutableDefault<UPjcFileExcludeSettings>();
	const TArray<FFilePath> SavedFiles = FileSettings->ExcludedFiles;
	const TArray<FString> SavedExtensions = FileSettings->ExcludedExtensions;
	FileSettings->ExcludedFiles.Reset();
	FileSettings->ExcludedExtensions = {TEXT("md")};
	const FString FileSection = UPjcFileExcludeSettings::StaticClass()->GetPathName();
	GConfig->SetArray(*FileSection, TEXT("ExcludedFiles"), {TEXT("(FilePath=\"Content/PjcMoved/notes.txt\")")}, GEditorPerProjectIni);
	GConfig->SetArray(*FileSection, TEXT("ExcludedExtensions"), {TEXT("md"), TEXT("zip")}, GEditorPerProjectIni);
	TestEqual(TEXT("moved file exclusions, the shared extension is not doubled"), UPjcSubsystem::MoveExclusionsToProject(), 2);
	const FString MovedFile = FileSettings->ExcludedFiles.Num() == 1 ? FileSettings->ExcludedFiles[0].FilePath : FString {};
	TestEqual(TEXT("moved file"), MovedFile, FString {TEXT("Content/PjcMoved/notes.txt")});
	TestTrue(TEXT("moved extension"), FileSettings->ExcludedExtensions.Contains(TEXT("zip")) && FileSettings->ExcludedExtensions.Num() == 2);
	GConfig->GetArray(*FileSection, TEXT("ExcludedExtensions"), Left, GEditorPerProjectIni);
	TestEqual(TEXT("per user file settings emptied"), Left.Num(), 0);
	FileSettings->ExcludedFiles = SavedFiles;
	FileSettings->ExcludedExtensions = SavedExtensions;
	PjcShim::UpdateDefaultConfigFile(FileSettings);

	Settings->ExcludedFolders = SavedFolders;
	Settings->ExcludedClasses = SavedClasses;
	Settings->ExcludedAssets = SavedAssets;
	PjcShim::UpdateDefaultConfigFile(Settings);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPjcTestCookShipped, "ProjectCleaner.Cook.Shipped", PJC_TEST_FLAGS)

bool FPjcTestCookShipped::RunTest(const FString& Parameters) {
	if (!FPjcTestFixture::IsTestHost()) {
		AddInfo(TEXT("Skipped, writes a cook into Saved/Cooked, runs only in test host projects"));
		return true;
	}

	FPjcScopedScanSettings ScopedSettings {false};
	FPjcTestFixture Fixture;
	if (!Fixture.Build(*this)) return false;
	if (!TestTrue(TEXT("scanned"), UPjcSubsystem::Scan(false))) return false;
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();

	// a cook as the cooker leaves it: a development asset registry, negative size for what it did not cook
	const FString PlatformDir = FPjcCookInfo::GetCookedDir() / TEXT("PjcTestPlatform");
	const FString RegistryFile = PlatformDir / FApp::GetProjectName() / TEXT("Metadata") / TEXT("DevelopmentAssetRegistry.bin");
	// Saved/Cooked goes too when the test made it
	const bool bCookedDirExisted = IFileManager::Get().DirectoryExists(*FPjcCookInfo::GetCookedDir());
	ON_SCOPE_EXIT {
		IFileManager::Get().DeleteDirectory(*(bCookedDirExisted ? PlatformDir : FPjcCookInfo::GetCookedDir()), false, true);
	};
	{
		FAssetRegistryState State;
		const auto AddPackage = [&State](const FName Package, const int64 DiskSize) {
			State.CreateOrGetAssetPackageData(Package)->DiskSize = DiskSize;
		};
		// made from the editor package as it is now: same saved hash (guid before 5.4)
		const FName Primary = PackageOf(Fixture, TEXT("BP_Primary"));
		*State.CreateOrGetAssetPackageData(Primary) = PjcShim::GetAssetPackageData(Primary);
		AddPackage(PackageOf(Fixture, TEXT("BP_Primary")), 1000);
		AddPackage(FName {TEXT("/Script/Engine")}, 0);
		AddPackage(PackageOf(Fixture, TEXT("C_UnusedLeaf")), 77);
		AddPackage(PackageOf(Fixture, TEXT("OL_UnusedRoot")), 88);
		AddPackage(PackageOf(Fixture, TEXT("EUB_Tool")), -1);
		AddPackage(FName {TEXT("/Engine/PjcTest/EngineThing")}, 500);
		AddPackage(FName {*(FPjcTestFixture::RootPath() / TEXT("_Generated_/Cell"))}, 10);
		AddPackage(FName {*(FPjcTestFixture::RootPath() / TEXT("DeletedSince"))}, 20);

		FAssetRegistrySerializationOptions Options;
		Options.bSerializeAssetRegistry = true;
		Options.bSerializePackageData = true;
		TArray<uint8> Bytes;
		FMemoryWriter Writer {Bytes};
		State.Serialize(Writer, Options);
		TestTrue(TEXT("cook written"), FFileHelper::SaveArrayToFile(Bytes, *RegistryFile));
	}

	const FPjcCookInfo& Cook = FPjcCookInfo::GetLast();
	TestTrue(TEXT("cook found"), Cook.IsValid());
	TestEqual(TEXT("cook platform"), Cook.Platform, FString {TEXT("PjcTestPlatform")});
	TestEqual(TEXT("cooked packages"), Cook.Cooked.Num(), 6);
	TestEqual(TEXT("cooked size"), Cook.Cooked.FindRef(PackageOf(Fixture, TEXT("BP_Primary"))), static_cast<int64>(1000));
	TestFalse(TEXT("not cooked is left out"), Cook.Cooked.Contains(PackageOf(Fixture, TEXT("EUB_Tool"))));

	// the scan made before the cook: its unused assets in the cook were saved before it, so the scan did not check them;
	// assets to review that were not cooked go with the unused ones
	int32 NumReview = 0;
	for (const auto& Pair : Result.Status) {
		NumReview += Pair.Value == EPjcAssetStatus::Review ? 1 : 0;
	}
	FPjcOverview::FShipped Shipped = FPjcOverview::MakeShipped(Result, Cook);
	TestEqual(TEXT("shipped"), Shipped.Shipped.Num, 1);
	TestEqual(TEXT("shipped cooked size"), Shipped.ShippedCooked, static_cast<int64>(1000));
	TestEqual(TEXT("unused shipped, nothing in review yet"), Shipped.UnusedShipped.Num, 0);
	TestEqual(TEXT("used, not shipped"), Shipped.UsedNotShipped.Num, Result.Used.Num() - 1 - NumReview);
	TestEqual(TEXT("unused, not shipped"), Shipped.Unused.Num, Result.Unused.Num() + NumReview);
	TestEqual(TEXT("outside the scan"), Shipped.NumOutside, 1);
	TestEqual(TEXT("outside size"), Shipped.SizeOutside, static_cast<int64>(500));
	TestTrue(TEXT("outside root"), Shipped.OutsideRoots.Num() == 1 && Shipped.OutsideRoots[0].Key == TEXT("/Engine"));
	TestEqual(TEXT("generated"), Shipped.NumGenerated, 1);
	TestEqual(TEXT("deleted since"), Shipped.NumDeleted, 1);
	TestEqual(TEXT("nothing changed since"), Shipped.NumChanged, 0);

	TestFalse(TEXT("script packages left out"), Cook.Cooked.Contains(FName {TEXT("/Script/Engine")}));
	TestTrue(TEXT("saved digest read"), Cook.SavedDigests.FindRef(PackageOf(Fixture, TEXT("BP_Primary"))) != 0);
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
	FPjcTestFixture::Case(*this, TEXT("cook saved hash"));
#else
	FPjcTestFixture::Case(*this, TEXT("cook saved hash"), TEXT("UE before 5.4 saves no package hash, the package guid is compared"));
#endif

	// files touched after the cook: changed only when what they were saved with differs, file time when that is not known
	FString PrimaryFile;
	FString LeafFile;
	const bool bPrimaryFile = PjcShim::DoesPackageExist(PackageOf(Fixture, TEXT("BP_Primary")).ToString(), &PrimaryFile);
	const bool bFiles = bPrimaryFile && PjcShim::DoesPackageExist(PackageOf(Fixture, TEXT("C_UnusedLeaf")).ToString(), &LeafFile);
	if (TestTrue(TEXT("asset files"), bFiles)) {
		IFileManager::Get().SetTimeStamp(*PrimaryFile, Cook.Time + FTimespan::FromMinutes(1.0));
		Shipped = FPjcOverview::MakeShipped(Result, Cook);
		TestEqual(TEXT("touched, same content"), Shipped.NumChanged, 0);

		IFileManager::Get().SetTimeStamp(*LeafFile, Cook.Time + FTimespan::FromMinutes(1.0));
		Shipped = FPjcOverview::MakeShipped(Result, Cook);
		TestEqual(TEXT("changed since"), Shipped.NumChanged, 1);
	}

	// not seen by the cook: counts only when the game uses it (new since the cook)
	FString UnusedFile;
	FString IndirectFile;
	const bool bUnusedFile = PjcShim::DoesPackageExist(PackageOf(Fixture, TEXT("C_Unused_fr")).ToString(), &UnusedFile);
	const bool bUnseenFiles = bUnusedFile && PjcShim::DoesPackageExist(PackageOf(Fixture, TEXT("C_Indirect")).ToString(), &IndirectFile);
	if (TestTrue(TEXT("unseen asset files"), bUnseenFiles)) {
		IFileManager::Get().SetTimeStamp(*UnusedFile, Cook.Time + FTimespan::FromMinutes(1.0));
		IFileManager::Get().SetTimeStamp(*IndirectFile, Cook.Time + FTimespan::FromMinutes(1.0));
		Shipped = FPjcOverview::MakeShipped(Result, Cook);
		TestEqual(TEXT("new used asset since"), Shipped.NumChanged, 2);
	}

	// cook check: an unused asset the cook packaged goes to Review with what it uses (C_UnusedLeaf, saved since the
	// cook, so not checked itself). Cleanup keeps both
	if (!TestTrue(TEXT("scanned with the cook"), UPjcSubsystem::Scan(false))) return false;
	const FPjcScanResult& Checked = UPjcSubsystem::GetScanResult();
	const FName UnusedRoot = PackageOf(Fixture, TEXT("OL_UnusedRoot"));
	const FName UnusedLeaf = PackageOf(Fixture, TEXT("C_UnusedLeaf"));
	const FPjcAssetUsage* RootUsage = Checked.Usage.Find(UnusedRoot);
	const bool bPackagedRoot = RootUsage && RootUsage->Reason == EPjcUsageReason::CookPackaged;
	TestTrue(TEXT("packaged by the last cook"), bPackagedRoot && RootUsage->Detail == TEXT("PjcTestPlatform"));
	const int32 ReviewStatus = static_cast<int32>(EPjcAssetStatus::Review);
	TestEqual(TEXT("packaged unused asset in review"), static_cast<int32>(Checked.GetStatus(UnusedRoot)), ReviewStatus);
	TestEqual(TEXT("what it uses in review"), static_cast<int32>(Checked.GetStatus(UnusedLeaf)), ReviewStatus);
	TestEqual(TEXT("cook check platforms"), Checked.CookCheck.Platforms, TArray<FString> {TEXT("PjcTestPlatform")});
	TestEqual(TEXT("cook check packaged"), Checked.CookCheck.NumPackaged, 1);
	// left out by the cook and not saved since: checked, like a real cook that never visits what nothing references.
	// C_Unused_fr was touched after the cook
	TestEqual(TEXT("cook check not checked"), Checked.CookCheck.NumNotChecked, 1);
	TestEqual(TEXT("cook check checked, with the packaged one"), Checked.CookCheck.NumChecked, Checked.Unused.Num() - 1 + 1);
	TArray<FAssetData> UnusedWithCook;
	UPjcSubsystem::GetAssetsUnused(UnusedWithCook, false);
	const auto IsReviewAsset = [&UnusedRoot, &UnusedLeaf](const FAssetData& Asset) {
		return Asset.PackageName == UnusedRoot || Asset.PackageName == UnusedLeaf;
	};
	TestFalse(TEXT("cleanup keeps review assets"), UnusedWithCook.ContainsByPredicate(IsReviewAsset));
	TestEqual(TEXT("packaged review assets are unused, shipped"), FPjcOverview::MakeShipped(Checked, Cook).UnusedShipped.Num, 2);

	// a build machine's cook, from the setting: its platform from the path
	const FString MachineDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("PjcBuildMachine"));
	const FString MachineMetadata = MachineDir / TEXT("Cooked/PjcMachine") / FApp::GetProjectName() / TEXT("Metadata");
	IFileManager::Get().Copy(*(MachineMetadata / TEXT("DevelopmentAssetRegistry.bin")), *RegistryFile);
	UPjcScanSettings* ScanSettings = GetMutableDefault<UPjcScanSettings>();
	ScanSettings->CookMetadataFolder.Path = MachineMetadata;
	const bool bMachineCook = FPjcCookInfo::GetAll().ContainsByPredicate([](const FPjcCookInfo* Other) {
		return Other->Platform == TEXT("PjcMachine (build machine)") && Other->IsValid();
	});
	TestTrue(TEXT("build machine cook read"), bMachineCook);
	ScanSettings->CookMetadataFolder.Path.Reset();
	IFileManager::Get().DeleteDirectory(*MachineDir, false, true);

	IFileManager::Get().DeleteDirectory(*PlatformDir, false, true);
	TestFalse(TEXT("no cook"), FPjcCookInfo::GetLast().IsFound());
	UPjcSubsystem::GetAssetsUnused(UnusedWithCook, false);
	TestTrue(TEXT("unused again without the cook"), UnusedWithCook.ContainsByPredicate(IsReviewAsset));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPjcTestCookAll, "ProjectCleaner.Cook.All", PJC_TEST_FLAGS)

bool FPjcTestCookAll::RunTest(const FString& Parameters) {
	if (!FPjcTestFixture::IsTestHost()) {
		AddInfo(TEXT("Skipped, writes a cook into Saved/Cooked, runs only in test host projects"));
		return true;
	}

	FPjcScopedScanSettings ScopedSettings {false};
	FPjcTestFixture Fixture;
	if (!Fixture.Build(*this)) return false;
	if (!TestTrue(TEXT("scanned"), UPjcSubsystem::Scan(false))) return false;

	const TArray<FAssetData> Assets = UPjcSubsystem::GetScanResult().Assets;
	const int32 NumUnused = UPjcSubsystem::GetScanResult().Unused.Num();
	TestTrue(TEXT("enough unused assets to tell"), NumUnused >= 10);

	// a cook of everything: unused assets too
	const FString PlatformDir = FPjcCookInfo::GetCookedDir() / TEXT("PjcTestCookAll");
	const FString RegistryFile = PlatformDir / FApp::GetProjectName() / TEXT("Metadata") / TEXT("DevelopmentAssetRegistry.bin");
	const bool bCookedDirExisted = IFileManager::Get().DirectoryExists(*FPjcCookInfo::GetCookedDir());
	ON_SCOPE_EXIT {
		IFileManager::Get().DeleteDirectory(*(bCookedDirExisted ? PlatformDir : FPjcCookInfo::GetCookedDir()), false, true);
	};
	{
		FAssetRegistryState State;
		for (const auto& Asset : Assets) {
			State.CreateOrGetAssetPackageData(Asset.PackageName)->DiskSize = 100;
		}
		FAssetRegistrySerializationOptions Options;
		Options.bSerializeAssetRegistry = true;
		Options.bSerializePackageData = true;
		TArray<uint8> Bytes;
		FMemoryWriter Writer {Bytes};
		State.Serialize(Writer, Options);
		TestTrue(TEXT("cook written"), FFileHelper::SaveArrayToFile(Bytes, *RegistryFile));
	}

	// it says nothing about them: not in Review, and not counted as checked
	if (!TestTrue(TEXT("scanned with the cook"), UPjcSubsystem::Scan(false))) return false;
	const FPjcScanResult& Checked = UPjcSubsystem::GetScanResult();
	TestTrue(TEXT("cook of everything found"), Checked.CookCheck.IsCookAll());
	TestEqual(TEXT("its platform"), Checked.CookCheck.PlatformsCookAll, TArray<FString> {TEXT("PjcTestCookAll")});
	TestEqual(TEXT("nothing moved to review"), Checked.CookCheck.NumPackaged, 0);
	TestEqual(TEXT("unused stay unused"), Checked.Unused.Num(), NumUnused);
	TestEqual(TEXT("not counted as checked"), Checked.CookCheck.NumChecked, 0);
	TestEqual(TEXT("not counted as not checked"), Checked.CookCheck.NumNotChecked, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPjcTestUnusedDelete, "ProjectCleaner.Unused.Delete", PJC_TEST_FLAGS)

bool FPjcTestUnusedDelete::RunTest(const FString& Parameters) {
	if (!FPjcTestFixture::IsTestHost()) {
		AddInfo(TEXT("Skipped, deletes fixture assets, runs only in test host projects"));
		return true;
	}

	FPjcScopedScanSettings ScopedSettings {false};
	FPjcTestFixture Fixture;
	if (!Fixture.Build(*this)) return false;
	if (!TestTrue(TEXT("scanned"), UPjcSubsystem::Scan(false))) return false;
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();

	// folders with their unused totals
	const FPjcUnusedTree Tree = FPjcUnusedTree::Make(Result);
	const FString UnusedPath = FPjcTestFixture::RootPath() / TEXT("Unused");
	const TSharedPtr<FPjcUnusedFolder> Root = Tree.Find(TEXT("/Game"));
	const TSharedPtr<FPjcUnusedFolder> Unused = Tree.Find(UnusedPath);
	if (!TestTrue(TEXT("folders found"), Root.IsValid() && Unused.IsValid())) return false;
	TestTrue(TEXT("/Game is the first root"), Tree.Roots.Num() > 0 && Tree.Roots[0] == Root);
	TestTrue(TEXT("unused folder is all unused"), Unused->IsAllListed());
	TestTrue(TEXT("parent has the subfolder totals"), Tree.Find(FPjcTestFixture::RootPath())->Listed.Num >= Unused->Listed.Num);
	for (const auto& EmptyFolder : Fixture.EmptyFolders()) {
		const TSharedPtr<FPjcUnusedFolder> Folder = Tree.Find(UPjcSubsystem::PathConvertToRelative(EmptyFolder));
		TestTrue(*FString::Printf(TEXT("%s marked empty"), *EmptyFolder), Folder.IsValid() && Folder->bEmpty);
	}
	TArray<int32> Direct;
	TArray<int32> WithSubfolders;
	Tree.GetListed(Result, UnusedPath, false, Direct);
	Tree.GetListed(Result, UnusedPath, true, WithSubfolders);
	TestEqual(TEXT("unused with subfolders"), WithSubfolders.Num(), Unused->Listed.Num);
	TestTrue(TEXT("unused directly in the folder"), Direct.Num() > 0 && Direct.Num() < WithSubfolders.Num());

	// every list counts its own assets: unused, used, review and excluded
	const auto NumInSet = [&Result](const EPjcAssetSet Set) {
		int32 Num = 0;
		for (const auto& Asset : Result.Assets) {
			Num += FPjcUnusedTree::IsInSet(Result, Asset.PackageName, Set) ? 1 : 0;
		}
		return Num;
	};
	const int32 NumUsed = NumInSet(EPjcAssetSet::Used);
	const int32 NumReview = NumInSet(EPjcAssetSet::Review);
	TestEqual(TEXT("lists cover every asset"), NumInSet(EPjcAssetSet::Unused) + NumUsed + NumReview, Result.Assets.Num());
	TestEqual(TEXT("unused list"), NumInSet(EPjcAssetSet::Unused), Result.Unused.Num());
	TestEqual(TEXT("review list"), NumReview, Fixture.ReviewNames().Num());
	for (const EPjcAssetSet Set : {EPjcAssetSet::Used, EPjcAssetSet::Review, EPjcAssetSet::Excluded}) {
		const FPjcUnusedTree SetTree = FPjcUnusedTree::Make(Result, Set);
		int32 NumRoots = 0;
		for (const auto& SetRoot : SetTree.Roots) {
			NumRoots += SetRoot->Listed.Num;
		}
		TestEqual(*FString::Printf(TEXT("list %d totals"), static_cast<int32>(Set)), NumRoots, NumInSet(Set));
	}

	// exclusion rules: which one keeps each asset, and that only the rule keeps it
	const auto TestExclusion = [&](const TCHAR* Name, const EPjcExclusionKind Kind) {
		const FPjcExclusion* Exclusion = Result.Excluded.Find(PackageOf(Fixture, Name));
		if (!TestNotNull(*FString::Printf(TEXT("%s excluded"), Name), Exclusion)) return;

		TestEqual(*FString::Printf(TEXT("%s rule"), Name), static_cast<int32>(Exclusion->Kind), static_cast<int32>(Kind));
		TestFalse(*FString::Printf(TEXT("%s unused without its rule"), Name), Exclusion->bUsedAnyway);
	};
	TestExclusion(TEXT("C_ByAsset"), EPjcExclusionKind::Asset);
	TestExclusion(TEXT("C_ByFolder"), EPjcExclusionKind::Folder);
	TestExclusion(TEXT("CLC_ByClass"), EPjcExclusionKind::Class);
	TestEqual(TEXT("excluded list"), NumInSet(EPjcAssetSet::Excluded), Result.Excluded.Num());

	// a used asset that references the selection refuses the delete: it would break
	TArray<FName> Holders;
	FPjcUnusedTree::GetUsedHolders(Result, {PackageOf(Fixture, TEXT("C_ChainLeaf"))}, Holders);
	TestEqual(TEXT("used referencer holds a used asset"), Holders, TArray<FName> {PackageOf(Fixture, TEXT("OL_Chain"))});
	FPjcUnusedTree::GetUsedHolders(Result, {PackageOf(Fixture, TEXT("C_UnusedLeaf"))}, Holders);
	TestEqual(TEXT("nothing used holds an unused asset"), Holders.Num(), 0);

	// the unused asset that references the leaf goes with it
	const FName Leaf = PackageOf(Fixture, TEXT("C_UnusedLeaf"));
	const FName LeafRoot = PackageOf(Fixture, TEXT("OL_UnusedRoot"));
	TArray<FName> Group;
	FPjcUnusedTree::GetDeleteGroup(Result, {Leaf}, Group);
	TestEqual(TEXT("leaf goes with its unused referencer"), Group, TArray<FName> {LeafRoot});
	FPjcUnusedTree::GetDeleteGroup(Result, {LeafRoot}, Group);
	TestEqual(TEXT("nothing references the root"), Group.Num(), 0);

	// deletes exactly the given assets
	TArray<FAssetData> ToDelete;
	for (const auto& Asset : Result.Assets) {
		if (Asset.PackageName == Leaf || Asset.PackageName == LeafRoot) {
			ToDelete.Add(Asset);
		}
	}
	// what Clean project would delete and keep
	const FPjcCleanPlan Plan = FPjcCleanPlan::Make(Result);
	int64 UnusedSize = 0;
	for (const auto& Asset : Result.Unused) {
		UnusedSize += Result.GetSize(Asset.PackageName);
	}
	TestEqual(TEXT("plan deletes the unused assets"), Plan.Assets.Num(), Result.Unused.Num());
	TestEqual(TEXT("plan size"), Plan.Size, UnusedSize);
	TestEqual(TEXT("plan keeps review assets"), Plan.NumReview, Fixture.ReviewNames().Num());
	TestEqual(TEXT("plan keeps excluded assets"), Plan.NumExcluded, Result.Excluded.Num());
	TestTrue(TEXT("plan folders and classes"), Plan.Folders.Num() > 0 && Plan.Classes.Num() > 0);
	TestTrue(TEXT("unused folder left empty"), Plan.NumFoldersEmpty > Result.FoldersEmpty.Num());

	// held in memory by a live object (the engine ignores an asset's own root flag): kept with its holder, what it
	// uses stays (still referenced), each asset tried once
	TestEqual(TEXT("assets to delete"), ToDelete.Num(), 2);
	UObject* Held = FSoftObjectPath {Fixture.ObjectPath(TEXT("OL_UnusedRoot"))}.TryLoad();
	UObjectLibrary* Holder = NewObject<UObjectLibrary>(GetTransientPackage());
	if (TestNotNull(TEXT("held asset loaded"), Held)) {
		Holder->AddToRoot();
		TestTrue(TEXT("holder references the asset"), Holder->AddObject(Held));
		int32 MaxTried = 0;
		FPjcDeleteProgress HeldProgress;
		HeldProgress.OnBatch = [&MaxTried](const int32 NumTried, const int32 NumTotal, const FString& Current) {
			MaxTried = FMath::Max(MaxTried, NumTried);
		};
		UPjcSubsystem::DeleteAssets(ToDelete, false, false, &HeldProgress);
		Holder->RemoveObject(Held);
		Holder->RemoveFromRoot();
		TestTrue(TEXT("held asset kept"), UPjcSubsystem::GetLastCleanupKept().Contains(LeafRoot));
		TestTrue(TEXT("held asset file stays"), FPackageName::DoesPackageExist(LeafRoot.ToString()));
		TestTrue(TEXT("what it uses stays"), FPackageName::DoesPackageExist(Leaf.ToString()));
		TestEqual(TEXT("each asset tried once"), MaxTried, ToDelete.Num());
	}

	// a window follows the delete batch by batch
	int32 NumBatches = 0;
	FPjcDeleteProgress Progress;
	Progress.OnBatch = [&NumBatches](const int32 NumTried, const int32 NumTotal, const FString& Current) {
		++NumBatches;
	};
	TestTrue(TEXT("deleted"), UPjcSubsystem::DeleteAssets(ToDelete, false, false, &Progress));
	TestTrue(TEXT("progress told"), NumBatches > 0 && Progress.NumTried == 2 && !Progress.bStopped);
	TestFalse(TEXT("leaf gone"), FPackageName::DoesPackageExist(Leaf.ToString()));
	TestFalse(TEXT("root gone"), FPackageName::DoesPackageExist(LeafRoot.ToString()));
	TestTrue(TEXT("other unused asset kept"), FPackageName::DoesPackageExist(PackageOf(Fixture, TEXT("C_Unused")).ToString()));
	TArray<FName> Broken;
	FPjcCleanPlan::GetBrokenReferencers({Leaf, LeafRoot}, Broken);
	TestEqual(TEXT("nothing kept references what was deleted"), Broken.Num(), 0);

	// a weak cache does not hold an asset: a used blueprint lists the unused one as its dependent, as compiling does
	// (macro libraries and parents in Lyra). The engine's delete ignores weak references, so must the folder label pass
	// this asset needs (its folder label references it on disk)
	const FName WeakHeld = PackageOf(Fixture, TEXT("BP_Labeled"));
	UBlueprint* Dependent = Cast<UBlueprint>(FSoftObjectPath {Fixture.ObjectPath(TEXT("BP_Labeled"))}.TryLoad());
	UBlueprint* Dependency = Cast<UBlueprint>(FSoftObjectPath {Fixture.ObjectPath(TEXT("BP_Primary"))}.TryLoad());
	if (TestTrue(TEXT("blueprints loaded"), Dependent && Dependency)) {
		Dependency->CachedDependents.Add(Dependent);
		const FAssetData WeakAsset = PjcShim::GetAssetByObjectPath(Fixture.ObjectPath(TEXT("BP_Labeled")));
		UPjcSubsystem::DeleteAssets({WeakAsset}, false, false);
		TestFalse(TEXT("blueprint only a weak cache lists is deleted"), FPackageName::DoesPackageExist(WeakHeld.ToString()));
		TestFalse(TEXT("not listed as kept"), UPjcSubsystem::GetLastCleanupKept().Contains(WeakHeld));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPjcTestIssuesRedirectors, "ProjectCleaner.Issues.Redirectors", PJC_TEST_FLAGS)

bool FPjcTestIssuesRedirectors::RunTest(const FString& Parameters) {
	if (!FPjcTestFixture::IsTestHost()) {
		AddInfo(TEXT("Skipped, fixes fixture redirectors, runs only in test host projects"));
		return true;
	}

	FPjcScopedScanSettings ScopedSettings {false};
	FPjcTestFixture Fixture;
	if (!Fixture.Build(*this, true, true, true)) return false;
	if (!TestTrue(TEXT("scanned"), UPjcSubsystem::Scan(false))) return false;
	TestInspectorActorFiles(*this, Fixture);
	TestActorFileReason(*this, Fixture);

	// why external files are kept
	const FPjcScanResult& ScanResult = UPjcSubsystem::GetScanResult();
	TestEqual(TEXT("a reason per kept file"), ScanResult.FilesExternalKeptReasons.Num(), ScanResult.FilesExternalKept.Num());
	const auto Reason = [&ScanResult](const FString& File) {
		const FString Wanted = UPjcSubsystem::PathNormalize(File);
		for (int32 Index = 0; Index < ScanResult.FilesExternalKept.Num(); ++Index) {
			if (UPjcSubsystem::PathNormalize(ScanResult.FilesExternalKept[Index]) == Wanted) return ScanResult.FilesExternalKeptReasons[Index];
		}
		return FString {TEXT("not kept")};
	};
	TArray<FString> KeptFiles = Fixture.LyraKeptFiles();
	// the external files of the scanned content (plugins not scanned, disabled plugins never touched)
	KeptFiles.RemoveAll([](const FString& File) {
		const bool bKeptFile = File.Contains(TEXT("/Content/Localization/PjcTests/")) || File.EndsWith(TEXT("/Staged/license.txt"));
		return !bKeptFile && !File.EndsWith(TEXT("/T_RawIcon.svg"));
	});
	TestEqual(TEXT("fixture kept files"), KeptFiles.Num(), 3);
	for (const auto& File : KeptFiles) {
		const TCHAR* OtherReason = File.Contains(TEXT("/Staged/")) ? TEXT("Staged folder") : TEXT("Named in source or config");
		const FString Expected = File.Contains(TEXT("/Localization/")) ? TEXT("Engine folder (Localization)") : OtherReason;
		TestEqual(*FString::Printf(TEXT("%s kept reason"), *FPaths::GetCleanFilename(File)), Reason(File), Expected);
	}
	TestEqual(TEXT("excluded file kept reason"), Reason(Fixture.ExternalExcludedFile()), FString {TEXT("Excluded in settings")});

	// states: used through, nothing points through it, target missing
	TArray<FPjcRedirectorInfo> Redirectors;
	PjcIssues::GetRedirectors(UPjcSubsystem::GetScanResult(), Redirectors);
	const auto Find = [&](const TCHAR* Name) {
		return Redirectors.FindByPredicate([&](const FPjcRedirectorInfo& Info) {
			return Info.Asset.PackageName == PackageOf(Fixture, Name);
		});
	};
	const FPjcRedirectorInfo* Moved = Find(TEXT("R_Moved"));
	const FPjcRedirectorInfo* Stale = Find(TEXT("R_Stale"));
	const FPjcRedirectorInfo* Dangling = Find(TEXT("R_Dangling"));
	const FPjcRedirectorInfo* ToUnused = Find(TEXT("R_ToUnused"));
	if (!TestTrue(TEXT("fixture redirectors found"), Moved && Stale && Dangling && ToUnused)) return false;
	TestEqual(TEXT("R_Moved in use"), static_cast<int32>(Moved->State), static_cast<int32>(EPjcRedirectorState::InUse));
	TestTrue(TEXT("R_Moved used by PAL_Root"), Moved->UsedBy.Contains(PackageOf(Fixture, TEXT("PAL_Root"))));
	TestEqual(TEXT("R_Moved points to C_Target"), Moved->Target, PackageOf(Fixture, TEXT("C_Target")));
	TestEqual(TEXT("R_Stale not referenced"), static_cast<int32>(Stale->State), static_cast<int32>(EPjcRedirectorState::NotReferenced));
	TestEqual(TEXT("R_Dangling broken"), static_cast<int32>(Dangling->State), static_cast<int32>(EPjcRedirectorState::Broken));
	TestTrue(TEXT("unused broken redirector can be fixed"), Dangling->CanFix());

	// redirectors are not assets; a real asset with a redirector inside is an asset, not a redirector
	TestNull(TEXT("C_WithRedirector is no redirector"), Find(TEXT("C_WithRedirector")));
	TArray<FAssetData> AssetsAll;
	UPjcSubsystem::GetAssetsAll(AssetsAll);
	const TSet<FString> Assets = FixtureAssets(AssetsAll);
	TestTrue(TEXT("C_WithRedirector is an asset"), Assets.Contains(Fixture.ObjectPath(TEXT("C_WithRedirector"))));
	for (const auto& Name : Fixture.RedirectorNames()) {
		TestFalse(*FString::Printf(TEXT("%s is no asset"), *Name), Assets.Contains(Fixture.ObjectPath(Name)));
	}

	// fix: users resaved, every redirector deleted, targets kept
	const TArray<FAssetData> ToFix {Moved->Asset, Stale->Asset, Dangling->Asset, ToUnused->Asset};
	const FPjcRedirectorFixResult Result = UPjcSubsystem::FixRedirectorsWithReport(ToFix, false);
	TestEqual(TEXT("nothing kept"), Result.Kept.Num(), 0);
	TestEqual(TEXT("nothing failed to resave"), Result.NotResaved.Num(), 0);
	TestEqual(TEXT("redirectors deleted"), Result.Deleted.Num(), ToFix.Num());
	TestTrue(TEXT("user resaved"), Result.NumResaved >= 1);
	for (const auto& Name : Fixture.RedirectorNames()) {
		TestFalse(*FString::Printf(TEXT("%s gone"), *Name), FPackageName::DoesPackageExist(PackageOf(Fixture, *Name).ToString()));
	}
	TestTrue(TEXT("C_Target kept"), FPackageName::DoesPackageExist(PackageOf(Fixture, TEXT("C_Target")).ToString()));
	TestTrue(TEXT("C_ChainLeaf kept"), FPackageName::DoesPackageExist(PackageOf(Fixture, TEXT("C_ChainLeaf")).ToString()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPjcTestIssuesFiles, "ProjectCleaner.Issues.Files", PJC_TEST_FLAGS)

bool FPjcTestIssuesFiles::RunTest(const FString& Parameters) {
	if (!FPjcTestFixture::IsTestHost()) {
		AddInfo(TEXT("Skipped, deletes fixture files, runs only in test host projects"));
		return true;
	}

	FPjcScopedScanSettings ScopedSettings {false};
	FPjcTestFixture Fixture;
	if (!Fixture.Build(*this)) return false;

	// a folder exclusion keeps every file in it
	UPjcFileExcludeSettings* FileSettings = GetMutableDefault<UPjcFileExcludeSettings>();
	const FString ExternalFile = Fixture.ExternalFile();
	TestFalse(TEXT("external file to review"), UPjcSubsystem::FileIsExcluded(ExternalFile));
	FileSettings->ExcludedFolders = {FDirectoryPath {UPjcSubsystem::PathConvertToRelative(FPaths::GetPath(ExternalFile))}};
	TestTrue(TEXT("folder exclusion keeps it"), UPjcSubsystem::FileIsExcluded(ExternalFile));
	TestEqual(TEXT("kept reason"), UPjcSubsystem::GetFileKeptReason(ExternalFile), FString {TEXT("Excluded in settings")});
	FileSettings->ExcludedFolders.Reset();

	// deleting files and empty folders (subfolders go with their parent)
	TestEqual(TEXT("file deleted"), UPjcSubsystem::DeleteFiles({ExternalFile}), 1);
	TestFalse(TEXT("file gone"), FPaths::FileExists(ExternalFile));
	TArray<FString> Folders = Fixture.EmptyFolders();
	Folders.Sort([](const FString& A, const FString& B) {
		return A.Len() < B.Len();
	});
	TestEqual(TEXT("folders deleted"), UPjcSubsystem::DeleteFolders(Folders), Folders.Num());
	for (const auto& Folder : Folders) {
		TestFalse(*FString::Printf(TEXT("%s gone"), *Folder), IFileManager::Get().DirectoryExists(*Folder));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPjcTestInspectorGraph, "ProjectCleaner.Inspector.Graph", PJC_TEST_FLAGS)

bool FPjcTestInspectorGraph::RunTest(const FString& Parameters) {
	if (!FPjcTestFixture::IsTestHost()) {
		AddInfo(TEXT("Skipped, fixture tests run only in test host projects"));
		return true;
	}

	FPjcScopedScanSettings ScopedSettings {false};
	FPjcTestFixture Fixture;
	if (!Fixture.Build(*this)) return false;
	if (!TestTrue(TEXT("scanned"), UPjcSubsystem::Scan(false))) return false;
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();

	// used: the root on top with its source, the path down to the asset
	const FName Leaf = PackageOf(Fixture, TEXT("C_ChainLeaf"));
	const FPjcInspectorGraph Used = FPjcInspectorGraph::Make(Result, Leaf);
	TestFalse(TEXT("used graph is no unused group"), Used.bUnusedGroup);
	TestTrue(TEXT("used graph has roots"), Used.NumRoots > 0);

	// a fan: many roots, each through an asset of its own to the same asset. The assets between stand as one box
	{
		TArray<TArray<FName>> FanPaths;
		TArray<bool> bFan;
		for (int32 Index = 0; Index < 6; ++Index) {
			FanPaths.Add(
				{FName {*FString::Printf(TEXT("/Game/Root%d"), Index)},
				 FName {*FString::Printf(TEXT("/Game/Shot%d"), Index)},
				 TEXT("/Game/Map"),
				 TEXT("/Game/Focus")}
			);
			bFan.Add(true);
		}
		// a path drawn step by step keeps its assets as they are, also for the others
		FanPaths.Add({TEXT("/Game/Alone"), TEXT("/Game/Shot5"), TEXT("/Game/Map"), TEXT("/Game/Focus")});
		bFan.Add(false);
		TMap<FName, TArray<FName>> FanStacks;
		FPjcInspectorGraph::StackPaths(
			FanPaths,
			bFan,
			[](const FName) {
				return FName {TEXT("LevelSequence")};
			},
			FanStacks
		);
		TestEqual(TEXT("the fan is one stack"), FanStacks.Num(), 1);
		for (const auto& Pair : FanStacks) {
			TestEqual(TEXT("the stack has the assets no other path needs"), Pair.Value.Num(), 5);
			TestEqual(TEXT("the path goes through the stack"), FanPaths[0][1], Pair.Key);
		}
		TestEqual(TEXT("roots stay"), FanPaths[0][0], FName {TEXT("/Game/Root0")});
		TestEqual(TEXT("the asset all lead to stays"), FanPaths[0][2], FName {TEXT("/Game/Map")});
		TestEqual(TEXT("an asset of a single path stays"), FanPaths[5][1], FName {TEXT("/Game/Shot5")});

		// two levels and paths of different length: a stack that leads to a stack, a short path beside them
		TArray<TArray<FName>> Deep;
		TArray<bool> bDeep;
		for (int32 Index = 0; Index < 4; ++Index) {
			Deep.Add({
			  FName {*FString::Printf(TEXT("/Game/R%d"), Index)},
			  FName {*FString::Printf(TEXT("/Game/A%d"), Index)},
			  FName {*FString::Printf(TEXT("/Game/B%d"), Index)},
			  TEXT("/Game/Focus"),
			});
			bDeep.Add(true);
		}
		Deep.Add({TEXT("/Game/R9"), TEXT("/Game/Focus")});
		bDeep.Add(true);
		// a path that is not drawn has no say
		Deep.Add({});
		bDeep.Add(false);
		TMap<FName, TArray<FName>> DeepStacks;
		FPjcInspectorGraph::StackPaths(
			Deep,
			bDeep,
			[](const FName) {
				return FName {TEXT("LevelSequence")};
			},
			DeepStacks
		);
		TestEqual(TEXT("two levels are two stacks"), DeepStacks.Num(), 2);
		TestEqual(TEXT("the paths share the lower stack"), Deep[0][2], Deep[3][2]);
		TestEqual(TEXT("the paths share the upper stack"), Deep[0][1], Deep[3][1]);
		TestNotEqual(TEXT("the two stacks are not one"), Deep[0][1], Deep[0][2]);
		TestEqual(TEXT("the short path is as it was"), Deep[4].Num(), 2);

		// fewer than a group, and two classes to the same asset: no stack
		TArray<TArray<FName>> Few;
		TArray<bool> bFew;
		for (int32 Index = 0; Index < 6; ++Index) {
			Few.Add(
				{FName {*FString::Printf(TEXT("/Game/R%d"), Index)},
				 FName {*FString::Printf(TEXT("/Game/%s%d"), Index < 3 ? TEXT("Seq") : TEXT("Mat"), Index)},
				 TEXT("/Game/Focus")}
			);
			bFew.Add(true);
		}
		TMap<FName, TArray<FName>> FewStacks;
		FPjcInspectorGraph::StackPaths(
			Few,
			bFew,
			[](const FName Package) {
				return FName {Package.ToString().Contains(TEXT("Seq")) ? TEXT("LevelSequence") : TEXT("Material")};
			},
			FewStacks
		);
		TestEqual(TEXT("three of a class are no stack"), FewStacks.Num(), 0);
	}

	// reference locations: the asset is loaded and says which of its properties holds the asset it uses
	{
		const FName Library = PackageOf(Fixture, TEXT("OL_Chain"));
		FPjcLocations Read;
		TestTrue(TEXT("an asset can be read"), FPjcLocationReader::CanRead(Library));
		if (TestTrue(TEXT("locations read"), FPjcLocationReader::Read(Library, Read))) {
			const FPjcLocation* Place = Read.Places.FindByPredicate([Leaf](const FPjcLocation& Existing) {
				return Existing.Target == Leaf;
			});
			if (TestNotNull(TEXT("the property that holds the used asset is found"), Place)) {
				TestFalse(TEXT("the place names a property"), Place->Property.IsEmpty());
				TestTrue(TEXT("the place names a part of the asset"), Place->Parts.Num() > 0);
			}
			TestFalse(TEXT("an asset found in a property is not listed as not found"), Read.NotFound.Contains(Leaf));
			TArray<FPjcInspectorLink> Uses;
			int32 Through = 0;
			FPjcInspectorGraph::GetLinks(Library, false, Uses, Through);
			TSet<FName> Listed;
			for (const FPjcInspectorLink& Link : Uses) {
				Listed.Add(Link.Package);
				const bool bPlaced = Read.Places.ContainsByPredicate([&Link](const FPjcLocation& Existing) {
					return Existing.Target == Link.Package;
				});
				TestTrue(TEXT("every used asset has a place or is listed as not found"), bPlaced || Read.NotFound.Contains(Link.Package));
			}
			for (const FPjcLocation& Existing : Read.Places) {
				TestTrue(TEXT("every place is of an asset the lists show"), Listed.Contains(Existing.Target));
			}
		}
		TestFalse(TEXT("nothing to read for no asset"), FPjcLocationReader::CanRead(TEXT("/Game/Pjc/NoSuchAsset")));
	}
	const int32 LeafNode = Used.FindNode(Leaf);
	const int32 ChainNode = Used.FindNode(PackageOf(Fixture, TEXT("OL_Chain")));
	const int32 RootNode = Used.FindNode(PackageOf(Fixture, TEXT("PAL_Root")));
	if (TestTrue(TEXT("chain nodes"), LeafNode != INDEX_NONE && ChainNode != INDEX_NONE && RootNode != INDEX_NONE)) {
		TestTrue(TEXT("root above the chain"), Used.Nodes[RootNode].Layer < Used.Nodes[ChainNode].Layer);
		TestTrue(TEXT("chain above the asset"), Used.Nodes[ChainNode].Layer < Used.Nodes[LeafNode].Layer);
		TestTrue(TEXT("root references the chain"), Used.Edges.ContainsByPredicate([&](const FPjcInspectorEdge& Edge) {
			return Edge.From == RootNode && Edge.To == ChainNode;
		}));
		TestTrue(TEXT("chain references the asset"), Used.Edges.ContainsByPredicate([&](const FPjcInspectorEdge& Edge) {
			return Edge.From == ChainNode && Edge.To == LeafNode;
		}));
		const FName Library = PackageOf(Fixture, TEXT("OL_Chain"));
		const int32 LabelLink = static_cast<int32>(FPjcInspectorGraph::GetLinkKind(PackageOf(Fixture, TEXT("PAL_Root")), Library));
		TestEqual(TEXT("labels reference editor only (labels do not ship)"), LabelLink, static_cast<int32>(EPjcLinkKind::EditorOnly));
		const int32 LibraryLink = static_cast<int32>(FPjcInspectorGraph::GetLinkKind(Library, Leaf));
		TestEqual(TEXT("object library references hard"), LibraryLink, static_cast<int32>(EPjcLinkKind::Hard));
	}
	const int32 SourceNode = Used.Nodes.IndexOfByPredicate([&](const FPjcInspectorNode& Node) {
		return Node.Kind == EPjcInspectorNodeKind::Source && Node.Package == PackageOf(Fixture, TEXT("PAL_Root"));
	});
	if (TestTrue(TEXT("source above the root"), SourceNode != INDEX_NONE && RootNode != INDEX_NONE)) {
		TestTrue(TEXT("source tells why"), !Used.Nodes[SourceNode].Label.IsEmpty());
		TestTrue(TEXT("source row above the root"), Used.Nodes[SourceNode].Layer < Used.Nodes[RootNode].Layer);
	}

	// unused: the group around it, what nothing references on top
	const FName UnusedLeaf = PackageOf(Fixture, TEXT("C_UnusedLeaf"));
	const FPjcInspectorGraph Unused = FPjcInspectorGraph::Make(Result, UnusedLeaf);
	TestTrue(TEXT("unused group"), Unused.bUnusedGroup);
	const int32 UnusedNode = Unused.FindNode(UnusedLeaf);
	const int32 UnusedRootNode = Unused.FindNode(PackageOf(Fixture, TEXT("OL_UnusedRoot")));
	if (TestTrue(TEXT("unused group nodes"), UnusedNode != INDEX_NONE && UnusedRootNode != INDEX_NONE)) {
		TestEqual(TEXT("unreferenced on top"), Unused.Nodes[UnusedRootNode].Layer, 0);
		TestEqual(TEXT("referenced below"), Unused.Nodes[UnusedNode].Layer, 1);
		TestTrue(TEXT("unused reference drawn"), Unused.Edges.ContainsByPredicate([&](const FPjcInspectorEdge& Edge) {
			return Edge.From == UnusedRootNode && Edge.To == UnusedNode;
		}));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPjcTestInspectorWiring, "ProjectCleaner.Inspector.Wiring", PJC_TEST_FLAGS)

bool FPjcTestInspectorWiring::RunTest(const FString& Parameters) {
	// boxes of the graph, no assets. Top uses Bottom with Between in the way, and Left and Right in the row of Bottom;
	// Bottom uses Top too (a line that goes up); Top and Beside stand in one row
	enum
	{
		Top,
		Between,
		Bottom,
		Left,
		Right,
		Beside,
	};
	const FVector2D Size {214.0f, 46.0f};
	const TArray<FVector2D> Boxes {
	  FVector2D {400.0f,   0.0f},
	  FVector2D {400.0f, 120.0f},
	  FVector2D {400.0f, 240.0f},
	  FVector2D {130.0f, 240.0f},
	  FVector2D {670.0f, 240.0f},
	  FVector2D {800.0f,   0.0f}
	};
	const TArray<FPjcRouteLink> Links {
	  FPjcRouteLink {	Top, Bottom},
		 FPjcRouteLink {   Top,	  Left},
		  FPjcRouteLink {	Top,  Right},
		FPjcRouteLink {	  Top, Beside},
	   FPjcRouteLink {Bottom,	 Top}
	};
	TArray<TArray<FVector2D>> Routes;
	PjcGraphRouter::Route(Boxes, Size, Links, Routes);
	if (!TestEqual(TEXT("a line for every link"), Routes.Num(), Links.Num())) return false;

	const auto X = [](const FVector2D& Point) {
		return static_cast<float>(Point.X);
	};
	const auto Y = [](const FVector2D& Point) {
		return static_cast<float>(Point.Y);
	};
	// inside a box, not on its border: a line starts and ends on a border
	const auto InBox = [&](const float AtX, const float AtY) {
		for (const FVector2D& Box : Boxes) {
			if (AtX > X(Box) + 1.0f && AtX < X(Box) + X(Size) - 1.0f && AtY > Y(Box) + 1.0f && AtY < Y(Box) + Y(Size) - 1.0f) return true;
		}
		return false;
	};
	for (int32 Index = 0; Index < Routes.Num(); ++Index) {
		const TArray<FVector2D>& Line = Routes[Index];
		if (!TestTrue(FString::Printf(TEXT("line %d has a start and an end"), Index), Line.Num() >= 2)) return false;

		bool bRightAngles = true;
		bool bThroughBox = false;
		for (int32 Point = 1; Point < Line.Num(); ++Point) {
			const float DeltaX = X(Line[Point]) - X(Line[Point - 1]);
			const float DeltaY = Y(Line[Point]) - Y(Line[Point - 1]);
			bRightAngles &= FMath::Abs(DeltaX) < 0.01f || FMath::Abs(DeltaY) < 0.01f;
			const int32 Steps = FMath::Max(1, FMath::CeilToInt((FMath::Abs(DeltaX) + FMath::Abs(DeltaY)) / 4.0f));
			for (int32 Step = 0; Step <= Steps; ++Step) {
				const float At = static_cast<float>(Step) / Steps;
				bThroughBox |= InBox(X(Line[Point - 1]) + DeltaX * At, Y(Line[Point - 1]) + DeltaY * At);
			}
		}
		TestTrue(FString::Printf(TEXT("line %d runs at right angles"), Index), bRightAngles);
		TestFalse(FString::Printf(TEXT("line %d goes through no box"), Index), bThroughBox);
	}

	// a box uses what is below it: out at the bottom, in at the top. A line that goes up does the opposite
	TestEqual(TEXT("leaves the bottom of the box that uses"), Y(Routes[0][0]), 46.0f, 0.01f);
	TestEqual(TEXT("arrives at the top of the used box"), Y(Routes[0].Last()), 240.0f, 0.01f);
	TestTrue(TEXT("goes round the box in the way"), Routes[0].Num() > 2);
	TestEqual(TEXT("a line up leaves the top"), Y(Routes[4][0]), 240.0f, 0.01f);
	TestEqual(TEXT("a line up arrives at the bottom"), Y(Routes[4].Last()), 46.0f, 0.01f);
	// one row: side to side, straight
	TestEqual(TEXT("neighbours: out at the side"), X(Routes[3][0]), 614.0f, 0.01f);
	TestEqual(TEXT("neighbours: in at the side"), X(Routes[3].Last()), 800.0f, 0.01f);
	TestEqual(TEXT("neighbours: a straight line"), Routes[3].Num(), 2);
	// ends on one side are spread along it, in the order of where they lead
	const TSet<int32> Ends {
	  FMath::RoundToInt(X(Routes[0][0])),
	  FMath::RoundToInt(X(Routes[1][0])),
	  FMath::RoundToInt(X(Routes[2][0])),
	  FMath::RoundToInt(X(Routes[4].Last()))
	};
	TestEqual(TEXT("four ends on the bottom of one box, each at its own place"), Ends.Num(), 4);
	TestTrue(TEXT("the end that leads left is left of the one that leads right"), X(Routes[1][0]) < X(Routes[2][0]));

	// round corners keep the ends
	const TArray<FVector2D> Rounded = PjcGraphRouter::Round(Routes[1], 8.0f);
	TestTrue(TEXT("round corners add points"), Rounded.Num() > Routes[1].Num());
	TestTrue(TEXT("round corners keep the start"), Rounded[0].Equals(Routes[1][0], 0.01f));
	TestTrue(TEXT("round corners keep the end"), Rounded.Last().Equals(Routes[1].Last(), 0.01f));
	const FVector2D Middle = PjcGraphRouter::Middle(Routes[3]);
	TestEqual(TEXT("the middle of a straight line"), X(Middle), 707.0f, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPjcTestInspectorGroup, "ProjectCleaner.Inspector.Group", PJC_TEST_FLAGS)

bool FPjcTestInspectorGroup::RunTest(const FString& Parameters) {
	// a group built by hand, no assets: User uses Focus; Focus uses three textures and P; P and Q use each other;
	// R uses nothing of them; all eight use Shared. One texture has an editor-only reference back to User
	enum
	{
		Focus,
		User,
		T1,
		T2,
		T3,
		Shared,
		P,
		Q,
		R,
		Num
	};
	const TArray<FName> Names {
	  TEXT("/Game/Focus"),
	  TEXT("/Game/User"),
	  TEXT("/Game/T1"),
	  TEXT("/Game/T2"),
	  TEXT("/Game/T3"),
	  TEXT("/Game/Shared"),
	  TEXT("/Game/P"),
	  TEXT("/Game/Q"),
	  TEXT("/Game/R")
	};
	const TArray<FName> Classes {
	  TEXT("Blueprint"),
	  TEXT("Blueprint"),
	  TEXT("Texture2D"),
	  TEXT("Texture2D"),
	  TEXT("Texture2D"),
	  TEXT("Skeleton"),
	  TEXT("Material"),
	  TEXT("Material"),
	  TEXT("SoundWave")
	};
	TArray<FPjcGroupLink> Links;
	Links.Add(FPjcGroupLink {User, Focus, EPjcLinkKind::Hard});
	for (const int32 Texture : {T1, T2, T3}) {
		Links.Add(FPjcGroupLink {Focus, Texture, EPjcLinkKind::Hard});
	}
	Links.Add(FPjcGroupLink {Focus, P, EPjcLinkKind::Soft});
	Links.Add(FPjcGroupLink {P, Q, EPjcLinkKind::Hard});
	Links.Add(FPjcGroupLink {Q, P, EPjcLinkKind::Hard});
	for (const int32 Each : {Focus, User, T1, T2, T3, P, Q, R}) {
		Links.Add(FPjcGroupLink {Each, Shared, EPjcLinkKind::Hard});
	}
	Links.Add(FPjcGroupLink {T1, User, EPjcLinkKind::EditorOnly});
	if (!TestEqual(TEXT("names of the hand made group"), Names.Num(), static_cast<int32>(Num))) return false;

	const FPjcInspectorGraph Graph = FPjcInspectorGraph::MakeGroup(Names, Classes, Links, false);
	TestTrue(TEXT("unused group"), Graph.bUnusedGroup);
	TestEqual(TEXT("assets in the group"), Graph.NumGroup, static_cast<int32>(Num));
	TestEqual(TEXT("boxes: focus, user, the textures as one, shared, P, Q, R"), Graph.Nodes.Num(), 7);

	const auto BoxOf = [&Graph, &Names](const int32 Index) {
		return Graph.Nodes.IndexOfByPredicate([&](const FPjcInspectorNode& Node) {
			return Node.Kind == EPjcInspectorNodeKind::Stack ? Node.Roots.Contains(Names[Index]) : Node.Package == Names[Index];
		});
	};
	const auto EdgeOf = [&Graph](const int32 From, const int32 To) {
		return Graph.Edges.FindByPredicate([From, To](const FPjcInspectorEdge& Edge) {
			return Edge.From == From && Edge.To == To;
		});
	};
	const int32 FocusBox = BoxOf(Focus);
	const int32 UserBox = BoxOf(User);
	const int32 Stack = BoxOf(T1);
	const int32 SharedBox = BoxOf(Shared);
	const int32 PBox = BoxOf(P);
	const int32 QBox = BoxOf(Q);
	const int32 RBox = BoxOf(R);
	for (const int32 Box : {FocusBox, UserBox, Stack, SharedBox, PBox, QBox, RBox}) {
		if (!TestTrue(TEXT("every asset has a box"), Graph.Nodes.IsValidIndex(Box))) return false;
	}

	// same class, same users, same uses: one stack. The asset in focus is never part of one
	TestEqual(TEXT("focus is found as an asset"), Graph.FindNode(Names[Focus]), FocusBox);
	TestTrue(TEXT("the textures are a stack"), Graph.Nodes[Stack].Kind == EPjcInspectorNodeKind::Stack);
	TestEqual(TEXT("stack size"), Graph.Nodes[Stack].Roots.Num(), 3);
	TestEqual(TEXT("stack class"), Graph.Nodes[Stack].GroupClass, Classes[T1]);
	TestEqual(TEXT("textures share their box"), BoxOf(T2), Stack);
	TestTrue(TEXT("P and Q are not merged: they use each other, not the same things"), PBox != QBox);
	const FPjcInspectorEdge* ToStack = EdgeOf(FocusBox, Stack);
	if (TestNotNull(TEXT("focus uses the stack"), ToStack)) {
		TestEqual(TEXT("one line for the three references"), ToStack->NumLinks, 3);
	}

	// shared: used by 8 of the group, so no lines, a dot on each user
	TestTrue(TEXT("shared asset stands apart"), Graph.Nodes[SharedBox].bHub);
	TestEqual(TEXT("shared asset users"), Graph.Nodes[SharedBox].NumUsers, static_cast<int32>(FPjcInspectorGraph::HubAt));
	TestFalse(TEXT("no line touches the shared asset"), Graph.Edges.ContainsByPredicate([SharedBox](const FPjcInspectorEdge& Edge) {
		return Edge.From == SharedBox || Edge.To == SharedBox;
	}));
	for (const int32 Box : {FocusBox, UserBox, Stack, PBox, QBox, RBox}) {
		TestTrue(TEXT("every user of the shared asset carries its dot"), Graph.Nodes[Box].Hubs.Contains(SharedBox));
	}

	// rows: a box is below what uses it; boxes linked only through the shared asset and the shared ones come last
	TestTrue(TEXT("user above focus"), Graph.Nodes[UserBox].Layer < Graph.Nodes[FocusBox].Layer);
	TestTrue(TEXT("focus above the stack"), Graph.Nodes[FocusBox].Layer < Graph.Nodes[Stack].Layer);
	int32 LastLinked = 0;
	for (const int32 Box : {FocusBox, UserBox, Stack, PBox, QBox}) {
		LastLinked = FMath::Max(LastLinked, Graph.Nodes[Box].Layer);
	}
	TestFalse(TEXT("R has no lines"), Graph.Edges.ContainsByPredicate([RBox](const FPjcInspectorEdge& Edge) {
		return Edge.From == RBox || Edge.To == RBox;
	}));
	TestTrue(TEXT("R below the linked boxes"), Graph.Nodes[RBox].Layer > LastLinked);
	TestTrue(TEXT("shared asset in the last row"), Graph.Nodes[SharedBox].Layer > Graph.Nodes[RBox].Layer);
	TestEqual(TEXT("a caption for each of the two extra rows"), Graph.Captions.Num(), 2);
	if (Graph.Captions.Num() == 2) {
		TestEqual(TEXT("caption above R"), Graph.Captions[0].Layer, Graph.Nodes[RBox].Layer);
		TestEqual(TEXT("caption above the shared asset"), Graph.Captions[1].Layer, Graph.Nodes[SharedBox].Layer);
	}

	// P and Q use each other: both references are there, one of them closes the circle
	const FPjcInspectorEdge* PToQ = EdgeOf(PBox, QBox);
	const FPjcInspectorEdge* QToP = EdgeOf(QBox, PBox);
	if (TestTrue(TEXT("both references of the pair"), PToQ && QToP)) {
		TestTrue(TEXT("exactly one closes the circle"), PToQ->bCircle != QToP->bCircle);
	}
	const FPjcInspectorEdge* ToP = EdgeOf(FocusBox, PBox);
	if (TestNotNull(TEXT("focus uses P"), ToP)) {
		TestEqual(TEXT("soft reference kept"), static_cast<int32>(ToP->Kind), static_cast<int32>(EPjcLinkKind::Soft));
		TestFalse(TEXT("a plain reference closes no circle"), ToP->bCircle);
	}

	// editor-only references: left out unless asked for, then drawn on top without moving a box
	TestFalse(TEXT("no editor-only line by default"), Graph.Edges.ContainsByPredicate([](const FPjcInspectorEdge& Edge) {
		return Edge.Kind == EPjcLinkKind::EditorOnly;
	}));
	const FPjcInspectorGraph WithEditor = FPjcInspectorGraph::MakeGroup(Names, Classes, Links, true);
	TestEqual(TEXT("editor-only: same boxes"), WithEditor.Nodes.Num(), Graph.Nodes.Num());
	TestEqual(TEXT("editor-only: one more line"), WithEditor.Edges.Num(), Graph.Edges.Num() + 1);
	for (int32 Box = 0; Box < Graph.Nodes.Num() && Box < WithEditor.Nodes.Num(); ++Box) {
		TestEqual(TEXT("editor-only: rows stay"), WithEditor.Nodes[Box].Layer, Graph.Nodes[Box].Layer);
		TestEqual(TEXT("editor-only: places stay"), WithEditor.Nodes[Box].Column, Graph.Nodes[Box].Column);
	}
	const FPjcInspectorEdge* BackUp = WithEditor.Edges.FindByPredicate([Stack, UserBox](const FPjcInspectorEdge& Edge) {
		return Edge.From == Stack && Edge.To == UserBox;
	});
	if (TestNotNull(TEXT("editor-only line from the stack back to the user"), BackUp)) {
		TestEqual(TEXT("it is editor only"), static_cast<int32>(BackUp->Kind), static_cast<int32>(EPjcLinkKind::EditorOnly));
		TestTrue(TEXT("it closes a circle: the user leads to the stack"), BackUp->bCircle);
	}

	// one asset alone
	const FPjcInspectorGraph Alone = FPjcInspectorGraph::MakeGroup({Names[Focus]}, {Classes[Focus]}, {}, false);
	TestEqual(TEXT("alone: one box"), Alone.Nodes.Num(), 1);
	TestEqual(TEXT("alone: no lines"), Alone.Edges.Num(), 0);
	TestEqual(TEXT("alone: no captions"), Alone.Captions.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPjcTestInspectorLinks, "ProjectCleaner.Inspector.Links", PJC_TEST_FLAGS)

bool FPjcTestInspectorLinks::RunTest(const FString& Parameters) {
	if (!FPjcTestFixture::IsTestHost()) {
		AddInfo(TEXT("Skipped, fixture tests run only in test host projects"));
		return true;
	}

	TestTrue(TEXT("actor file"), FPjcInspectorGraph::IsExternal(TEXT("/Game/__ExternalActors__/Maps/Main/A/BC/Hash")));
	TestTrue(TEXT("object file"), FPjcInspectorGraph::IsExternal(TEXT("/Game/__ExternalObjects__/Maps/Main/A/BC/Hash")));
	TestFalse(TEXT("an asset is no actor file"), FPjcInspectorGraph::IsExternal(TEXT("/Game/Maps/Main")));

	// every menu icon is one the editor style of this engine has
	const FSlateBrush* Missing = PjcShim::GetStyle().GetDefaultBrush();
	for (const EPjcMenuIcon Icon :
		 {EPjcMenuIcon::Inspect,
		  EPjcMenuIcon::Browse,
		  EPjcMenuIcon::Delete,
		  EPjcMenuIcon::Exclude,
		  EPjcMenuIcon::Include,
		  EPjcMenuIcon::Explorer,
		  EPjcMenuIcon::SizeMap,
		  EPjcMenuIcon::Expand,
		  EPjcMenuIcon::Collapse,
		  EPjcMenuIcon::Reset}) {
		const FSlateIcon SlateIcon = PjcWidgets::GetMenuIcon(Icon);
		if (!SlateIcon.IsSet()) continue;

		TestTrue(*FString::Printf(TEXT("menu icon %s exists"), *SlateIcon.GetStyleName().ToString()), SlateIcon.GetIcon() != Missing);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPjcTestUiTabs, "ProjectCleaner.UI.Tabs", PJC_TEST_FLAGS)

bool FPjcTestUiTabs::RunTest(const FString& Parameters) {
	if (!FSlateApplication::IsInitialized()) {
		AddInfo(TEXT("Skipped, Slate is not initialized"));
		return true;
	}

	// tab widgets with fixture content, Assets Unused scans when constructed. Built without window: headless editors
	// (-nullrhi) can not create one before UE 5.8
	FPjcTestFixture Fixture;
	if (FPjcTestFixture::IsTestHost() && !Fixture.Build(*this)) return false;

	// tabs never scan by themselves: they show "Not scanned yet" until a scan
	if (FPjcTestFixture::IsTestHost()) {
		TestTrue(TEXT("scanned"), UPjcSubsystem::Scan(false));
	}

	const TSharedRef<SPjcTabInspector> TabInspector = SNew(SPjcTabInspector);
	const TSharedRef<SPjcTabUnused> TabUnused = SNew(SPjcTabUnused);
	const TSharedRef<SPjcTabIssues> TabIssues = SNew(SPjcTabIssues);
	const TSharedRef<SWidget> Tabs[] = {SNew(SPjcOverviewPanel), TabUnused, TabIssues, TabInspector, SNew(SPjcTabHelp)};
	for (const auto& Tab : Tabs) {
		TestTrue(FString::Printf(TEXT("%s has content"), *Tab->GetTypeAsString()), Tab->GetChildren()->Num() > 0);
	}

	// every Issues category lists
	for (const EPjcIssueCategory Category :
		 {EPjcIssueCategory::External, EPjcIssueCategory::Corrupted, EPjcIssueCategory::Empty, EPjcIssueCategory::Redirectors}) {
		TabIssues->ShowCategory(Category);
		TestEqual(TEXT("issues category shown"), static_cast<int32>(TabIssues->GetCategory()), static_cast<int32>(Category));
	}
	if (FPjcTestFixture::IsTestHost()) {
		TabIssues->ShowCategory(EPjcIssueCategory::External);
		TestTrue(TEXT("issues lists the fixture external file"), TabIssues->GetNumRows() > 0);
	}

	// a folder the Overview links to
	if (FPjcTestFixture::IsTestHost()) {
		TabUnused->SelectFolder(FPjcTestFixture::RootPath() / TEXT("Unused"));
		TestEqual(TEXT("folder shown in Assets Unused"), TabUnused->GetSelectedFolder(), FPjcTestFixture::RootPath() / TEXT("Unused"));
	}

	// used through chain, unused, kept by folder label and package outside scan
	if (FPjcTestFixture::IsTestHost()) {
		TestTrue(TEXT("inspector finder lists the scanned assets"), TabInspector->GetNumFinderRows() > 0);
		for (const TCHAR* Name : {TEXT("C_ChainLeaf"), TEXT("C_UnusedLeaf"), TEXT("C_Labeled"), TEXT("C_RefByPlugin")}) {
			const FName Package = PackageOf(Fixture, Name);
			TabInspector->Inspect(Package);
			TestEqual(FString::Printf(TEXT("inspector shows %s"), Name), TabInspector->GetFocus(), Package);
			TestTrue(FString::Printf(TEXT("inspector graph of %s has the asset"), Name), TabInspector->GetGraph().FindNode(Package) != INDEX_NONE);
		}
	}
	const FName Outside {TEXT("/Engine/EngineMaterials/WorldGridMaterial")};
	TabInspector->Inspect(Outside);
	TestEqual(TEXT("inspector shows a package outside the scan"), TabInspector->GetFocus(), Outside);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPjcTestUiReport, "ProjectCleaner.UI.Report", PJC_TEST_FLAGS)

bool FPjcTestUiReport::RunTest(const FString& Parameters) {
	// system info: plugin, engine and OS
	const FString Info = SPjcTabHelp::GetSystemInfo();
	TestFalse(TEXT("plugin version known"), SPjcTabHelp::GetPluginVersion() == TEXT("?"));
	TestTrue(TEXT("system info has the plugin version"), Info.Contains(TEXT("Project Cleaner ") + SPjcTabHelp::GetPluginVersion()));
	TestTrue(TEXT("system info has the engine version"), Info.Contains(FEngineVersion::Current().ToString()));
	TestTrue(TEXT("scan settings listed"), SPjcReportWindow::GetScanSettingsText().Contains(TEXT("excluded folders")));

	// private parts hidden
	const FString User = FPlatformProcess::UserName();
	const FString Machine = FPlatformProcess::ComputerName();
	const FString Private = FString::Printf(TEXT("C:/Users/%s/Proj on %s: /Game/Maps/L_Main.L_Main kept"), *User, *Machine);
	const FString Hidden = SPjcReportWindow::HidePrivate(Private, true, false);
	TestFalse(TEXT("user name hidden"), !User.IsEmpty() && Hidden.Contains(User));
	TestFalse(TEXT("machine name hidden"), !Machine.IsEmpty() && Hidden.Contains(Machine));
	TestTrue(TEXT("asset path kept"), Hidden.Contains(TEXT("/Game/Maps/L_Main.L_Main")));
	const FString NoPaths = SPjcReportWindow::HidePrivate(Private, false, true);
	TestTrue(TEXT("asset path hidden"), NoPaths.EndsWith(TEXT(": <asset> kept")));
	TestTrue(TEXT("user kept when not asked"), User.IsEmpty() || NoPaths.Contains(User));

	// own log lines only
	const FString Marker = FString::Printf(TEXT("PjcReportMarker %s"), *FGuid::NewGuid().ToString());
	UE_LOG(LogProjectCleaner, Display, TEXT("%s"), *Marker);
	if (IFileManager::Get().FileExists(*FPlatformOutputDevices::GetAbsoluteLogFilename())) {
		// the log file catches up on its own thread: UE 4.27 on Linux wrote the line a moment late (1 run in 6)
		FString LogText = SPjcReportWindow::GetLogText();
		for (int32 Try = 0; Try < 30 && !LogText.Contains(Marker); ++Try) {
			FPlatformProcess::Sleep(0.1f);
			LogText = SPjcReportWindow::GetLogText();
		}
		TestTrue(TEXT("log lines include the plugin line"), LogText.Contains(Marker));
		TestFalse(TEXT("log lines are only the plugin's"), LogText.Contains(TEXT("LogInit:")));
		FPjcTestFixture::Case(*this, TEXT("report log lines"));
	}
	else {
		FPjcTestFixture::Case(*this, TEXT("report log lines"), TEXT("this editor writes no log file"));
	}

	// the last crash, only with Project Cleaner in its call stack
	const FString CrashDir = FPaths::ProjectSavedDir() / TEXT("Crashes") / TEXT("PjcTestCrash");
	const FString CrashFile = CrashDir / TEXT("CrashContext.runtime-xml");
	ON_SCOPE_EXIT {
		IFileManager::Get().DeleteDirectory(*CrashDir, false, true);
	};
	FFileHelper::SaveStringToFile(TEXT("<Root><CallStack>UnrealEditor_Pjc!UPjcSubsystem::Scan()\nUnrealEditor_Core</CallStack></Root>"), *CrashFile);
	TestTrue(TEXT("crash with the plugin offered"), SPjcReportWindow::GetLastCrashText().Contains(TEXT("UPjcSubsystem::Scan")));
	FFileHelper::SaveStringToFile(TEXT("<Root><CallStack>UnrealEditor_Renderer!Draw()</CallStack></Root>"), *CrashFile);
	TestTrue(TEXT("crash without the plugin not offered"), SPjcReportWindow::GetLastCrashText().IsEmpty());

	if (!FSlateApplication::IsInitialized()) {
		FPjcTestFixture::Case(*this, TEXT("report window"), TEXT("Slate is not initialized"));
		return true;
	}

	// the report: what happened first, every chosen part, user hidden by default
	const TSharedRef<SPjcReportWindow> Report = SNew(SPjcReportWindow);
	const FString Text = Report->MakeReport();
	TestTrue(TEXT("report starts with what happened"), Text.StartsWith(TEXT("### What happened")));
	TestTrue(TEXT("report has system info"), Text.Contains(TEXT("### System info")));
	TestTrue(TEXT("report has scan settings"), Text.Contains(TEXT("### Scan settings")));
	TestFalse(TEXT("report hides the user"), !User.IsEmpty() && Text.Contains(User));
	FPjcTestFixture::Case(*this, TEXT("report window"));
	return true;
}

// Not part of ProjectCleaner.* tests. Leaves fixture content on disk and writes expected results,
// so Scripts/pjc.py can verify the commandlet (-run=Pjc) in a separate process.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPjcFixtureBuild, "PjcFixture.Build", PJC_TEST_FLAGS)

bool FPjcFixtureBuild::RunTest(const FString& Parameters) {
	// commandlet must clean plugin content too
	UPjcScanSettings* ScanSettings = GetMutableDefault<UPjcScanSettings>();
	ScanSettings->bScanProjectPlugins = true;

	// Lyra like layout: game feature data registers its primary type when editor starts, only next process sees it
	FPjcTestFixture Fixture;
	if (!Fixture.Build(*this, true, false, true)) return false;

	// commandlet runs in another process, it must see same settings (host config files, removed by pjc.py)
	PjcShim::UpdateDefaultConfigFile(GetMutableDefault<UPjcAssetExcludeSettings>());
	PjcShim::UpdateDefaultConfigFile(GetMutableDefault<UPjcFileExcludeSettings>());
	PjcShim::UpdateDefaultConfigFile(ScanSettings);

	const TArray<FString> UsedNames = Fixture.UsedNames(true);
	const TArray<FString> UnusedNames = Fixture.UnusedNames(true);

	TArray<FString> KeptFiles;
	TArray<FString> DeletedFiles;
	for (const auto& Name : UsedNames) {
		KeptFiles.Add(RelativeToProject(Fixture.PackageFile(Name)));
	}
	for (const auto& Name : UnusedNames) {
		DeletedFiles.Add(RelativeToProject(Fixture.PackageFile(Name)));
	}
	// redirectors are not assets, only fixing them removes them; one to a deleted asset goes with it
	for (const auto& Name : Fixture.RedirectorNames()) {
		const FString File = RelativeToProject(Fixture.PackageFile(Name));
		(Name == TEXT("R_ToUnused") ? DeletedFiles : KeptFiles).Add(File);
	}
	KeptFiles.Add(RelativeToProject(Fixture.ExternalExcludedFile()));
	KeptFiles.Add(RelativeToProject(FPjcTestFixture::IndirectConfigFile()));
	KeptFiles.Add(RelativeToProject(FPjcTestFixture::PythonFile()));
	KeptFiles.Add(RelativeToProject(FPjcTestFixture::PlatformConfigFile()));
	KeptFiles.Add(RelativeToProject(FPjcTestFixture::FeatureConfigFile()));
	KeptFiles.Add(TEXT("Plugins/GameFeatures/PjcFeature/Content/__ExternalActors__"));
	for (const auto& File : Fixture.LyraKeptFiles()) {
		KeptFiles.Add(RelativeToProject(File));
	}
	DeletedFiles.Add(RelativeToProject(Fixture.ExternalFile()));
	DeletedFiles.Add(RelativeToProject(Fixture.CorruptedFile()));
	for (const auto& Folder : Fixture.EmptyFolders()) {
		DeletedFiles.Add(RelativeToProject(Folder));
	}

	TArray<FString> CleanupPaths {
	  TEXT("Content/PjcTests"),
	  TEXT("Content/L10N/fr/PjcTests"),
	  TEXT("Content/Python/PjcTests.py"),
	  TEXT("Saved/Collections/PjcTestCollection.collection"),
	  TEXT("Platforms/PjcPlatform"),
	  TEXT("Config/PjcTests.ini"),
	  TEXT("Plugins/PjcTestContent/Content/C_PluginUsed.uasset"),
	  TEXT("Plugins/PjcTestContent/Content/C_PluginUnused.uasset"),
	  TEXT("Plugins/PjcTestContent/Content/OL_PluginLib.uasset"),
	};
	CleanupPaths.Append(Fixture.LyraCleanupPaths());

	const int32 NumUsed = UsedNames.Num();
	const int32 NumUnused = UnusedNames.Num();

	FString Json;
	Json += TEXT("{\n");
	Json += FString::Printf(TEXT("  \"scan\": %s,\n"), *JsonStats(NumUsed + NumUnused, NumUsed, NumUnused, 1, 1, Fixture.EmptyFolders().Num()));
	Json += FString::Printf(TEXT("  \"after_cleanup\": %s,\n"), *JsonStats(NumUsed, NumUsed, 0, 0, 0, 0));
	Json += FString::Printf(TEXT("  \"kept_files\": %s,\n"), *JsonList(KeptFiles));
	Json += FString::Printf(TEXT("  \"deleted_files\": %s,\n"), *JsonList(DeletedFiles));
	Json += FString::Printf(TEXT("  \"cleanup_paths\": %s,\n"), *JsonList(CleanupPaths));
	Json += FString::Printf(TEXT("  \"create_files\": {\"%s\": \"this is not an unreal package\"},\n"), *RelativeToProject(Fixture.CorruptedFile()));
	Json += TEXT("  \"cleanup_user_config\": true\n");
	Json += TEXT("}\n");

	const FString ExpectedFile = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("PjcFixture/expected.json"));
	if (!TestTrue(TEXT("expected.json written"), FFileHelper::SaveStringToFile(Json, *ExpectedFile))) return false;

	Fixture.Keep();
	return true;
}

#endif
