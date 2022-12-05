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

bool UProjectCleanerLibrary::IsAssetRegistryWorking()
{
	return FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get().IsLoadingAssets();
}

bool UProjectCleanerLibrary::IsUnderFolder(const FString& InFolderPathAbs, const FString& RootFolder)
{
	if (InFolderPathAbs.IsEmpty() || RootFolder.IsEmpty()) return false;

	return InFolderPathAbs.Equals(RootFolder) || FPaths::IsUnderDirectory(InFolderPathAbs, RootFolder);
}

bool UProjectCleanerLibrary::IsUnderAnyFolder(const FString& InFolderPathAbs, const TSet<FString>& Folders)
{
	if (InFolderPathAbs.IsEmpty() || Folders.Num() == 0) return false;

	for (const auto& Folder : Folders)
	{
		if (IsUnderFolder(InFolderPathAbs, Folder)) return true;
	}

	return false;
}

bool UProjectCleanerLibrary::IsEngineFileExtension(const FString& Extension)
{
	TSet<FString> EngineExtensions;
	EngineExtensions.Reserve(3);
	EngineExtensions.Add(TEXT("uasset"));
	EngineExtensions.Add(TEXT("umap"));
	EngineExtensions.Add(TEXT("collection"));

	return EngineExtensions.Contains(Extension.ToLower());
}

bool UProjectCleanerLibrary::IsCorruptedEngineFile(const FString& InFilePathAbs)
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

void UProjectCleanerLibrary::GetAssetsWithExternalRefs(TArray<FAssetData>& Assets)
{
	Assets.Reset();

	TArray<FAssetData> AllAssets;

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	ModuleAssetRegistry.Get().GetAssetsByPath(FName{ProjectCleanerConstants::PathRelRoot}, AllAssets, true);

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
}

void UProjectCleanerLibrary::GetPrimaryAssetClasses(TArray<FName>& PrimaryAssetClasses, const bool bIncludeDerivedClasses)
{
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	PrimaryAssetClasses.Reset();

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
}

void UProjectCleanerLibrary::GetLinkedAssets(const TArray<FAssetData>& Assets, TArray<FAssetData>& LinkedAssets)
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

EAppReturnType::Type UProjectCleanerLibrary::ShowConfirmationWindow(const FText& Title, const FText& ContentText, const EAppMsgType::Type MsgType)
{
	return FMessageDialog::Open(MsgType, ContentText, &Title);
}

bool UProjectCleanerLibrary::ConfirmationWindowCancelled(const EAppReturnType::Type ReturnType)
{
	return ReturnType == EAppReturnType::Type::No || ReturnType == EAppReturnType::Cancel;
}

void UProjectCleanerLibrary::GetAssetsIndirect(TArray<FAssetData>& AssetsIndirect)
{
	AssetsIndirect.Reset();

	TArray<FProjectCleanerIndirectAsset> IndirectAssets;
	GetAssetsIndirectAdvanced(IndirectAssets);

	AssetsIndirect.Reserve(IndirectAssets.Num());
	for (const auto& Info : IndirectAssets)
	{
		AssetsIndirect.AddUnique(Info.AssetData);
	}
}

void UProjectCleanerLibrary::GetAssetsIndirectAdvanced(TArray<FProjectCleanerIndirectAsset>& AssetsIndirect)
{
	AssetsIndirect.Reset();

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

		if (!HasIndirectlyUsedAssets(FileContent)) continue;

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

void UProjectCleanerLibrary::GetAssetsPrimary(TArray<FAssetData>& AssetsPrimary, const bool bIncludeDerivedClasses)
{
	AssetsPrimary.Reset();

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	TArray<FName> PrimaryAssetClasses;
	GetPrimaryAssetClasses(PrimaryAssetClasses, bIncludeDerivedClasses);

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
		const FName BlueprintClass = FName{*GetAssetClassName(BlueprintAsset)};
		if (PrimaryAssetClasses.Contains(BlueprintClass))
		{
			AssetsPrimary.AddUnique(BlueprintAsset);
		}
	}
}

int64 UProjectCleanerLibrary::GetAssetsTotalSize(const TArray<FAssetData>& Assets)
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

FString UProjectCleanerLibrary::PathConvertToAbs(const FString& InRelPath)
{
	if (InRelPath.IsEmpty()) return {};

	FString Path = InRelPath;
	FPaths::RemoveDuplicateSlashes(Path);
	FPaths::NormalizeDirectoryName(Path);

	if (!Path.StartsWith(ProjectCleanerConstants::PathRelRoot.ToString())) return {};

	// /Game => C:/{OurProjectPath}/Content
	// /Game/MyFolder => C:/{OurProjectPath}/Content/MyFolder
	// /Game/MyFile.uasset => C:/{OurProjectPath}/Content/MyFile.uasset

	const FString From = ProjectCleanerConstants::PathRelRoot.ToString();
	const FString To = FPaths::ProjectDir() / ProjectCleanerConstants::FolderContent.ToString();

	return Path.Replace(*From, *To, ESearchCase::CaseSensitive);;
}

