// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Tests/PjcTestFixture.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "PjcTypes.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveLinearColor.h"
#include "Curves/CurveVector.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "PjcShim.h"
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
#include "UObject/ObjectRedirector.h"

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

	FString FeaturePluginDir() {
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectPluginsDir() / TEXT("GameFeatures/PjcFeature"));
	}

	FString DisabledPluginContentDir() {
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectPluginsDir() / TEXT("PjcDisabled/Content"));
	}

	bool ImportPropertyText(UObject* Object, const TCHAR* PropertyName, const TCHAR* Text) {
		FProperty* Property = Object->GetClass()->FindPropertyByName(PropertyName);
		if (!Property) return false;

		void* Value = Property->ContainerPtrToValuePtr<void>(Object);
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
		return Property->ImportText_Direct(Text, Value, Object, PPF_None) != nullptr;
#else
		return Property->ImportText(Text, Value, PPF_None, Object) != nullptr;
#endif
	}

	UPackage* NewPackage(const FString& PackageName, TArray<UPackage*>& OutPackages) {
		UPackage* Package = CreatePackage(*PackageName);
		OutPackages.Add(Package);
		return Package;
	}

	// game feature data found by class path, so tests need no GameFeatures module dependency
	UObject* NewGameFeatureData(const FString& PackageName, const TCHAR* PrimaryAssetTypes, TArray<UPackage*>& OutPackages) {
		UClass* DataClass = FSoftClassPath {TEXT("/Script/GameFeatures.GameFeatureData")}.ResolveClass();
		if (!DataClass) return nullptr;

		UPackage* Package = NewPackage(PackageName, OutPackages);
		UObject* Data
			= NewObject<UObject>(Package, DataClass, FName {*FPackageName::GetLongPackageAssetName(PackageName)}, RF_Public | RF_Standalone);
		if (!ImportPropertyText(Data, TEXT("PrimaryAssetTypesToScan"), PrimaryAssetTypes)) return nullptr;

		FAssetRegistryModule::AssetCreated(Data);
		Package->MarkPackageDirty();
		return Data;
	}

	// level with one file per actor: actor is saved in its own package under __ExternalActors__
	UWorld* NewWorldWithExternalActor(const FString& PackageName, UClass* ActorClass, TArray<UPackage*>& OutPackages) {
		UPackage* Package = NewPackage(PackageName, OutPackages);
		UWorld* World = UWorld::CreateWorld(EWorldType::Inactive, false, FName {*FPackageName::GetLongPackageAssetName(PackageName)}, Package);
		if (!World) return nullptr;

		World->SetFlags(RF_Public | RF_Standalone);
		World->PersistentLevel->SetUseExternalActors(true);

		// UE4 does not create actor packages by default
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.bCreateActorPackage = true;
		const AActor* Actor = World->SpawnActor(ActorClass, nullptr, nullptr, SpawnParameters);
		UPackage* ActorPackage = Actor ? Actor->GetExternalPackage() : nullptr;
		if (!ActorPackage) return nullptr;

		OutPackages.Add(ActorPackage);
		FAssetRegistryModule::AssetCreated(World);
		Package->MarkPackageDirty();
		ActorPackage->MarkPackageDirty();
		return World;
	}

	// what the engine leaves behind when an asset is renamed or moved
	UObjectRedirector* NewRedirector(const FString& PackageName, UObject* Destination, TArray<UPackage*>& OutPackages) {
		UPackage* Package = NewPackage(PackageName, OutPackages);
		const FName Name {*FPackageName::GetLongPackageAssetName(PackageName)};
		UObjectRedirector* Redirector = NewObject<UObjectRedirector>(Package, Name, RF_Public | RF_Standalone);
		Redirector->DestinationObject = Destination;
		FAssetRegistryModule::AssetCreated(Redirector);
		Package->MarkPackageDirty();
		return Redirector;
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

FString FPjcTestFixture::FeatureRootPath() {
	return TEXT("/PjcFeature");
}

FString FPjcTestFixture::IndirectConfigFile() {
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir() / TEXT("PjcTests.ini"));
}

FString FPjcTestFixture::FeatureConfigFile() {
	return FeaturePluginDir() / TEXT("Config/PjcFeatureTests.ini");
}

FPjcTestFixture::FPjcTestFixture() {}

FPjcTestFixture::~FPjcTestFixture() {
	Cleanup();
}

