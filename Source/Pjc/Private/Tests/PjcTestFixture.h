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
 * test host projects (Scripts/pjc.py passes -PjcTestHost). Layout (all under /Game/PjcTests):
 *
 *   Used/PAL_Root            PrimaryAssetLabel (primary), soft references OL_Chain (and R_Moved in Lyra layout)
 *   Used/OL_Chain            ObjectLibrary, hard references C_ChainLeaf and /PjcTestContent/C_PluginUsed
 *   Used/C_ChainLeaf         used through dependency chain
 *   Unused/C_Unused          not referenced
 *   Unused/OL_UnusedRoot     not referenced, references C_UnusedLeaf
 *   Unused/C_UnusedLeaf      referenced only by unused asset
 *   Unused/Bulk/C_Bulk_XX    many unused assets, more than engine "load many assets" warning threshold
 *   Circular/OL_CircA,B      reference each other, nothing references them
 *   Indirect/C_Indirect      referenced only from Config/PjcTests.ini (object path)
 *   Indirect/C_IndirectPackage  referenced only from Config/PjcTests.ini (package name)
 *   Cinematics/LS_Cinematic  LevelSequence, not referenced (used by "cinematic assets used" setting)
 *   Misc/C_RefByPlugin       referenced only by /PjcTestContent/OL_PluginLib
 *   Excluded/C_ByFolder      excluded by folder
 *   Misc/CLC_ByClass         excluded by class (CurveLinearColor)
 *   Misc/C_ByAsset           excluded by asset
 *   Editor/EUB_Tool          EditorUtilityBlueprint
 *   Blueprints/BP_Primary    Blueprint derived from PlayerController, primary type "PjcTestPrimary" in host config
 *   Blueprints/BP_Unused     Blueprint derived from Actor, not referenced
 *   Labeled/PAL_Folder       PrimaryAssetLabel with "Label Assets In My Directory"
 *   Labeled/C_Labeled        only referenced by folder label: used unless bFolderLabelsUsed is off
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
	static FString RootPath();
	static FString PluginRootPath();
	static FString FeatureRootPath();
	static FString IndirectConfigFile();
	static FString FeatureConfigFile();

	FPjcTestFixture();
	~FPjcTestFixture();

	/**
	 * Creates all fixture content and applies exclude settings. Adds test errors on failure.
	 * @param bCreateCorruptedFile false when content is kept for another process: editor asset discovery
	 * logs errors for corrupted packages a frame later, CLI tests create that file themselves.
	 */
	bool Build(
		FAutomationTestBase& Test,
		const bool bApplyExcludeSettings = true,
		const bool bCreateCorruptedFile = true,
		const bool bInLyraLayout = false
	);

	/** Removes all fixture content and restores settings. Called on destruction too. */
	void Cleanup();

	/** Prevent cleanup, so content stays on disk (used by CLI tests). */
	void Keep();

	FString ObjectPath(const FString& Name) const;
	FString PackageFile(const FString& Name) const;

	TArray<FString> UsedNames(const bool bScanPlugins, const bool bFolderLabelsUsed = true) const;
	TArray<FString> UnusedNames(const bool bScanPlugins, const bool bFolderLabelsUsed = true) const;
	TArray<FString> PrimaryNames() const;
	TArray<FString> ExcludedNames() const;
	TArray<FString> EmptyFolders() const;
	FString ExternalFile() const;
	FString ExternalExcludedFile() const;
	FString CorruptedFile() const;

	/** Lyra layout files and folders that cleanup must leave untouched */
	TArray<FString> LyraKeptFiles() const;
	/** Project relative paths pjc.py removes after commandlet tests */
	TArray<FString> LyraCleanupPaths() const;

  private:

	bool CreateAssets(FAutomationTestBase& Test);
	bool CreateLyraAssets(FAutomationTestBase& Test);
	void CreateFiles(const bool bCreateCorruptedFile);
	void CreateLyraFiles();
	void ApplyExcludeSettings();
	void RestoreSettings();

	TMap<FString, FString> PackageNames;   // short name -> long package name
	TSet<FString> MapNames;
	TArray<UPackage*> Packages;
	bool bLyraLayout = false;
	TArray<FString> SavedExcludedFolders;
	TArray<FSoftClassPath> SavedExcludedClasses;
	TArray<FSoftObjectPath> SavedExcludedAssets;
	TArray<FString> SavedExcludedExtensions;
	bool bSettingsSaved = false;
	bool bBuilt = false;
	bool bKeep = false;
};

#endif
