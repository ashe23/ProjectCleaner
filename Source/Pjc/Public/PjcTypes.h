// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "Engine/EngineTypes.h"
#include "Layout/Margin.h"
#include "PjcTypes.generated.h"

/**
 * Assets cleanup always keeps. Shared with the team: saved to DefaultEditor.ini (moved from per user settings in 3.1,
 * see UPjcSubsystem::MoveExclusionsToProject).
 */
UCLASS(Config = Editor, DefaultConfig)
class UPjcAssetExcludeSettings : public UObject
{
	GENERATED_BODY()

  public:

	UPROPERTY(
		BlueprintReadWrite,
		EditAnywhere,
		Config,
		Category = "AssetExcludeSettings",
		meta = (ContentDir, ToolTip = "Assets in these folders count as used")
	)
	TArray<FDirectoryPath> ExcludedFolders;

	UPROPERTY(
		BlueprintReadWrite,
		EditAnywhere,
		Config,
		Category = "AssetExcludeSettings",
		meta = (AllowAbstract = "true", ToolTip = "Assets of these classes and their subclasses count as used")
	)
	TArray<TSoftClassPtr<UObject>> ExcludedClasses;

	UPROPERTY(Config)
	TArray<TSoftObjectPtr<UObject>> ExcludedAssets;

	// Assets to review the user marked unused (Issues): the scan does not keep them for review, so they are unused
	// unless a used asset references them
	UPROPERTY(Config)
	TArray<TSoftObjectPtr<UObject>> MarkedUnusedAssets;

  protected:

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

// Files in content folders that are not assets and are kept: shared with the team in Config/DefaultEditor.ini
UCLASS(Config = Editor, DefaultConfig)
class UPjcFileExcludeSettings : public UObject
{
	GENERATED_BODY()

  public:

	UPROPERTY(
		BlueprintReadWrite,
		EditAnywhere,
		Config,
		Category = "FileExcludeSettings",
		meta = (RelativeToGameDir, ToolTip = "Excludes these files from the scan")
	)
	TArray<FFilePath> ExcludedFiles;

	UPROPERTY(
		BlueprintReadWrite,
		EditAnywhere,
		Config,
		Category = "FileExcludeSettings",
		meta = (ToolTip = "Excludes files with these extensions from the scan")
	)
	TArray<FString> ExcludedExtensions;

	UPROPERTY(
		BlueprintReadWrite,
		EditAnywhere,
		Config,
		Category = "FileExcludeSettings",
		meta = (ContentDir, ToolTip = "Excludes all files in these folders from the scan")
	)
	TArray<FDirectoryPath> ExcludedFolders;

  protected:

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

USTRUCT(BlueprintType)
struct FPjcFileInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "FileInfo")
	int32 FileNum = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "FileInfo")
	FString FilePath;

	bool operator==(const FPjcFileInfo& Other) const {
		return FilePath.Equals(Other.FilePath) && FileNum == Other.FileNum;
	}

	bool operator!=(const FPjcFileInfo& Other) const {
		return !(FilePath.Equals(Other.FilePath) && FileNum == Other.FileNum);
	}
};

USTRUCT(BlueprintType)
struct FPjcAssetIndirectInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "AssetIndirectInfo")
	FAssetData Asset;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "AssetIndirectInfo")
	FString FilePath;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "AssetIndirectInfo")
	int32 FileNum = 0;

	// a file only the editor reads: a source file of an editor module, a Python script, an editor config file
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "AssetIndirectInfo")
	bool bEditorOnly = false;

	bool operator==(const FPjcAssetIndirectInfo& Other) const {
		return Asset == Other.Asset && FilePath.Equals(Other.FilePath) && FileNum == Other.FileNum;
	}

	bool operator!=(const FPjcAssetIndirectInfo& Other) const {
		return !(Asset == Other.Asset && FilePath.Equals(Other.FilePath) && FileNum == Other.FileNum);
	}
};

