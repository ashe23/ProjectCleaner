// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "Styling/SlateColor.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Settings/ContentBrowserSettings.h"

class FTabManager;
enum class EPjcToolbarLocation : uint8;
class FMultiBox;
struct FARFilter;
struct FButtonStyle;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
#include "Styling/AppStyle.h"
#else
#include "EditorStyleSet.h"
#endif

// This is compatibility layer for handling different engine api versions
namespace PjcShim
{
	const ISlateStyle& GetStyle();
	const FSlateBrush* GetBrush(const FName PropertyName, const ANSICHAR* Specifier = nullptr);
	FSlateColor GetSlateColor(const FName PropertyName, const ANSICHAR* Specifier = nullptr);
	// editor icons and styles under their name in each engine version
	const FSlateBrush* GetIconBack();
	const FSlateBrush* GetIconForward();
	const FSlateBrush* GetIconClose();
	const FSlateBrush* GetIconLock();
	const FSlateBrush* GetIconSettings();
	/** Background of a card in the Overview */
	const FSlateBrush* GetBrushCard();
	const FButtonStyle& GetPrimaryButtonStyle();

	FName GetLevelEditorToolBarName(const EPjcToolbarLocation Location);
	TSet<FName> GetCachedEmptyPackages();
	FAssetPackageData GetAssetPackageData(const FName PackageName);
	/** Digest of what a package file was saved with (5.4+ saved hash, a new guid per save before), 0 when unknown */
	uint32 GetPackageSavedDigest(const FAssetPackageData& PackageData);
	/** Same for a package of the editor asset registry, 0 when it has none */
	uint32 GetPackageSavedDigest(const FName PackageName);
	FString GetPathExternalActors();
	FString GetPathExternalObjects();
	// one file per actor folder names (World Partition and OFPA levels)
	FString GetExternalActorsFolderName();
	FString GetExternalObjectsFolderName();

	void SetTabManagerMenuMultiBox(const TSharedPtr<FTabManager>& InTabManager, const TSharedRef<FMultiBox>& InMultiBox);

	void AddFilterClass(FARFilter& InFilter, const UClass* InClass);
	void AddFilterObjectPath(FARFilter& InFilter, const FAssetData& InAssetData);
	void ReserveFilterObjectPaths(FARFilter& InFilter, const int32 InNum);
	bool HasFilterObjectPaths(const FARFilter& InFilter);
	FName GetAssetClassName(const FAssetData& InAssetData);
	FName GetClassName(const UClass* InClass);
	FName GetClassNameFromObjectPath(const FString& InClassObjectPath);

	FAssetData GetAssetByObjectPath(const FString& InObjectPath);
	bool DoesPackageExist(const FString& InPackageName, FString* OutFilename);
	/** Waits for async asset compilation (UE5: textures, meshes, animations, ...) and shader jobs, they hold loaded assets */
	void FinishAssetCompilation();
	/**
	 * Drops transient helpers assets keep for themselves while loaded in the editor (UE5 animation data controller).
	 * Helper references asset internals, so engine treats it as outside reference and refuses deleting the asset.
	 * Asset creates it again when needed. Returns true if any was dropped (collect garbage before deleting).
	 */
	bool ReleaseEditorHelpers(const TArray<UObject*>& Assets);
	/**
	 * Before 5.5 deleting a redirector removes its redirection and ensures it was registered; loaded redirectors
	 * are not always registered (4.27). Registers it to TargetObjectPath when missing.
	 */
	void RegisterRedirection(const UObject* Redirector, const FString& TargetObjectPath);
	void GetDerivedClassNames(const TArray<FName>& ClassNames, const TSet<FName>& ExcludedClassNames, TSet<FName>& OutDerivedClassNames);
	void OpenAssetEditor(const FAssetData& InAssetData);
	FString GetObjectPathString(const FAssetData& InAssetData);

	bool IsAssetManagerValid();
	/** Packages of the assets in a collection of any share type, with its child collections */
	void GetPackagesInCollection(const FName CollectionName, TArray<FName>& OutPackages);
	EAppReturnType::Type ShowDialog(const FText& Title, const FText& Message, const EAppMsgType::Type MessageType);

	/** Writes object config to project default config file (e.g. Config/DefaultEditor.ini) */
	void UpdateDefaultConfigFile(UObject* InObject);

	bool LoadAssetsIfNeeded(const TArray<FString>& ObjectPaths, TArray<UObject*>& LoadedObjects, bool bAllowedToPromptToLoad, bool bLoadRedirects);

	// UE5.5 EAllowShrinking Deprecations
	template<typename ArrayType>
	auto PopArray(ArrayType& InArray) {
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
		return InArray.Pop(EAllowShrinking::No);
#else
		return InArray.Pop(false);
#endif
	}

	template<typename ArrayType, typename Predicate>
	void RemoveAllSwapArray(ArrayType& InArray, Predicate Pred) {
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
		InArray.RemoveAllSwap(Pred, EAllowShrinking::No);
#else
		InArray.RemoveAllSwap(Pred, false);
#endif
	}

	inline void LeftInlineString(FString& InString, const int32 Count) {
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
		InString.LeftInline(Count, EAllowShrinking::No);
#else
		InString.LeftInline(Count, false);
#endif
	}

}	// namespace PjcShim
