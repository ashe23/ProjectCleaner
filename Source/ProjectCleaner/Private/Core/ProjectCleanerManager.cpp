// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "Core/ProjectCleanerManager.h"
#include "Core/ProjectCleanerUtility.h"
#include "ProjectCleaner.h"
#include "UI/ProjectCleanerNotificationManager.h"
// Engine Headers
#include "AssetRegistryModule.h"
#include "Misc/ScopedSlowTask.h"
#include "Settings/ContentBrowserSettings.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "IDirectoryWatcher.h"
#include "DirectoryWatcherModule.h"
#include "Misc/FileHelper.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Engine/AssetManagerSettings.h"
#include "Engine/AssetManager.h"
#include "Internationalization/Regex.h"

#define LOCTEXT_NAMESPACE "FProjectCleanerModule"
// ProjectCleanerManager::ProjectCleanerManager()
// {
// 	CleanerConfigs = GetMutableDefault<UCleanerConfigs>();
// 	ExcludeOptions = GetMutableDefault<UExcludeOptions>();
//
// 	ensure(CleanerConfigs && ExcludeOptions);
// }
//
// FProjectCleanerData* ProjectCleanerManager::GetCleanerData()
// {
// 	return &CleanerData;
// }
//
// UCleanerConfigs* ProjectCleanerManager::GetCleanerConfigs() const
// {
// 	return CleanerConfigs;
// }
//
// UExcludeOptions* ProjectCleanerManager::GetExcludeOptions() const
// {
// 	return ExcludeOptions;
// }
//
// TArray<FAssetNode> const* ProjectCleanerManager::GetAdjacencyList() const
// {
// 	return &AdjacencyList;
// }
//
// void ProjectCleanerManager::UpdateData()
// {
// 	// FScopedSlowTask SlowTask{ 1.0f, FText::FromString("Scanning...") };
// 	// SlowTask.MakeDialog();
//
// 	ProjectCleanerUtility::FixupRedirectors();
// 	ProjectCleanerUtility::SaveAllAssets();
// 	UpdateAssetRegistry();
// 	
// 	CleanerData.Empty();
// 	AdjacencyList.Empty();
// 	
// 	ProjectCleanerUtility::GetPrimaryAssetClasses(CleanerData.PrimaryAssetClasses);
// 	ProjectCleanerUtility::GetEmptyFolders(CleanerData.EmptyFolders, CleanerConfigs->bScanDeveloperContents);
// 	ProjectCleanerUtility::GetInvalidFiles(CleanerData.CorruptedFiles, CleanerData.NonEngineFiles);
// 	ProjectCleanerUtility::GetUnusedAssets(CleanerData.UnusedAssets);
// 	// ProjectCleanerUtility::FindAssetsUsedIndirectly(CleanerData.UnusedAssets, CleanerData.IndirectFileInfos);// todo:ashe23 optimization candidate #1
//
// 	// GenerateAdjacencyList();// todo:ashe23 optimization candidate #2
// 	// RemoveAssetsWithExternalReferencers();
// 	// RemoveAssetsFromDeveloperFolder();
// 	// RemoveIndirectAssets();
// 	// RemoveExcludedAssets();
// 	
// 	CleanerData.TotalSize = ProjectCleanerUtility::GetTotalSize(CleanerData.UnusedAssets);
//
// 	auto ContentBrowserSettings = GetMutableDefault<UContentBrowserSettings>();
// 	ContentBrowserSettings->SetDisplayDevelopersFolder(CleanerConfigs->bScanDeveloperContents, true);
// 	ContentBrowserSettings->PostEditChange();
//
// 	// SlowTask.EnterProgressFrame(1.0f);
// }
//
// void ProjectCleanerManager::DeleteUnusedAssets()
// {
// 	if (CleanerData.UnusedAssets.Num() == 0) return;
//
// 	TArray<FAssetData> Chunk;
// 	Chunk.Reserve(CleanerConfigs->DeleteChunkLimit);
//
// 	const auto ProgressNotification = ProjectCleanerNotificationManager::Add(
// 		FStandardCleanerText::StartingCleanup,
// 		SNotificationItem::ECompletionState::CS_Pending
// 	);
//
// 	CleanerData.TotalAssetsNum = CleanerData.UnusedAssets.Num();
//
// 	while (AdjacencyList.Num() != 0)
// 	{
// 		GetAssetsChunk(Chunk);
// 		CleanerData.DeletedAssetsNum += ProjectCleanerUtility::DeleteAssets(Chunk);
// 	
// 		const uint32 Percent = CleanerData.TotalAssetsNum > 0 ?
// 			(CleanerData.DeletedAssetsNum * 100.0f) / CleanerData.TotalAssetsNum : 0;
// 		const FText ProgressText = FText::FromString(
// 			FString::Printf(
// 				TEXT("Deleted %d of %d assets. %d %%"),
// 				CleanerData.DeletedAssetsNum,
// 				CleanerData.TotalAssetsNum,
// 				Percent
// 			)
// 		);
// 		
// 		ProjectCleanerNotificationManager::Update(ProgressNotification, ProgressText);
//
// 		CleanerData.UnusedAssets.RemoveAllSwap([&] (const FAssetData& Elem)
// 		{
// 			return Chunk.Contains(Elem);
// 		});
//
// 		GenerateAdjacencyList();
// 		
// 		Chunk.Reset();
// 	}
//
// 	const FString PostFixText = CleanerData.DeletedAssetsNum > 1 ? TEXT(" assets") : TEXT(" asset");
// 	const FString FinalText = FString{ "Deleted " } + FString::FromInt(CleanerData.DeletedAssetsNum) + PostFixText;
// 	ProjectCleanerNotificationManager::Hide(ProgressNotification, FText::FromString(FinalText));
// 	
// 	UpdateData();
// 	
// 	if (CleanerConfigs->bAutomaticallyDeleteEmptyFolders)
// 	{
// 		DeleteEmptyFolders();
// 	}
// 	else
// 	{
// 		UpdateAssetRegistry();
// 		FocusOnGameFolder();
// 	}
// }
//
// void ProjectCleanerManager::DeleteEmptyFolders()
// {
// 	if (CleanerData.EmptyFolders.Num() == 0) return;
// 	
// 	const FString PostFixText = CleanerData.EmptyFolders.Num() > 1 ? TEXT(" empty folders") : TEXT(" empty folder");
//  	const FString DisplayText = FString{ "Deleted " } + FString::FromInt(CleanerData.EmptyFolders.Num()) + PostFixText;
//  	
//  	if (ProjectCleanerUtility::DeleteEmptyFolders(CleanerData.EmptyFolders))
//  	{
//  		ProjectCleanerNotificationManager::AddTransient(
//  			FText::FromString(DisplayText),
//  			SNotificationItem::ECompletionState::CS_Success,
//  			5.0f
//  		);
//  	}
//  	else
//  	{
//  		ProjectCleanerNotificationManager::AddTransient(
//  			FText::FromString(FStandardCleanerText::FailedToDeleteSomeFolders),
//  			SNotificationItem::ECompletionState::CS_Fail,
//  			5.0f
//  		);
//  	}
//  
//  	UpdateAssetRegistry();
// 	FocusOnGameFolder();
// }
//
// void ProjectCleanerManager::UpdateAssetRegistry() const
// {
// 	FAssetRegistryModule& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
// 	
// 	TArray<FString> ScanFolders;
// 	ScanFolders.Add("/Game");
//
// 	AssetRegistry.Get().ScanPathsSynchronous(ScanFolders, true);
// 	AssetRegistry.Get().SearchAllAssets(false);
// }
//
// void ProjectCleanerManager::FocusOnGameFolder()
// {
// 	TArray<FString> FocusFolders;
// 	FocusFolders.Add("/Game");
// 	
// 	FContentBrowserModule& ContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
// 	ContentBrowser.Get().SetSelectedPaths(FocusFolders, true);
// }
//
// void ProjectCleanerManager::GetRelatedAssets(const FName& PackageName, const ERelationType RelationType, TArray<FName>& RelatedAssets) const
// {
// 	FAssetRegistryModule& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
//
// 	if (RelationType == ERelationType::Reference)
// 	{
// 		AssetRegistry.Get().GetReferencers(PackageName, RelatedAssets);
// 	}
// 	else
// 	{
// 		AssetRegistry.Get().GetDependencies(PackageName, RelatedAssets);
// 	}
//
// 	RelatedAssets.RemoveAll([&] (const FName& Elem)
// 	{
// 		return Elem.IsEqual(PackageName);
// 	});
// }
//
// void ProjectCleanerManager::GetLinkedAssets(FAssetNode& Node, FAssetNode& RootNode, TSet<FName>& Visited)
// {
// 	if (Visited.Contains(Node.AssetData.AssetName)) return;
//
// 	Visited.Add(Node.AssetData.AssetName);
//
// 	// Recursively traversing dependencies
// 	for (const auto& Dep : Node.Deps)
// 	{
// 		const auto& DepAsset = FindByPackageName(Dep);
// 		if (!DepAsset) continue;
// 		RootNode.LinkedAssets.AddUnique(DepAsset->AssetData);
// 		GetLinkedAssets(*DepAsset, RootNode, Visited);
// 	}
// 	// Recursively traversing referencers
// 	for (const auto& Ref : Node.Refs)
// 	{
// 		const auto& RefAsset = FindByPackageName(Ref);
// 		if (!RefAsset) continue;
// 		RootNode.LinkedAssets.AddUnique(RefAsset->AssetData);
// 		GetLinkedAssets(*RefAsset, RootNode, Visited);
// 	}
// }
//
// FAssetNode* ProjectCleanerManager::FindByPackageName(const FName& PackageName)
// {
// 	return AdjacencyList.FindByPredicate([&](const FAssetNode& Elem)
// 	{
// 		return Elem.AssetData.PackageName.IsEqual(PackageName);
// 	});
// }
//
// void ProjectCleanerManager::GenerateAdjacencyList()
// {
// 	AdjacencyList.Empty();
// 	
// 	for (const FAssetData& UnusedAsset : CleanerData.UnusedAssets)
// 	{
// 		FAssetNode Node;
// 		Node.AssetData = UnusedAsset;
// 		
// 		GetRelatedAssets(UnusedAsset.PackageName, ERelationType::Reference, Node.Refs);
// 		GetRelatedAssets(UnusedAsset.PackageName, ERelationType::Dependency, Node.Deps);
// 	
// 		AdjacencyList.Add(Node);
// 	}
//
// 	for (auto& Node : AdjacencyList)
// 	{
// 		TSet<FName> Visited;
// 		GetLinkedAssets(Node, Node, Visited);
// 		Visited.Reset();
// 	}
// }
//
// bool ProjectCleanerManager::IsExcludedByPath(const FAssetData& AssetData)
// {
// 	if (!ExcludeOptions) return false;
// 	
// 	return ExcludeOptions->Paths.ContainsByPredicate([&](const FDirectoryPath& DirectoryPath)
// 	{
// 		return AssetData.PackagePath.ToString().StartsWith(DirectoryPath.Path);
// 	});
// }
//
// bool ProjectCleanerManager::IsExcludedByClass(const FAssetData& AssetData)
// {
// 	if (!ExcludeOptions) return false;
// 	
// 	const UBlueprint* BlueprintAsset = Cast<UBlueprint>(AssetData.GetAsset());
// 	const bool IsBlueprint = (BlueprintAsset != nullptr);
//
// 	FName ClassName;
// 	FName ParentClassName;
//
// 	if (IsBlueprint && BlueprintAsset->GeneratedClass && BlueprintAsset->ParentClass)
// 	{
// 		ClassName = BlueprintAsset->GeneratedClass->GetFName();
// 		ParentClassName = BlueprintAsset->ParentClass->GetFName();
// 	}
// 	else
// 	{
// 		ClassName = AssetData.AssetClass;
// 	}
// 	
// 	return ExcludeOptions->Classes.ContainsByPredicate([&](const UClass* ElemClass)
// 	{
// 		if (!ElemClass) return false;
// 		return ClassName.IsEqual(ElemClass->GetFName()) ||
// 		(IsBlueprint &&
// 			(
// 				ClassName.IsEqual(ElemClass->GetFName()) ||
// 				ParentClassName.IsEqual(ElemClass->GetFName()) ||
// 				ElemClass->GetFName().IsEqual(UBlueprint::StaticClass()->GetFName())
// 			)
// 		);
// 	});
// }
//
// bool ProjectCleanerManager::IsCircular(const FAssetNode& AssetNode)
// {
// 	return AssetNode.Refs.ContainsByPredicate([&] (const FName& Elem)
// 	{
// 		return AssetNode.Deps.Contains(Elem);
// 	});
// }
//
// bool ProjectCleanerManager::HasReferencers(const FAssetNode& AssetNode)
// {
// 	return AssetNode.Refs.Num() > 0;
// }
//
// bool ProjectCleanerManager::IsUnderDeveloperFolder(const FString& PackagePath) const
// {
// 	const FString InPath = ProjectCleanerUtility::ConvertInternalToAbsolutePath(PackagePath);
// 	const FString DeveloperFolderPath = FPaths::ConvertRelativePathToFull(FPaths::GameDevelopersDir());
// 	const FString CollectionsFolderPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + TEXT("Collections/"));
//
// 	return
// 		FPaths::IsUnderDirectory(InPath, DeveloperFolderPath) ||
// 		FPaths::IsUnderDirectory(InPath, CollectionsFolderPath);
// }
//
// bool ProjectCleanerManager::HasReferencersInDeveloperFolder(const FAssetNode& AssetNode) const
// {
// 	for (const auto& Ref : AssetNode.Refs)
// 	{
// 		if (IsUnderDeveloperFolder(Ref.ToString()))
// 		{
// 			return true;
// 		}
// 	}
//
// 	return false;
// }
//
// bool ProjectCleanerManager::HasExternalReferencers(const FAssetNode& AssetNode)
// {
// 	return AssetNode.Refs.ContainsByPredicate([&] (const FName& Elem)
// 	{
// 		return !Elem.ToString().StartsWith("/Game");
// 	});
// }
//
// void ProjectCleanerManager::RemoveAssetsWithExternalReferencers()
// {
// 	TSet<FAssetData> FilteredAssets;
// 	FilteredAssets.Reserve(AdjacencyList.Num());
// 	
// 	for (const auto& AssetNode : AdjacencyList)
// 	{
// 		if (!HasExternalReferencers(AssetNode)) continue;
// 		FilteredAssets.Add(AssetNode.AssetData);
//
// 		for (const auto& LinkedAsset : AssetNode.LinkedAssets)
// 		{
// 			FilteredAssets.Add(LinkedAsset);
// 		}
// 	}
// 	
// 	CleanerData.UnusedAssets.RemoveAllSwap([&] (const FAssetData& Elem)
// 	{
// 		return FilteredAssets.Contains(Elem);
// 	});
//
// 	AdjacencyList.RemoveAllSwap([&](const FAssetNode& Elem)
// 	{
// 		return FilteredAssets.Contains(Elem.AssetData);
// 	});
// }
//
//
// void ProjectCleanerManager::RemoveAssetsFromDeveloperFolder()
// {
// 	for (const auto& Node : AdjacencyList)
// 	{
// 		if (!IsUnderDeveloperFolder(Node.AssetData.PackagePath.ToString()) && HasReferencersInDeveloperFolder(Node))
// 		{
// 			ProjectCleanerNotificationManager::AddTransient(
// 				FText::FromString(FStandardCleanerText::SomeAssetsHaveRefsInDevFolder),
// 				SNotificationItem::ECompletionState::CS_None,
// 				5.0f
// 			);
// 			CleanerConfigs->bScanDeveloperContents = true;
// 			break;
// 		}
// 	}
// 	
// 	if (!CleanerConfigs->bScanDeveloperContents)
// 	{
// 		CleanerData.UnusedAssets.RemoveAllSwap([&](const FAssetData& Elem) {
// 			return IsUnderDeveloperFolder(Elem.PackagePath.ToString());
// 		});
//
// 		AdjacencyList.RemoveAllSwap([&] (const FAssetNode& Elem)
// 		{
// 			return IsUnderDeveloperFolder(Elem.AssetData.PackagePath.ToString());
// 		});
// 	}
// }
//
// void ProjectCleanerManager::RemoveIndirectAssets()
// {
// 	TSet<FAssetData> FilteredAssets;
// 	FilteredAssets.Reserve(CleanerData.UnusedAssets.Num());
// 	
// 	for (const auto& IndirectFileInfo : CleanerData.IndirectFileInfos)
// 	{
// 		FilteredAssets.Add(IndirectFileInfo.AssetData);
//
// 		FAssetNode* Node = FindByPackageName(IndirectFileInfo.AssetData.PackageName);
// 		if (!Node) continue;
// 		
// 		for (const auto& LinkedAsset : Node->LinkedAssets)
// 		{
// 			FilteredAssets.Add(LinkedAsset);
// 		}
// 	}
//
// 	FilteredAssets.Shrink();
//
// 	CleanerData.UnusedAssets.RemoveAllSwap([&] (const FAssetData& Elem)
// 	{
// 		return FilteredAssets.Contains(Elem);
// 	});
//
// 	AdjacencyList.RemoveAllSwap([&] (const FAssetNode& Elem)
// 	{
// 		return FilteredAssets.Contains(Elem.AssetData);
// 	});
// }
//
// void ProjectCleanerManager::RemoveExcludedAssets()
// {
// 	// excluded by user
// 	for (const auto& Asset : CleanerData.UserExcludedAssets)
// 	{
// 		CleanerData.ExcludedAssets.AddUnique(Asset);
// 	}
//
// 	// excluded by path or class
// 	for (const auto& Asset : CleanerData.UnusedAssets)
// 	{
// 		if (!IsExcludedByPath(Asset) && !IsExcludedByClass(Asset)) continue;
// 		
// 		CleanerData.ExcludedAssets.AddUnique(Asset);
// 	}
//
// 	for (const auto& ExcludedAsset : CleanerData.ExcludedAssets)
// 	{
// 		FAssetNode* Node = FindByPackageName(ExcludedAsset.PackageName);
// 		if (!Node) continue;
// 		
// 		for (const auto& LinkedAsset : Node->LinkedAssets)
// 		{
// 			CleanerData.LinkedAssets.Add(LinkedAsset);
// 		}
// 	}
//
// 	CleanerData.LinkedAssets.RemoveAllSwap([&] (const FAssetData& Elem)
// 	{
// 		return CleanerData.ExcludedAssets.Contains(Elem);
// 	});
// 	
// 	CleanerData.UnusedAssets.RemoveAllSwap([&] (const FAssetData& Elem)
// 	{
// 		return CleanerData.ExcludedAssets.Contains(Elem) || CleanerData.LinkedAssets.Contains(Elem);
// 	});
//
// 	AdjacencyList.RemoveAllSwap([&] (const FAssetNode& Elem)
// 	{
// 		return CleanerData.ExcludedAssets.Contains(Elem.AssetData) || CleanerData.LinkedAssets.Contains(Elem.AssetData);
// 	});
// }
//
// void ProjectCleanerManager::GetAssetsChunk(TArray<FAssetData>& Chunk)
// {
// 	Chunk.Reserve(CleanerData.UnusedAssets.Num());
// 	// 1) trying to find circular assets
// 	for (const auto& AssetNode : AdjacencyList)
// 	{
// 		if (IsCircular(AssetNode))
// 		{
// 			Chunk.Add(AssetNode.AssetData);
// 		}
// 	}
// 	
// 	if (Chunk.Num() > 0) return;
// 	
// 	// 2) trying to find assets without referencers
// 	for (const auto& AssetNode : AdjacencyList)
// 	{
// 		if (Chunk.Num() > CleanerConfigs->DeleteChunkLimit) break;
// 		if (!HasReferencers(AssetNode))
// 		{
// 			Chunk.Add(AssetNode.AssetData);
// 		}
// 	}
// 	
// 	if (Chunk.Num() > 0) return;
// 	
// 	for (const auto& AssetNode : AdjacencyList)
// 	{
// 		if (Chunk.Num() > CleanerConfigs->DeleteChunkLimit) break;
// 		Chunk.Add(AssetNode.AssetData);
// 	}
// }

