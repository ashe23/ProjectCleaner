// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/SoftObjectPath.h"

#define PJC_TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/**
 * Test content with known expected scan results.
 *
 * Creates real assets, folders and files inside the project, so it refuses to run outside of generated
 * test host projects (the dev tool passes -PjcTestHost). Layout (all under /Game/PjcTests):
 *
 *   Used/PAL_Root            PrimaryAssetLabel (primary), soft references OL_Chain (and R_Moved in Lyra layout)
 *   Used/OL_Chain            ObjectLibrary, hard references C_ChainLeaf and /PjcTestContent/C_PluginUsed
 *   Used/C_ChainLeaf         used through dependency chain
 *   Used/C_Renamed_1         holds asset C_Renamed (named unlike its file), referenced by OL_Chain: used, not corrupted
 *   Unused/C_Unused          not referenced
 *   Unused/OL_UnusedRoot     not referenced, references C_UnusedLeaf
 *   Unused/C_UnusedLeaf      referenced only by unused asset
 *   Unused/C_RenamedUnused_1 holds asset C_RenamedUnused, not referenced: unused, not corrupted
 *   Unused/Bulk/C_Bulk_XX    many unused assets, more than engine "load many assets" warning threshold
 *   Circular/OL_CircA,B      reference each other, nothing references them
 *   Indirect/C_Indirect      referenced only from Config/PjcTests.ini (object path)
 *   Indirect/C_IndirectPackage  referenced only from Config/PjcTests.ini (package name)
 *   Indirect/C_InPython      named only in Content/Python/PjcTests.py (editor Python script)
 *   Indirect/C_InPlatformConfig  named only in Platforms/PjcPlatform/Config/PjcPlatformTests.ini
 *   Code/...                 named in code (CreateFiles): the host game module (Source/<Project>/PjcTestCode.cpp), a
 *                            project plugin (Plugins/PjcTestGameCode: Source, Config) and the Python script
 *     C_CodeFull             full object path in FObjectFinder: used
 *     BP_CodeClass           package path without _C in FClassFinder: used
 *     BP_CodeDoor            class path without its folders ("BP_CodeDoor.BP_CodeDoor_C"): used
 *     C_CodeMacro            TEXT(MACRO "/C_CodeMacro.C_CodeMacro"), the macro names the folder: used
 *     C_CodeMacroOther       in that folder, nothing names it: Review (folder in code)
 *     C_CodeSplit            folder and name in two literals on two lines: used
 *     C_CodeSplitOther       in that folder ("/Game/.../Split/"): Review
 *     C_CodeBuilt            folder in FPaths::Combine, the name built at runtime: Review
 *     C_CodeBuiltSub         in a subfolder of it: unused
 *     C_CodeFormat           folder in a format string ("/Game/.../Format/%s.%s"): Review
 *     C_CodeCase             full path in lower case: used
 *     C_CodeTwin, C_CodeTwinB  two assets named C_CodeTwin, code names "C_CodeTwin.C_CodeTwin": both used
 *     C_CodePython           f"{folder}/C_CodePython.C_CodePython" in the Python script: editor only
 *     C_PluginMacro          macro case in the plugin source (like CitySample HoverDrone): used
 *     C_PluginMacroOther     in that folder: Review
 *     C_PluginConfig         full path in the plugin's Config/DefaultPjcTestGameCode.ini: used
 *     C_CodeWord             its name only as a plain word: unused
 *     C_CodeLonger           code names a path inside its folder that does not exist: unused
 *   AlwaysCook/C_AlwaysCooked  in Additional Asset Directories to Cook (host config): used
 *   Registry/C_RegistryData    in Data Registry Directories to Scan (host config): used
 *   /Game/L10N/fr/PjcTests/Indirect/C_Indirect  localized copy of used C_Indirect (key C_Indirect_fr): used
 *   /Game/L10N/fr/PjcTests/Unused/C_Unused      localized copy of unused C_Unused (key C_Unused_fr): unused
 *   Cinematics/LS_Cinematic  LevelSequence, not referenced (used by "cinematic assets used" setting)
 *   Misc/C_RefByPlugin       referenced only by /PjcTestContent/OL_PluginLib
 *   Excluded/C_ByFolder      excluded by folder
 *   Misc/CLC_ByClass         excluded by class (CurveLinearColor)
 *   Misc/C_ByAsset           excluded by asset
 *   Editor/EUB_Tool          EditorUtilityBlueprint
 *   Editor/BP_Validator      Blueprint derived from EditorValidatorBase (only when DataValidation is loaded): editor asset
 *   Blueprints/BP_Primary    Blueprint derived from PlayerController, primary type "PjcTestPrimary" in host config
 *   Blueprints/BP_Unused     Blueprint derived from Actor, not referenced
 *   Misc/BP_PrimaryOutside   PlayerController Blueprint outside the PjcTestPrimary folder: primary class, not registered
 *   Used/PAL_Collection      PrimaryAssetLabel naming local collection PjcTestCollection
 *   Misc/C_InCollection      only in collection PjcTestCollection: used through PAL_Collection
 *   Labeled/PAL_Folder       PrimaryAssetLabel with "Label Assets In My Directory", cook rule Unknown
 *   Labeled/C_Labeled        only referenced by that folder label: unused, its cook rule does not package it
 *   Packaged/PAL_Packaged    PrimaryAssetLabel with "Label Assets In My Directory", cook rule Always Cook (label type
 *                            is not editor only in host config)
 *   Packaged/C_Packaged      only in that label's folder: used, the cook packages it
 *   Cues/GC_PjcTest_*        GameplayCue notifies (only when GameplayAbilities is loaded), tag from the name, cue path in
 *                            host config: Fired (fired by BP_CueUser), Parent (by its child tag), Native (named in config),
 *                            Unfired (tag only defined) and ByUnused (fired only by BP_CueUnused) are Review
 *   Misc/GC_PjcTest_Outside  notify outside the cue notify path, fired by BP_CueUser: Review
 *   Blueprints/BP_CueUser    references cue tags, used through PAL_Root
 *   Blueprints/BP_CueUnused  references a cue tag, unused
 *   Empty/A/B                empty folders
 *   Files/readme.txt         external file
 *   Files/notes.md           external file excluded by extension
 *   Files/Broken.uasset      corrupted asset file
 *
 *   /PjcTestContent/C_PluginUnused   unused asset inside project plugin
 *   /PjcTestContent/C_PluginUsed     plugin asset used by /Game content
 *   /PjcTestContent/OL_PluginLib     plugin asset, references /Game content
 *
 * Expected results depend on plugin scanning (UPjcScanSettings::bScanProjectPlugins):
 * off - plugin assets are not scanned, C_RefByPlugin is used because referencer is outside scanned roots
 * on  - plugin assets are scanned, OL_PluginLib and C_RefByPlugin are unused
 *
 * Lyra like layout (bLyraLayout, commandlet tests only: game feature data is read when the editor starts):
 *
 *   /PjcFeature                          game feature plugin in Plugins/GameFeatures (like Lyra ShooterCore)
 *   /PjcFeature/PjcFeature               GameFeatureData, registers primary type PjcFeatureExperience (CurveVector)
 *   /PjcFeature/Experiences/CV_Experience  primary only through type registered by GameFeatureData
 *   /PjcFeature/Unused/C_FeatureUnused   unused
 *   /PjcFeature/Indirect/C_FeatureIndirect  named in plugin config file
 *   /PjcFeature/Maps/L_Feature           one file per actor level, its actor is instance of BP_Placed
 *   /Game/PjcTests/Lyra/BP_Placed        referenced only by external actor package of L_Feature
 *   Content/Localization/PjcTests/...    localization data (engine folder), not external file
 *   Content/PjcTests/Staged/license.txt  staged as raw file (packaging settings), not external file
 *   Content/PjcTests/Files/T_RawIcon.svg named in config, loaded by file name, not external file
 *   Plugins/PjcDisabled/Content/...      plugin that is not enabled (like Lyra GreenRoom), never touched
 *   /Game/PjcTests/Redirectors/R_Moved     redirector to C_Target, used through PAL_Root (C_Target used through it)
 *   /Game/PjcTests/Redirectors/R_Stale     redirector to used C_ChainLeaf, unused: deleting it must keep C_ChainLeaf
 *   /Game/PjcTests/Redirectors/R_Dangling  redirector to asset that does not exist (like Lyra 5.8), unused
 * Redirectors are part of this layout because engine expects them loaded from disk when deleting them.
 */
