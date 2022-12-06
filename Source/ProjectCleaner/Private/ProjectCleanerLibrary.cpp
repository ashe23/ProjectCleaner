// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleaner/Public/ProjectCleanerLibrary.h"
#include "ProjectCleanerConstants.h"
// Engine Headers
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "FileHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"
#include "Widgets/Notifications/SNotificationList.h"

// AssetRegistry

bool UProjectCleanerLibrary::AssetRegistryWorking()
{
	return FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get().IsLoadingAssets();
}

void UProjectCleanerLibrary::AssetRegistryUpdate(const bool bSyncScan)
{
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	TArray<FString> ScanFolders;
	ScanFolders.Add(ProjectCleanerConstants::PathRelRoot.ToString());

	ModuleAssetRegistry.Get().ScanPathsSynchronous(ScanFolders, true);
	ModuleAssetRegistry.Get().SearchAllAssets(bSyncScan);
}

void UProjectCleanerLibrary::AssetRegistryFixupRedirectors(const FString& InPathRel)
{
	if (InPathRel.IsEmpty() || !InPathRel.StartsWith(ProjectCleanerConstants::PathRelRoot.ToString())) return;

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	const FAssetToolsModule& ModuleAssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));

	FScopedSlowTask FixRedirectorsTask{
		1.0f,
		FText::FromString(ProjectCleanerConstants::MsgFixingRedirectors)
	};
	FixRedirectorsTask.MakeDialog();

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Emplace(InPathRel);
	Filter.ClassNames.Emplace(UObjectRedirector::StaticClass()->GetFName());

	// Getting all redirectors in given path
	TArray<FAssetData> AssetList;
	ModuleAssetRegistry.Get().GetAssets(Filter, AssetList);

	if (AssetList.Num() == 0) return;

	FScopedSlowTask FixRedirectorsLoadingTask(
		AssetList.Num(),
		FText::FromString(ProjectCleanerConstants::MsgLoadingRedirectors)
	);
	FixRedirectorsLoadingTask.MakeDialog();

	TArray<UObjectRedirector*> Redirectors;
	Redirectors.Reserve(AssetList.Num());

	for (const auto& Asset : AssetList)
	{
		FixRedirectorsLoadingTask.EnterProgressFrame();

		UObject* AssetObj = Asset.GetAsset();
		if (!AssetObj) continue;

		UObjectRedirector* Redirector = CastChecked<UObjectRedirector>(AssetObj);
		if (!Redirector) continue;

		Redirectors.Add(Redirector);
	}

	Redirectors.Shrink();

	// Fix up all founded redirectors
	ModuleAssetTools.Get().FixupReferencers(Redirectors);

	FixRedirectorsTask.EnterProgressFrame(1.0f);
}

// Assets

void UProjectCleanerLibrary::AssetsSaveAll(const bool bPromptUser)
{
	FEditorFileUtils::SaveDirtyPackages(
		bPromptUser,
		true,
		true,
		false,
		false,
		false
	);
}

void UProjectCleanerLibrary::AssetsGetWithExternalRefs(TArray<FAssetData>& Assets)
{
	Assets.Reset();

	TArray<FAssetData> AllAssets;

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	ModuleAssetRegistry.Get().GetAssetsByPath(ProjectCleanerConstants::PathRelRoot, AllAssets, true);

	Assets.Reserve(AllAssets.Num());

	TArray<FName> Refs;
	for (const auto& Asset : AllAssets)
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

	Assets.Shrink();
}

void UProjectCleanerLibrary::AssetsGetIndirect(TArray<FAssetData>& AssetsIndirect)
{
	AssetsIndirect.Reset();

	TArray<FProjectCleanerIndirectAsset> IndirectAssets;
	AssetsGetIndirectAdvanced(IndirectAssets);

	AssetsIndirect.Reserve(IndirectAssets.Num());
	for (const auto& Info : IndirectAssets)
	{
		AssetsIndirect.AddUnique(Info.AssetData);
	}
}

