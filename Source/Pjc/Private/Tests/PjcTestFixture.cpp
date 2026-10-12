// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Tests/PjcTestFixture.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "PjcTypes.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveLinearColor.h"
#include "AssetToolsModule.h"
#include "EditorUtilityBlueprint.h"
#include "EditorUtilityObject.h"
#include "FileHelpers.h"
#include "ObjectTools.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/ObjectLibrary.h"
#include "Engine/PrimaryAssetLabel.h"
#include "Factories/Factory.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

namespace
{
	IAssetRegistry& AssetRegistry() {
		return FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	}

	FString ContentDir() {
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
	}

	FString PluginContentDir() {
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectPluginsDir() / TEXT("PjcTestContent") / TEXT("Content"));
	}

	UPackage* NewPackage(const FString& PackageName, TArray<UPackage*>& OutPackages) {
		UPackage* Package = CreatePackage(*PackageName);
		OutPackages.Add(Package);
		return Package;
	}

	template<typename T>
	T* NewAsset(const FString& PackageName, TArray<UPackage*>& OutPackages) {
		UPackage* Package = NewPackage(PackageName, OutPackages);
		const FName AssetName {*FPackageName::GetLongPackageAssetName(PackageName)};
		T* Asset = NewObject<T>(Package, T::StaticClass(), AssetName, RF_Public | RF_Standalone);
		FAssetRegistryModule::AssetCreated(Asset);
		Package->MarkPackageDirty();
		return Asset;
	}

	// created through engine factory found by class path, so tests need no LevelSequence module dependency
	UObject* NewLevelSequence(const FString& PackageName, TArray<UPackage*>& OutPackages) {
		UClass* AssetClass = FSoftClassPath {TEXT("/Script/LevelSequence.LevelSequence")}.ResolveClass();
		UClass* FactoryClass = FSoftClassPath {TEXT("/Script/LevelSequenceEditor.LevelSequenceFactoryNew")}.ResolveClass();
		if (!AssetClass || !FactoryClass) return nullptr;

		UFactory* Factory = NewObject<UFactory>(GetTransientPackage(), FactoryClass);
		UObject* Asset
			= FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"))
				  .Get()
				  .CreateAsset(
					  FPackageName::GetLongPackageAssetName(PackageName), FPackageName::GetLongPackagePath(PackageName), AssetClass, Factory
				  );
		if (!Asset) return nullptr;

		OutPackages.Add(Asset->GetOutermost());
		return Asset;
	}

	UBlueprint*
	NewBlueprint(const FString& PackageName, UClass* ParentClass, TSubclassOf<UBlueprint> BlueprintClass, TArray<UPackage*>& OutPackages) {
		UPackage* Package = NewPackage(PackageName, OutPackages);
		const FName AssetName {*FPackageName::GetLongPackageAssetName(PackageName)};
		UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
			ParentClass, Package, AssetName, BPTYPE_Normal, BlueprintClass, UBlueprintGeneratedClass::StaticClass()
		);
		if (!Blueprint) return nullptr;

		FAssetRegistryModule::AssetCreated(Blueprint);
		Package->MarkPackageDirty();
		return Blueprint;
	}
}	// namespace

bool FPjcTestFixture::IsTestHost() {
	return FParse::Param(FCommandLine::Get(), TEXT("PjcTestHost"));
}

FString FPjcTestFixture::RootPath() {
	return TEXT("/Game/PjcTests");
}

FString FPjcTestFixture::PluginRootPath() {
	return TEXT("/PjcTestContent");
}

FString FPjcTestFixture::IndirectConfigFile() {
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir() / TEXT("PjcTests.ini"));
}

FPjcTestFixture::FPjcTestFixture() {}

FPjcTestFixture::~FPjcTestFixture() {
	Cleanup();
}

bool FPjcTestFixture::Build(FAutomationTestBase& Test, const bool bApplyExcludeSettings, const bool bCreateCorruptedFile) {
	if (!IsTestHost()) {
		Test.AddError(TEXT("Fixture tests create and delete assets, run them only in test host projects (-PjcTestHost)"));
		return false;
	}

	// editor asset discovery is async, all checks below need complete registry
	AssetRegistry().SearchAllAssets(true);

	// leftovers from crashed or kept runs
	Cleanup();
	bKeep = false;

	if (!CreateAssets(Test)) return false;

	CreateFiles(bCreateCorruptedFile);

	if (bApplyExcludeSettings) {
		ApplyExcludeSettings();
	}

	bBuilt = true;
	return true;
}

