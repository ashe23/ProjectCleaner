// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.h"
#include "ProjectCleanerDelegates.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "ProjectCleanerSubsystem.generated.h"

class FAssetRegistryModule;
class FAssetToolsModule;

UCLASS(Config=EditorPerProjectUserSettings)
class UProjectCleanerSubsystem final : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	UProjectCleanerSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns all assets in project"))
	void GetAssetsAll(TArray<FAssetData>& Assets) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns all assets by given search filter"))
	void GetAssetsByFilter(TArray<FAssetData>& Assets, const FProjectCleanerAssetSearchFilter& SearchFilter) const;

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner", meta=(ToolTip="For given list of assets excludes assets that are apply to given search filter"))
	void GetAssetsExcludedByFilter(UPARAM(ref) TArray<FAssetData>& Assets, const FProjectCleanerAssetSearchFilter& SearchFilter) const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns all primary assets in project"))
	void GetAssetsPrimary(TArray<FAssetData>& AssetsPrimary) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns all indirect assets in project"))
	void GetAssetsIndirect(TArray<FAssetData>& AssetsIndirect) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns all indirect assets in project with info about where they used"))
	void GetAssetsIndirectInfo(TArray<FProjectCleanerIndirectAssetInfo>& AssetsIndirectInfos) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns all used assets in project"))
	void GetAssetsUsed(TArray<FAssetData>& AssetsUsed) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns all unused assets in project"))
	void GetAssetsUnused(TArray<FAssetData>& AssetsUnused) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns all dependencies for given assets"))
	void GetAssetsDependencies(const TArray<FAssetData>& Assets, TArray<FAssetData>& Dependencies) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns all referencers for given assets"))
	void GetAssetsReferencers(const TArray<FAssetData>& Assets, TArray<FAssetData>& Referencers) const;

	// void GetClassNamesPrimary(TArray<FName>& ClassNames, const bool bIncludeDerivedClasses) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Return all primary assets class names"))
	void GetClassNamesPrimary(TSet<FName>& ClassNamesPrimary) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Return all assets class names that are considered to be used"))
	void GetClassNamesUsed(TSet<FName>& ClassNamesUsed) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Return all derived class names for given class names"))
	void GetClassNamesDerived(const TArray<FName>& ClassNames, TSet<FName>& DerivedClassNames) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns total size of given assets"))
	int64 GetAssetsTotalSize(const TArray<FAssetData>& Assets) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns given size in bytes as human readable string"))
	FString SizeToString(const float Size) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns all files in given path"))
	static void GetFiles(const FString& InPath, TArray<FString>& Files, const bool bRecursive);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns all corrupted engine files (.uassets, .umap or .ucollection)"))
	void GetFilesCorrupted(TArray<FString>& FilesCorrupted) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns all non engine files"))
	void GetFilesNonEngine(TArray<FString>& FilesNonEngine) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(Tooltip="Returns subfolders under given path"))
	static void GetFolders(const FString& InPath, TArray<FString>& Folders, const bool bRecursive);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(Tooltip="Returns all empty folder under given path"))
	static void GetFoldersEmpty(const FString& InPath, TArray<FString>& FoldersEmpty);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns total size of given files"))
	static int64 GetFilesTotalSize(const TArray<FString>& Files);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Returns given asset class name.For blueprints returns GeneratedClass name"))
	FName GetAssetExactClassName(const FAssetData& AssetData) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Checks if given asset is blueprint"))
	bool AssetIsBlueprint(const FAssetData& AssetData, const bool bCheckDerivedClasses) const;

	bool AssetIsEditorUtility(const FAssetData& AssetData) const;

	EProjectCleanerAssetUsageCategory GetAssetUsageCategory(const FAssetData& AssetData) const;
	
	bool AssetIsPrimary(const FAssetData& AssetData) const;

	bool AssetIsUsed(const FAssetData& AssetData) const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Checks if given file is corrupted engine file"))
	bool FileIsCorrupted(const FString& FilePathAbs) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Checks if given file is non engine"))
	static bool FileIsNonEngine(const FString& FilePathAbs);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner", meta=(ToolTip="Checks if folder is empty"))
	static bool FolderIsEmpty(const FString& FolderPath);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Path", meta=(ToolTip="Returns normalized file or directory path"))
	static FString PathNormalize(const FString& InPath);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Path", meta=(ToolTip="Converts given path to absolute. If path is outside Content folder will return empty string."))
	static FString PathConvertToAbs(const FString& InPath);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Path", meta=(ToolTip="Converts given path to relative. If path is outside Content folder will return empty string."))
	static FString PathConvertToRel(const FString& InPath);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Path", meta=(ToolTip="Returns absolute path to projects Content folder"))
	static FString GetPathContentFolder();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Path", meta=(ToolTip="Returns absolute path to projects Developers folder"))
	static FString GetPathDevelopersFolder();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Path", meta=(ToolTip="Returns absolute path to projects Collections folder"))
	static FString GetPathCollectionsFolder();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Path", meta=(ToolTip="Returns absolute path to current Developers folder"))
	static FString GetPathDevelopersUserFolder();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ProjectCleaner|Path", meta=(ToolTip="Returns absolute path to current Developers Collections folder"))
	static FString GetPathCollectionsUserFolder();

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner")
	void ProjectScanInitial();
	
	void ProjectScan();
	FProjectCleanerDelegateProjectScanned& OnProjectScanned();
	const FProjectCleanerScanData& GetScanData() const;
	bool AssetRegistryWorking() const;
	static bool EditorInPlayMode();
	static void ShowModal(const FString& Msg, const EProjectCleanerModalState State = EProjectCleanerModalState::None, const float Duration = 2.0f);
	static void ShowModalOutputLog(const FString& Msg, const EProjectCleanerModalState State = EProjectCleanerModalState::None, const float Duration = 2.0f);
	static EAppReturnType::Type ShowDialogWindow(const FString& Title, const FString& Msg, const EAppMsgType::Type MsgType);

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner")
	void GetAssetsWithExternalRefs(TArray<FAssetData>& AssetsWithExternalRefs, const TArray<FAssetData>& Assets) const;
private:
	void FixupRedirectors() const;
	static bool PathIsUnderContentFolder(const FString& InPath);
	static SNotificationItem::ECompletionState GetCompletionStateFromModalState(const EProjectCleanerModalState ModalState);

	bool bScanningInProgress = false;
	bool bCleaningInProgress = false;
	bool bInitialScan = true;

	FProjectCleanerScanData ScanData;
	FProjectCleanerDelegateProjectScanned DelegateProjectScanned;

	IPlatformFile* PlatformFile;
	FAssetRegistryModule* ModuleAssetRegistry;
	FAssetToolsModule* ModuleAssetTools;
};
