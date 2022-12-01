// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerApi.h"
#include "ProjectCleanerConstants.h"
#include "ProjectCleanerLibrary.h"
// Engine Headers
#include "EditorUtilityBlueprint.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/MapBuildDataRegistry.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"

void UProjectCleanerApi::GetAssetsUnused(const FString& InFolderPathRel, const UProjectCleanerScanSettings* ScanSettings, TArray<FAssetData>& UnusedAssets)
{
	if (!ScanSettings) return;

	UnusedAssets.Reset();
	
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	ModuleAssetRegistry.Get().GetAssetsByPath(ProjectCleanerConstants::PathRelRoot, UnusedAssets, true);

	TSet<FString> BlackListFolders;
	GetFoldersBlacklist(*ScanSettings, BlackListFolders);

	TArray<FAssetData> UsedAssetsContainer;
	TArray<FAssetData> BlacklistAssets;
	TArray<FAssetData> PrimaryAssets;
	TArray<FAssetData> IndirectAssets;
	TArray<FAssetData> AssetsWithExternalRefs;
	
	GetAssetsBlacklist(BlacklistAssets);
	GetAssetsIndirect(IndirectAssets);
	GetAssetsWithExternalRefs(AssetsWithExternalRefs);
	UProjectCleanerLibrary::GetAssetsPrimary(PrimaryAssets, true);

	UsedAssetsContainer.Append(BlacklistAssets);
	UsedAssetsContainer.Append(IndirectAssets);
	UsedAssetsContainer.Append(PrimaryAssets);
	UsedAssetsContainer.Append(AssetsWithExternalRefs);
	UsedAssetsContainer.Append(ScanSettings->ExcludedAssets);

	TArray<FAssetData> LinkedAssets;
	UProjectCleanerLibrary::GetLinkedAssets(UsedAssetsContainer, LinkedAssets);

	UsedAssetsContainer.Append(LinkedAssets);
	
	FARFilter Filter;
	Filter.bRecursivePaths = true;

	// Filter.PackagePaths.Add(FName{*UProjectCleanerLibrary::PathConvertToRel(InFolderPathRel)});

	// filtering blacklisted folder
	for (const auto& BlackListFolder : BlackListFolders)
	{
		Filter.PackagePaths.AddUnique(FName{*UProjectCleanerLibrary::PathConvertToRel(BlackListFolder)});
	}

	// filtering excluded by folders
	for (const auto& ExcludedFolder : ScanSettings->ExcludedFolders)
	{
		Filter.PackagePaths.AddUnique(FName{*ExcludedFolder.Path});
	}
	
	// filtering excluded by class
	for (const auto& ExcludedClass : ScanSettings->ExcludedClasses)
	{
		if (!ExcludedClass.LoadSynchronous()) continue;
			
		Filter.ClassNames.AddUnique(ExcludedClass->GetFName());
	}
	
	for (const auto& Asset : UsedAssetsContainer)
	{
		if (!Asset.IsValid()) continue;
	
		Filter.ObjectPaths.AddUnique(Asset.ObjectPath);
	}

	ModuleAssetRegistry.Get().UseFilterToExcludeAssets(UnusedAssets, Filter);

	// UnusedAssets.RemoveAllSwap([&](const FAssetData& AssetData)
	// {
	// 	return !AssetData.PackagePath.ToString().StartsWith(InFolderPathRel);
	// });
}

void UProjectCleanerApi::GetAssetsIndirect(TArray<FAssetData>& IndirectAssets)
{
	IndirectAssets.Reset();

	TArray<FProjectCleanerIndirectAsset> Assets;
	GetAssetsIndirect(Assets);

	IndirectAssets.Reserve(Assets.Num());

	for (const auto& Asset : Assets)
	{
		IndirectAssets.Add(Asset.AssetData);
	}
}