bool FPjcTestFixture::CreateAssets(FAutomationTestBase& Test) {
	auto Register = [&](const FString& Folder, const FString& Name) {
		const FString PackageName = Folder.IsEmpty() ? RootPath() / Name : RootPath() / Folder / Name;
		PackageNames.Add(Name, PackageName);
		return PackageName;
	};

	auto RegisterPlugin = [&](const FString& Name) {
		const FString PackageName = PluginRootPath() / Name;
		PackageNames.Add(Name, PackageName);
		return PackageName;
	};

	// used chain: primary label -> object library -> curves
	UCurveFloat* ChainLeaf = NewAsset<UCurveFloat>(Register(TEXT("Used"), TEXT("C_ChainLeaf")), Packages);
	UCurveFloat* PluginUsed = NewAsset<UCurveFloat>(RegisterPlugin(TEXT("C_PluginUsed")), Packages);
	UObjectLibrary* Chain = NewAsset<UObjectLibrary>(Register(TEXT("Used"), TEXT("OL_Chain")), Packages);
	Chain->ObjectBaseClass = UCurveFloat::StaticClass();
	Chain->AddObject(ChainLeaf);
	Chain->AddObject(PluginUsed);
	UPrimaryAssetLabel* Label = NewAsset<UPrimaryAssetLabel>(Register(TEXT("Used"), TEXT("PAL_Root")), Packages);
	Label->ExplicitAssets.Add(TSoftObjectPtr<UObject> {Chain});

	// unused
	NewAsset<UCurveFloat>(Register(TEXT("Unused"), TEXT("C_Unused")), Packages);
	NewAsset<UCurveFloat>(RegisterPlugin(TEXT("C_PluginUnused")), Packages);
	UCurveFloat* UnusedLeaf = NewAsset<UCurveFloat>(Register(TEXT("Unused"), TEXT("C_UnusedLeaf")), Packages);
	UObjectLibrary* UnusedRoot = NewAsset<UObjectLibrary>(Register(TEXT("Unused"), TEXT("OL_UnusedRoot")), Packages);
	UnusedRoot->ObjectBaseClass = UCurveFloat::StaticClass();
	UnusedRoot->AddObject(UnusedLeaf);

	for (int32 i = 0; i < NumBulkAssets; ++i) {
		NewAsset<UCurveFloat>(Register(TEXT("Unused/Bulk"), FString::Printf(TEXT("C_Bulk_%02d"), i)), Packages);
	}

	// circular, unused
	UObjectLibrary* CircA = NewAsset<UObjectLibrary>(Register(TEXT("Circular"), TEXT("OL_CircA")), Packages);
	UObjectLibrary* CircB = NewAsset<UObjectLibrary>(Register(TEXT("Circular"), TEXT("OL_CircB")), Packages);
	CircA->ObjectBaseClass = UObjectLibrary::StaticClass();
	CircB->ObjectBaseClass = UObjectLibrary::StaticClass();
	CircA->AddObject(CircB);
	CircB->AddObject(CircA);

	// plugin content referencing /Game content
	UCurveFloat* RefByPlugin = NewAsset<UCurveFloat>(Register(TEXT("Misc"), TEXT("C_RefByPlugin")), Packages);
	UObjectLibrary* PluginLib = NewAsset<UObjectLibrary>(RegisterPlugin(TEXT("OL_PluginLib")), Packages);
	PluginLib->ObjectBaseClass = UCurveFloat::StaticClass();
	PluginLib->AddObject(RefByPlugin);

	// used by other means
	if (!NewLevelSequence(Register(TEXT("Cinematics"), TEXT("LS_Cinematic")), Packages)) {
		Test.AddError(TEXT("Failed to create fixture level sequence"));
		return false;
	}
	NewAsset<UCurveFloat>(Register(TEXT("Indirect"), TEXT("C_Indirect")), Packages);
	NewAsset<UCurveFloat>(Register(TEXT("Indirect"), TEXT("C_IndirectPackage")), Packages);
	NewAsset<UCurveFloat>(Register(TEXT("Excluded"), TEXT("C_ByFolder")), Packages);
	NewAsset<UCurveLinearColor>(Register(TEXT("Misc"), TEXT("CLC_ByClass")), Packages);
	NewAsset<UCurveFloat>(Register(TEXT("Misc"), TEXT("C_ByAsset")), Packages);

	// blueprints
	const UBlueprint* EditorTool = NewBlueprint(
		Register(TEXT("Editor"), TEXT("EUB_Tool")), UEditorUtilityObject::StaticClass(), UEditorUtilityBlueprint::StaticClass(), Packages
	);
	const UBlueprint* BlueprintPrimary
		= NewBlueprint(Register(TEXT("Blueprints"), TEXT("BP_Primary")), APlayerController::StaticClass(), UBlueprint::StaticClass(), Packages);
	const UBlueprint* BlueprintUnused
		= NewBlueprint(Register(TEXT("Blueprints"), TEXT("BP_Unused")), AActor::StaticClass(), UBlueprint::StaticClass(), Packages);

	if (!EditorTool || !BlueprintPrimary || !BlueprintUnused) {
		Test.AddError(TEXT("Failed to create fixture blueprints"));
		return false;
	}

	if (!UEditorLoadingAndSavingUtils::SavePackages(Packages, false)) {
		Test.AddError(TEXT("Failed to save fixture packages"));
		return false;
	}

	// dependencies are gathered from saved package files
	TArray<FString> Files;
	for (const auto& Pair : PackageNames) {
		Files.Add(PackageFile(Pair.Key));
	}
	AssetRegistry().ScanFilesSynchronous(Files, true);

	for (const auto& File : Files) {
		if (!FPaths::FileExists(File)) {
			Test.AddError(FString::Printf(TEXT("Fixture file was not saved: %s"), *File));
			return false;
		}
	}

	return true;
}