FName ProjectCleanerManager::GameUserDeveloperDir = FName{FPaths::GameUserDeveloperDir()};
FName ProjectCleanerManager::GameDevelopersDir = FName{FPaths::GameUserDeveloperDir() };
FName ProjectCleanerManager::GameUserDeveloperCollectionsDir = FName{FPaths::GameUserDeveloperDir() + TEXT("Collections")};
FName ProjectCleanerManager::CollectionsDir = FName{FPaths::ProjectContentDir() + TEXT("Collections")};

ProjectCleanerManager::ProjectCleanerManager() :
	CleanerConfigs(nullptr),
	ExcludeOptions(nullptr),
	AssetRegistry(nullptr),
	DirectoryWatcher(nullptr)
{
	AssetRegistry = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	DirectoryWatcher = &FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));

	CleanerConfigs = GetMutableDefault<UCleanerConfigs>();
	ExcludeOptions = GetMutableDefault<UExcludeOptions>();

	ensure(CleanerConfigs);
	ensure(ExcludeOptions);
	ensure(AssetRegistry);
	ensure(DirectoryWatcher);
}

void ProjectCleanerManager::UpdateData()
{
	if (AssetRegistry->Get().IsLoadingAssets())
	{
		FNotificationInfo Info{FText::FromString(FStandardCleanerText::AssetRegistryStillWorking)};
		Info.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
		
		return;
	}
	
	ProjectCleanerUtility::FixupRedirectors();
	ProjectCleanerUtility::SaveAllAssets(true);
	
	if (!bInitialLoading)
	{
		// Excluded assets calculation must be here
		// todo:ashe23 just update exclusion data
		return;
	}
	
	CleanerData.Empty();

	FScopedSlowTask SlowTask{ 3.0f, LOCTEXT("scanning_project_title", "Scanning. Please wait...") };
	SlowTask.MakeDialog(true);
		
	LoadInitialData();
	GetAllAssets();
	FindPrimaryAssetsAndItsDependencies();
	FindCorruptedAssets();

	SlowTask.EnterProgressFrame();

	// indirectly used assets
	FScopedSlowTask IndirectlyUsedAssetsTask(
		CleanerData.ProjectSourceAndConfigsFiles.Num(),
		LOCTEXT("scanning_for_indirectlyUsedAssets", "Looking for indirectly used assets")
	);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	for (const auto& File : CleanerData.ProjectSourceAndConfigsFiles)
	{
		IndirectlyUsedAssetsTask.EnterProgressFrame();
		if (!PlatformFile.FileExists(*File)) continue;
	
		FString FileContent;
		FFileHelper::LoadFileToString(FileContent, *File);

		// search any sub string that has asset package path in it
		static FRegexPattern Pattern(TEXT(R"(\/Game(.*)\b)"));
		FRegexMatcher Matcher(Pattern, FileContent);
		if (!Matcher.FindNext()) continue;
		const FName FoundedAssetPackageName =  FName{Matcher.GetCaptureGroup(0)};

		// if founded asset exist in AssetRegistry database, then we finding line number where its used
		// if (!ObjectPaths.Contains(FoundedAssetPackageName)) continue;
		const FAssetData* AssetData = CleanerData.ProjectAllAssets.FindByPredicate([&] (const FAssetData& Elem)
		{
			return Elem.ObjectPath.IsEqual(FoundedAssetPackageName);
		});
		if (!AssetData) continue;

		TArray<FString> Lines;
		FFileHelper::LoadFileToStringArray(Lines, *File);
		for (int32 i = 0; i < Lines.Num(); ++i)
		{
			if (!Lines.IsValidIndex(i)) continue;
			if (!Lines[i].Contains(FoundedAssetPackageName.ToString())) continue;

			FIndirectAsset IndirectAsset;
			IndirectAsset.File = FPaths::ConvertRelativePathToFull(File);
			IndirectAsset.Line = i + 1;
			CleanerData.IndirectlyUsedAssets.Add(*AssetData, IndirectAsset);
		}
	}

	SlowTask.EnterProgressFrame(1.0f);
	
	GetUnusedAssets();
	CleanerData.TotalSize = ProjectCleanerUtility::GetTotalSize(CleanerData.UnusedAssets);

	SlowTask.EnterProgressFrame(1.0f);

	//todo:ashe23 Before change cache ContentBrowser settings and when user closed cleaner tab, return to old state
	auto ContentBrowserSettings = GetMutableDefault<UContentBrowserSettings>();
	ContentBrowserSettings->SetDisplayDevelopersFolder(CleanerConfigs->bScanDeveloperContents, true);
	ContentBrowserSettings->PostEditChange();
	bInitialLoading = false;
	// todo:ashe23 register delegates
}

