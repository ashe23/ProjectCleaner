// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PjcSettings.generated.h"

UENUM()
enum class EPjcToolbarLocation : uint8
{
	Hidden UMETA(DisplayName = "Hidden"),
	PlayToolbar UMETA(DisplayName = "Play Toolbar", ToolTip = "Next to Play buttons (UE4: main level editor toolbar)"),
	AssetsToolbar UMETA(DisplayName = "Assets Toolbar", ToolTip = "Next to Content and Blueprint buttons (UE5 only)"),
	UserToolbar UMETA(DisplayName = "User Toolbar", ToolTip = "Right side of toolbar (UE5 only)"),
};

/**
 * Project wide scanning rules. Saved to DefaultEditor.ini, so whole team shares them.
 * Project Settings > Plugins > Project Cleaner
 */
UCLASS(Config = Editor, DefaultConfig, meta = (DisplayName = "Project Cleaner"))
class UPjcScanSettings final : public UDeveloperSettings
{
	GENERATED_BODY()

  public:

	UPjcScanSettings();

	virtual FName GetCategoryName() const override;
#if WITH_EDITOR
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;
#endif

	/**
	 * Scan content of project plugins too, not only /Game.
	 * Needed for plugin based projects (e.g. Lyra GameFeatures). Plugin assets used only by unused assets become unused.
	 * When off, references from project plugins still keep /Game assets used.
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Scan Roots")
	bool bScanProjectPlugins = false;

	/** Project plugins that are never scanned (plugin names). Their assets are never deleted. */
	UPROPERTY(EditAnywhere, Config, Category = "Scan Roots", meta = (EditCondition = "bScanProjectPlugins"))
	TArray<FString> IgnoredPlugins;

	/** Assets of primary asset types (Project Settings > Asset Manager) and their derived classes are used. */
	UPROPERTY(EditAnywhere, Config, Category = "Used Assets")
	bool bPrimaryAssetsUsed = true;

	/**
	 * Primary asset types that do not make assets used.
	 * Use it when a project registers a type with very broad base class and everything becomes used.
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Used Assets", meta = (EditCondition = "bPrimaryAssetsUsed"))
	TArray<FName> IgnoredPrimaryAssetTypes;

	/**
	 * Primary Asset Labels with "Label Assets In My Directory" make every asset in their folder used.
	 * Such labels are cook rules, often added for content loaded by path, so by default their assets stay used.
	 * A label in content root (e.g. Lyra DefaultGame_Label) makes whole project used: turn off to review those assets.
	 * Label itself and its explicit assets, blueprints and collection are always used.
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Used Assets", meta = (EditCondition = "bPrimaryAssetsUsed"))
	bool bFolderLabelsUsed = true;

	/**
	 * Cinematic assets are used: Level Sequences, Template Sequences, Movie Render Queue and Movie Render Graph configs.
	 * They are usually rendered or played without being referenced by any level.
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Used Assets")
	bool bCinematicAssetsUsed = true;

	/** Editor utility assets (Editor Utility Widgets and Blueprints) are used. */
	UPROPERTY(EditAnywhere, Config, Category = "Used Assets")
	bool bEditorAssetsUsed = true;

	/** Assets mentioned in project source code (.h, .cpp, .cs) or config (.ini) files are used. */
	UPROPERTY(EditAnywhere, Config, Category = "Used Assets")
	bool bIndirectAssetsUsed = true;

	/** Assets of these classes (and derived classes) are always used. */
	UPROPERTY(EditAnywhere, Config, Category = "Used Assets", meta = (AllowAbstract = "true"))
	TArray<TSoftClassPtr<UObject>> UsedClasses;

	/** Classes that are cinematic assets, resolved for current engine version */
	static TArray<FString> GetCinematicClassPaths();
};

/**
 * Per user editor preferences.
 * Editor Preferences > Plugins > Project Cleaner
 */
UCLASS(Config = EditorPerProjectUserSettings, meta = (DisplayName = "Project Cleaner"))
class UPjcEditorSettings final : public UDeveloperSettings
{
	GENERATED_BODY()

  public:

	virtual FName GetContainerName() const override;
	virtual FName GetCategoryName() const override;
#if WITH_EDITOR
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** Where Project Cleaner button is shown in level editor toolbar. Window menu entry is always available. */
	UPROPERTY(EditAnywhere, Config, Category = "Interface")
	EPjcToolbarLocation ToolbarLocation = EPjcToolbarLocation::PlayToolbar;

	/** Scan project when Project Cleaner window opens. When off, press Scan Project manually. */
	UPROPERTY(EditAnywhere, Config, Category = "Scanning")
	bool bScanOnOpen = true;

	/**
	 * Ask before Fix Redirectors runs. Fixing resaves (and checks out from source control) every asset
	 * that references a redirector. Scanning does not need fixed redirectors.
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Scanning")
	bool bConfirmRedirectorsFix = true;

	/**
	 * Skip engine "About to load N assets" question while Project Cleaner deletes assets (UE 5.4+).
	 * Project Cleaner already asks for confirmation before deleting. Always skipped in commandlets.
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Deletion")
	bool bSkipAssetLoadWarning = false;

	DECLARE_MULTICAST_DELEGATE(FPjcDelegateSettingsChanged);
	FPjcDelegateSettingsChanged OnChanged;
};