void FPjcTestFixture::CreateFiles(const bool bCreateCorruptedFile) {
	const FString Root = ContentDir() / TEXT("PjcTests");

	IFileManager::Get().MakeDirectory(*(Root / TEXT("Empty/A/B")), true);
	FFileHelper::SaveStringToFile(TEXT("external file"), *ExternalFile());
	FFileHelper::SaveStringToFile(TEXT("external file excluded by extension"), *ExternalExcludedFile());
	if (bCreateCorruptedFile) {
		FFileHelper::SaveStringToFile(TEXT("this is not an unreal package"), *CorruptedFile());
	}

	const FString Config = FString::Printf(
		TEXT("[PjcTests]\nIndirectAsset=%s\nIndirectPackage=%s\n"), *ObjectPath(TEXT("C_Indirect")), **PackageNames.Find(TEXT("C_IndirectPackage"))
	);
	FFileHelper::SaveStringToFile(Config, *IndirectConfigFile());
}

void FPjcTestFixture::ApplyExcludeSettings() {
	UPjcAssetExcludeSettings* AssetSettings = GetMutableDefault<UPjcAssetExcludeSettings>();
	UPjcFileExcludeSettings* FileSettings = GetMutableDefault<UPjcFileExcludeSettings>();

	if (!bSettingsSaved) {
		for (const auto& Folder : AssetSettings->ExcludedFolders) {
			SavedExcludedFolders.Add(Folder.Path);
		}
		for (const auto& Class : AssetSettings->ExcludedClasses) {
			SavedExcludedClasses.Add(FSoftClassPath {Class.ToSoftObjectPath().ToString()});
		}
		for (const auto& Asset : AssetSettings->ExcludedAssets) {
			SavedExcludedAssets.Add(Asset.ToSoftObjectPath());
		}
		SavedExcludedExtensions = FileSettings->ExcludedExtensions;
		bSettingsSaved = true;
	}

	AssetSettings->ExcludedFolders.Reset();
	AssetSettings->ExcludedClasses.Reset();
	AssetSettings->ExcludedAssets.Reset();
	AssetSettings->ExcludedFolders.Add(FDirectoryPath {RootPath() / TEXT("Excluded")});
	AssetSettings->ExcludedClasses.Add(UCurveLinearColor::StaticClass());
	AssetSettings->ExcludedAssets.Add(TSoftObjectPtr<UObject> {FSoftObjectPath {ObjectPath(TEXT("C_ByAsset"))}});

	FileSettings->ExcludedExtensions.Reset();
	FileSettings->ExcludedExtensions.Add(TEXT("md"));
}

void FPjcTestFixture::RestoreSettings() {
	if (!bSettingsSaved) return;

	UPjcAssetExcludeSettings* AssetSettings = GetMutableDefault<UPjcAssetExcludeSettings>();
	UPjcFileExcludeSettings* FileSettings = GetMutableDefault<UPjcFileExcludeSettings>();

	AssetSettings->ExcludedFolders.Reset();
	AssetSettings->ExcludedClasses.Reset();
	AssetSettings->ExcludedAssets.Reset();

	for (const auto& Folder : SavedExcludedFolders) {
		AssetSettings->ExcludedFolders.Add(FDirectoryPath {Folder});
	}
	for (const auto& Class : SavedExcludedClasses) {
		AssetSettings->ExcludedClasses.Add(TSoftClassPtr<UObject> {Class});
	}
	for (const auto& Asset : SavedExcludedAssets) {
		AssetSettings->ExcludedAssets.Add(TSoftObjectPtr<UObject> {Asset});
	}
	FileSettings->ExcludedExtensions = SavedExcludedExtensions;

	SavedExcludedFolders.Reset();
	SavedExcludedClasses.Reset();
	SavedExcludedAssets.Reset();
	SavedExcludedExtensions.Reset();
	bSettingsSaved = false;
}

void FPjcTestFixture::Keep() {
	bKeep = true;
}