FProjectCleanerData& ProjectCleanerManager::GetCleanerData()
{
	return CleanerData;
}

UCleanerConfigs* ProjectCleanerManager::GetCleanerConfigs() const
{
	return CleanerConfigs;
}

UExcludeOptions* ProjectCleanerManager::GetExcludeOptions() const
{
	return ExcludeOptions;
}

void ProjectCleanerManager::LoadInitialData()
{
	FindEmptyFoldersAndNonEngineFiles();
	FindSourceAndConfigFiles();
}

void ProjectCleanerManager::FindEmptyFoldersAndNonEngineFiles()
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	
	struct DirectoryVisitor : IPlatformFile::FDirectoryVisitor
	{
		DirectoryVisitor(TSet<FName>& NonEngine, TSet<FName>& EmpFolders, TSet<FName>& Files) :
			NonEngineFiles(NonEngine),
			EmptyFolders(EmpFolders),
			AllFiles(Files)
		{}
		
		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			const FString FullPath = FPaths::ConvertRelativePathToFull(FilenameOrDirectory);
			if (bIsDirectory)
			{
				if (IsEmpty(FullPath))
				{
					EmptyFolders.Add(FName{FullPath});
				}
			}
			else
			{
				if (!ProjectCleanerUtility::IsEngineExtension(FPaths::GetExtension(FullPath, false)))
				{
					NonEngineFiles.Add(FName{FullPath});
				}
				AllFiles.Add(FName{FullPath});
			}

			return true;
		}

		static bool IsEmpty(const FString& InPath)
		{
			const FString SearchPath = InPath / TEXT("*");
			TArray<FString> Files;
			IFileManager::Get().FindFiles(Files, *SearchPath, true, true);
			return Files.Num() == 0;
		}

		static bool HasFiles(const FString& InPath)
		{
			const FString SearchPath = InPath / TEXT("*");
			TArray<FString> Files;
			IFileManager::Get().FindFiles(Files, *SearchPath, true, false);
			return Files.Num() != 0;
		}

		TSet<FName>& NonEngineFiles;
		TSet<FName>& EmptyFolders;
		TSet<FName>& AllFiles;
	};

	DirectoryVisitor Visitor{CleanerData.NonEngineFiles, CleanerData.EmptyFolders, CleanerData.ProjectAllFiles};
	PlatformFile.IterateDirectoryRecursively(*FPaths::ProjectContentDir(), Visitor);

	// removing some paths
	CleanerData.EmptyFolders.Remove(GameUserDeveloperDir);
	CleanerData.EmptyFolders.Remove(GameUserDeveloperCollectionsDir);
	CleanerData.EmptyFolders.Remove(GameDevelopersDir);
	CleanerData.EmptyFolders.Remove(CollectionsDir);
	
	// finding empty folder trees
	TSet<FName> NewEmptyFolders;
	for (const auto& EmptyFolder : CleanerData.EmptyFolders)
	{
		FString EmptyFolderPath = EmptyFolder.ToString();

		FString CurrentPath;
		EmptyFolderPath.Split(TEXT("/"), &CurrentPath, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		CurrentPath += TEXT("/");

		while (!DirectoryVisitor::HasFiles(CurrentPath) || !CurrentPath.Equals(FPaths::ProjectContentDir()))
		{
			CurrentPath.RemoveFromEnd(TEXT("/"));
			NewEmptyFolders.Add(FName{CurrentPath});
			
			FString Parent;
			CurrentPath.Split(TEXT("/"), &Parent, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
			CurrentPath = Parent;
			CurrentPath += TEXT("/");
		}
	}

	for (const auto& Folder : NewEmptyFolders)
	{
		CleanerData.EmptyFolders.Add(Folder);
	}
}

void ProjectCleanerManager::FindPrimaryAssetsAndItsDependencies()
{
	GetPrimaryAssetClasses();
	
	CleanerData.UsedAssets.Reserve(CleanerData.ProjectAllAssets.Num());
	
	FARFilter Filter;
	Filter.ClassNames.Add(UWorld::StaticClass()->GetFName());
	Filter.ClassNames.Append(CleanerData.PrimaryAssetClasses.Array());
	Filter.PackagePaths.Add("/Game");
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	
	TArray<FName> PackageNamesToProcess;
	{
		TArray<FAssetData> FoundAssets;
		AssetRegistry->Get().GetAssets(Filter, FoundAssets);
		for (const FAssetData& AssetData : FoundAssets)
		{
			PackageNamesToProcess.Add(AssetData.PackageName);
			CleanerData.UsedAssets.Add(AssetData.PackageName);
		}
	}

	TArray<FAssetIdentifier> AssetDependencies;
	while (PackageNamesToProcess.Num() > 0)
	{
		const FName PackageName = PackageNamesToProcess.Pop(false);
		AssetDependencies.Reset();
		AssetRegistry->Get().GetDependencies(FAssetIdentifier(PackageName), AssetDependencies);
		for (const FAssetIdentifier& Dependency : AssetDependencies)
		{
			bool bIsAlreadyInSet = false;
			CleanerData.UsedAssets.Add(Dependency.PackageName, &bIsAlreadyInSet);
			if (!bIsAlreadyInSet && Dependency.IsValid())
			{
				PackageNamesToProcess.Add(Dependency.PackageName);
			}
		}
	}
}

void ProjectCleanerManager::FindCorruptedAssets()
{
	TSet<FName> ObjectPaths;
	ObjectPaths.Reserve(CleanerData.ProjectAllAssets.Num());
	for (const auto& AssetData : CleanerData.ProjectAllAssets)
	{
		ObjectPaths.Add(AssetData.ObjectPath);
	}
	
	for (const auto& File : CleanerData.ProjectAllFiles)
	{
		if (!ProjectCleanerUtility::IsEngineExtension(FPaths::GetExtension(File.ToString(), false))) continue;
		
		// here we got absolute path "C:/MyProject/Content/material.uasset"
		// we must first convert that path to In Engine Internal Path like "/Game/material.uasset"
		const FString InternalFilePath = ProjectCleanerUtility::ConvertAbsolutePathToInternal(File.ToString());
		// Converting file path to object path (This is for searching in AssetRegistry)
		// example "/Game/Name.uasset" => "/Game/Name.Name"
		FString ObjectPath = InternalFilePath;
		ObjectPath.RemoveFromEnd(FPaths::GetExtension(InternalFilePath, true));
		ObjectPath.Append(TEXT(".") + FPaths::GetBaseFilename(InternalFilePath));
		
		if (!ObjectPaths.Contains(FName{ObjectPath}))
		{
			CleanerData.CorruptedAssets.Add(File);
		}
	}
}

void ProjectCleanerManager::FindLinkedAssets(TSet<FName>& LinkedAssets)
{
	TArray<FName> Stack;
	for (const auto& Asset : CleanerData.IndirectlyUsedAssets)
	{
		FAssetNode Node;
		AssetRegistry->Get().GetReferencers(Asset.Key.PackageName, Node.Refs);
		AssetRegistry->Get().GetDependencies(Asset.Key.PackageName, Node.Deps);
	
		Node.Refs.Remove(Asset.Key.PackageName);
		Node.Deps.Remove(Asset.Key.PackageName);
	
		Stack.Add(Asset.Key.PackageName);
		
		TArray<FAssetIdentifier> AssetDependencies;
		TArray<FAssetIdentifier> AssetReferencers;
		
		while (Stack.Num() > 0)
		{
			const FName PackageName = Stack.Pop(false);
	
			AssetDependencies.Reset();
			AssetReferencers.Reset();
			AssetRegistry->Get().GetDependencies(FAssetIdentifier(PackageName), AssetDependencies);
			AssetRegistry->Get().GetReferencers(FAssetIdentifier(PackageName), AssetReferencers);
	
			AssetDependencies.RemoveAllSwap([&](const FAssetIdentifier& Elem)
			{
				return !Elem.PackageName.ToString().StartsWith("/Game");
			});
			AssetReferencers.RemoveAllSwap([&](const FAssetIdentifier& Elem)
			{
				return !Elem.PackageName.ToString().StartsWith("/Game");
			});
			
			for (const auto Dep : AssetDependencies)
			{
				bool bIsAlreadyInSet = false;
				LinkedAssets.Add(Dep.PackageName, &bIsAlreadyInSet);
				if (!bIsAlreadyInSet && Dep.IsValid())
				{
					Stack.Add(Dep.PackageName);
				}
			}
			for (const auto Ref : AssetReferencers)
			{
				bool bIsAlreadyInSet = false;
				LinkedAssets.Add(Ref.PackageName, &bIsAlreadyInSet);
				if (!bIsAlreadyInSet && Ref.IsValid())
				{
					Stack.Add(Ref.PackageName);
				}
			}
		}
	}
}

void ProjectCleanerManager::FindSourceAndConfigFiles()
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	
	CleanerData.ProjectSourceAndConfigsFiles.Reserve(200); // reserving some space
	
	// 1) finding all source files in main project "Source" directory (<yourproject>/Source/*)
	TArray<FString> FilesToScan;
	PlatformFile.FindFilesRecursively(FilesToScan, *FPaths::GameSourceDir(), TEXT(".cs"));
	PlatformFile.FindFilesRecursively(FilesToScan, *FPaths::GameSourceDir(), TEXT(".cpp"));
	PlatformFile.FindFilesRecursively(FilesToScan, *FPaths::GameSourceDir(), TEXT(".h"));
	PlatformFile.FindFilesRecursively(FilesToScan, *FPaths::ProjectConfigDir(), TEXT(".ini"));
	CleanerData.ProjectSourceAndConfigsFiles.Append(FilesToScan);
	
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
	PlatformFile.IterateDirectory(*FPaths::ProjectPluginsDir(), Visitor);
	
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
	
	CleanerData.ProjectSourceAndConfigsFiles.Append(ProjectPluginsFiles);
	CleanerData.ProjectSourceAndConfigsFiles.Shrink();
}

