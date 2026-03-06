// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcShim.h"
#include "PjcConstants.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/ARFilter.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
#include "AssetRegistry/AssetData.h"
#else
#include "AssetData.h"
#endif
#include "Framework/Docking/TabManager.h"
#include "Framework/MultiBox/MultiBox.h"
#include "Engine/AssetManager.h"
#include "Misc/MessageDialog.h"
#include "AssetViewUtils.h"

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

	void AddFilterClass(FARFilter& InFilter, const UClass* InClass) {
		if (!InClass) return;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		InFilter.ClassPaths.Emplace(InClass->GetClassPathName());
#else
		InFilter.ClassNames.Emplace(InClass->GetFName());
#endif
	}

	void AddFilterObjectPath(FARFilter& InFilter, const FAssetData& InAssetData) {
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		InFilter.SoftObjectPaths.Emplace(InAssetData.GetSoftObjectPath());
#else
		InFilter.ObjectPaths.Emplace(InAssetData.ToSoftObjectPath().GetAssetPathName());
#endif
	}

	void ReserveFilterObjectPaths(FARFilter& InFilter, const int32 InNum) {
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		InFilter.SoftObjectPaths.Reserve(InNum);
#else
		InFilter.ObjectPaths.Reserve(InNum);
#endif
	}

	bool HasFilterObjectPaths(const FARFilter& InFilter) {
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		return InFilter.SoftObjectPaths.Num() > 0;
#else
		return InFilter.ObjectPaths.Num() > 0;
#endif
	}

	FName GetAssetClassName(const FAssetData& InAssetData) {
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		return FName {*InAssetData.AssetClassPath.ToString()};
#else
		return InAssetData.AssetClass;
#endif
	}

	FName GetClassName(const UClass* InClass) {
		if (!InClass) return NAME_None;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		return FName {*InClass->GetClassPathName().ToString()};
#else
		return InClass->GetFName();
#endif
	}

	FAssetData GetAssetByObjectPath(const FString& InObjectPath) {
		const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistry);
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		return AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath {InObjectPath});
#else
		return AssetRegistryModule.Get().GetAssetByObjectPath(FName {*InObjectPath});
#endif
	}

	void GetDerivedClassNames(const TArray<FName>& ClassNames, const TSet<FName>& ExcludedClassNames, TSet<FName>& OutDerivedClassNames) {
		const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistry);
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		TArray<FTopLevelAssetPath> ClassPaths;
		ClassPaths.Reserve(ClassNames.Num());
		for (const auto& ClassName : ClassNames) {
			ClassPaths.Emplace(ClassName.ToString());
		}

		TSet<FTopLevelAssetPath> ExcludedClassPaths;
		ExcludedClassPaths.Reserve(ExcludedClassNames.Num());
		for (const auto& ExcludedClassName : ExcludedClassNames) {
			ExcludedClassPaths.Emplace(ExcludedClassName.ToString());
		}

		TSet<FTopLevelAssetPath> OutDerivedClassPaths;
		AssetRegistryModule.Get().GetDerivedClassNames(ClassPaths, ExcludedClassPaths, OutDerivedClassPaths);

		OutDerivedClassNames.Reserve(OutDerivedClassPaths.Num());
		for (const auto& OutDerivedClassPath : OutDerivedClassPaths) {
			OutDerivedClassNames.Emplace(OutDerivedClassPath.ToString());
		}
#else
		AssetRegistryModule.Get().GetDerivedClassNames(ClassNames, ExcludedClassNames, OutDerivedClassNames);
#endif
	}

	void OpenAssetEditor(const FAssetData& InAssetData) {
		if (!InAssetData.IsValid() || !GEditor) return;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		TArray<FSoftObjectPath> AssetPaths;
		AssetPaths.Add(InAssetData.GetSoftObjectPath());
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorsForAssets(AssetPaths);
#else
		TArray<FName> AssetNames;
		AssetNames.Add(InAssetData.ToSoftObjectPath().GetAssetPathName());
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorsForAssets(AssetNames);
#endif
	}

	FString GetObjectPathString(const FAssetData& InAssetData) {
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		return InAssetData.GetObjectPathString();
#else
		return InAssetData.ObjectPath.ToString();
#endif
	}

	bool IsAssetManagerValid() {
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
		return UAssetManager::IsInitialized();
#else
		return UAssetManager::Get().IsValid();
#endif
	}

	EAppReturnType::Type ShowDialog(const FText& Title, const FText& Message, const EAppMsgType::Type MessageType) {
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
		return FMessageDialog::Open(MessageType, Message, Title);
#else
		return FMessageDialog::Open(MessageType, Message, &Title);
#endif
	}

	UContentBrowserSettings* GetContentBrowserSettingsForUnusedAssetsTab() {
		UContentBrowserSettings* ContentBrowserSettings = GetMutableDefault<UContentBrowserSettings>();
		if (!ContentBrowserSettings) return nullptr;

		ContentBrowserSettings->SetDisplayDevelopersFolder(true);
		ContentBrowserSettings->SetDisplayEngineFolder(false);
		ContentBrowserSettings->SetDisplayCppFolders(false);
		ContentBrowserSettings->SetDisplayPluginFolders(true);
#if ENGINE_MAJOR_VERSION == 5
		ContentBrowserSettings->bShowAllFolder = false;
		ContentBrowserSettings->bOrganizeFolders = false;
#endif

		return ContentBrowserSettings;
	}

	bool LoadAssetsIfNeeded(const TArray<FString>& ObjectPaths, TArray<UObject*>& LoadedObjects, bool bAllowedToPromptToLoad, bool bLoadRedirects)
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
		AssetViewUtils::FLoadAssetsSettings Settings;
		Settings.bAlwaysPromptBeforeLoading = bAllowedToPromptToLoad;
		Settings.bFollowRedirectors = bLoadRedirects;
		return AssetViewUtils::LoadAssetsIfNeeded(ObjectPaths, LoadedObjects, Settings) == AssetViewUtils::ELoadAssetsResult::Success;
#else
		return AssetViewUtils::LoadAssetsIfNeeded(ObjectPaths, LoadedObjects, bAllowedToPromptToLoad, bLoadRedirects);
#endif
	}

}	// namespace PjcShim
