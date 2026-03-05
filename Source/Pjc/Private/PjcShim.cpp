// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcShim.h"
#include "PjcConstants.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/MultiBox/MultiBox.h"

namespace PjcShim
{
	const ISlateStyle& GetStyle() {
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		return FAppStyle::Get();
#else
		return FEditorStyle::Get();
#endif
	}

	const FSlateBrush* GetBrush(const FName PropertyName, const ANSICHAR* Specifier) {
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		return FAppStyle::GetBrush(PropertyName, Specifier);
#else
		return FEditorStyle::GetBrush(PropertyName, Specifier);
#endif
	}

	FSlateColor GetSlateColor(const FName PropertyName, const ANSICHAR* Specifier) {
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		return FAppStyle::GetSlateColor(PropertyName, Specifier);
#else
		return FEditorStyle::GetSlateColor(PropertyName, Specifier);
#endif
	}

	FName GetLevelEditorToolBarName() {
#if ENGINE_MAJOR_VERSION == 5
		return TEXT("LevelEditor.LevelEditorToolBar.PlayToolBar");
#else
		return TEXT("LevelEditor.LevelEditorToolBar");
#endif
	}

	TSet<FName> GetCachedEmptyPackages() {
		const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistry);

#if ENGINE_MAJOR_VERSION == 5
		return AssetRegistryModule.Get().GetCachedEmptyPackagesCopy();
#else
		return AssetRegistryModule.Get().GetCachedEmptyPackages();
#endif
	}

	FAssetPackageData GetAssetPackageData(const FName PackageName) {
		const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistry);

#if ENGINE_MAJOR_VERSION == 5
		return AssetRegistryModule.Get().GetAssetPackageDataCopy(PackageName).GetValue();
#else
		const FAssetPackageData* Data = AssetRegistryModule.Get().GetAssetPackageData(PackageName);
		return Data ? *Data : FAssetPackageData {};
#endif
	}

	FString GetPathExternalActors() {
#if ENGINE_MAJOR_VERSION == 5
		return FString::Printf(TEXT("/Game/%s"), FPackagePath::GetExternalActorsFolderName());
#else
		return {};
#endif
	}

	FString GetPathExternalObjects() {
#if ENGINE_MAJOR_VERSION == 5
		return FString::Printf(TEXT("/Game/%s"), FPackagePath::GetExternalObjectsFolderName());
#else
		return {};
#endif
	}

	void SetTabManagerMenuMultiBox(const TSharedPtr<FTabManager>& InTabManager, const TSharedRef<FMultiBox>& InMultiBox) {
		if (!InTabManager.IsValid()) return;

#if ENGINE_MAJOR_VERSION == 5
		InTabManager->SetMenuMultiBox(InMultiBox, nullptr);
#else
		InTabManager->SetMenuMultiBox(InMultiBox);
#endif
	}
}	// namespace PjcShim