void ProjectCleanerManager::GetAllAssets()
{
	CleanerData.ProjectAllAssets.Reserve(CleanerData.ProjectAllFiles.Num());
	AssetRegistry->Get().GetAssetsByPath(FName{ "/Game" }, CleanerData.ProjectAllAssets, true);
}

void ProjectCleanerManager::GetUnusedAssets()
{
	TSet<FName> UsedAssets;
	UsedAssets.Reserve(CleanerData.ProjectAllAssets.Num());
	
	FARFilter Filter;
	Filter.ClassNames.Add(UWorld::StaticClass()->GetFName());
	Filter.ClassNames.Append(CleanerData.PrimaryAssetClasses.Array());
	Filter.PackagePaths.Add("/Game");
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	
	TArray<FName> PackageNamesToProcess;
	{
		TArray<FAssetData> FoundAssets;
		AssetRegistry->Get().GetAssets(Filter, FoundAssets);
		for (const FAssetData& AssetData : FoundAssets)
		{
			PackageNamesToProcess.Add(AssetData.PackageName);
			UsedAssets.Add(AssetData.PackageName);
		}
	}

	TArray<FAssetIdentifier> AssetDependencies;
	while (PackageNamesToProcess.Num() > 0)
	{
		const FName PackageName = PackageNamesToProcess.Pop(false);
		AssetDependencies.Reset();
		AssetRegistry->Get().GetDependencies(FAssetIdentifier(PackageName), AssetDependencies);
		for (const FAssetIdentifier& Dependency : AssetDependencies)
		{
			bool bIsAlreadyInSet = false;
			UsedAssets.Add(Dependency.PackageName, &bIsAlreadyInSet);
			if (!bIsAlreadyInSet && Dependency.IsValid())
			{
				PackageNamesToProcess.Add(Dependency.PackageName);
			}
		}
	}
	
	CleanerData.UnusedAssets.Reserve(CleanerData.ProjectAllAssets.Num());
	for (const auto& Asset : CleanerData.ProjectAllAssets)
	{
		if (UsedAssets.Contains(Asset.PackageName) || !Asset.PackagePath.ToString().StartsWith("/Game")) continue;
		CleanerData.UnusedAssets.AddUnique(Asset);
	}

	// todo:ashe23 make as option?
	const bool IsMegascansLoaded = FModuleManager::Get().IsModuleLoaded("MegascansPlugin");
	if (!IsMegascansLoaded) return;
	
	CleanerData.UnusedAssets.RemoveAllSwap([&](const FAssetData& Elem)
	{
		return FPaths::IsUnderDirectory(Elem.PackagePath.ToString(), TEXT("/Game/MSPresets"));
	}, false);
	CleanerData.UnusedAssets.Shrink();

	TSet<FName> FilteredAssets;
	FindLinkedAssets(FilteredAssets);

	CleanerData.UnusedAssets.RemoveAllSwap([&] (const FAssetData& Elem)
	{
		return FilteredAssets.Contains(Elem.PackageName);
	});
}