// Why an asset counts as used, see UPjcSubsystem::GetAssetsUsage
UENUM(BlueprintType)
enum class EPjcUsageReason : uint8
{
	// nothing used references it
	None,
	// primary asset: every map, and every asset the Asset Manager registered (labels, types from project settings or game features)
	Primary,
	// asset of a primary asset class that the Asset Manager did not register (outside the type's folders, or game feature not registered yet)
	PrimaryClass,
	// referenced from outside scanned content: engine, not scanned plugins, actors saved in their own files
	EngineReferenced,
	// named in project or plugin Source/Config files
	Indirect,
	// exclude settings or always used classes
	Excluded,
	// editor utility blueprint or widget
	Editor,
	// GameplayCue notify a used asset, source or config file fires by its tag or a child tag (Detail: tag and by what)
	GameplayCue,
	// localized copy (L10N/<culture> folder) of a used asset, loaded in its place for that culture (Parent: the source)
	Localized,
	// in a folder the cook always takes whole: Additional Asset Directories to Cook, Data Registry scan folders
	ConfigFolder,
	// in the project's MSPresets folder: presets the Megascans plugin loads by path when it imports. Editor only
	Megascans,
	// listed explicitly by a Primary Asset Label
	FolderLabelAsset,
	// referenced by another used asset (Parent)
	Dependency,
	// in folder of a Primary Asset Label with "Label Assets In My Directory" whose cook rule packages it (Parent)
	FolderLabel,
	// Review: nothing the scan sees uses it, but the last cook packaged it (Detail: the platform)
	CookPackaged,
	// Review: GameplayCue notify whose tag nothing used, no source and no config file names, or outside the cue notify paths
	GameplayCueNotFired,
	// Level Sequence, Template Sequence, Movie Render Queue or Movie Render Graph config while Cinematics is on
	Cinematic,
	// in the project's Splash folder: texture copies of the splash images the editor imports, and imports again when
	// deleted (the engine shows the image files). Editor only
	Splash,
	// named only in files the editor reads: editor modules' source, Python scripts, editor config. Editor only
	EditorSource,
	// Review: only engine content references it (the engine, or a plugin in the engine folder), and nothing used in the
	// project uses that engine content (Detail: the engine package)
	EngineContentOnly,
	// Review: in a folder that C++ or Python code names as a whole string ("/Game/UI/Icons"), and nothing else uses it.
	// Code can add the asset name at runtime (Detail: file and line)
	FolderInCode,
};

// What fixing redirectors did
struct FPjcRedirectorFixResult
{
	// Assets that used a redirector and were resaved
	int32 NumResaved = 0;
	// Redirector packages deleted
	TArray<FName> Deleted;
	// Assets that use a redirector and could not be resaved, and why
	TMap<FName, FString> NotResaved;
	// Redirector packages that stay, and why
	TMap<FName, FString> Kept;
	// What to do about each redirector that stays
	TMap<FName, FString> KeptHints;
	// Of those, the ones revision control keeps: the redirector or an asset that uses it is checked out by someone else
	// or not at the latest revision. A fix again works only after that changes
	TSet<FName> KeptRevisionControl;
	// Redirectors whose old path now resolves through a redirect in Config/DefaultEngine.ini
	int32 NumRedirects = 0;
	// Folders the fix left empty and deleted
	int32 NumFoldersDeleted = 0;
};

// How redirectors are fixed
struct FPjcRedirectorFixOptions
{
	// Keep the old paths that C++, Python or config files name working: add redirects to Config/DefaultEngine.ini
	// ([CoreRedirects]), then fix those redirectors too. Off: they stay
	bool bAddRedirects = false;
	// Delete the folders the fix leaves empty
	bool bDeleteEmptyFolders = false;
	// Keep redirectors files name or content outside the scan uses. Only Clean project skips it: the redirectors it removes point
	// to deleted assets, nothing can need them
	bool bCheckNeeded = true;
	// Redirectors whose target is missing: the assets that use them are resaved with the reference empty (it already loads
	// empty), then the redirectors go. Off: they stay with their users
	bool bClearBroken = false;
};

USTRUCT(BlueprintType)
struct FPjcAssetUsage
{
	GENERATED_BODY()

	// False for unused assets. Assets to review are kept by cleanup: used, with a review reason starting their chain
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "AssetUsage")
	bool bUsed = false;

	// Only a folder label keeps it, directly or through assets that only a folder label keeps
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "AssetUsage")
	bool bFolderLabelOnly = false;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "AssetUsage")
	EPjcUsageReason Reason = EPjcUsageReason::None;

	// Package that pulled it in (Dependency, FolderLabel, FolderLabelAsset). Following parents leads to the asset that is used for its own reason
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "AssetUsage")
	FName Parent;

	// Reason specifics: primary asset id, file and line, exclude rule, referencing package outside scanned content
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "AssetUsage")
	FString Detail;
};