FString UProjectCleanerLibrary::PathConvertToRel(const FString& InAbsPath)
{
	if (InAbsPath.IsEmpty()) return {};

	FString Path = InAbsPath;
	FPaths::RemoveDuplicateSlashes(Path);
	FPaths::NormalizeDirectoryName(Path);

	const FString ProjectContentDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / ProjectCleanerConstants::FolderContent.ToString());

	if (!Path.StartsWith(ProjectContentDir)) return {};

	// C:/{OurProjectPath}/Content => /Game
	// C:/{OurProjectPath}/Content/ => /Game
	// C:/{OurProjectPath}/Content/MyFolder => /Game/MyFolder
	// C:/{OurProjectPath}/Content/MyFile.uasset => /Game/MyFile.uasset

	const FString From = ProjectContentDir;
	const FString To = ProjectCleanerConstants::PathRelRoot.ToString();

	return Path.Replace(*From, *To, ESearchCase::CaseSensitive);
}

FString UProjectCleanerLibrary::GetAssetClassName(const FAssetData& AssetData)
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

void UProjectCleanerLibrary::FixupRedirectors()
{
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	const FAssetToolsModule& ModuleAssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));

	FScopedSlowTask FixRedirectorsTask{
		1.0f,
		FText::FromString(ProjectCleanerConstants::MsgFixingRedirectors)
	};
	FixRedirectorsTask.MakeDialog();

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Emplace(ProjectCleanerConstants::PathRelRoot);
	Filter.ClassNames.Emplace(UObjectRedirector::StaticClass()->GetFName());

	// Getting all redirectors in given path
	TArray<FAssetData> AssetList;
	ModuleAssetRegistry.Get().GetAssets(Filter, AssetList);

	if (AssetList.Num() > 0)
	{
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

			const auto Redirector = CastChecked<UObjectRedirector>(AssetObj);
			if (!Redirector) continue;

			Redirectors.Add(Redirector);
		}

		Redirectors.Shrink();

		// Fix up all founded redirectors
		ModuleAssetTools.Get().FixupReferencers(Redirectors);
	}

	FixRedirectorsTask.EnterProgressFrame(1.0f);
}

void UProjectCleanerLibrary::SaveAllAssets(const bool bPromptUser)
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

void UProjectCleanerLibrary::UpdateAssetRegistry(const bool bSyncScan)
{
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	TArray<FString> ScanFolders;
	ScanFolders.Add(ProjectCleanerConstants::PathRelRoot.ToString());

	ModuleAssetRegistry.Get().ScanPathsSynchronous(ScanFolders, true);
	ModuleAssetRegistry.Get().SearchAllAssets(bSyncScan);
}

void UProjectCleanerLibrary::FocusOnDirectory(const FString& InRelPath)
{
	if (InRelPath.IsEmpty()) return;

	TArray<FString> FocusFolders;
	FocusFolders.Add(InRelPath);

	const FContentBrowserModule& ModuleContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ModuleContentBrowser.Get().SyncBrowserToFolders(FocusFolders);
}

void UProjectCleanerLibrary::FocusOnAssets(const TArray<FAssetData>& Assets)
{
	const FContentBrowserModule& ModuleContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ModuleContentBrowser.Get().SyncBrowserToAssets(Assets);
}

bool UProjectCleanerLibrary::HasIndirectlyUsedAssets(const FString& FileContent)
{
	if (FileContent.IsEmpty()) return false;

	// search any sub string that has asset package path in it
	static FRegexPattern Pattern(TEXT(R"(\/Game([A-Za-z0-9_.\/]+)\b)"));
	FRegexMatcher Matcher(Pattern, FileContent);
	return Matcher.FindNext();
}

void UProjectCleanerLibrary::ShowModal(const FString& Msg, const EProjectCleanerModalStatus ModalStatus, const float Duration)
{
	FNotificationInfo Info{FText::FromString(Msg)};
	Info.ExpireDuration = Duration;

	const auto NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
	if (!NotificationPtr.IsValid()) return;

	NotificationPtr.Get()->SetCompletionState(GetCompletionStateFromModalStatus(ModalStatus));
}

void UProjectCleanerLibrary::ShowModalOutputLog(const FString& Msg, const EProjectCleanerModalStatus ModalStatus, const float Duration)
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