void UProjectCleanerLibrary::AssetsGetIndirectAdvanced(TArray<FProjectCleanerIndirectAsset>& AssetsIndirect)
{
	AssetsIndirect.Reset();

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	const FString SourceDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + TEXT("Source/"));
	const FString ConfigDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + TEXT("Config/"));
	const FString PluginsDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + TEXT("Plugins/"));

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

		if (!FileContainsIndirectAssets(FileContent)) continue;

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
				AssetsIndirect.AddUnique(IndirectAsset);
			}
		}
	}
}

void UProjectCleanerLibrary::AssetsGetPrimary(TArray<FAssetData>& AssetsPrimary, const bool bIncludeDerivedClasses)
{
	AssetsPrimary.Reset();

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	TArray<FName> PrimaryAssetClasses;

	// getting list of primary asset classes that are defined in AssetManager
	const auto& AssetManager = UAssetManager::Get();
	if (!AssetManager.IsValid()) return;

	TArray<FPrimaryAssetTypeInfo> AssetTypeInfos;
	AssetManager.Get().GetPrimaryAssetTypeInfoList(AssetTypeInfos);
	PrimaryAssetClasses.Reserve(AssetTypeInfos.Num());

	for (const auto& AssetTypeInfo : AssetTypeInfos)
	{
		if (!AssetTypeInfo.AssetBaseClassLoaded) continue;

		PrimaryAssetClasses.AddUnique(AssetTypeInfo.AssetBaseClassLoaded->GetFName());
	}

	if (bIncludeDerivedClasses)
	{
		// getting list of primary assets classes that are derived from main primary assets
		TSet<FName> DerivedFromPrimaryAssets;
		{
			const TSet<FName> ExcludedClassNames;
			ModuleAssetRegistry.Get().GetDerivedClassNames(PrimaryAssetClasses, ExcludedClassNames, DerivedFromPrimaryAssets);
		}

		for (const auto& DerivedClassName : DerivedFromPrimaryAssets)
		{
			PrimaryAssetClasses.AddUnique(DerivedClassName);
		}
	}

	FARFilter Filter;
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(FName{ProjectCleanerConstants::PathRelRoot});

	for (const auto& ClassName : PrimaryAssetClasses)
	{
		Filter.ClassNames.Add(ClassName);
	}
	ModuleAssetRegistry.Get().GetAssets(Filter, AssetsPrimary);

	FARFilter FilterBlueprint;
	FilterBlueprint.bRecursivePaths = true;
	FilterBlueprint.bRecursiveClasses = true;
	FilterBlueprint.PackagePaths.Add(FName{ProjectCleanerConstants::PathRelRoot});
	FilterBlueprint.ClassNames.Add(UBlueprint::StaticClass()->GetFName());

	TArray<FAssetData> BlueprintAssets;
	ModuleAssetRegistry.Get().GetAssets(FilterBlueprint, BlueprintAssets);

	for (const auto& BlueprintAsset : BlueprintAssets)
	{
		const FName BlueprintClass = FName{*AssetGetClassName(BlueprintAsset)};
		if (PrimaryAssetClasses.Contains(BlueprintClass))
		{
			AssetsPrimary.AddUnique(BlueprintAsset);
		}
	}
}

void UProjectCleanerLibrary::AssetsGetLinked(const TArray<FAssetData>& Assets, TArray<FAssetData>& LinkedAssets)
{
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	LinkedAssets.Reset();

	TSet<FName> UsedAssetsDeps;
	TArray<FName> Stack;
	for (const auto& Asset : Assets)
	{
		UsedAssetsDeps.Add(Asset.PackageName);
		Stack.Add(Asset.PackageName);

		TArray<FName> Deps;
		while (Stack.Num() > 0)
		{
			const auto CurrentPackageName = Stack.Pop(false);
			Deps.Reset();

			ModuleAssetRegistry.Get().GetDependencies(CurrentPackageName, Deps);

			Deps.RemoveAllSwap([&](const FName& Dep)
			{
				return !Dep.ToString().StartsWith(*ProjectCleanerConstants::PathRelRoot.ToString());
			}, false);

			for (const auto& Dep : Deps)
			{
				bool bIsAlreadyInSet = false;
				UsedAssetsDeps.Add(Dep, &bIsAlreadyInSet);
				if (!bIsAlreadyInSet)
				{
					Stack.Add(Dep);
				}
			}
		}
	}

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(FName{*ProjectCleanerConstants::PathRelRoot.ToString()});

	for (const auto& Dep : UsedAssetsDeps)
	{
		Filter.PackageNames.Add(Dep);
	}

	ModuleAssetRegistry.Get().GetAssets(Filter, LinkedAssets);
}