bool FPjcTestFixture::Build(FAutomationTestBase& Test, const bool bApplyExcludeSettings, const bool bCreateCorruptedFile, const bool bInLyraLayout) {
	if (!IsTestHost()) {
		Test.AddError(TEXT("Fixture tests create and delete assets, run them only in test host projects (-PjcTestHost)"));
		return false;
	}

	// editor asset discovery is async, all checks below need complete registry
	AssetRegistry().SearchAllAssets(true);

	// leftovers from crashed or kept runs
	Cleanup();
	bKeep = false;
	bLyraLayout = bInLyraLayout;

	if (!CreateAssets(Test)) return false;
	if (bLyraLayout && !CreateLyraAssets(Test)) return false;

	CreateFiles(bCreateCorruptedFile);
	if (bLyraLayout) {
		CreateLyraFiles();
	}

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

	// redirectors: used one keeps its target used, unused ones are deleted without touching their targets
	UCurveFloat* Gone = nullptr;
	if (bLyraLayout) {
		UCurveFloat* Target = NewAsset<UCurveFloat>(Register(TEXT("Redirectors"), TEXT("C_Target")), Packages);
		NewRedirector(Register(TEXT("Redirectors"), TEXT("R_Moved")), Target, Packages);
		NewRedirector(Register(TEXT("Redirectors"), TEXT("R_Stale")), ChainLeaf, Packages);
		// target never saved, moved out of project below (like Lyra 5.8 STT_PlayAnimMontage1 pointing to deleted asset)
		Gone = NewObject<UCurveFloat>(CreatePackage(*(RootPath() / TEXT("Redirectors/C_Gone"))), TEXT("C_Gone"), RF_Public);
		NewRedirector(Register(TEXT("Redirectors"), TEXT("R_Dangling")), Gone, Packages);
		Label->ExplicitAssets.Add(TSoftObjectPtr<UObject> {FSoftObjectPath {ObjectPath(TEXT("R_Moved"))}});
	}

	// folder label (cook rule) references everything in its folder
	NewAsset<UCurveFloat>(Register(TEXT("Labeled"), TEXT("C_Labeled")), Packages);
	UPrimaryAssetLabel* FolderLabel = NewAsset<UPrimaryAssetLabel>(Register(TEXT("Labeled"), TEXT("PAL_Folder")), Packages);
	FolderLabel->bLabelAssetsInMyDirectory = true;

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

	// R_Dangling on disk points to /Game/PjcTests/Redirectors/C_Gone, which never exists as asset
	if (Gone) {
		Gone->ClearFlags(RF_Public);
		Gone->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
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

bool FPjcTestFixture::CreateLyraAssets(FAutomationTestBase& Test) {
	auto Register = [&](const FString& PackageName) {
		PackageNames.Add(FPackageName::GetLongPackageAssetName(PackageName), PackageName);
		return PackageName;
	};

	TArray<UPackage*> LyraPackages;

#if ENGINE_MAJOR_VERSION >= 5
	// game feature data registers its own primary type, like ShooterCore registers experiences.
	// UE4 game features (experimental) do not register data saved this way (Plugin_Missing_GameFeatureData)
	const TCHAR* PrimaryAssetTypes
		= TEXT("((PrimaryAssetType=\"PjcFeatureExperience\",AssetBaseClass=\"/Script/Engine.CurveVector\",bHasBlueprintClasses=False,")
			TEXT("bIsEditorOnly=False,Directories=((Path=\"/PjcFeature/Experiences\")),Rules=(Priority=-1,ChunkId=-1,bApplyRecursively=True)))");
	if (!NewGameFeatureData(Register(FeatureRootPath() / TEXT("PjcFeature")), PrimaryAssetTypes, LyraPackages)) {
		Test.AddError(TEXT("Failed to create game feature data, GameFeatures plugin must be enabled in test host"));
		return false;
	}

	NewAsset<UCurveVector>(Register(FeatureRootPath() / TEXT("Experiences/CV_Experience")), LyraPackages);
#endif
	NewAsset<UCurveFloat>(Register(FeatureRootPath() / TEXT("Unused/C_FeatureUnused")), LyraPackages);
	NewAsset<UCurveFloat>(Register(FeatureRootPath() / TEXT("Indirect/C_FeatureIndirect")), LyraPackages);

	const UBlueprint* Placed
		= NewBlueprint(Register(RootPath() / TEXT("Lyra/BP_Placed")), AActor::StaticClass(), UBlueprint::StaticClass(), LyraPackages);
	if (!Placed || !Placed->GeneratedClass) {
		Test.AddError(TEXT("Failed to create fixture blueprint BP_Placed"));
		return false;
	}

	UWorld* World = NewWorldWithExternalActor(Register(FeatureRootPath() / TEXT("Maps/L_Feature")), Placed->GeneratedClass, LyraPackages);
	if (!World) {
		Test.AddError(TEXT("Failed to create fixture level with external actors"));
		return false;
	}
	MapNames.Add(TEXT("L_Feature"));

	const bool bSaved = UEditorLoadingAndSavingUtils::SavePackages(LyraPackages, false);

	// level is not needed in memory anymore, a world left alive would block its deletion and leak on exit
	World->DestroyWorld(false);
	World->RemoveFromRoot();
	Packages.Append(LyraPackages);

	if (!bSaved) {
		Test.AddError(TEXT("Failed to save fixture Lyra layout packages"));
		return false;
	}

	TArray<FString> Files;
	for (const auto* Package : LyraPackages) {
		FString Filename;
		if (PjcShim::DoesPackageExist(Package->GetName(), &Filename)) {
			Files.Add(FPaths::ConvertRelativePathToFull(Filename));
		}
	}
	AssetRegistry().ScanFilesSynchronous(Files, true);

	for (const auto& Name : {TEXT("C_FeatureUnused"), TEXT("C_FeatureIndirect"), TEXT("BP_Placed"), TEXT("L_Feature")}) {
		if (!FPaths::FileExists(PackageFile(Name))) {
			Test.AddError(FString::Printf(TEXT("Fixture file was not saved: %s"), *PackageFile(Name)));
			return false;
		}
	}

	if (!IFileManager::Get().DirectoryExists(*(FeaturePluginDir() / TEXT("Content/__ExternalActors__")))) {
		Test.AddError(TEXT("Fixture level was saved without external actors"));
		return false;
	}

	return true;
}

void FPjcTestFixture::CreateLyraFiles() {
	for (const auto& File : LyraKeptFiles()) {
		if (FPaths::GetExtension(File).IsEmpty()) {
			IFileManager::Get().MakeDirectory(*File, true);
		}
		else if (!FPaths::FileExists(File)) {
			FFileHelper::SaveStringToFile(TEXT("fixture file"), *File);
		}
	}

	// plugin that is not enabled holds a real package file too, nothing may load or delete it
	IFileManager::Get().Copy(*(DisabledPluginContentDir() / TEXT("C_Disabled.uasset")), *PackageFile(TEXT("C_Unused")));

	const FString Config = FString::Printf(TEXT("[PjcFeatureTests]\nIndirectAsset=%s\n"), *ObjectPath(TEXT("C_FeatureIndirect")));
	FFileHelper::SaveStringToFile(Config, *FeatureConfigFile());
}

TArray<FString> FPjcTestFixture::LyraKeptFiles() const {
	const FString Content = ContentDir();
	return {
	  Content / TEXT("Localization/PjcTests/en/PjcTests.locres"),
	  FeaturePluginDir() / TEXT("Content/Localization/PjcFeature/en/PjcFeature.locres"),
	  Content / TEXT("PjcTests/Staged/license.txt"),
	  Content / TEXT("PjcTests/Files/T_RawIcon.svg"),
	  DisabledPluginContentDir() / TEXT("C_Disabled.uasset"),
	  DisabledPluginContentDir() / TEXT("readme.txt"),
	  DisabledPluginContentDir() / TEXT("Empty"),
	};
}

TArray<FString> FPjcTestFixture::LyraCleanupPaths() const {
	return {
	  TEXT("Content/Localization/PjcTests"),
	  TEXT("Plugins/GameFeatures/PjcFeature/Content/*"),
	  TEXT("Plugins/GameFeatures/PjcFeature/Config/PjcFeatureTests.ini"),
	  TEXT("Plugins/PjcDisabled/Content/*"),
	};
}

void FPjcTestFixture::CreateFiles(const bool bCreateCorruptedFile) {
	const FString Root = ContentDir() / TEXT("PjcTests");

	IFileManager::Get().MakeDirectory(*(Root / TEXT("Empty/A/B")), true);
	FFileHelper::SaveStringToFile(TEXT("external file"), *ExternalFile());
	FFileHelper::SaveStringToFile(TEXT("external file excluded by extension"), *ExternalExcludedFile());
	if (bCreateCorruptedFile) {
		FFileHelper::SaveStringToFile(TEXT("this is not an unreal package"), *CorruptedFile());
	}

	FString Config = FString::Printf(
		TEXT("[PjcTests]\nIndirectAsset=%s\nIndirectPackage=%s\n"), *ObjectPath(TEXT("C_Indirect")), **PackageNames.Find(TEXT("C_IndirectPackage"))
	);
	// Lyra layout: raw file loaded by its file name
	if (bLyraLayout) {
		Config += TEXT("RawIcon=PjcTests/Files/T_RawIcon.svg\n");
	}
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
		// worlds are destroyed right after saving, deleting their files is enough
		UObject* Object = FSoftObjectPath {ObjectPath(Pair.Key)}.ResolveObject();
		if (IsValid(Object) && !Object->IsA<UWorld>()) {
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
	IFileManager::Get().Delete(*FeatureConfigFile(), false, true, true);
	IFileManager::Get().DeleteDirectory(*(ContentDir() / TEXT("Localization/PjcTests")), false, true);
	for (const auto& Dir : {FeaturePluginDir() / TEXT("Content"), DisabledPluginContentDir()}) {
		TArray<FString> Children;
		IFileManager::Get().FindFiles(Children, *(Dir / TEXT("*")), true, true);
		for (const auto& Child : Children) {
			IFileManager::Get().DeleteDirectory(*(Dir / Child), false, true);
			IFileManager::Get().Delete(*(Dir / Child), false, true, true);
		}
	}

	AssetRegistry().ScanPathsSynchronous({RootPath(), PluginRootPath(), FeatureRootPath()}, true);
	AssetRegistry().RemovePath(RootPath());

	PackageNames.Reset();
	MapNames.Reset();
	Packages.Reset();
	bBuilt = false;
	bLyraLayout = false;
}

FString FPjcTestFixture::ObjectPath(const FString& Name) const {
	const FString* PackageName = PackageNames.Find(Name);
	if (!PackageName) return {};

	return FString::Printf(TEXT("%s.%s"), **PackageName, *Name);
}

FString FPjcTestFixture::PackageFile(const FString& Name) const {
	const FString* PackageName = PackageNames.Find(Name);
	if (!PackageName) return {};

	const FString& Extension = MapNames.Contains(Name) ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension();
	return FPaths::ConvertRelativePathToFull(FPackageName::LongPackageNameToFilename(*PackageName, Extension));
}

TArray<FString> FPjcTestFixture::UsedNames(const bool bScanPlugins, const bool bFolderLabelsUsed) const {
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
	  TEXT("PAL_Folder"),
	};

	if (bFolderLabelsUsed) {
		Names.Add(TEXT("C_Labeled"));
	}

	if (bScanPlugins) {
		Names.Add(TEXT("C_PluginUsed"));
	}
	else {
		// referencer is outside scanned roots
		Names.Add(TEXT("C_RefByPlugin"));
	}

	if (bLyraLayout) {
		// used redirector and its target, BP_Placed referenced only by external actor package of L_Feature
		Names.Append({TEXT("R_Moved"), TEXT("C_Target"), TEXT("BP_Placed")});
		if (bScanPlugins) {
			Names.Append({TEXT("C_FeatureIndirect"), TEXT("L_Feature")});
#if ENGINE_MAJOR_VERSION >= 5
			Names.Append({TEXT("PjcFeature"), TEXT("CV_Experience")});
#endif
		}
	}

	return Names;
}

TArray<FString> FPjcTestFixture::UnusedNames(const bool bScanPlugins, const bool bFolderLabelsUsed) const {
	TArray<FString> Names {
	  TEXT("C_Unused"),
	  TEXT("OL_UnusedRoot"),
	  TEXT("C_UnusedLeaf"),
	  TEXT("OL_CircA"),
	  TEXT("OL_CircB"),
	  TEXT("BP_Unused"),
	};

	if (!bFolderLabelsUsed) {
		Names.Add(TEXT("C_Labeled"));
	}

	for (int32 i = 0; i < NumBulkAssets; ++i) {
		Names.Add(FString::Printf(TEXT("C_Bulk_%02d"), i));
	}

	if (bScanPlugins) {
		Names.Add(TEXT("C_PluginUnused"));
		Names.Add(TEXT("OL_PluginLib"));
		Names.Add(TEXT("C_RefByPlugin"));
	}

	if (bLyraLayout) {
		Names.Append({TEXT("R_Stale"), TEXT("R_Dangling")});
		if (bScanPlugins) {
			Names.Add(TEXT("C_FeatureUnused"));
		}
	}

	return Names;
}

TArray<FString> FPjcTestFixture::PrimaryNames() const {
	return {TEXT("PAL_Root"), TEXT("BP_Primary"), TEXT("PAL_Folder")};
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