void ProjectCleanerManager::GetPrimaryAssetClasses()
{
	const auto& AssetManager = UAssetManager::Get();
	if (!AssetManager.IsValid()) return;

	CleanerData.PrimaryAssetClasses.Reserve(10);
	
	const UAssetManagerSettings& Settings = AssetManager.GetSettings();
	TArray<FPrimaryAssetId> Ids;
	for (const auto& Type : Settings.PrimaryAssetTypesToScan)
	{
		AssetManager.Get().GetPrimaryAssetIdList(Type.PrimaryAssetType, Ids);
		for(const auto& Id : Ids)
		{
			FAssetData Data;
			AssetManager.Get().GetPrimaryAssetData(Id, Data);
			if(!Data.IsValid()) continue;
			CleanerData.PrimaryAssetClasses.Add(Data.AssetClass);
		}
		Ids.Reset();
	}

	CleanerData.PrimaryAssetClasses.Shrink();
}

bool ProjectCleanerManager::IsExcludedByPath(const FAssetData& AssetData)
{
	if (!ExcludeOptions) return false;
	
	return ExcludeOptions->Paths.ContainsByPredicate([&](const FDirectoryPath& DirectoryPath)
	{
		return AssetData.PackagePath.ToString().StartsWith(DirectoryPath.Path);
	});
}