int64 UProjectCleanerLibrary::AssetsGetTotalSize(const TArray<FAssetData>& Assets)
{
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	int64 Size = 0;
	for (const auto& Asset : Assets)
	{
		const auto AssetPackageData = ModuleAssetRegistry.Get().GetAssetPackageData(Asset.PackageName);
		if (!AssetPackageData) continue;
		Size += AssetPackageData->DiskSize;
	}

	return Size;
}

FString UProjectCleanerLibrary::AssetGetClassName(const FAssetData& AssetData)
{
	if (!AssetData.IsValid()) return {};

	if (AssetData.AssetClass.IsEqual("Blueprint"))
	{
		const auto GeneratedClassName = AssetData.TagsAndValues.FindTag(TEXT("GeneratedClass")).GetValue();
		const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(*GeneratedClassName);
		return FPackageName::ObjectPathToObjectName(ClassObjectPath);
	}

	return AssetData.AssetClass.ToString();
}

// Paths

FString UProjectCleanerLibrary::PathGetContentFolder(const bool bAbsolutePath)
{
	if (bAbsolutePath)
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / ProjectCleanerConstants::FolderContent.ToString());
	}

	return ProjectCleanerConstants::PathRelRoot.ToString();
}

FString UProjectCleanerLibrary::PathGetDevelopersFolder(const bool bAbsolutePath)
{
	if (bAbsolutePath)
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / ProjectCleanerConstants::FolderDevelopers.ToString());
	}

	return ProjectCleanerConstants::PathRelDevelopers.ToString();
}

FString UProjectCleanerLibrary::PathGetDeveloperFolder(const bool bAbsolutePath)
{
	if (bAbsolutePath)
	{
		return FPaths::ConvertRelativePathToFull(PathGetDevelopersFolder(true) / FPaths::GameUserDeveloperFolderName());
	}

	return FString::Printf(TEXT("%s/%s"), *ProjectCleanerConstants::PathRelDevelopers.ToString(), *FPaths::GameUserDeveloperFolderName());
}

FString UProjectCleanerLibrary::PathGetCollectionsFolder(const bool bAbsolutePath)
{
	if (bAbsolutePath)
	{
		return FPaths::ConvertRelativePathToFull(PathGetContentFolder(true) / ProjectCleanerConstants::FolderCollections.ToString());
	}

	return ProjectCleanerConstants::PathRelCollections.ToString();
}

FString UProjectCleanerLibrary::PathGetDeveloperCollectionFolder(const bool bAbsolutePath)
{
	if (bAbsolutePath)
	{
		return FPaths::ConvertRelativePathToFull(
			FString::Printf(
				TEXT("%s/%s"),
				*PathGetDeveloperFolder(true),
				*ProjectCleanerConstants::FolderCollections.ToString()
			)
		);
	}

	return FString::Printf(
		TEXT("%s/%s/%s"),
		*ProjectCleanerConstants::PathRelDevelopers.ToString(),
		*FPaths::GameUserDeveloperFolderName(),
		*ProjectCleanerConstants::FolderCollections.ToString()
	);
}

FString UProjectCleanerLibrary::PathGetMsPresetsFolder(const bool bAbsolutePath)
{
	if (bAbsolutePath)
	{
		return FPaths::ConvertRelativePathToFull(PathGetContentFolder(true) / ProjectCleanerConstants::FolderMsPresets.ToString());
	}

	return ProjectCleanerConstants::PathRelMegascansPresets.ToString();
}