void FPjcTestFixture::Cleanup() {
	if (bKeep || !IsTestHost()) return;

	RestoreSettings();

	// before deleting assets: UE4 rescans folders on delete and logs errors for corrupted packages
	IFileManager::Get().Delete(*CorruptedFile(), false, true, true);

	TArray<UObject*> Objects;
	for (const auto& Pair : PackageNames) {
		UObject* Object = FSoftObjectPath {ObjectPath(Pair.Key)}.ResolveObject();
		if (IsValid(Object)) {
			Objects.Add(Object);
		}
	}

	if (Objects.Num() > 0) {
		ObjectTools::ForceDeleteObjects(Objects, false);
	}

	// leftovers that were not loaded (kept runs) and non asset files
	IFileManager::Get().DeleteDirectory(*(ContentDir() / TEXT("PjcTests")), false, true);
	IFileManager::Get().Delete(*(PluginContentDir() / TEXT("C_PluginUsed.uasset")), false, true, true);
	IFileManager::Get().Delete(*(PluginContentDir() / TEXT("C_PluginUnused.uasset")), false, true, true);
	IFileManager::Get().Delete(*(PluginContentDir() / TEXT("OL_PluginLib.uasset")), false, true, true);
	IFileManager::Get().Delete(*IndirectConfigFile(), false, true, true);

	AssetRegistry().ScanPathsSynchronous({RootPath(), PluginRootPath()}, true);
	AssetRegistry().RemovePath(RootPath());

	PackageNames.Reset();
	Packages.Reset();
	bBuilt = false;
}

FString FPjcTestFixture::ObjectPath(const FString& Name) const {
	const FString* PackageName = PackageNames.Find(Name);
	if (!PackageName) return {};

	return FString::Printf(TEXT("%s.%s"), **PackageName, *Name);
}

FString FPjcTestFixture::PackageFile(const FString& Name) const {
	const FString* PackageName = PackageNames.Find(Name);
	if (!PackageName) return {};

	return FPaths::ConvertRelativePathToFull(FPackageName::LongPackageNameToFilename(*PackageName, FPackageName::GetAssetPackageExtension()));
}

TArray<FString> FPjcTestFixture::UsedNames(const bool bScanPlugins) const {
	TArray<FString> Names {
	  TEXT("PAL_Root"),
	  TEXT("OL_Chain"),
	  TEXT("C_ChainLeaf"),
	  TEXT("C_Indirect"),
	  TEXT("C_IndirectPackage"),
	  TEXT("C_ByFolder"),
	  TEXT("CLC_ByClass"),
	  TEXT("C_ByAsset"),
	  TEXT("LS_Cinematic"),
	  TEXT("EUB_Tool"),
	  TEXT("BP_Primary"),
	};

	if (bScanPlugins) {
		Names.Add(TEXT("C_PluginUsed"));
	}
	else {
		// referencer is outside scanned roots
		Names.Add(TEXT("C_RefByPlugin"));
	}

	return Names;
}

TArray<FString> FPjcTestFixture::UnusedNames(const bool bScanPlugins) const {
	TArray<FString> Names {
	  TEXT("C_Unused"),
	  TEXT("OL_UnusedRoot"),
	  TEXT("C_UnusedLeaf"),
	  TEXT("OL_CircA"),
	  TEXT("OL_CircB"),
	  TEXT("BP_Unused"),
	};

	for (int32 i = 0; i < NumBulkAssets; ++i) {
		Names.Add(FString::Printf(TEXT("C_Bulk_%02d"), i));
	}

	if (bScanPlugins) {
		Names.Add(TEXT("C_PluginUnused"));
		Names.Add(TEXT("OL_PluginLib"));
		Names.Add(TEXT("C_RefByPlugin"));
	}

	return Names;
}

TArray<FString> FPjcTestFixture::PrimaryNames() const {
	return {TEXT("PAL_Root"), TEXT("BP_Primary")};
}

TArray<FString> FPjcTestFixture::ExcludedNames() const {
	return {TEXT("C_ByFolder"), TEXT("CLC_ByClass"), TEXT("C_ByAsset"), TEXT("LS_Cinematic")};
}

TArray<FString> FPjcTestFixture::EmptyFolders() const {
	const FString Root = ContentDir() / TEXT("PjcTests");
	return {Root / TEXT("Empty"), Root / TEXT("Empty/A"), Root / TEXT("Empty/A/B")};
}

FString FPjcTestFixture::ExternalFile() const {
	return ContentDir() / TEXT("PjcTests/Files/readme.txt");
}

FString FPjcTestFixture::ExternalExcludedFile() const {
	return ContentDir() / TEXT("PjcTests/Files/notes.md");
}

FString FPjcTestFixture::CorruptedFile() const {
	return ContentDir() / TEXT("PjcTests/Files/Broken.uasset");
}

#endif
