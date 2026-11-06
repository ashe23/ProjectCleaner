// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Tests/PjcTestFixture.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "PjcAssetGraph.h"
#include "PjcIssues.h"
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
#include "Engine/AssetManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SPjcTabAssetsCorrupted.h"
#include "Slate/SPjcTabAssetsIndirect.h"
#include "Slate/SPjcTabIssues.h"
#include "Slate/SPjcTabUnused.h"
#include "Slate/SPjcTabAssetsUsage.h"
#include "Slate/SPjcTabOverview.h"
#include "Slate/SPjcTabFilesExternal.h"
#include "HAL/FileManager.h"
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
		return Path.StartsWith(FPjcTestFixture::RootPath() + TEXT("/")) || Path.StartsWith(FPjcTestFixture::LocalizedRootPath() + TEXT("/"))
			|| Path.StartsWith(FPjcTestFixture::PluginRootPath() + TEXT("/"));
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

	/** Changes scan settings for one test, restores them on destruction */
	struct FPjcScopedScanSettings
	{
		UPjcScanSettings* Settings;
		bool bScanProjectPlugins;
		TArray<FString> IgnoredPlugins;
		bool bUnregisteredPrimaryUsed;
		bool bFolderLabelsUsed;
		bool bCinematicAssetsUsed;
		TArray<TSoftClassPtr<UObject>> UsedClasses;

		explicit FPjcScopedScanSettings(const bool bInScanProjectPlugins) : Settings(GetMutableDefault<UPjcScanSettings>()) {
			bScanProjectPlugins = Settings->bScanProjectPlugins;
			IgnoredPlugins = Settings->IgnoredPlugins;
			bUnregisteredPrimaryUsed = Settings->bUnregisteredPrimaryUsed;
			bFolderLabelsUsed = Settings->bFolderLabelsUsed;
			bCinematicAssetsUsed = Settings->bCinematicAssetsUsed;
			UsedClasses = Settings->UsedClasses;

			// known state, fixture expectations are written for defaults
			Settings->bScanProjectPlugins = bInScanProjectPlugins;
			Settings->IgnoredPlugins.Reset();
			Settings->bUnregisteredPrimaryUsed = true;
			Settings->bFolderLabelsUsed = true;
			Settings->bCinematicAssetsUsed = true;
			Settings->UsedClasses.Reset();
		}

		~FPjcScopedScanSettings() {
			Settings->bScanProjectPlugins = bScanProjectPlugins;
			Settings->IgnoredPlugins = IgnoredPlugins;
			Settings->bUnregisteredPrimaryUsed = bUnregisteredPrimaryUsed;
			Settings->bFolderLabelsUsed = bFolderLabelsUsed;
			Settings->bCinematicAssetsUsed = bCinematicAssetsUsed;
			Settings->UsedClasses = UsedClasses;
		}
	};

	FName PackageOf(const FPjcTestFixture& Fixture, const TCHAR* Name) {
		return FName {*FPackageName::ObjectPathToPackageName(Fixture.ObjectPath(Name))};
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

	/** Same keys as Scripts/pjc.py parses from commandlet output */
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
	TestEqual(
		TEXT("relative of content subdir"), UPjcSubsystem::PathConvertToRelative(ContentDir / TEXT("Foo/Bar")), FString {TEXT("/Game/Foo/Bar")}
	);

	TestEqual(TEXT("object path"), UPjcSubsystem::PathConvertToObjectPath(TEXT("/Game/Foo/Bar.Bar")), FString {TEXT("/Game/Foo/Bar.Bar")});
	TestEqual(TEXT("object path of class"), UPjcSubsystem::PathConvertToObjectPath(TEXT("/Game/Foo/BP.BP_C")), FString {TEXT("/Game/Foo/BP.BP")});
	TestEqual(
		TEXT("object path of export text"),
		UPjcSubsystem::PathConvertToObjectPath(TEXT("Blueprint'/Game/Foo/BP.BP'")),
		FString {TEXT("/Game/Foo/BP.BP")}
	);
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
		AddInfo(
			FString::Printf(
				TEXT("primary type %s, base class %s"), *TypeInfo.PrimaryAssetType.ToString(), BaseClass ? *BaseClass->GetPathName() : TEXT("none")
			)
		);
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
	TArray<FString> EditorNames {TEXT("EUB_Tool")};
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
	TestSetEqual(
		*this,
		TEXT("indirect"),
		FixtureAssets(Assets),
		ObjectPaths(Fixture, {TEXT("C_Indirect"), TEXT("C_IndirectPackage"), TEXT("C_InPython"), TEXT("C_InPlatformConfig")})
	);

	const FPjcAssetIndirectInfo* IndirectInfo = IndirectInfos.FindByPredicate([&](const FPjcAssetIndirectInfo& Info) {
		return PjcShim::GetObjectPathString(Info.Asset).Equals(Fixture.ObjectPath(TEXT("C_Indirect")));
	});
	if (TestNotNull(TEXT("indirect info"), IndirectInfo)) {
		TestEqual(
			TEXT("indirect info file"),
			UPjcSubsystem::PathNormalize(IndirectInfo->FilePath),
			UPjcSubsystem::PathNormalize(FPjcTestFixture::IndirectConfigFile())
		);
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

	// plugins scanned: deletion inside plugin content is covered too. Folder labels off: asset only a folder label
	// references is deleted (engine refuses, label pass does it) and label is kept
	FPjcScopedScanSettings ScopedSettings {true};
	ScopedSettings.Settings->bFolderLabelsUsed = false;

	FPjcTestFixture Fixture;
	if (!Fixture.Build(*this)) return false;

	TestTrue(TEXT("DeleteAssetsUnused succeeded"), UPjcSubsystem::DeleteAssetsUnused(false, false));

	for (const auto& Name : Fixture.UnusedNames(true, false)) {
		TestFalse(FString::Printf(TEXT("unused %s deleted"), *Name), FPaths::FileExists(Fixture.PackageFile(Name)));
	}
	for (const auto& Name : Fixture.UsedNames(true, false)) {
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
	TestSetEqual(
		*this,
		TEXT("unregistered primary classes not used"),
		UnusedFixtureAssets(),
		UnusedDefault.Union(ObjectPaths(Fixture, {TEXT("BP_PrimaryOutside")}))
	);
	Settings->bUnregisteredPrimaryUsed = true;

	TArray<FAssetData> LabelOnly;
	UPjcSubsystem::GetAssetsFolderLabelOnly(LabelOnly, false);
	TestSetEqual(*this, TEXT("kept only by folder labels"), FixtureAssets(LabelOnly), ObjectPaths(Fixture, {TEXT("C_Labeled")}));

	Settings->bFolderLabelsUsed = false;
	TestSetEqual(*this, TEXT("folder labels not used"), UnusedFixtureAssets(), UnusedDefault.Union(ObjectPaths(Fixture, {TEXT("C_Labeled")})));
	UPjcSubsystem::GetAssetsFolderLabelOnly(LabelOnly, false);
	TestSetEqual(*this, TEXT("only by folder labels, setting off"), FixtureAssets(LabelOnly), ObjectPaths(Fixture, {TEXT("C_Labeled")}));
	Settings->bFolderLabelsUsed = true;

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
		const bool bReachesRoot = Step && Step->Parent.IsNone() && Step->Reason != EPjcUsageReason::None
			&& Step->Reason != EPjcUsageReason::Dependency && Step->Reason != EPjcUsageReason::FolderLabel;
		TestTrue(*FString::Printf(TEXT("%s chain reaches asset used for its own reason"), *Asset.PackageName.ToString()), bReachesRoot);
	}

	const auto TestReason = [&](const TCHAR* Name, const EPjcUsageReason Reason, const TCHAR* Parent, const TCHAR* DetailPart) {
		const FPjcAssetUsage* AssetUsage = Usage.Find(PackageOf(Fixture, Name));
		if (!TestNotNull(*FString::Printf(TEXT("usage of %s"), Name), AssetUsage)) return;

		TestEqual(*FString::Printf(TEXT("%s reason"), Name), static_cast<int32>(AssetUsage->Reason), static_cast<int32>(Reason));
		TestEqual(*FString::Printf(TEXT("%s parent"), Name), AssetUsage->Parent, Parent ? PackageOf(Fixture, Parent) : FName {NAME_None});
		if (DetailPart) {
			TestTrue(
				*FString::Printf(TEXT("%s detail '%s' has '%s'"), Name, *AssetUsage->Detail, DetailPart), AssetUsage->Detail.Contains(DetailPart)
			);
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
	if (FPjcTestFixture::ValidatorBaseClass()) {
		TestReason(TEXT("BP_Validator"), EPjcUsageReason::Editor, nullptr, nullptr);
	}
	else {
		AddInfo(TEXT("DataValidation not loaded, validator blueprint not tested"));
	}
	TestReason(TEXT("C_ByAsset"), EPjcUsageReason::Excluded, nullptr, TEXT("Excluded asset"));
	TestReason(TEXT("C_ByFolder"), EPjcUsageReason::Excluded, nullptr, TEXT("In excluded folder"));
	TestReason(TEXT("CLC_ByClass"), EPjcUsageReason::Excluded, nullptr, TEXT("Excluded class"));
	TestReason(TEXT("LS_Cinematic"), EPjcUsageReason::Excluded, nullptr, TEXT("Cinematic"));
	TestReason(TEXT("C_Labeled"), EPjcUsageReason::FolderLabel, TEXT("PAL_Folder"), nullptr);
	TestReason(TEXT("C_UnusedLeaf"), EPjcUsageReason::None, nullptr, nullptr);
	TestReason(TEXT("C_Indirect_fr"), EPjcUsageReason::Localized, TEXT("C_Indirect"), nullptr);
	TestReason(TEXT("C_AlwaysCooked"), EPjcUsageReason::ConfigFolder, nullptr, TEXT("/Game/PjcTests/AlwaysCook"));
	TestReason(TEXT("C_InPython"), EPjcUsageReason::Indirect, nullptr, TEXT("PjcTests.py"));
	TestReason(TEXT("PAL_Collection"), EPjcUsageReason::Primary, nullptr, nullptr);
	TestReason(TEXT("C_InCollection"), EPjcUsageReason::Dependency, TEXT("PAL_Collection"), TEXT("PjcTestCollection"));
	TestReason(TEXT("C_InPlatformConfig"), EPjcUsageReason::Indirect, nullptr, TEXT("PjcPlatformTests.ini"));
	TestReason(TEXT("C_RegistryData"), EPjcUsageReason::ConfigFolder, nullptr, TEXT("/Game/PjcTests/Registry"));
	TestReason(TEXT("C_Unused_fr"), EPjcUsageReason::None, nullptr, nullptr);

	const FPjcAssetUsage* Labeled = Usage.Find(PackageOf(Fixture, TEXT("C_Labeled")));
	if (TestNotNull(TEXT("usage of C_Labeled"), Labeled)) {
		TestTrue(TEXT("C_Labeled kept only by folder label"), Labeled->bFolderLabelOnly);
		TestTrue(TEXT("C_Labeled used while folder labels are used"), Labeled->bUsed);
	}

	ScopedSettings.Settings->bFolderLabelsUsed = false;
	UPjcSubsystem::GetAssetsUsage(Usage, false);
	Labeled = Usage.Find(PackageOf(Fixture, TEXT("C_Labeled")));
	if (TestNotNull(TEXT("usage of C_Labeled, folder labels off"), Labeled)) {
		TestFalse(TEXT("C_Labeled unused while folder labels are not used"), Labeled->bUsed);
		TestTrue(TEXT("C_Labeled still tells its folder label"), Labeled->bFolderLabelOnly && Labeled->Reason == EPjcUsageReason::FolderLabel);
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
	TestTrue(TEXT("R_Test is an unused redirector"), AssetsUnused.ContainsByPredicate(IsRedirectorPackage));
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
		TestEqual(
			*FString::Printf(TEXT("%s chain reason"), Name), static_cast<int32>(Result.GetChainReason(Package)), static_cast<int32>(ChainReason)
		);
	};
	// labels cook nothing themselves; without cook rules what they list or keep ships only through other references
	TestStatus(TEXT("PAL_Root"), EPjcAssetStatus::EditorOnly, EPjcUsageReason::Primary);
	TestStatus(TEXT("OL_Chain"), EPjcAssetStatus::EditorOnly, EPjcUsageReason::Primary);
	TestStatus(TEXT("C_ChainLeaf"), EPjcAssetStatus::EditorOnly, EPjcUsageReason::Primary);
	TestStatus(TEXT("C_Labeled"), EPjcAssetStatus::EditorOnly, EPjcUsageReason::FolderLabel);
	TestStatus(TEXT("BP_Primary"), EPjcAssetStatus::Used, EPjcUsageReason::Primary);
	TestStatus(TEXT("C_Indirect"), EPjcAssetStatus::Used, EPjcUsageReason::Indirect);
	TestStatus(TEXT("C_Indirect_fr"), EPjcAssetStatus::Used, EPjcUsageReason::Localized);
	TestStatus(TEXT("C_AlwaysCooked"), EPjcAssetStatus::Used, EPjcUsageReason::ConfigFolder);
	TestStatus(TEXT("C_Unused_fr"), EPjcAssetStatus::Unused, EPjcUsageReason::None);
	TestStatus(TEXT("EUB_Tool"), EPjcAssetStatus::EditorOnly, EPjcUsageReason::Editor);
	TestStatus(TEXT("C_UnusedLeaf"), EPjcAssetStatus::Unused, EPjcUsageReason::None);

	// overview: every asset counted once by status, by reason and by class
	const FPjcOverview Overview = FPjcOverview::Make(Result);
	TestEqual(TEXT("overview all"), Overview.All.Num, Result.Assets.Num());
	TestEqual(TEXT("overview by status"), Overview.Used.Num + Overview.EditorOnly.Num + Overview.Unused.Num, Overview.All.Num);
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
	TestEqual(TEXT("overview by reason"), NumByReason, Overview.All.Num);
	TestEqual(TEXT("overview size by reason"), SizeByReason, Overview.All.Size);
	TestTrue(TEXT("overview unused reason"), Unreferenced && Unreferenced->Total.Num == Overview.Unused.Num);
	TestTrue(TEXT("overview source and config reason"), Indirect && Indirect->Total.Num > 0);
	int32 NumByClass = 0;
	for (int32 Index = 0; Index < Overview.Classes.Num(); ++Index) {
		const FPjcOverview::FClass& Class = Overview.Classes[Index];
		NumByClass += Class.Total.Num;
		TestEqual(
			*FString::Printf(TEXT("%s size by status"), *Class.Name.ToString()),
			Class.SizeUsed + Class.SizeEditorOnly + Class.SizeUnused,
			Class.Total.Size
		);
		TestTrue(TEXT("classes biggest first"), Index == 0 || Overview.Classes[Index - 1].Total.Size >= Class.Total.Size);
	}
	TestEqual(TEXT("overview by class"), NumByClass, Overview.All.Num);
	TestEqual(
		TEXT("largest unused"), Overview.LargestUnused.Num(), FMath::Min(Result.Unused.Num(), static_cast<int32>(FPjcOverview::MaxLargestUnused))
	);
	for (int32 Index = 1; Index < Overview.LargestUnused.Num(); ++Index) {
		TestTrue(TEXT("largest unused biggest first"), Overview.LargestUnused[Index - 1].Size >= Overview.LargestUnused[Index].Size);
	}
	TestTrue(TEXT("unused folders found"), Overview.UnusedFolders.Num() > 0);
	for (const auto& Folder : Overview.UnusedFolders) {
		TestTrue(*FString::Printf(TEXT("%s mostly unused"), *Folder.Path), Folder.Unused.Size * 2 >= Folder.All.Size && Folder.Unused.Num > 0);
		TestFalse(
			*FString::Printf(TEXT("%s parent not listed"), *Folder.Path),
			Overview.UnusedFolders.ContainsByPredicate([&Folder](const FPjcOverview::FFolder& Other) {
				return Folder.Path.StartsWith(Other.Path + TEXT("/"));
			})
		);
	}

	// issues: same answers as the separate queries
	TestSetEqual(*this, TEXT("result external files"), FixtureFiles(Result.FilesExternal), Normalized({Fixture.ExternalFile()}));
	TestSetEqual(*this, TEXT("result kept external files"), FixtureFiles(Result.FilesExternalKept), Normalized({Fixture.ExternalExcludedFile()}));
	TestSetEqual(*this, TEXT("result corrupted files"), FixtureFiles(Result.FilesCorrupted), Normalized({Fixture.CorruptedFile()}));
	TestSetEqual(*this, TEXT("result empty folders"), FixtureFiles(Result.FoldersEmpty), Normalized(Fixture.EmptyFolders()));

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
	ScopedSettings.Settings->bFolderLabelsUsed = false;
	TestFalse(TEXT("draft follows settings until edited"), FPjcScanDraft::Get().bFolderLabelsUsed || FPjcScanDraft::IsChanged());
	ScopedSettings.Settings->bFolderLabelsUsed = true;

	// editing the draft leaves the settings alone until applied
	FPjcScanDraft::Get().bFolderLabelsUsed = false;
	FPjcScanDraft::NotifyEdited();
	TestTrue(TEXT("edited draft is changed"), FPjcScanDraft::IsChanged());
	TestTrue(TEXT("settings untouched by the draft"), ScopedSettings.Settings->bFolderLabelsUsed);
	FPjcScanDraft::Revert();
	TestFalse(TEXT("undo back to the settings"), FPjcScanDraft::IsChanged());
	TestTrue(TEXT("undo restores the value"), FPjcScanDraft::Get().bFolderLabelsUsed);

	FPjcScanDraft::Get().bCinematicAssetsUsed = false;
	FPjcScanDraft::Get().UsedClasses.Add(UCurveFloat::StaticClass()->GetPathName());
	FPjcScanDraft::NotifyEdited();
	FPjcScanDraft::Get().ApplyToSettings();
	TestFalse(TEXT("applied draft is not changed"), FPjcScanDraft::IsChanged());
	TestFalse(TEXT("applied to settings"), ScopedSettings.Settings->bCinematicAssetsUsed);
	if (TestEqual(TEXT("used classes applied"), ScopedSettings.Settings->UsedClasses.Num(), 1)) {
		TestEqual(TEXT("used class resolves"), ScopedSettings.Settings->UsedClasses[0].LoadSynchronous(), UCurveFloat::StaticClass());
	}

	TestTrue(TEXT("plugin defaults"), FPjcScanDraft {}.bUnregisteredPrimaryUsed && FPjcScanDraft {}.bFolderLabelsUsed);

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
	TestTrue(
		TEXT("moved class"),
		Settings->ExcludedClasses.Num() == 1 && Settings->ExcludedClasses[0].ToSoftObjectPath().ToString() == TEXT("/Script/Engine.CurveFloat")
	);
	TestTrue(
		TEXT("moved asset"),
		Settings->ExcludedAssets.Num() == 1 && Settings->ExcludedAssets[0].ToSoftObjectPath().ToString() == TEXT("/Game/PjcMoved/C_Moved.C_Moved")
	);

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
	TestTrue(
		TEXT("moved file"), FileSettings->ExcludedFiles.Num() == 1 && FileSettings->ExcludedFiles[0].FilePath == TEXT("Content/PjcMoved/notes.txt")
	);
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
		*State.CreateOrGetAssetPackageData(PackageOf(Fixture, TEXT("BP_Primary")))
			= PjcShim::GetAssetPackageData(PackageOf(Fixture, TEXT("BP_Primary")));
		AddPackage(PackageOf(Fixture, TEXT("BP_Primary")), 1000);
		AddPackage(FName {TEXT("/Script/Engine")}, 0);
		AddPackage(PackageOf(Fixture, TEXT("C_UnusedLeaf")), 77);
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
	TestEqual(TEXT("cooked packages"), Cook.Cooked.Num(), 5);
	TestEqual(TEXT("cooked size"), Cook.Cooked.FindRef(PackageOf(Fixture, TEXT("BP_Primary"))), static_cast<int64>(1000));
	TestFalse(TEXT("not cooked is left out"), Cook.Cooked.Contains(PackageOf(Fixture, TEXT("EUB_Tool"))));

	FPjcOverview::FShipped Shipped = FPjcOverview::MakeShipped(Result, Cook);
	TestEqual(TEXT("shipped"), Shipped.Shipped.Num, 1);
	TestEqual(TEXT("shipped cooked size"), Shipped.ShippedCooked, static_cast<int64>(1000));
	TestEqual(TEXT("unused shipped"), Shipped.UnusedShipped.Num, 1);
	TestEqual(TEXT("used, not shipped"), Shipped.UsedNotShipped.Num, Result.Used.Num() - 1);
	TestEqual(TEXT("unused, not shipped"), Shipped.Unused.Num, Result.Unused.Num() - 1);
	TestEqual(TEXT("outside the scan"), Shipped.NumOutside, 1);
	TestEqual(TEXT("outside size"), Shipped.SizeOutside, static_cast<int64>(500));
	TestTrue(TEXT("outside root"), Shipped.OutsideRoots.Num() == 1 && Shipped.OutsideRoots[0].Key == TEXT("/Engine"));
	TestEqual(TEXT("generated"), Shipped.NumGenerated, 1);
	TestEqual(TEXT("deleted since"), Shipped.NumDeleted, 1);
	TestEqual(TEXT("nothing changed since"), Shipped.NumChanged, 0);

	TestFalse(TEXT("script packages left out"), Cook.Cooked.Contains(FName {TEXT("/Script/Engine")}));
	TestTrue(TEXT("saved digest read"), Cook.SavedDigests.FindRef(PackageOf(Fixture, TEXT("BP_Primary"))) != 0);

	// files touched after the cook: changed only when what they were saved with differs, file time when that is not known
	FString PrimaryFile;
	FString LeafFile;
	const bool bFiles = PjcShim::DoesPackageExist(PackageOf(Fixture, TEXT("BP_Primary")).ToString(), &PrimaryFile)
		&& PjcShim::DoesPackageExist(PackageOf(Fixture, TEXT("C_UnusedLeaf")).ToString(), &LeafFile);
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
	const bool bUnseenFiles = PjcShim::DoesPackageExist(PackageOf(Fixture, TEXT("C_Unused_fr")).ToString(), &UnusedFile)
		&& PjcShim::DoesPackageExist(PackageOf(Fixture, TEXT("C_Indirect")).ToString(), &IndirectFile);
	if (TestTrue(TEXT("unseen asset files"), bUnseenFiles)) {
		IFileManager::Get().SetTimeStamp(*UnusedFile, Cook.Time + FTimespan::FromMinutes(1.0));
		IFileManager::Get().SetTimeStamp(*IndirectFile, Cook.Time + FTimespan::FromMinutes(1.0));
		Shipped = FPjcOverview::MakeShipped(Result, Cook);
		TestEqual(TEXT("new used asset since"), Shipped.NumChanged, 2);
	}

	IFileManager::Get().DeleteDirectory(*PlatformDir, false, true);
	TestFalse(TEXT("no cook"), FPjcCookInfo::GetLast().IsFound());
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
	TestTrue(TEXT("unused folder is all unused"), Unused->IsAllUnused());
	TestTrue(TEXT("parent has the subfolder totals"), Tree.Find(FPjcTestFixture::RootPath())->Unused.Num >= Unused->Unused.Num);
	for (const auto& EmptyFolder : Fixture.EmptyFolders()) {
		const TSharedPtr<FPjcUnusedFolder> Folder = Tree.Find(UPjcSubsystem::PathConvertToRelative(EmptyFolder));
		TestTrue(*FString::Printf(TEXT("%s marked empty"), *EmptyFolder), Folder.IsValid() && Folder->bEmpty);
	}
	TArray<int32> Direct;
	TArray<int32> WithSubfolders;
	Tree.GetUnused(Result, UnusedPath, false, Direct);
	Tree.GetUnused(Result, UnusedPath, true, WithSubfolders);
	TestEqual(TEXT("unused with subfolders"), WithSubfolders.Num(), Unused->Unused.Num);
	TestTrue(TEXT("unused directly in the folder"), Direct.Num() > 0 && Direct.Num() < WithSubfolders.Num());

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
	TestEqual(TEXT("assets to delete"), ToDelete.Num(), 2);
	TestTrue(TEXT("deleted"), UPjcSubsystem::DeleteAssets(ToDelete, false, false));
	TestFalse(TEXT("leaf gone"), FPackageName::DoesPackageExist(Leaf.ToString()));
	TestFalse(TEXT("root gone"), FPackageName::DoesPackageExist(LeafRoot.ToString()));
	TestTrue(TEXT("other unused asset kept"), FPackageName::DoesPackageExist(PackageOf(Fixture, TEXT("C_Unused")).ToString()));
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
		return !File.Contains(TEXT("/Content/Localization/PjcTests/")) && !File.EndsWith(TEXT("/Staged/license.txt"))
			&& !File.EndsWith(TEXT("/T_RawIcon.svg"));
	});
	TestEqual(TEXT("fixture kept files"), KeptFiles.Num(), 3);
	for (const auto& File : KeptFiles) {
		const FString Expected = File.Contains(TEXT("/Localization/")) ? TEXT("Engine folder (Localization)")
			: File.Contains(TEXT("/Staged/"))						   ? TEXT("Staged folder")
																	   : TEXT("Named in source or config");
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
	if (!TestTrue(TEXT("fixture redirectors found"), Moved && Stale && Dangling)) return false;
	TestEqual(TEXT("R_Moved in use"), static_cast<int32>(Moved->State), static_cast<int32>(EPjcRedirectorState::InUse));
	TestTrue(TEXT("R_Moved used by PAL_Root"), Moved->UsedBy.Contains(PackageOf(Fixture, TEXT("PAL_Root"))));
	TestEqual(TEXT("R_Moved points to C_Target"), Moved->Target, PackageOf(Fixture, TEXT("C_Target")));
	TestEqual(TEXT("R_Stale not referenced"), static_cast<int32>(Stale->State), static_cast<int32>(EPjcRedirectorState::NotReferenced));
	TestEqual(TEXT("R_Dangling broken"), static_cast<int32>(Dangling->State), static_cast<int32>(EPjcRedirectorState::Broken));
	TestTrue(TEXT("unused broken redirector can be fixed"), Dangling->CanFix());

	// fix: users resaved, every redirector deleted, targets kept
	const FPjcRedirectorFixResult Result = UPjcSubsystem::FixRedirectorsWithReport({Moved->Asset, Stale->Asset, Dangling->Asset}, false);
	TestEqual(TEXT("nothing kept"), Result.Kept.Num(), 0);
	TestEqual(TEXT("nothing failed to resave"), Result.NotResaved.Num(), 0);
	TestEqual(TEXT("redirectors deleted"), Result.Deleted.Num(), 3);
	TestTrue(TEXT("user resaved"), Result.NumResaved >= 1);
	for (const TCHAR* Name : {TEXT("R_Moved"), TEXT("R_Stale"), TEXT("R_Dangling")}) {
		TestFalse(*FString::Printf(TEXT("%s gone"), Name), FPackageName::DoesPackageExist(PackageOf(Fixture, Name).ToString()));
	}
	TestTrue(TEXT("C_Target kept"), FPackageName::DoesPackageExist(PackageOf(Fixture, TEXT("C_Target")).ToString()));
	TestTrue(TEXT("C_ChainLeaf kept"), FPackageName::DoesPackageExist(PackageOf(Fixture, TEXT("C_ChainLeaf")).ToString()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPjcTestScanGraph, "ProjectCleaner.Scan.Graph", PJC_TEST_FLAGS)

bool FPjcTestScanGraph::RunTest(const FString& Parameters) {
	if (!FPjcTestFixture::IsTestHost()) {
		AddInfo(TEXT("Skipped, fixture tests run only in test host projects"));
		return true;
	}

	FPjcScopedScanSettings ScopedSettings {false};

	FPjcTestFixture Fixture;
	if (!Fixture.Build(*this)) return false;

	TArray<FAssetData> AssetsAll;
	TMap<FName, FPjcAssetUsage> Usage;
	UPjcSubsystem::GetAssetsAll(AssetsAll);
	UPjcSubsystem::GetAssetsUsage(Usage, false);

	TArray<TSharedPtr<FPjcUsageItem>> Items;
	for (const auto& Asset : AssetsAll) {
		const TSharedPtr<FPjcUsageItem> Item = MakeShared<FPjcUsageItem>();
		Item->PackageName = Asset.PackageName;
		Item->Asset = Asset;
		Item->Usage = Usage.FindRef(Asset.PackageName);
		Item->Size = UPjcSubsystem::GetAssetSize(Asset);
		Item->bScanned = true;
		Items.Add(Item);
	}

	FPjcAssetGraph Graph;
	Graph.Build(Items);
	const TArray<FPjcGraphNode>& Nodes = Graph.GetNodes();
	TestEqual(TEXT("node for every scanned asset"), Nodes.Num(), AssetsAll.Num());

	// folders hold every asset, totals add up, circles stay inside their folder
	const TArray<FPjcGraphFolder>& Folders = Graph.GetFolders();
	int32 NumInRoots = 0;
	for (const int32 Root : Graph.GetRootFolders()) {
		NumInRoots += Folders[Root].NumAssets;
	}
	TestEqual(TEXT("folders hold every asset"), NumInRoots, Nodes.Num());

	for (const auto& Folder : Folders) {
		int32 NumAssets = Folder.Assets.Num();
		for (const int32 Subfolder : Folder.Folders) {
			NumAssets += Folders[Subfolder].NumAssets;
			const double Reach = FVector2D::Distance(Folders[Subfolder].Center, Folder.Center) + Folders[Subfolder].Radius;
			TestTrue(*FString::Printf(TEXT("%s inside %s"), *Folders[Subfolder].Path, *Folder.Path), Reach <= Folder.Radius + 0.01);
		}
		TestEqual(*FString::Printf(TEXT("%s asset count"), *Folder.Path), Folder.NumAssets, NumAssets);
		for (const int32 Asset : Folder.Assets) {
			const double Reach = FVector2D::Distance(Nodes[Asset].Position, Folder.Center) + Nodes[Asset].Radius;
			TestTrue(*FString::Printf(TEXT("%s inside its folder"), *Nodes[Asset].Item->PackageName.ToString()), Reach <= Folder.Radius + 0.01);
		}
	}

	const FPjcGraphFolder* UnusedFolder = Folders.FindByPredicate([](const FPjcGraphFolder& Folder) {
		return Folder.Path == FPjcTestFixture::RootPath() / TEXT("Unused");
	});
	if (TestNotNull(TEXT("folder of unused fixture assets"), UnusedFolder)) {
		TestTrue(TEXT("unused folder counts its unused assets"), UnusedFolder->NumAssets >= 3 && UnusedFolder->NumUnused == UnusedFolder->NumAssets);
		TestEqual(TEXT("unused folder size is unused"), UnusedFolder->SizeUnused, UnusedFolder->Size);
	}

	// owner has same verdict, owned sizes add up
	for (const auto& Node : Nodes) {
		const FString Name = Node.Item->PackageName.ToString();
		int64 Size = Node.Item->Size;
		for (const int32 Owned : Node.Owned) {
			Size += Nodes[Owned].SizeOwned;
		}
		TestEqual(*FString::Printf(TEXT("%s owned size"), *Name), Node.SizeOwned, Size);
		if (Node.Owner == INDEX_NONE) continue;

		TestEqual(*FString::Printf(TEXT("%s owner verdict"), *Name), Nodes[Node.Owner].Item->Usage.bUsed, Node.Item->Usage.bUsed);
	}

	const auto TestOwner = [&](const TCHAR* Name, const TCHAR* OwnerName) {
		const int32 Index = Graph.Find(PackageOf(Fixture, Name));
		if (!TestTrue(*FString::Printf(TEXT("%s in graph"), Name), Index != INDEX_NONE)) return;

		const int32 Owner = Nodes[Index].Owner;
		TestEqual(
			*FString::Printf(TEXT("%s owner"), Name),
			Owner == INDEX_NONE ? FName {NAME_None} : Nodes[Owner].Item->PackageName,
			OwnerName ? PackageOf(Fixture, OwnerName) : FName {NAME_None}
		);
	};

	// only user owns, chains nest, unused groups and cycles have their own owners
	TestOwner(TEXT("PAL_Root"), nullptr);
	TestOwner(TEXT("OL_Chain"), TEXT("PAL_Root"));
	TestOwner(TEXT("C_ChainLeaf"), TEXT("OL_Chain"));
	TestOwner(TEXT("C_Labeled"), TEXT("PAL_Folder"));
	TestOwner(TEXT("OL_UnusedRoot"), nullptr);
	TestOwner(TEXT("C_UnusedLeaf"), TEXT("OL_UnusedRoot"));
	TestOwner(TEXT("OL_CircA"), nullptr);
	TestOwner(TEXT("OL_CircB"), TEXT("OL_CircA"));

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

	UPjcEditorSettings* EditorSettings = GetMutableDefault<UPjcEditorSettings>();
	const bool bScanOnOpen = EditorSettings->bScanOnOpen;
	EditorSettings->bScanOnOpen = true;

	const TSharedRef<SPjcTabAssetsUsage> TabUsage = SNew(SPjcTabAssetsUsage);
	const TSharedRef<SPjcTabUnused> TabUnused = SNew(SPjcTabUnused);
	const TSharedRef<SPjcTabIssues> TabIssues = SNew(SPjcTabIssues);
	const TSharedRef<SWidget> Tabs[] = {
	  SNew(SPjcTabOverview), TabUnused, TabIssues, SNew(SPjcTabAssetsIndirect), SNew(SPjcTabAssetsCorrupted), SNew(SPjcTabFilesExternal), TabUsage
	};
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
		for (const TCHAR* Name : {TEXT("C_ChainLeaf"), TEXT("C_UnusedLeaf"), TEXT("C_Labeled"), TEXT("C_RefByPlugin")}) {
			TabUsage->Inspect(PackageOf(Fixture, Name));
		}
	}
	TabUsage->Inspect(FName {TEXT("/Engine/EngineMaterials/WorldGridMaterial")});

	EditorSettings->bScanOnOpen = bScanOnOpen;
	return true;
}

/**
 * Not part of ProjectCleaner.* tests. Leaves fixture content on disk and writes expected results,
 * so Scripts/pjc.py can verify the commandlet (-run=Pjc) in a separate process.
 */
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