FString UProjectCleanerLibrary::PathConvertToAbs(const FString& InRelPath)
{
	if (InRelPath.IsEmpty()) return {};

	FString Path = InRelPath;
	FPaths::RemoveDuplicateSlashes(Path);
	FPaths::NormalizeDirectoryName(Path);

	if (Path.StartsWith(PathGetContentFolder(true))) return Path;
	if (!Path.StartsWith(ProjectCleanerConstants::PathRelRoot.ToString())) return {};

	// /Game => C:/{OurProjectPath}/Content
	// /Game/MyFolder => C:/{OurProjectPath}/Content/MyFolder
	// /Game/MyFile.uasset => C:/{OurProjectPath}/Content/MyFile.uasset

	const FString From = ProjectCleanerConstants::PathRelRoot.ToString();
	const FString To = PathGetContentFolder(true);

	return Path.Replace(*From, *To, ESearchCase::CaseSensitive);;
}

FString UProjectCleanerLibrary::PathConvertToRel(const FString& InAbsPath)
{
	if (InAbsPath.IsEmpty()) return {};

	FString Path = FPaths::ConvertRelativePathToFull(InAbsPath);
	FPaths::RemoveDuplicateSlashes(Path);
	FPaths::NormalizeDirectoryName(Path);

	const FString ProjectContentDir = PathGetContentFolder(true);
	if (Path.StartsWith(ProjectCleanerConstants::PathRelRoot.ToString())) return Path;
	if (!Path.StartsWith(ProjectContentDir)) return {};

	// C:/{OurProjectPath}/Content => /Game
	// C:/{OurProjectPath}/Content/ => /Game
	// C:/{OurProjectPath}/Content/MyFolder => /Game/MyFolder
	// C:/{OurProjectPath}/Content/MyFile.uasset => /Game/MyFile.uasset

	const FString From = ProjectContentDir;
	const FString To = ProjectCleanerConstants::PathRelRoot.ToString();

	return Path.Replace(*From, *To, ESearchCase::CaseSensitive);
}

bool UProjectCleanerLibrary::PathIsUnderFolder(const FString& InSearchFolderPath, const FString& InRootFolderPath)
{
	const FString SearchFolderPathAbs = PathConvertToAbs(InSearchFolderPath);
	const FString RootFolderPathAbs = PathConvertToAbs(InRootFolderPath);

	if (SearchFolderPathAbs.IsEmpty() || RootFolderPathAbs.IsEmpty()) return false;

	return SearchFolderPathAbs.Equals(RootFolderPathAbs) || FPaths::IsUnderDirectory(SearchFolderPathAbs, RootFolderPathAbs);
}

bool UProjectCleanerLibrary::PathIsUnderFolders(const FString& InSearchFolderPath, const TSet<FString>& Folders)
{
	if (InSearchFolderPath.IsEmpty() || Folders.Num() == 0) return false;

	for (const auto& Folder : Folders)
	{
		if (PathIsUnderFolder(InSearchFolderPath, Folder)) return true;
	}

	return false;
}

// Utility

bool UProjectCleanerLibrary::FileHasEngineExtension(const FString& Extension)
{
	TSet<FString> EngineExtensions;
	EngineExtensions.Reserve(3);
	EngineExtensions.Add(TEXT("uasset"));
	EngineExtensions.Add(TEXT("umap"));
	EngineExtensions.Add(TEXT("collection"));

	return EngineExtensions.Contains(Extension.ToLower());
}