class FPjcTestFixture
{
  public:

	static constexpr int32 NumBulkAssets = 25;

	static bool IsTestHost();
	// Removes a test's own content folder (long package path, e.g. /Game/PjcSaveFirst). UE 4.27 on Linux: files only,
	// the empty folder stays (its directory watcher logs an error when a watched folder goes; the dev tool removes
	// test folders before the next fixture)
	static void DeleteTestFolder(const FString& LongPackagePath);
	// Coverage for the dev tool's test run: a feature case that ran, or why it was skipped (the feature is not in this
	// engine or project). The table lists every case by engine.
	static void Case(FAutomationTestBase& Test, const TCHAR* Name, const FString& SkippedWhy = FString {});
	static FString RootPath();
	static FString LocalizedRootPath();
	// Name of the local collection PAL_Collection names
	static FName CollectionName();
	// EditorValidatorBase, null when DataValidation is not loaded
	static UClass* ValidatorBaseClass();
	// GameplayCueNotify_Static, null when GameplayAbilities is not loaded
	static UClass* CueNotifyClass();
	static FString PluginRootPath();
	static FString FeatureRootPath();
	// asset in the project's Splash folder (outside the fixture root)
	static FString SplashPackage();
	// asset in the project's MSPresets folder (outside the fixture root)
	static FString PresetPackage();
	// asset at a path sample content of the engine names (outside the fixture root): CR_Mannequin_Body
	static FString EngineNamedPackage();
	// engine content of this engine references EngineNamedPackage
	static bool HasEngineNamed();
	static FString IndirectConfigFile();
	static FString PythonFile();
	static FString PlatformConfigFile();
	// an editor module of a plugin nothing enables (no build compiles it), and an editor config file
	static FString EditorCodePluginDir();
	static FString EditorConfigFile();
	static FString FeatureConfigFile();
	// a source file of the host game module, kept out of builds with #if 0
	static FString GameCodeFile();
	// a project plugin with a game module nothing enables: source and config file
	static FString GameCodePluginDir();

