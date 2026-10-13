// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Tests/PjcTestFixture.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "PjcAssetGraph.h"
#include "PjcSettings.h"
#include "PjcShim.h"
#include "PjcSubsystem.h"
#include "PjcTypes.h"
#include "Curves/CurveFloat.h"
#include "Engine/AssetManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SPjcTabAssetsCorrupted.h"
#include "Slate/SPjcTabAssetsIndirect.h"
#include "Slate/SPjcTabAssetsUnused.h"
#include "Slate/SPjcTabAssetsUsage.h"
#include "Slate/SPjcTabFilesExternal.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"

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
		return Path.StartsWith(FPjcTestFixture::RootPath() + TEXT("/")) || Path.StartsWith(FPjcTestFixture::PluginRootPath() + TEXT("/"));
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
		bool bPrimaryAssetsUsed;
		TArray<FName> IgnoredPrimaryAssetTypes;
		bool bFolderLabelsUsed;
		bool bCinematicAssetsUsed;
		bool bEditorAssetsUsed;
		bool bIndirectAssetsUsed;
		TArray<TSoftClassPtr<UObject>> UsedClasses;

		explicit FPjcScopedScanSettings(const bool bInScanProjectPlugins) : Settings(GetMutableDefault<UPjcScanSettings>()) {
			bScanProjectPlugins = Settings->bScanProjectPlugins;
			IgnoredPlugins = Settings->IgnoredPlugins;
			bPrimaryAssetsUsed = Settings->bPrimaryAssetsUsed;
			IgnoredPrimaryAssetTypes = Settings->IgnoredPrimaryAssetTypes;
			bFolderLabelsUsed = Settings->bFolderLabelsUsed;
			bCinematicAssetsUsed = Settings->bCinematicAssetsUsed;
			bEditorAssetsUsed = Settings->bEditorAssetsUsed;
			bIndirectAssetsUsed = Settings->bIndirectAssetsUsed;
			UsedClasses = Settings->UsedClasses;

			// known state, fixture expectations are written for defaults
			Settings->bScanProjectPlugins = bInScanProjectPlugins;
			Settings->IgnoredPlugins.Reset();
			Settings->bPrimaryAssetsUsed = true;
			Settings->IgnoredPrimaryAssetTypes.Reset();
			Settings->bFolderLabelsUsed = true;
			Settings->bCinematicAssetsUsed = true;
			Settings->bEditorAssetsUsed = true;
			Settings->bIndirectAssetsUsed = true;
			Settings->UsedClasses.Reset();
		}

		~FPjcScopedScanSettings() {
			Settings->bScanProjectPlugins = bScanProjectPlugins;
			Settings->IgnoredPlugins = IgnoredPlugins;
			Settings->bPrimaryAssetsUsed = bPrimaryAssetsUsed;
			Settings->IgnoredPrimaryAssetTypes = IgnoredPrimaryAssetTypes;
			Settings->bFolderLabelsUsed = bFolderLabelsUsed;
			Settings->bCinematicAssetsUsed = bCinematicAssetsUsed;
			Settings->bEditorAssetsUsed = bEditorAssetsUsed;
			Settings->bIndirectAssetsUsed = bIndirectAssetsUsed;
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
	TestSetEqual(*this, TEXT("editor"), FixtureAssets(Assets), ObjectPaths(Fixture, {TEXT("EUB_Tool")}));

	UPjcSubsystem::GetAssetsExcluded(Assets, false);
	TestSetEqual(*this, TEXT("excluded"), FixtureAssets(Assets), ObjectPaths(Fixture, Fixture.ExcludedNames()));

	UPjcSubsystem::GetAssetsCircular(Assets, false);
	TestSetEqual(*this, TEXT("circular"), FixtureAssets(Assets), ObjectPaths(Fixture, {TEXT("OL_CircA"), TEXT("OL_CircB")}));

	TArray<FPjcAssetIndirectInfo> IndirectInfos;
	UPjcSubsystem::GetAssetsIndirect(Assets, IndirectInfos, false);
	TestSetEqual(*this, TEXT("indirect"), FixtureAssets(Assets), ObjectPaths(Fixture, {TEXT("C_Indirect"), TEXT("C_IndirectPackage")}));

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

	Settings->bEditorAssetsUsed = false;
	TestSetEqual(*this, TEXT("editor assets not used"), UnusedFixtureAssets(), UnusedDefault.Union(ObjectPaths(Fixture, {TEXT("EUB_Tool")})));
	Settings->bEditorAssetsUsed = true;

	Settings->bIndirectAssetsUsed = false;
	TestSetEqual(
		*this,
		TEXT("indirect assets not used"),
		UnusedFixtureAssets(),
		UnusedDefault.Union(ObjectPaths(Fixture, {TEXT("C_Indirect"), TEXT("C_IndirectPackage")}))
	);
	Settings->bIndirectAssetsUsed = true;

	Settings->IgnoredPrimaryAssetTypes.Add(TEXT("PjcTestPrimary"));
	TestSetEqual(*this, TEXT("ignored primary type"), UnusedFixtureAssets(), UnusedDefault.Union(ObjectPaths(Fixture, {TEXT("BP_Primary")})));
	Settings->IgnoredPrimaryAssetTypes.Reset();

	Settings->bPrimaryAssetsUsed = false;
	TestSetEqual(
		*this,
		TEXT("primary assets not used"),
		UnusedFixtureAssets(),
		UnusedDefault.Union(
			ObjectPaths(Fixture, {TEXT("PAL_Root"), TEXT("OL_Chain"), TEXT("C_ChainLeaf"), TEXT("BP_Primary"), TEXT("PAL_Folder"), TEXT("C_Labeled")})
		)
	);
	Settings->bPrimaryAssetsUsed = true;

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
	TestReason(TEXT("C_Indirect"), EPjcUsageReason::Indirect, nullptr, TEXT("PjcTests.ini, line 2"));
	TestReason(TEXT("C_RefByPlugin"), EPjcUsageReason::EngineReferenced, nullptr, TEXT("/PjcTestContent/"));
	TestReason(TEXT("EUB_Tool"), EPjcUsageReason::Editor, nullptr, nullptr);
	TestReason(TEXT("C_ByAsset"), EPjcUsageReason::Excluded, nullptr, TEXT("Excluded asset"));
	TestReason(TEXT("C_ByFolder"), EPjcUsageReason::Excluded, nullptr, TEXT("In excluded folder"));
	TestReason(TEXT("CLC_ByClass"), EPjcUsageReason::Excluded, nullptr, TEXT("Excluded class"));
	TestReason(TEXT("LS_Cinematic"), EPjcUsageReason::Excluded, nullptr, TEXT("Cinematic"));
	TestReason(TEXT("C_Labeled"), EPjcUsageReason::FolderLabel, TEXT("PAL_Folder"), nullptr);
	TestReason(TEXT("C_UnusedLeaf"), EPjcUsageReason::None, nullptr, nullptr);

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
	const TSharedRef<SWidget> Tabs[]
		= {SNew(SPjcTabAssetsUnused), SNew(SPjcTabAssetsIndirect), SNew(SPjcTabAssetsCorrupted), SNew(SPjcTabFilesExternal), TabUsage};
	for (const auto& Tab : Tabs) {
		TestTrue(FString::Printf(TEXT("%s has content"), *Tab->GetTypeAsString()), Tab->GetChildren()->Num() > 0);
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
	GetMutableDefault<UPjcAssetExcludeSettings>()->SaveConfig();
	GetMutableDefault<UPjcFileExcludeSettings>()->SaveConfig();
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