bool UProjectCleanerLibrary::FileIsCorrupted(const FString& InFilePathAbs)
{
	// here we got absolute path "C:/MyProject/Content/material.uasset"
	// we must first convert that path to In Engine Internal Path like "/Game/material.uasset"
	const FString RelativePath = PathConvertToRel(InFilePathAbs);
	if (RelativePath.IsEmpty()) return false;

	// Converting file path to object path (This is for searching in AssetRegistry)
	// example "/Game/Name.uasset" => "/Game/Name.Name"
	FString ObjectPath = RelativePath;
	ObjectPath.RemoveFromEnd(FPaths::GetExtension(RelativePath, true));
	ObjectPath.Append(TEXT(".") + FPaths::GetBaseFilename(RelativePath));

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	const FAssetData AssetData = ModuleAssetRegistry.Get().GetAssetByObjectPath(FName{*ObjectPath});

	// if its does not exist in asset registry, then something wrong with asset
	return !AssetData.IsValid();
}

bool UProjectCleanerLibrary::FileContainsIndirectAssets(const FString& FileContent)
{
	if (FileContent.IsEmpty()) return false;

	// search any sub string that has asset package path in it
	static FRegexPattern Pattern(TEXT(R"(\/Game([A-Za-z0-9_.\/]+)\b)"));
	FRegexMatcher Matcher(Pattern, FileContent);
	return Matcher.FindNext();
}

// Confirmation Windows

EAppReturnType::Type UProjectCleanerLibrary::ConfirmationWindowShow(const FText& Title, const FText& ContentText, const EAppMsgType::Type MsgType)
{
	return FMessageDialog::Open(MsgType, ContentText, &Title);
}

bool UProjectCleanerLibrary::ConfirmationWindowCancelled(const EAppReturnType::Type ReturnType)
{
	return ReturnType == EAppReturnType::Type::No || ReturnType == EAppReturnType::Cancel;
}

// Notifications

void UProjectCleanerLibrary::NotificationShow(const FString& Msg, const EProjectCleanerModalStatus ModalStatus, const float Duration)
{
	FNotificationInfo Info{FText::FromString(Msg)};
	Info.ExpireDuration = Duration;

	const auto NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
	if (!NotificationPtr.IsValid()) return;

	NotificationPtr.Get()->SetCompletionState(GetCompletionStateFromModalStatus(ModalStatus));
}

void UProjectCleanerLibrary::NotificationShowWithOutputLog(const FString& Msg, const EProjectCleanerModalStatus ModalStatus, const float Duration)
{
	FNotificationInfo Info{FText::FromString(Msg)};
	Info.ExpireDuration = Duration;
	Info.Hyperlink = FSimpleDelegate::CreateLambda([]()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(FName{TEXT("OutputLog")});
	});
	Info.HyperlinkText = FText::FromString(TEXT("Show OutputLog..."));

	const auto NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
	if (!NotificationPtr.IsValid()) return;

	NotificationPtr.Get()->SetCompletionState(GetCompletionStateFromModalStatus(ModalStatus));
}

// Content Browser

void UProjectCleanerLibrary::FocusOnDirectory(const FString& InPathRel)
{
	const FString RelativePath = PathConvertToRel(InPathRel);

	if (RelativePath.IsEmpty()) return;

	TArray<FString> FocusFolders;
	FocusFolders.Add(RelativePath);

	const FContentBrowserModule& ModuleContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ModuleContentBrowser.Get().SyncBrowserToFolders(FocusFolders);
}

void UProjectCleanerLibrary::FocusOnAssets(const TArray<FAssetData>& Assets)
{
	const FContentBrowserModule& ModuleContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ModuleContentBrowser.Get().SyncBrowserToAssets(Assets);
}

// Private

SNotificationItem::ECompletionState UProjectCleanerLibrary::GetCompletionStateFromModalStatus(const EProjectCleanerModalStatus ModalStatus)
{
	if (ModalStatus == EProjectCleanerModalStatus::Pending)
	{
		return SNotificationItem::CS_Pending;
	}
	if (ModalStatus == EProjectCleanerModalStatus::Error)
	{
		return SNotificationItem::CS_Fail;
	}
	if (ModalStatus == EProjectCleanerModalStatus::OK)
	{
		return SNotificationItem::CS_Success;
	}

	return SNotificationItem::CS_None;
}