bool ProjectCleanerManager::IsExcludedByClass(const FAssetData& AssetData)
{
	if (!ExcludeOptions) return false;
	
	const UBlueprint* BlueprintAsset = Cast<UBlueprint>(AssetData.GetAsset());
	const bool IsBlueprint = (BlueprintAsset != nullptr);

	FName ClassName;
	FName ParentClassName;

	if (IsBlueprint && BlueprintAsset->GeneratedClass && BlueprintAsset->ParentClass)
	{
		ClassName = BlueprintAsset->GeneratedClass->GetFName();
		ParentClassName = BlueprintAsset->ParentClass->GetFName();
	}
	else
	{
		ClassName = AssetData.AssetClass;
	}
	
	return ExcludeOptions->Classes.ContainsByPredicate([&](const UClass* ElemClass)
	{
		if (!ElemClass) return false;
		return ClassName.IsEqual(ElemClass->GetFName()) ||
		(IsBlueprint &&
			(
				ClassName.IsEqual(ElemClass->GetFName()) ||
				ParentClassName.IsEqual(ElemClass->GetFName()) ||
				ElemClass->GetFName().IsEqual(UBlueprint::StaticClass()->GetFName())
			)
		);
	});
}

void ProjectCleanerManager::RegisterDelegates()
{
	// AssetRegistry->Get().OnAssetAdded().AddRaw(this, &FProjectCleanerModule::OnAssetAdded);
	// AssetRegistry->Get().OnAssetRemoved().AddRaw(this, &FProjectCleanerModule::OnAssetRemoved);
	// AssetRegistry->Get().OnAssetRenamed().AddRaw(this, &FProjectCleanerModule::OnAssetRenamed);
	// AssetRegistry->Get().OnAssetUpdated().AddRaw(this, &FProjectCleanerModule::OnAssetUpdated); // todo:ashe23 not sure, too often called
	
	// FDelegateHandle WatcherDelegate;
	// const uint32 Flags = 0;
	// const FString WorkingDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
	// UE_LOG(LogTemp, Warning, TEXT("Working Dir : %s"), *WorkingDir);
	// const auto Callback = IDirectoryWatcher::FDirectoryChanged::CreateRaw(this, &FProjectCleanerModule::OnDirectoryChanged);
	// DirectoryWatcher->Get()->RegisterDirectoryChangedCallback_Handle(WorkingDir, Callback, WatcherDelegate, Flags);
}

#undef LOCTEXT_NAMESPACE