void UProjectCleanerApi::GetAssetsIndirect(TArray<FProjectCleanerIndirectAsset>& IndirectAssets)
{
	IndirectAssets.Reset();

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	const FString SourceDir = FPaths::ProjectDir() + TEXT("Source/");
	const FString ConfigDir = FPaths::ProjectDir() + TEXT("Config/");
	const FString PluginsDir = FPaths::ProjectDir() + TEXT("Plugins/");

	TSet<FString> Files;
	Files.Reserve(200); // reserving some space

	// 1) finding all source files in main project "Source" directory (<yourproject>/Source/*)
	TArray<FString> FilesToScan;
	PlatformFile.FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".cs"));
	PlatformFile.FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".cpp"));
	PlatformFile.FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".h"));
	PlatformFile.FindFilesRecursively(FilesToScan, *ConfigDir, TEXT(".ini"));
	Files.Append(FilesToScan);

	// 2) we should find all source files in plugins folder (<yourproject>/Plugins/*)
	TArray<FString> ProjectPluginsFiles;
	// finding all installed plugins in "Plugins" directory
	struct DirectoryVisitor : public IPlatformFile::FDirectoryVisitor
	{
		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (bIsDirectory)
			{
				InstalledPlugins.Add(FilenameOrDirectory);
			}

			return true;
		}

		TArray<FString> InstalledPlugins;
	};

	DirectoryVisitor Visitor;
	PlatformFile.IterateDirectory(*PluginsDir, Visitor);

	// 3) for every installed plugin we scanning only "Source" and "Config" folders
	for (const auto& Dir : Visitor.InstalledPlugins)
	{
		const FString PluginSourcePathDir = Dir + "/Source";
		const FString PluginConfigPathDir = Dir + "/Config";

		PlatformFile.FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".cs"));
		PlatformFile.FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".cpp"));
		PlatformFile.FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".h"));
		PlatformFile.FindFilesRecursively(ProjectPluginsFiles, *PluginConfigPathDir, TEXT(".ini"));
	}

	Files.Append(ProjectPluginsFiles);
	Files.Shrink();

	TArray<FAssetData> AllAssets;
	AllAssets.Reserve(ModuleAssetRegistry.Get().GetAllocatedSize());
	ModuleAssetRegistry.Get().GetAssetsByPath(ProjectCleanerConstants::PathRelRoot, AllAssets, true);

	for (const auto& File : Files)
	{
		if (!PlatformFile.FileExists(*File)) continue;

		FString FileContent;
		FFileHelper::LoadFileToString(FileContent, *File);

		if (!UProjectCleanerLibrary::HasIndirectlyUsedAssets(FileContent)) continue;

		static FRegexPattern Pattern(TEXT(R"(\/Game([A-Za-z0-9_.\/]+)\b)"));
		FRegexMatcher Matcher(Pattern, FileContent);
		while (Matcher.FindNext())
		{
			FString FoundedAssetObjectPath = Matcher.GetCaptureGroup(0);


			// if ObjectPath ends with "_C" , then its probably blueprint, so we trim that
			if (FoundedAssetObjectPath.EndsWith("_C"))
			{
				FString TrimmedObjectPath = FoundedAssetObjectPath;
				TrimmedObjectPath.RemoveFromEnd("_C");

				FoundedAssetObjectPath = TrimmedObjectPath;
			}


			const FAssetData* AssetData = AllAssets.FindByPredicate([&](const FAssetData& Elem)
			{
				return
					Elem.ObjectPath.ToString() == (FoundedAssetObjectPath) ||
					Elem.PackageName.ToString() == (FoundedAssetObjectPath);
			});

			if (!AssetData) continue;

			// if founded asset is ok, we loading file by lines to determine on what line its used
			TArray<FString> Lines;
			FFileHelper::LoadFileToStringArray(Lines, *File);
			for (int32 i = 0; i < Lines.Num(); ++i)
			{
				if (!Lines.IsValidIndex(i)) continue;
				if (!Lines[i].Contains(FoundedAssetObjectPath)) continue;

				FProjectCleanerIndirectAsset IndirectAsset;
				IndirectAsset.AssetData = *AssetData;
				IndirectAsset.FilePath = FPaths::ConvertRelativePathToFull(File);
				IndirectAsset.LineNum = i + 1;
				IndirectAssets.AddUnique(IndirectAsset);
			}
		}
	}
}

void UProjectCleanerApi::GetFoldersBlacklist(const UProjectCleanerScanSettings& ScanSettings, TSet<FString>& BlacklistFolders)
{
	// blacklist folder will never be scanned nor deleted
	BlacklistFolders.Reset();
	
	BlacklistFolders.Add(FPaths::ProjectContentDir() / TEXT("Collections"));
	BlacklistFolders.Add(FPaths::GameUserDeveloperDir() / TEXT("Collections"));
	// todo:ashe23 for ue5 add __ExternalObject__ and __ExternalActors__ folders
	
	if (FModuleManager::Get().IsModuleLoaded(ProjectCleanerConstants::PluginMegascans))
	{
		BlacklistFolders.Add(FPaths::ProjectContentDir() / ProjectCleanerConstants::PluginMegascansMsPresetsFolder.ToString());
	}
	
	if (!ScanSettings.bScanDeveloperContents)
	{
		BlacklistFolders.Add(FPaths::ProjectContentDir() / TEXT("Developers"));
	}
}

