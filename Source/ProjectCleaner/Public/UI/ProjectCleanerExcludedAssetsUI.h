// // Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.
//
// #pragma once
//
// // Engine Headers
// #include "CoreMinimal.h"
// #include "Widgets/SCompoundWidget.h"
// #include "StructsContainer.h"
// #include "Editor/ContentBrowser/Public/ContentBrowserDelegates.h"
// #include "IContentBrowserSingleton.h"
//
// DECLARE_DELEGATE_TwoParams(FOnUserIncludedAsset, const TArray<FAssetData>&, const bool);
//
// class UCleanerConfigs;
//
// class SProjectCleanerExcludedAssetsUI : public SCompoundWidget
// {
// public:
// 	
// 	SLATE_BEGIN_ARGS(SProjectCleanerExcludedAssetsUI) {}
// 		SLATE_ARGUMENT(TArray<FAssetData>*, ExcludedAssets)
// 		SLATE_ARGUMENT(TArray<FAssetData>*, LinkedAssets)
// 		SLATE_ARGUMENT(TSet<FName>*, PrimaryAssetClasses)
// 		SLATE_ARGUMENT(UCleanerConfigs*, CleanerConfigs)
// 	SLATE_END_ARGS()
// 	
// 	void Construct(const FArguments& InArgs);
// 	void SetUIData(const TArray<FAssetData>& NewExcludedAssets, const TArray<FAssetData>& NewLinkedAssets, const TSet<FName>& NewPrimaryAssetClasses, UCleanerConfigs* NewConfigs);
// 	
// 	FOnUserIncludedAsset OnUserIncludedAssets;
// private:
// 	void RegisterCommands();
// 	void SetExcludedAssets(const TArray<FAssetData>& Assets);
// 	void SetLinkedAssets(const TArray<FAssetData>& Assets);
// 	void SetCleanerConfigs(UCleanerConfigs* Configs);
// 	void SetPrimaryAssetClasses(const TSet<FName>& NewPrimaryAssetClasses);
// 	
// 	void UpdateUI();
// 	TSharedPtr<SWidget> GetExcludedAssetsView();
// 	TSharedPtr<SWidget> GetLinkedAssetsView();
// 	TSharedPtr<SWidget> OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets) const;
// 	static void OnAssetDblClicked(const FAssetData& AssetData);
// 	static void OnAssetSelected(const FAssetData& AssetData);
// 	void FindInContentBrowser() const;
// 	bool IsAnythingSelected() const;
// 	void IncludeAssets() const;
// 	FReply IncludeAllAssets() const;
//
// 	/* Data */
// 	UCleanerConfigs* CleanerConfigs = nullptr;
// 	FGetCurrentSelectionDelegate GetCurrentSelectionDelegate;
// 	TSharedPtr<FUICommandList> Commands;
// 	TArray<FAssetData> ExcludedAssets;
// 	TArray<FAssetData> LinkedAssets;
// 	TSet<FName> PrimaryAssetClasses;
//
// 	/* PathPickerConfig */
// 	FName SelectedPath;
// 	void OnPathSelected(const FString& Path);
// 	struct FPathPickerConfig PathPickerConfig;
// 	FAssetData* SelectedAsset = nullptr;
// 	
// 	/* ContentBrowserModule */
// 	class FContentBrowserModule* ContentBrowserModule = nullptr;
// };