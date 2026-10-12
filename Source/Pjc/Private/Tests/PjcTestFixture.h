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
 *   Used/PAL_Root            PrimaryAssetLabel (primary), soft references OL_Chain
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
 */
class FPjcTestFixture
{
  public:

	static constexpr int32 NumBulkAssets = 25;

	static bool IsTestHost();
	static FString RootPath();
	static FString PluginRootPath();
	static FString IndirectConfigFile();

	FPjcTestFixture();
	~FPjcTestFixture();

	/**
	 * Creates all fixture content and applies exclude settings. Adds test errors on failure.
	 * @param bCreateCorruptedFile false when content is kept for another process: editor asset discovery
	 * logs errors for corrupted packages a frame later, CLI tests create that file themselves.
	 */
	bool Build(FAutomationTestBase& Test, const bool bApplyExcludeSettings = true, const bool bCreateCorruptedFile = true);

	/** Removes all fixture content and restores settings. Called on destruction too. */
	void Cleanup();

	/** Prevent cleanup, so content stays on disk (used by CLI tests). */
	void Keep();

	FString ObjectPath(const FString& Name) const;
	FString PackageFile(const FString& Name) const;

	TArray<FString> UsedNames(const bool bScanPlugins) const;
	TArray<FString> UnusedNames(const bool bScanPlugins) const;
	TArray<FString> PrimaryNames() const;
	TArray<FString> ExcludedNames() const;
	TArray<FString> EmptyFolders() const;
	FString ExternalFile() const;
	FString ExternalExcludedFile() const;
	FString CorruptedFile() const;

  private:

	bool CreateAssets(FAutomationTestBase& Test);
	void CreateFiles(const bool bCreateCorruptedFile);
	void ApplyExcludeSettings();
	void RestoreSettings();

	TMap<FString, FString> PackageNames;   // short name -> long package name
	TArray<UPackage*> Packages;
	TArray<FString> SavedExcludedFolders;
	TArray<FSoftClassPath> SavedExcludedClasses;
	TArray<FSoftObjectPath> SavedExcludedAssets;
	TArray<FString> SavedExcludedExtensions;
	bool bSettingsSaved = false;
	bool bBuilt = false;
	bool bKeep = false;
};

#endif