void UProjectCleanerApi::GetAssetsBlacklist(TArray<FAssetData>& BlacklistAssets)
{
	BlacklistAssets.Reset();
	
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	
	//  assets that we considering as used, things like editor utility assets, are used in editor only, so we cant really know if its used or not by user
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	Filter.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
	Filter.ClassNames.Add(UEditorUtilityWidget::StaticClass()->GetFName());
	Filter.ClassNames.Add(UEditorUtilityBlueprint::StaticClass()->GetFName());
	Filter.ClassNames.Add(UEditorUtilityWidgetBlueprint::StaticClass()->GetFName());
	Filter.ClassNames.Add(UMapBuildDataRegistry::StaticClass()->GetFName());

	ModuleAssetRegistry.Get().GetAssets(Filter, BlacklistAssets);
}

void UProjectCleanerApi::GetAssetsPrimary(TArray<FAssetData>& PrimaryAssets)
{
	PrimaryAssets.Reset();

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	
	TArray<FName> PrimaryAssetClasses;
	UProjectCleanerLibrary::GetPrimaryAssetClasses(PrimaryAssetClasses, true);

	FARFilter Filter;
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);

	for (const auto& ClassName : PrimaryAssetClasses)
	{
		Filter.ClassNames.Add(ClassName);
	}
	ModuleAssetRegistry.Get().GetAssets(Filter, PrimaryAssets);

	// searching for blueprint assets
	FARFilter FilterBlueprint;
	FilterBlueprint.bRecursivePaths = true;
	FilterBlueprint.bRecursiveClasses = true;
	FilterBlueprint.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
	FilterBlueprint.ClassNames.Add(UBlueprint::StaticClass()->GetFName());

	TArray<FAssetData> BlueprintAssets;
	ModuleAssetRegistry.Get().GetAssets(FilterBlueprint, BlueprintAssets);

	for (const auto& BlueprintAsset : BlueprintAssets)
	{
		const FName BlueprintClass = FName{*UProjectCleanerLibrary::GetAssetClassName(BlueprintAsset)};
		if (PrimaryAssetClasses.Contains(BlueprintClass))
		{
			PrimaryAssets.AddUnique(BlueprintAsset);
		}
	}
}

void UProjectCleanerApi::GetAssetsWithExternalRefs(TArray<FAssetData>& Assets)
{
	Assets.Reset();
	
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	TArray<FAssetData> AssetsAll;
	ModuleAssetRegistry.Get().GetAssetsByPath(ProjectCleanerConstants::PathRelRoot, AssetsAll, true);
	
	TArray<FName> Refs;
	for (const auto& Asset : AssetsAll)
	{
		ModuleAssetRegistry.Get().GetReferencers(Asset.PackageName, Refs);

		const bool HasExternalRefs = Refs.ContainsByPredicate([](const FName& Ref)
		{
			return !Ref.ToString().StartsWith(ProjectCleanerConstants::PathRelRoot.ToString());
		});

		if (HasExternalRefs)
		{
			Assets.AddUnique(Asset);
		}

		Refs.Reset();
	}
}

void UProjectCleanerApi::GetAssetsExcluded(const UProjectCleanerScanSettings& ScanSettings, TArray<FAssetData>& ExcludedAssets)
{
	// todo:ashe23 not sure
	ExcludedAssets.Reset();
	
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	FARFilter Filter;

	if (ScanSettings.ExcludedFolders.Num() > 0)
	{
		Filter.bRecursivePaths = true;
		for (const auto& ExcludedFolder : ScanSettings.ExcludedFolders)
		{
			Filter.PackagePaths.AddUnique(FName{*ExcludedFolder.Path});
		}
	}

	if (ScanSettings.ExcludedClasses.Num() > 0)
	{
		Filter.bRecursivePaths = true;
		Filter.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
		
		for (const auto& ExcludedClass : ScanSettings.ExcludedClasses)
		{
			if (!ExcludedClass.LoadSynchronous()) continue;
			
			Filter.ClassNames.AddUnique(ExcludedClass->GetFName());
		}
	}
	
	if (!Filter.IsEmpty())
	{
		ModuleAssetRegistry.Get().GetAssets(Filter, ExcludedAssets);
	}

	for (const auto& Asset : ScanSettings.ExcludedAssets)
	{
		if (!Asset.IsValid()) continue;

		ExcludedAssets.Add(Asset);
	}
}