	FPjcTestFixture();
	~FPjcTestFixture();

	// Creates all fixture content and applies exclude settings. Adds test errors on failure.
	// @param bCreateCorruptedFile false when content is kept for another process: editor asset discovery
	// logs errors for corrupted packages a frame later, CLI tests create that file themselves.
	bool Build(
		FAutomationTestBase& Test,
		const bool bApplyExcludeSettings = true,
		const bool bCreateCorruptedFile = true,
		const bool bInLyraLayout = false
	);

	// Removes all fixture content and restores settings. Called on destruction too.
	void Cleanup();

	// Prevent cleanup, so content stays on disk (used by CLI tests).
	void Keep();

	FString ObjectPath(const FString& Name) const;
	FString PackageFile(const FString& Name) const;

	TArray<FString> UsedNames(const bool bScanPlugins) const;
	TArray<FString> UnusedNames(const bool bScanPlugins) const;
	// redirectors are in neither list above, cleanups leave them (R_ToUnused goes with its unused target)
	TArray<FString> RedirectorNames() const;
	// in UsedNames too: cleanup keeps them
	TArray<FString> ReviewNames() const;
	// GameplayCue cases were made (GameplayAbilities loaded)
	bool HasCues() const {
		return bCues;
	}
	TArray<FString> PrimaryNames() const;
	TArray<FString> ExcludedNames() const;
	TArray<FString> EmptyFolders() const;
	FString ExternalFile() const;
	FString ExternalExcludedFile() const;
	FString CorruptedFile() const;

	// Lyra layout files and folders that cleanup must leave untouched
	TArray<FString> LyraKeptFiles() const;
	// Project relative paths the dev tool removes after commandlet tests
	TArray<FString> LyraCleanupPaths() const;

  private:

	bool CreateAssets(FAutomationTestBase& Test);
	bool CreateLyraAssets(FAutomationTestBase& Test);
	// Open world level L_Open in the game feature plugin: what its actors and data layers use. Each part only where its
	// feature exists (World Partition, data layer assets, PCG, MetaSound), reported as feature cases
	UWorld* CreateOpenWorld(FAutomationTestBase& Test, TArray<UPackage*>& OutPackages);
	void CreateFiles(const bool bCreateCorruptedFile);
	void CreateLyraFiles();
	void ApplyExcludeSettings();
	static bool CreateCollection(const FString& ObjectPath);
	static void DestroyCollection();
	static void RefreshAssetManager();
	void RestoreSettings();

	TMap<FString, FString> PackageNames;   // short name -> long package name
	TMap<FString, FString> ObjectNames;	  // short name -> asset name, when the asset is not named like its package
	TSet<FString> MapNames;
	TArray<UPackage*> Packages;
	bool bLyraLayout = false;
	bool bCues = false;
	// made by CreateOpenWorld, used and unused when scanning plugins
	TArray<FString> OpenUsed;
	TArray<FString> OpenUnused;
	TArray<FString> SavedExcludedFolders;
	TArray<FSoftClassPath> SavedExcludedClasses;
	TArray<FSoftObjectPath> SavedExcludedAssets;
	TArray<FString> SavedExcludedExtensions;
	bool bSettingsSaved = false;
	bool bBuilt = false;
	bool bKeep = false;
};

#endif
