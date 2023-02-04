// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerSubsystem.h"
#include "ProjectCleanerConstants.h"
// Engine Headers
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/ScopedSlowTask.h"
#include "EditorUtilityBlueprint.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "Engine/AssetManager.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"

UProjectCleanerSubsystem::UProjectCleanerSubsystem()
	:
	PlatformFile(&FPlatformFileManager::Get().GetPlatformFile()),
	ModuleAssetRegistry(&FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName)),
	ModuleAssetTools(&FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")))
{
}

void UProjectCleanerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UProjectCleanerSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

#if WITH_EDITOR
void UProjectCleanerSubsystem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();
}
#endif

void UProjectCleanerSubsystem::GetAssetsAll(TArray<FAssetData>& Assets) const
{
	if (ModuleAssetRegistry->Get().IsLoadingAssets()) return;

	Assets.Empty();

	ModuleAssetRegistry->Get().GetAssetsByPath(ProjectCleanerConstants::PathRelRoot, Assets, true);
}

void UProjectCleanerSubsystem::GetAssetsByFilter(TArray<FAssetData>& Assets, const FProjectCleanerAssetSearchFilter& SearchFilter) const
{
	if (ModuleAssetRegistry->Get().IsLoadingAssets()) return;
	if (SearchFilter.IsEmpty()) return;
	if (SearchFilter.PathsScan.Num() == 0) return;

	{
		FARFilter Filter;
		Filter.bRecursivePaths = SearchFilter.bRecursivePaths;

		for (const auto& ScanPath : SearchFilter.PathsScan)
		{
			const FString RelativePath = PathConvertToRel(ScanPath);
			if (RelativePath.IsEmpty()) continue;

			Filter.PackagePaths.Add(FName{*RelativePath});
		}

		if (Filter.PackagePaths.Num() == 0) return;

		ModuleAssetRegistry->Get().GetAssets(Filter, Assets);
	}

	if (SearchFilter.PathsExclude.Num() > 0)
	{
		FARFilter Filter;
		Filter.bRecursivePaths = true;

		for (const auto& ExcludePath : SearchFilter.PathsExclude)
		{
			const FString RelativePath = PathConvertToRel(ExcludePath);
			if (RelativePath.IsEmpty()) continue;

			Filter.PackagePaths.Add(FName{*RelativePath});
		}

		if (Filter.PackagePaths.Num() > 0)
		{
			ModuleAssetRegistry->Get().UseFilterToExcludeAssets(Assets, Filter);
		}
	}

	if (SearchFilter.AssetsExclude.Num() > 0)
	{
		FARFilter Filter;

		for (const auto& ExcludeAssetObjectPath : SearchFilter.AssetsExclude)
		{
			const FAssetData AssetData = ModuleAssetRegistry->Get().GetAssetByObjectPath(FName{*ExcludeAssetObjectPath});
			if (!AssetData.IsValid()) continue;

			Filter.ObjectPaths.Add(AssetData.ObjectPath);
		}

		if (Filter.ObjectPaths.Num() > 0)
		{
			ModuleAssetRegistry->Get().UseFilterToExcludeAssets(Assets, Filter);
		}
	}

	if (SearchFilter.ClassesExclude.Num() > 0)
	{
		TSet<FName> ClassNames;
		if (SearchFilter.bRecursiveClasses)
		{
			GetClassNamesDerived(SearchFilter.ClassesExclude, ClassNames);
		}
		else
		{
			ClassNames.Append(SearchFilter.ClassesExclude);
		}

		FARFilter Filter;

		for (const auto& Asset : Assets)
		{
			if (ClassNames.Contains(Asset.AssetClass) || ClassNames.Contains(GetAssetExactClassName(Asset)))
			{
				Filter.ObjectPaths.Add(Asset.ObjectPath);
			}
		}

		if (Filter.ObjectPaths.Num() > 0)
		{
			ModuleAssetRegistry->Get().UseFilterToExcludeAssets(Assets, Filter);
		}
	}

	if (SearchFilter.ClassesScan.Num() > 0)
	{
		TSet<FName> ClassNames;
		if (SearchFilter.bRecursiveClasses)
		{
			GetClassNamesDerived(SearchFilter.ClassesScan, ClassNames);
		}
		else
		{
			ClassNames.Append(SearchFilter.ClassesScan);
		}

		FARFilter Filter;

		for (const auto& Asset : Assets)
		{
			if (ClassNames.Contains(Asset.AssetClass) || ClassNames.Contains(GetAssetExactClassName(Asset)))
			{
				Filter.ObjectPaths.Add(Asset.ObjectPath);
			}
		}

		if (Filter.ObjectPaths.Num() > 0)
		{
			ModuleAssetRegistry->Get().RunAssetsThroughFilter(Assets, Filter);
		}
		else
		{
			Assets.Empty();
		}
	}
}


// void UProjectCleanerSubsystem::GetAssetsByPath(TArray<FAssetData>& Assets, const TArray<FName>& Paths, const TArray<FName>& ExcludePaths, const bool bRecursive) const
// {
// 	if (ModuleAssetRegistry->Get().IsLoadingAssets())
// 	{
// 		return;
// 	}
//
// 	if (Paths.Num() == 0) return;
//
// 	FARFilter Filter;
// 	Filter.bRecursivePaths = bRecursive;
//
// 	for (const auto& Path : Paths)
// 	{
// 		const FString PathRel = PathConvertToRel(Path.ToString());
// 		if (PathRel.IsEmpty()) continue;
//
// 		Filter.PackagePaths.Add(Path);
// 	}
//
// 	if (Filter.PackagePaths.Num() == 0) return;
//
// 	Assets.Empty();
// 	ModuleAssetRegistry->Get().GetAssets(Filter, Assets);
//
// 	if (ExcludePaths.Num() == 0) return;
//
// 	FARFilter FilterExclude;
// 	FilterExclude.bRecursivePaths = true;
//
// 	for (const auto& ExcludePath : ExcludePaths)
// 	{
// 		const FString PathRel = PathConvertToRel(ExcludePath.ToString());
// 		if (PathRel.IsEmpty()) continue;
//
// 		FilterExclude.PackagePaths.Add(ExcludePath);
// 	}
//
// 	if (FilterExclude.PackagePaths.Num() == 0) return;
//
// 	ModuleAssetRegistry->Get().UseFilterToExcludeAssets(Assets, FilterExclude);
// }
//
// void UProjectCleanerSubsystem::GetAssetsByClass(TArray<FAssetData>& Assets, const TArray<FName>& ClassNames, const TArray<FName>& ExcludeClassNames, const bool bIncludeDerivedClasses) const
// {
// 	if (ModuleAssetRegistry->Get().IsLoadingAssets())
// 	{
// 		ModuleAssetRegistry->Get().WaitForCompletion();
// 	}
//
// 	if (ClassNames.Num() == 0 && ExcludeClassNames.Num() == 0) return;
//
//
// 	TArray<FAssetData> AssetsAll;
// 	GetAssetsAll(AssetsAll);
//
//
// 	return;
// }

void UProjectCleanerSubsystem::GetAssetsPrimary(TArray<FAssetData>& AssetsPrimary) const
{
	if (ModuleAssetRegistry->Get().IsLoadingAssets())
	{
		ModuleAssetRegistry->Get().WaitForCompletion();
	}

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

	// getting list of primary assets classes that are derived from main primary assets
	TSet<FName> DerivedFromPrimaryAssets;
	{
		const TSet<FName> ExcludedClassNames;
		ModuleAssetRegistry->Get().GetDerivedClassNames(PrimaryAssetClasses, ExcludedClassNames, DerivedFromPrimaryAssets);
	}

	for (const auto& DerivedClassName : DerivedFromPrimaryAssets)
	{
		PrimaryAssetClasses.AddUnique(DerivedClassName);
	}

	FARFilter Filter;
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);

	for (const auto& ClassName : PrimaryAssetClasses)
	{
		Filter.ClassNames.Add(ClassName);
	}

	AssetsPrimary.Empty();
	AssetsPrimary.Reserve(PrimaryAssetClasses.Num());
	ModuleAssetRegistry->Get().GetAssets(Filter, AssetsPrimary);

	// getting primary blueprint assets
	FARFilter FilterBlueprint;
	FilterBlueprint.bRecursivePaths = true;
	FilterBlueprint.bRecursiveClasses = true;
	FilterBlueprint.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
	FilterBlueprint.ClassNames.Add(UBlueprint::StaticClass()->GetFName()); // todo:ashe23 add blueprint derived classes?

	TArray<FAssetData> BlueprintAssets;
	ModuleAssetRegistry->Get().GetAssets(FilterBlueprint, BlueprintAssets);

	AssetsPrimary.Reserve(AssetsPrimary.Num() + BlueprintAssets.Num());
	for (const auto& BlueprintAsset : BlueprintAssets)
	{
		const FName BlueprintClass = GetAssetExactClassName(BlueprintAsset);
		if (PrimaryAssetClasses.Contains(BlueprintClass))
		{
			AssetsPrimary.AddUnique(BlueprintAsset);
		}
	}
}

void UProjectCleanerSubsystem::GetAssetsIndirect(TArray<FAssetData>& AssetsIndirect) const
{
	TArray<FProjectCleanerIndirectAssetInfo> Infos;
	GetAssetsIndirectInfo(Infos);

	AssetsIndirect.Empty();
	AssetsIndirect.Reserve(Infos.Num());

	for (const auto& Info : Infos)
	{
		AssetsIndirect.AddUnique(Info.AssetData);
	}
}

void UProjectCleanerSubsystem::GetAssetsIndirectInfo(TArray<FProjectCleanerIndirectAssetInfo>& AssetsIndirectInfos) const
{
	AssetsIndirectInfos.Empty();

	if (ModuleAssetRegistry->Get().IsLoadingAssets())
	{
		ModuleAssetRegistry->Get().WaitForCompletion();
	}

	FScopedSlowTask SlowTask{
		2.0f,
		FText::FromString(TEXT("Scanning for indirect assets...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog();
	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Preparing scan directories...")));

	const FString SourceDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + TEXT("Source/"));
	const FString ConfigDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + TEXT("Config/"));
	const FString PluginsDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + TEXT("Plugins/"));

	TSet<FString> Files;
	Files.Reserve(200); // reserving some space

	// 1) finding all source files in main project "Source" directory (<yourproject>/Source/*)
	TArray<FString> FilesToScan;
	PlatformFile->FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".cs"));
	PlatformFile->FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".cpp"));
	PlatformFile->FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".h"));
	PlatformFile->FindFilesRecursively(FilesToScan, *ConfigDir, TEXT(".ini"));
	Files.Append(FilesToScan);

	// 2) we should find all source files in plugins folder (<yourproject>/Plugins/*)
	TArray<FString> ProjectPluginsFiles;
	// finding all installed plugins in "Plugins" directory
	struct FDirectoryVisitor : IPlatformFile::FDirectoryVisitor
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

	FDirectoryVisitor Visitor;
	PlatformFile->IterateDirectory(*PluginsDir, Visitor);

	// 3) for every installed plugin we scanning only "Source" and "Config" folders
	for (const auto& Dir : Visitor.InstalledPlugins)
	{
		const FString PluginSourcePathDir = Dir + "/Source";
		const FString PluginConfigPathDir = Dir + "/Config";

		PlatformFile->FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".cs"));
		PlatformFile->FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".cpp"));
		PlatformFile->FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".h"));
		PlatformFile->FindFilesRecursively(ProjectPluginsFiles, *PluginConfigPathDir, TEXT(".ini"));
	}

	Files.Append(ProjectPluginsFiles);
	Files.Shrink();

	FScopedSlowTask SlowTaskFiles{
		static_cast<float>(Files.Num()),
		FText::FromString(TEXT("Scanning files for indirect asset usages...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTaskFiles.MakeDialog();

	for (const auto& File : Files)
	{
		SlowTaskFiles.EnterProgressFrame(1.0f, FText::FromString(FString::Printf(TEXT("Scanning %s"), *File)));

		if (!PlatformFile->FileExists(*File)) continue;

		FString FileContent;
		FFileHelper::LoadFileToString(FileContent, *File);

		if (FileContent.IsEmpty()) continue;

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

			const FAssetData AssetData = ModuleAssetRegistry->Get().GetAssetByObjectPath(FName{*FoundedAssetObjectPath});
			if (!AssetData.IsValid()) continue;

			// if founded asset is ok, we loading file by lines to determine on what line its used
			TArray<FString> Lines;
			FFileHelper::LoadFileToStringArray(Lines, *File);
			for (int32 i = 0; i < Lines.Num(); ++i)
			{
				if (!Lines.IsValidIndex(i)) continue;
				if (!Lines[i].Contains(FoundedAssetObjectPath)) continue;

				FProjectCleanerIndirectAssetInfo IndirectAsset;
				IndirectAsset.AssetData = AssetData;
				IndirectAsset.FilePath = FPaths::ConvertRelativePathToFull(File);
				IndirectAsset.LineNum = i + 1;
				AssetsIndirectInfos.AddUnique(IndirectAsset);
			}
		}
	}

	SlowTask.EnterProgressFrame(1.0f);
}

void UProjectCleanerSubsystem::GetAssetsUsed(TArray<FAssetData>& AssetsUsed) const
{
	AssetsUsed.Empty();

	if (ModuleAssetRegistry->Get().IsLoadingAssets())
	{
		ModuleAssetRegistry->Get().WaitForCompletion();
	}

	TArray<FAssetData> AssetsAll;
	TArray<FAssetData> AssetsPrimary;
	TArray<FAssetData> AssetsIndirect;
	TArray<FAssetData> AssetsEditor;
	TArray<FAssetData> AssetsMegascans;
	TArray<FAssetData> AssetsWithExternalRefs;

	FScopedSlowTask SlowTask{
		3.0f,
		FText::FromString(TEXT("Searching used assets...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog();

	SlowTask.EnterProgressFrame(1.0f);
	GetAssetsAll(AssetsAll);
	GetAssetsPrimary(AssetsPrimary);
	GetAssetsIndirect(AssetsIndirect);
	GetAssetsEditor(AssetsEditor);
	GetAssetsMegascans(AssetsMegascans);
	GetAssetsWithExternalRefs(AssetsWithExternalRefs, AssetsAll);

	SlowTask.EnterProgressFrame(1.0f);

	AssetsUsed.Reserve(AssetsPrimary.Num() + AssetsPrimary.Num() + AssetsIndirect.Num() + AssetsEditor.Num() + AssetsMegascans.Num() + AssetsWithExternalRefs.Num());

	FScopedSlowTask SlowTaskPrimary{
		static_cast<float>(AssetsPrimary.Num()),
		FText::FromString(TEXT("Filling primary assets...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTaskPrimary.MakeDialog();

	for (const auto& Asset : AssetsPrimary)
	{
		SlowTaskPrimary.EnterProgressFrame(1.0f, FText::FromString(*Asset.AssetName.ToString()));

		AssetsUsed.AddUnique(Asset);
	}

	FScopedSlowTask SlowTaskIndirect{
		static_cast<float>(AssetsIndirect.Num()),
		FText::FromString(TEXT("Filling indirect assets...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTaskIndirect.MakeDialog();

	for (const auto& Asset : AssetsIndirect)
	{
		SlowTaskIndirect.EnterProgressFrame(1.0f, FText::FromString(*Asset.AssetName.ToString()));

		AssetsUsed.AddUnique(Asset);
	}

	FScopedSlowTask SlowTaskEditor{
		static_cast<float>(AssetsEditor.Num()),
		FText::FromString(TEXT("Filling editor assets...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTaskEditor.MakeDialog();

	for (const auto& Asset : AssetsEditor)
	{
		SlowTaskEditor.EnterProgressFrame(1.0f, FText::FromString(*Asset.AssetName.ToString()));

		AssetsUsed.AddUnique(Asset);
	}

	FScopedSlowTask SlowTaskMegascans{
		static_cast<float>(AssetsMegascans.Num()),
		FText::FromString(TEXT("Filling Megascans assets...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTaskMegascans.MakeDialog();

	for (const auto& Asset : AssetsMegascans)
	{
		SlowTaskMegascans.EnterProgressFrame(1.0f, FText::FromString(*Asset.AssetName.ToString()));

		AssetsUsed.AddUnique(Asset);
	}

	FScopedSlowTask SlowTaskExternal{
		static_cast<float>(AssetsWithExternalRefs.Num()),
		FText::FromString(TEXT("Filling assets with external refs...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTaskExternal.MakeDialog();

	for (const auto& Asset : AssetsWithExternalRefs)
	{
		SlowTaskExternal.EnterProgressFrame(1.0f, FText::FromString(*Asset.AssetName.ToString()));

		AssetsUsed.AddUnique(Asset);
	}

	SlowTask.EnterProgressFrame(1.0f);

	TArray<FAssetData> UsedAssetsDependencies;
	GetAssetsDependencies(AssetsUsed, UsedAssetsDependencies);

	FScopedSlowTask SlowTaskDeps{
		static_cast<float>(UsedAssetsDependencies.Num()),
		FText::FromString(TEXT("Filling dependencies...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTaskDeps.MakeDialog();

	AssetsUsed.Reserve(AssetsUsed.Num() + UsedAssetsDependencies.Num());
	for (const auto& Asset : UsedAssetsDependencies)
	{
		SlowTaskDeps.EnterProgressFrame(1.0f, FText::FromString(*Asset.AssetName.ToString()));

		AssetsUsed.AddUnique(Asset);
	}

	AssetsUsed.Shrink();
}

void UProjectCleanerSubsystem::GetAssetsUnused(TArray<FAssetData>& AssetsUnused) const
{
	AssetsUnused.Empty();

	if (ModuleAssetRegistry->Get().IsLoadingAssets())
	{
		ModuleAssetRegistry->Get().WaitForCompletion();
	}

	TArray<FAssetData> AssetsAll;
	TArray<FAssetData> AssetsUsed;

	FScopedSlowTask SlowTask{
		2.0f,
		FText::FromString(TEXT("Searching unused assets...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog();

	SlowTask.EnterProgressFrame(1.0f);

	GetAssetsAll(AssetsAll);
	GetAssetsUsed(AssetsUsed);

	SlowTask.EnterProgressFrame(1.0f);

	AssetsUnused.Reserve(AssetsAll.Num());

	FScopedSlowTask SlowTaskUnused{
		static_cast<float>(AssetsAll.Num()),
		FText::FromString(TEXT(" ")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTaskUnused.MakeDialog();

	for (const auto& Asset : AssetsAll)
	{
		SlowTaskUnused.EnterProgressFrame(1.0f, FText::FromString(*Asset.AssetName.ToString()));

		if (AssetsUsed.Contains(Asset)) continue;

		AssetsUnused.Add(Asset);
	}

	AssetsUnused.Shrink();
}


void UProjectCleanerSubsystem::GetAssetsDependencies(const TArray<FAssetData>& Assets, TArray<FAssetData>& Dependencies) const
{
	Dependencies.Empty();

	if (ModuleAssetRegistry->Get().IsLoadingAssets())
	{
		ModuleAssetRegistry->Get().WaitForCompletion();
	}

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

			ModuleAssetRegistry->Get().GetDependencies(CurrentPackageName, Deps);

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
	Filter.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);

	for (const auto& Dep : UsedAssetsDeps)
	{
		Filter.PackageNames.Add(Dep);
	}

	ModuleAssetRegistry->Get().GetAssets(Filter, Dependencies);
}

void UProjectCleanerSubsystem::GetAssetsReferencers(const TArray<FAssetData>& Assets, TArray<FAssetData>& Referencers) const
{
	Referencers.Empty();

	if (ModuleAssetRegistry->Get().IsLoadingAssets())
	{
		ModuleAssetRegistry->Get().WaitForCompletion();
	}

	TSet<FName> UsedAssetsRefs;
	TArray<FName> Stack;
	for (const auto& Asset : Assets)
	{
		UsedAssetsRefs.Add(Asset.PackageName);
		Stack.Add(Asset.PackageName);

		TArray<FName> Refs;
		while (Stack.Num() > 0)
		{
			const auto CurrentPackageName = Stack.Pop(false);
			Refs.Reset();

			ModuleAssetRegistry->Get().GetReferencers(CurrentPackageName, Refs);

			Refs.RemoveAllSwap([&](const FName& Ref)
			{
				return !Ref.ToString().StartsWith(*ProjectCleanerConstants::PathRelRoot.ToString());
			}, false);

			for (const auto& Ref : Refs)
			{
				bool bIsAlreadyInSet = false;
				UsedAssetsRefs.Add(Ref, &bIsAlreadyInSet);
				if (!bIsAlreadyInSet)
				{
					Stack.Add(Ref);
				}
			}
		}
	}

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);

	for (const auto& Ref : UsedAssetsRefs)
	{
		Filter.PackageNames.Add(Ref);
	}

	ModuleAssetRegistry->Get().GetAssets(Filter, Referencers);
}

void UProjectCleanerSubsystem::GetClassNamesPrimary(TArray<FName>& ClassNames, const bool bIncludeDerivedClasses) const
{
	if (ModuleAssetRegistry->Get().IsLoadingAssets())
	{
		ModuleAssetRegistry->Get().WaitForCompletion();
	}

	// getting list of primary asset classes that are defined in AssetManager
	TArray<FName> PrimaryAssetClasses;
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
		GetClassNamesDerived(PrimaryAssetClasses, DerivedFromPrimaryAssets);

		for (const auto& DerivedClassName : DerivedFromPrimaryAssets)
		{
			PrimaryAssetClasses.AddUnique(DerivedClassName);
		}
	}

	ClassNames.Empty();
	ClassNames.Reserve(PrimaryAssetClasses.Num());
	for (const auto& ClassName : PrimaryAssetClasses)
	{
		ClassNames.Add(ClassName);
	}
}

void UProjectCleanerSubsystem::GetClassNamesDerived(const TArray<FName>& ClassNames, TSet<FName>& DerivedClassNames) const
{
	DerivedClassNames.Empty();

	const TSet<FName> EmptySet;
	ModuleAssetRegistry->Get().GetDerivedClassNames(ClassNames, EmptySet, DerivedClassNames);
}

int64 UProjectCleanerSubsystem::GetAssetsTotalSize(const TArray<FAssetData>& Assets) const
{
	int64 Size = 0;

	for (const auto& Asset : Assets)
	{
		if (!Asset.IsValid()) continue;

		const auto AssetPackageData = ModuleAssetRegistry->Get().GetAssetPackageData(Asset.PackageName);
		if (!AssetPackageData) continue;

		Size += AssetPackageData->DiskSize;
	}

	return Size;
}

void UProjectCleanerSubsystem::GetFiles(const FString& InPath, TArray<FString>& Files, const bool bRecursive) const
{
	const FString PathAbs = PathConvertToAbs(InPath);
	if (PathAbs.IsEmpty()) return;

	Files.Empty();

	if (bRecursive)
	{
		IFileManager::Get().FindFilesRecursive(Files, *PathAbs, TEXT("*.*"), true, false);
		return;
	}

	TArray<FString> FilesAll;
	IFileManager::Get().FindFiles(FilesAll, *(PathAbs / TEXT("*")), true, false);

	Files.Reserve(FilesAll.Num());
	for (const auto& File : FilesAll)
	{
		Files.Add(PathAbs / File);
	}
}

void UProjectCleanerSubsystem::GetFilesCorrupted(TArray<FString>& FilesCorrupted) const
{
	FilesCorrupted.Empty();

	TArray<FString> FilesAll;
	IFileManager::Get().FindFilesRecursive(FilesAll, *GetPathContentFolder(), TEXT("*.*"), true, false);

	FilesCorrupted.Reserve(FilesAll.Num());

	FScopedSlowTask SlowTask{
		static_cast<float>(FilesAll.Num()),
		FText::FromString(TEXT("Searching corrupted asset files...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog();

	for (const auto& File : FilesAll)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(*File));

		if (FileIsNonEngine(File)) continue;
		if (!FileIsCorrupted(File)) continue;

		FilesCorrupted.Add(File);
	}

	FilesCorrupted.Shrink();
}

void UProjectCleanerSubsystem::GetFilesNonEngine(TArray<FString>& FilesNonEngine) const
{
	FilesNonEngine.Empty();

	TArray<FString> FilesAll;
	IFileManager::Get().FindFilesRecursive(FilesAll, *GetPathContentFolder(), TEXT("*.*"), true, false);

	FilesNonEngine.Reserve(FilesAll.Num());

	FScopedSlowTask SlowTask{
		static_cast<float>(FilesAll.Num()),
		FText::FromString(TEXT("Searching non engine files...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog();

	for (const auto& File : FilesAll)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(*File));

		if (!FileIsNonEngine(File)) continue;

		FilesNonEngine.Add(File);
	}

	FilesNonEngine.Shrink();
}

void UProjectCleanerSubsystem::GetFolders(const FString& InPath, TArray<FString>& Folders, const bool bRecursive)
{
	Folders.Empty();

	const FString PathAbs = PathConvertToAbs(InPath);
	if (PathAbs.IsEmpty()) return;

	if (bRecursive)
	{
		IFileManager::Get().FindFilesRecursive(Folders, *PathAbs, TEXT("*.*"), false, true);
	}
	else
	{
		TArray<FString> FoldersAll;
		IFileManager::Get().FindFiles(FoldersAll, *(PathAbs / TEXT("*")), false, true);

		Folders.Reserve(FoldersAll.Num());
		for (const auto& Folder : FoldersAll)
		{
			Folders.AddUnique(PathAbs / Folder);
		}
	}
}

void UProjectCleanerSubsystem::GetFoldersEmpty(const FString& InPath, TArray<FString>& FoldersEmpty)
{
	const FString PathAbs = PathConvertToAbs(InPath);
	if (PathAbs.IsEmpty()) return;

	FScopedSlowTask SlowTask{
		1,
		FText::FromString(TEXT("Searching empty folders...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog();

	SlowTask.EnterProgressFrame(1.0f);

	TArray<FString> FoldersAll;
	IFileManager::Get().FindFilesRecursive(FoldersAll, *PathAbs, TEXT("*.*"), false, true);

	FoldersEmpty.Empty();
	FoldersEmpty.Reserve(FoldersAll.Num());

	FScopedSlowTask SlowTaskEmptyFolders{
		static_cast<float>(FoldersAll.Num()),
		FText::FromString(TEXT(" ")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTaskEmptyFolders.MakeDialog();

	for (const auto& Folder : FoldersAll)
	{
		SlowTaskEmptyFolders.EnterProgressFrame(1.0f, FText::FromString(Folder));

		if (!FolderIsEmpty(Folder)) continue;

		FoldersEmpty.Add(Folder);
	}

	FoldersEmpty.Shrink();
}

int64 UProjectCleanerSubsystem::GetFilesTotalSize(const TArray<FString>& Files)
{
	if (Files.Num() == 0) return 0;

	int64 TotalSize = 0;
	for (const auto& File : Files)
	{
		if (File.IsEmpty() || !FPaths::FileExists(File)) continue;

		TotalSize += IFileManager::Get().FileSize(*File);
	}

	return TotalSize;
}

FName UProjectCleanerSubsystem::GetAssetExactClassName(const FAssetData& AssetData) const
{
	if (!AssetData.IsValid()) return {};

	if (AssetIsBlueprint(AssetData, true))
	{
		const auto GeneratedClassName = AssetData.TagsAndValues.FindTag(TEXT("GeneratedClass")).GetValue();
		const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(*GeneratedClassName);
		return FName{*FPackageName::ObjectPathToObjectName(ClassObjectPath)};
	}

	return AssetData.AssetClass;
}

bool UProjectCleanerSubsystem::AssetIsBlueprint(const FAssetData& AssetData, const bool bCheckDerivedClasses) const
{
	if (!AssetData.IsValid()) return false;

	if (bCheckDerivedClasses)
	{
		const TArray<FName> ClassNames{UBlueprint::StaticClass()->GetFName()};
		TSet<FName> DerivedClassNames;
		GetClassNamesDerived(ClassNames, DerivedClassNames);

		return DerivedClassNames.Contains(AssetData.AssetClass);
	}

	return AssetData.AssetClass.IsEqual(UBlueprint::StaticClass()->GetFName());
}

bool UProjectCleanerSubsystem::AssetIsUsed(const FAssetData& AssetData) const
{
	if (!AssetData.IsValid()) return false;

	// if primary or its dependency => true
	// if indirect or its dependency => true
	// if editor asset or its dependency => true
	// if megascans enabled and is root megascans assets => true
	// if has external refs or its dependency => true
	// else => false

	// TArray<FAssetData> AssetsPrimary;
	// TArray<FAssetData> AssetsIndirect;
	// TArray<FAssetData> AssetsEditor;
	// TArray<FAssetData> AssetsMegascans;
	// TArray<FAssetData> AssetsWithExternalRefs;
	// TArray<FAssetData> AssetsUsed;
	// GetAssetsUsed(AssetsUsed);
	//
	// return AssetsUsed.Contains(AssetData);
	return false;
}

bool UProjectCleanerSubsystem::AssetIsUnused(const FAssetData& AssetData) const
{
	if (!AssetData.IsValid()) return false;

	TArray<FAssetData> AssetsUnused;
	GetAssetsUnused(AssetsUnused);

	return AssetsUnused.Contains(AssetData);
}

bool UProjectCleanerSubsystem::AssetIsPrimary(const FAssetData& AssetData) const
{
	if (!AssetData.IsValid()) return false;

	TArray<FAssetData> AssetsPrimary;
	GetAssetsPrimary(AssetsPrimary);

	return AssetsPrimary.Contains(AssetData);
}

bool UProjectCleanerSubsystem::AssetIsIndirect(const FAssetData& AssetData) const
{
	if (!AssetData.IsValid()) return false;

	TArray<FAssetData> AssetsIndirect;
	GetAssetsIndirect(AssetsIndirect);

	return AssetsIndirect.Contains(AssetData);
}

bool UProjectCleanerSubsystem::FileIsCorrupted(const FString& FilePathAbs) const
{
	if (!FPaths::FileExists(FilePathAbs)) return false;

	const FString RelativePath = PathConvertToRel(FilePathAbs);

	if (RelativePath.IsEmpty()) return false;

	FString ObjectPath = RelativePath;
	ObjectPath.RemoveFromEnd(FPaths::GetExtension(RelativePath, true));
	ObjectPath.Append(TEXT(".") + FPaths::GetBaseFilename(RelativePath));

	const FAssetData AssetData = ModuleAssetRegistry->Get().GetAssetByObjectPath(FName{*ObjectPath});

	// if its does not exist in asset registry, then something wrong with asset
	return !AssetData.IsValid();
}

bool UProjectCleanerSubsystem::FileIsNonEngine(const FString& FilePathAbs) const
{
	const FString Extension = FPaths::GetExtension(FilePathAbs).ToLower();

	TSet<FString> EngineExtensions;
	EngineExtensions.Reserve(3);
	EngineExtensions.Add(TEXT("uasset"));
	EngineExtensions.Add(TEXT("umap"));
	EngineExtensions.Add(TEXT("collection"));

	return !EngineExtensions.Contains(Extension);
}

bool UProjectCleanerSubsystem::FolderIsEmpty(const FString& FolderPath)
{
	if (FolderPath.IsEmpty()) return false;
	if (!FPaths::DirectoryExists(FolderPath)) return false;

	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *FolderPath, TEXT("*.*"), true, false);

	return Files.Num() == 0;
}

FString UProjectCleanerSubsystem::PathNormalize(const FString& InPath)
{
	if (InPath.IsEmpty()) return InPath;

	FString Path = FPaths::ConvertRelativePathToFull(InPath);
	FPaths::RemoveDuplicateSlashes(Path);

	if (FPaths::GetExtension(Path).IsEmpty())
	{
		FPaths::NormalizeDirectoryName(Path);
	}
	else
	{
		FPaths::NormalizeFilename(Path);
	}

	return Path;
}

FString UProjectCleanerSubsystem::PathConvertToAbs(const FString& InPath)
{
	const FString PathNormalized = PathNormalize(InPath);

	if (!PathIsUnderContentFolder(PathNormalized)) return {};

	return PathNormalized.Replace(
		*ProjectCleanerConstants::PathRelRoot.ToString(),
		*GetPathContentFolder(),
		ESearchCase::CaseSensitive
	);
}

FString UProjectCleanerSubsystem::PathConvertToRel(const FString& InPath)
{
	const FString PathNormalized = PathNormalize(InPath);

	if (!PathIsUnderContentFolder(PathNormalized)) return {};

	return PathNormalized.Replace(
		*GetPathContentFolder(),
		*ProjectCleanerConstants::PathRelRoot.ToString(),
		ESearchCase::CaseSensitive
	);
}

FString UProjectCleanerSubsystem::GetPathContentFolder()
{
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Content"));
}

FString UProjectCleanerSubsystem::GetPathDevelopersFolder()
{
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Developers"));
}

FString UProjectCleanerSubsystem::GetPathCollectionsFolder()
{
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Collections"));
}

FString UProjectCleanerSubsystem::GetPathDevelopersUserFolder()
{
	return FPaths::ConvertRelativePathToFull(FPaths::GameDevelopersDir() / FPaths::GameUserDeveloperFolderName());
}

FString UProjectCleanerSubsystem::GetPathCollectionsUserFolder()
{
	return FPaths::ConvertRelativePathToFull(FPaths::GameUserDeveloperDir() / TEXT("Collections"));
}

bool UProjectCleanerSubsystem::EditorInPlayMode()
{
	if (!GEditor) return false;

	return GEditor->PlayWorld || GIsPlayInEditorWorld;
}

void UProjectCleanerSubsystem::ShowModal(const FString& Msg, const EProjectCleanerModalState State, const float Duration)
{
	FNotificationInfo Info{FText::FromString(Msg)};
	Info.ExpireDuration = Duration;

	const auto NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
	if (!NotificationPtr.IsValid()) return;

	NotificationPtr.Get()->SetCompletionState(GetCompletionStateFromModalState(State));
}

void UProjectCleanerSubsystem::ShowModalOutputLog(const FString& Msg, const EProjectCleanerModalState State, const float Duration)
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

	NotificationPtr.Get()->SetCompletionState(GetCompletionStateFromModalState(State));
}

EAppReturnType::Type UProjectCleanerSubsystem::ShowDialogWindow(const FString& Title, const FString& Msg, const EAppMsgType::Type MsgType)
{
	const FText DialogTitle = FText::FromString(Title);
	const FText DialogContent = FText::FromString(Msg);

	return FMessageDialog::Open(MsgType, DialogContent, &DialogTitle);
}

void UProjectCleanerSubsystem::GetAssetsEditor(TArray<FAssetData>& AssetsEditor) const
{
	AssetsEditor.Empty();

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	Filter.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
	Filter.ClassNames.Add(UEditorUtilityWidget::StaticClass()->GetFName());
	Filter.ClassNames.Add(UEditorUtilityBlueprint::StaticClass()->GetFName());
	Filter.ClassNames.Add(UEditorUtilityWidgetBlueprint::StaticClass()->GetFName());

	ModuleAssetRegistry->Get().GetAssets(Filter, AssetsEditor);
}

void UProjectCleanerSubsystem::GetAssetsMegascans(TArray<FAssetData>& AssetsMegascans) const
{
	AssetsMegascans.Empty();

	if (FModuleManager::Get().IsModuleLoaded(TEXT("MegascansPlugin")))
	{
		ModuleAssetRegistry->Get().GetAssetsByPath(ProjectCleanerConstants::PathRelMSPresets, AssetsMegascans, true);
	}
}

void UProjectCleanerSubsystem::GetAssetsWithExternalRefs(TArray<FAssetData>& AssetsWithExternalRefs, const TArray<FAssetData>& Assets) const
{
	AssetsWithExternalRefs.Empty();
	AssetsWithExternalRefs.Reserve(Assets.Num());

	TArray<FName> Refs;
	for (const auto& Asset : Assets)
	{
		ModuleAssetRegistry->Get().GetReferencers(Asset.PackageName, Refs);

		const bool HasExternalRefs = Refs.ContainsByPredicate([](const FName& Ref)
		{
			return !Ref.ToString().StartsWith(ProjectCleanerConstants::PathRelRoot.ToString());
		});

		if (HasExternalRefs)
		{
			AssetsWithExternalRefs.AddUnique(Asset);
		}

		Refs.Reset();
	}

	AssetsWithExternalRefs.Shrink();
}

void UProjectCleanerSubsystem::FixupRedirectors() const
{
	FScopedSlowTask SlowTask{
		1.0f,
		FText::FromString(TEXT("Fixing redirectors...")),
		GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog();

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
	Filter.ClassNames.Add(UObjectRedirector::StaticClass()->GetFName());

	TArray<FAssetData> AssetList;
	ModuleAssetRegistry->Get().GetAssets(Filter, AssetList);

	if (AssetList.Num() == 0) return;

	FScopedSlowTask LoadingTask(
		AssetList.Num(),
		FText::FromString(TEXT("Loading redirectors...")),
		GIsEditor && !IsRunningCommandlet()
	);
	LoadingTask.MakeDialog();

	TArray<UObjectRedirector*> Redirectors;
	Redirectors.Reserve(AssetList.Num());

	for (const auto& Asset : AssetList)
	{
		LoadingTask.EnterProgressFrame();

		UObject* AssetObj = Asset.GetAsset();
		if (!AssetObj) continue;

		UObjectRedirector* Redirector = CastChecked<UObjectRedirector>(AssetObj);
		if (!Redirector) continue;

		Redirectors.Add(Redirector);
	}

	Redirectors.Shrink();

	ModuleAssetTools->Get().FixupReferencers(Redirectors, false);

	SlowTask.EnterProgressFrame(1.0f);
}

bool UProjectCleanerSubsystem::PathIsUnderContentFolder(const FString& InPath)
{
	const FString PathNormalized = PathNormalize(InPath);

	if (
		!InPath.StartsWith(ProjectCleanerConstants::PathRelRoot.ToString()) &&
		!InPath.StartsWith(GetPathContentFolder())
	)
	{
		return false;
	}

	return true;
}

SNotificationItem::ECompletionState UProjectCleanerSubsystem::GetCompletionStateFromModalState(const EProjectCleanerModalState ModalState)
{
	switch (ModalState)
	{
		case EProjectCleanerModalState::None: return SNotificationItem::CS_None;
		case EProjectCleanerModalState::OK: return SNotificationItem::CS_Success;
		case EProjectCleanerModalState::Pending: return SNotificationItem::CS_Pending;
		case EProjectCleanerModalState::Error: return SNotificationItem::CS_Fail;
		default: return SNotificationItem::CS_None;
	}
}

// FProjectCleanerScanData UProjectCleanerSubsystem::ProjectScan() const
// {
// 	FProjectCleanerScanData ScanData;
//
// 	ModuleAssetRegistry->Get().GetAssetsByPath(ProjectCleanerConstants::PathRelRoot, ScanData.AssetsAll, true);
//
// 	return ScanData;
// }

// void UProjectCleanerSubsystem::Test(const FProjectCleanerScanSettings& ScanSettings)
// {
// 	TSet<FString> AllowedPaths = ScanSettings.ScanPaths.Difference(ScanSettings.ExcludePaths);
//
// 	FARFilter Filter;
// 	Filter.bRecursivePaths = true;
// 	Filter.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
//
//
// 	TArray<FAssetData> Assets;
// 	ModuleAssetRegistry->Get().GetAssets(Filter, Assets);
//
// 	return;
// }

// void UProjectCleanerSubsystem::ProjectScan(const FProjectCleanerScanSettings& ScanSettings, FProjectCleanerScanData& ScanData)
// {
// 	ScanData.Empty();
//
// 	// if (UProjectCleanerLibAsset::AssetRegistryWorking())
// 	// {
// 	// 	ScanData.ScanResult = EProjectCleanerScanResult::AssetRegistryWorking;
// 	// 	ScanData.ScanResultMsg = ScanResultToString(ScanData.ScanResult);
// 	//
// 	// 	return;
// 	// }
// 	//
// 	// if (UProjectCleanerLibEditor::EditorInPlayMode())
// 	// {
// 	// 	ScanData.ScanResult = EProjectCleanerScanResult::EditorInPlayMode;
// 	// 	ScanData.ScanResultMsg = ScanResultToString(ScanData.ScanResult);
// 	//
// 	// 	return;
// 	// }
// 	//
// 	// if (bScanningInProgress)
// 	// {
// 	// 	ScanData.ScanResult = EProjectCleanerScanResult::ScanningInProgress;
// 	// 	ScanData.ScanResultMsg = ScanResultToString(ScanData.ScanResult);
// 	//
// 	// 	return;
// 	// }
// 	//
// 	// if (bCleaningInProgress)
// 	// {
// 	// 	ScanData.ScanResult = EProjectCleanerScanResult::CleaningInProgress;
// 	// 	ScanData.ScanResultMsg = ScanResultToString(ScanData.ScanResult);
// 	//
// 	// 	return;
// 	// }
// 	//
// 	// if (!IsRunningCommandlet())
// 	// {
// 	// 	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllAssetEditors();
// 	// 	UProjectCleanerLibAsset::FixupRedirectors();
// 	// }
// 	//
// 	// if (!FEditorFileUtils::SaveDirtyPackages(false, true, true, false, false, false))
// 	// {
// 	// 	ScanData.ScanResult = EProjectCleanerScanResult::FailedToSaveAssets;
// 	// 	ScanData.ScanResultMsg = ScanResultToString(ScanData.ScanResult);
// 	//
// 	// 	return;
// 	// }
//
// 	bScanningInProgress = true;
//
// 	FScopedSlowTask SlowTask{
// 		4.0f,
// 		FText::FromString(TEXT("Scanning project...")),
// 		GIsEditor && !IsRunningCommandlet()
// 	};
// 	SlowTask.MakeDialog();
//
// 	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Scanning Content folder...")));
// 	ScanContentFolder(ScanData);
//
// 	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching for primary assets...")));
// 	UProjectCleanerLibAsset::GetAssetsPrimary(ScanData.AssetsPrimary);
//
// 	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching for indirect assets...")));
// 	UProjectCleanerLibAsset::GetAssetsIndirect(ScanData.AssetsIndirect);
// 	UProjectCleanerLibAsset::GetAssetsIndirectInfo(ScanData.AssetsIndirectInfo);
//
// 	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching for used assets...")));
// 	UProjectCleanerLibAsset::GetAssetsUsed(ScanData.AssetsUsed);
// 	
// 	// FindAssetsByScanSettings(ScanSettings, ScanData);
//
// 	bScanningInProgress = false;
// }

// void UProjectCleanerSubsystem::ProjectScan()
// {
// 	const UProjectCleanerExcludeSettings* ExcludeSettings = GetDefault<UProjectCleanerExcludeSettings>();
// 	if (!ExcludeSettings) return;
//
// 	FProjectCleanerScanSettings NewScanSettings;
//
// 	for (const auto& ExcludedFolder : ExcludeSettings->ExcludedFolders)
// 	{
// 		if (ExcludedFolder.Path.IsEmpty()) continue;
//
// 		const FString PathAbs = UProjectCleanerLibPath::ConvertToAbs(ExcludedFolder.Path);
// 		if (PathAbs.IsEmpty() || !FPaths::DirectoryExists(PathAbs)) continue;
//
// 		NewScanSettings.ExcludeFolders.AddUnique(ExcludedFolder.Path);
// 	}
//
// 	for (const auto& ExcludedClasses : ExcludeSettings->ExcludedClasses)
// 	{
// 		if (!ExcludedClasses.LoadSynchronous()) continue;
//
// 		NewScanSettings.ExcludeClasses.AddUnique(ExcludedClasses.Get());
// 	}
//
// 	for (const auto& ExcludedAsset : ExcludeSettings->ExcludedAssets)
// 	{
// 		if (!ExcludedAsset.LoadSynchronous()) continue;
//
// 		NewScanSettings.ExcludeAssets.AddUnique(ExcludedAsset.Get());
// 	}
//
// 	ProjectScan(NewScanSettings);
// }
//
// void UProjectCleanerSubsystem::ProjectScan(const FProjectCleanerScanSettings& InScanSettings)
// {
// 	// ScanSettings = InScanSettings;
// 	//
// 	// ScanDataReset();
// 	//
// 	// if (UProjectCleanerLibAsset::AssetRegistryWorking())
// 	// {
// 	// 	ScanData.ScanResult = EProjectCleanerScanResult::AssetRegistryWorking;
// 	// 	ScanData.ScanResultMsg = ScanResultToString(ScanData.ScanResult);
// 	//
// 	// 	UProjectCleanerLibNotification::ShowModalOutputLog(ScanData.ScanResultMsg, EProjectCleanerModalState::Error, 5.0f);
// 	//
// 	// 	return;
// 	// }
// 	//
// 	// if (UProjectCleanerLibEditor::EditorInPlayMode())
// 	// {
// 	// 	ScanData.ScanResult = EProjectCleanerScanResult::EditorInPlayMode;
// 	// 	ScanData.ScanResultMsg = ScanResultToString(ScanData.ScanResult);
// 	//
// 	// 	UProjectCleanerLibNotification::ShowModalOutputLog(ScanData.ScanResultMsg, EProjectCleanerModalState::Error, 5.0f);
// 	//
// 	// 	return;
// 	// }
// 	//
// 	// if (bScanningProject)
// 	// {
// 	// 	ScanData.ScanResult = EProjectCleanerScanResult::ScanningInProgress;
// 	// 	ScanData.ScanResultMsg = ScanResultToString(ScanData.ScanResult);
// 	//
// 	// 	UProjectCleanerLibNotification::ShowModalOutputLog(ScanData.ScanResultMsg, EProjectCleanerModalState::Error, 5.0f);
// 	//
// 	// 	return;
// 	// }
// 	//
// 	// if (bCleaningProject)
// 	// {
// 	// 	ScanData.ScanResult = EProjectCleanerScanResult::CleaningInProgress;
// 	// 	ScanData.ScanResultMsg = ScanResultToString(ScanData.ScanResult);
// 	//
// 	// 	UProjectCleanerLibNotification::ShowModalOutputLog(ScanData.ScanResultMsg, EProjectCleanerModalState::Error, 5.0f);
// 	//
// 	// 	return;
// 	// }
// 	//
// 	// ModuleAssetRegistry->Get().SearchAllAssets(true);
// 	// ModuleAssetRegistry->Get().WaitForCompletion();
// 	//
// 	// GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllAssetEditors();
// 	// UProjectCleanerLibAsset::FixupRedirectors();
// 	//
// 	// if (!FEditorFileUtils::SaveDirtyPackages(false, true, true, false, false, false))
// 	// {
// 	// 	ScanData.ScanResult = EProjectCleanerScanResult::FailedToSaveAssets;
// 	// 	ScanData.ScanResultMsg = ScanResultToString(ScanData.ScanResult);
// 	//
// 	// 	UProjectCleanerLibNotification::ShowModalOutputLog(ScanData.ScanResultMsg, EProjectCleanerModalState::Error, 5.0f);
// 	//
// 	// 	return;
// 	// }
// 	//
// 	// bScanningProject = true;
// 	//
// 	// FScopedSlowTask SlowTaskScan{
// 	// 	8.0f,
// 	// 	FText::FromString(TEXT("Scanning project...")),
// 	// 	GIsEditor && !IsRunningCommandlet()
// 	// };
// 	// SlowTaskScan.MakeDialog();
// 	//
// 	// SlowTaskScan.EnterProgressFrame(1.0f);
// 	//
// 	// FindAssetsTotal();
// 	//
// 	// SlowTaskScan.EnterProgressFrame(1.0f);
// 	//
// 	// // FindAssetsPrimary();
// 	//
// 	// SlowTaskScan.EnterProgressFrame(1.0f);
// 	//
// 	// // FindAssetsIndirect();
// 	//
// 	// SlowTaskScan.EnterProgressFrame(1.0f);
// 	//
// 	// FindAssetsExcluded();
// 	//
// 	// SlowTaskScan.EnterProgressFrame(1.0f);
// 	//
// 	// FindAssetsUsed();
// 	//
// 	// SlowTaskScan.EnterProgressFrame(1.0f);
// 	//
// 	// FindAssetsUnused();
// 	//
// 	// SlowTaskScan.EnterProgressFrame(1.0f);
// 	//
// 	// // todo:ashe23 very slow
// 	// // FindFilesCorrupted();
// 	// FindFilesAndFolders();
// 	//
// 	// SlowTaskScan.EnterProgressFrame(1.0f);
//
// 	// TArray<FString> Paths;
// 	// ModuleAssetRegistry->Get().GetSubPaths(ProjectCleanerConstants::PathRelRoot.ToString(), ScanData.FoldersAll, true);
// 	//
// 	// FScopedSlowTask SlowTask{
// 	// 	static_cast<float>(Paths.Num()),
// 	// 	FText::FromString(TEXT("Scanning paths...")),
// 	// 	GIsEditor && !IsRunningCommandlet()
// 	// };
// 	// SlowTask.MakeDialog();
// 	//
// 	// ScanData.FoldersAll.Reserve(Paths.Num());
// 	// for (const auto& Path : Paths)
// 	// {
// 	// 	SlowTask.EnterProgressFrame(1.0f, FText::FromString(Path));
// 	// 	
// 	// 	const FString PathAbs = UProjectCleanerLibPath::ConvertToAbs(Path);
// 	// 	if (PathAbs.IsEmpty()) continue;
// 	//
// 	// 	ScanData.FoldersAll.Add(PathAbs);
// 	// }
//
// 	// ScanData.FoldersAll.Shrink();
// 	
// 	// todo:ashe23 very slow
// 	// FindFilesNonEngine();
//
// 	// SlowTaskScan.EnterProgressFrame(1.0f);
//
// 	// todo:ashe23 very slow
// 	// FindFolders();
//
// 	bScanningProject = false;
// 	// ScanData.ScanResult = EProjectCleanerScanResult::Success;
//
// 	if (DelegateProjectScanned.IsBound())
// 	{
// 		DelegateProjectScanned.Broadcast();
// 	}
// }
//
// void UProjectCleanerSubsystem::ProjectClean(const bool bRemoveEmptyFolders)
// {
// 	// if (ScanData.ScanResult != EProjectCleanerScanResult::Success) return;
// 	// if (ScanData.AssetsUnused.Num() == 0) return;
// 	//
// 	// bCleaningProject = true;
// 	//
// 	// int32 AssetsDeletedNum = 0;
// 	// const int32 AssetsTotalNum = ScanData.AssetsUnused.Num();
// 	//
// 	// TArray<FAssetData> Bucket;
// 	// TArray<UObject*> LoadedAssets;
// 	// LoadedAssets.Reserve(ProjectCleanerConstants::DeletionBucketSize);
// 	// Bucket.Reserve(ProjectCleanerConstants::DeletionBucketSize);
// 	//
// 	// FScopedSlowTask SlowTask(
// 	// 	AssetsTotalNum,
// 	// 	FText::FromString(TEXT("Removing unused assets..."))
// 	// );
// 	// SlowTask.MakeDialog();
// 	//
// 	// while (ScanData.AssetsUnused.Num() > 0)
// 	// {
// 	// 	BucketFill(Bucket, ProjectCleanerConstants::DeletionBucketSize);
// 	//
// 	// 	if (Bucket.Num() == 0)
// 	// 	{
// 	// 		break;
// 	// 	}
// 	//
// 	// 	if (!BucketPrepare(Bucket, LoadedAssets))
// 	// 	{
// 	// 		UE_LOG(LogProjectCleaner, Error, TEXT("Failed to load some assets. Aborting."))
// 	// 		break;
// 	// 	}
// 	//
// 	// 	AssetsDeletedNum += BucketDelete(LoadedAssets);
// 	// 	SlowTask.EnterProgressFrame(
// 	// 		Bucket.Num(),
// 	// 		FText::FromString(FString::Printf(TEXT("Deleted %d of %d assets"), AssetsDeletedNum, AssetsTotalNum))
// 	// 	);
// 	//
// 	// 	Bucket.Reset();
// 	// 	LoadedAssets.Reset();
// 	// }
// 	//
// 	// // Cleaning empty packages
// 	// const TSet<FName> EmptyPackages = ModuleAssetRegistry->Get().GetCachedEmptyPackages();
// 	// TArray<UPackage*> AssetPackages;
// 	// for (const auto& EmptyPackage : EmptyPackages)
// 	// {
// 	// 	UPackage* Package = FindPackage(nullptr, *EmptyPackage.ToString());
// 	// 	if (Package && Package->IsValidLowLevel())
// 	// 	{
// 	// 		AssetPackages.Add(Package);
// 	// 	}
// 	// }
// 	//
// 	// if (AssetPackages.Num() > 0)
// 	// {
// 	// 	ObjectTools::CleanupAfterSuccessfulDelete(AssetPackages);
// 	// }
// 	//
// 	// bCleaningProject = false;
// 	//
// 	// const bool bSuccess = AssetsDeletedNum == AssetsTotalNum;
// 	// const FString Title = FString::Printf(TEXT("Deleted %d of %d assets"), AssetsDeletedNum, AssetsTotalNum);
// 	//
// 	// if (bSuccess)
// 	// {
// 	// 	UProjectCleanerLibNotification::ShowModal(Title, EProjectCleanerModalState::OK, 10.0f);
// 	// }
// 	// else
// 	// {
// 	// 	UProjectCleanerLibNotification::ShowModalOutputLog(Title, EProjectCleanerModalState::Error, 10.0f);
// 	// }
// 	//
// 	// ProjectScan();
// 	//
// 	// if (bRemoveEmptyFolders)
// 	// {
// 	// 	ProjectCleanEmptyFolders();
// 	// }
// }
//
// void UProjectCleanerSubsystem::ProjectCleanEmptyFolders()
// {
// 	// if (ScanData.ScanResult != EProjectCleanerScanResult::Success) return;
// 	// if (ScanData.FoldersEmpty.Num() == 0) return;
// 	//
// 	// bCleaningProject = true;
// 	//
// 	// FScopedSlowTask SlowTask{
// 	// 	static_cast<float>(ScanData.FoldersEmpty.Num()),
// 	// 	FText::FromString(TEXT("Removing empty folders...")),
// 	// 	GIsEditor && !IsRunningCommandlet()
// 	// };
// 	// SlowTask.MakeDialog();
// 	//
// 	// int32 DeletedFolderNum = 0;
// 	//
// 	// ScanData.FoldersEmpty.Sort([](const FString& FolderA, const FString& FolderB)
// 	// {
// 	// 	return FolderA.Len() > FolderB.Len();
// 	// });
// 	//
// 	// for (const auto& EmptyFolder : ScanData.FoldersEmpty)
// 	// {
// 	// 	SlowTask.EnterProgressFrame(1.0f, FText::FromString(FString::Printf(TEXT("Removing %s folder"), *EmptyFolder)));
// 	//
// 	// 	if (!IFileManager::Get().DeleteDirectory(*EmptyFolder, true, false))
// 	// 	{
// 	// 		UE_LOG(LogProjectCleaner, Error, TEXT("Failed to remove %s folder"), *EmptyFolder);
// 	// 	}
// 	// 	else
// 	// 	{
// 	// 		++DeletedFolderNum;
// 	// 	}
// 	// }
// 	//
// 	// bCleaningProject = false;
// 	//
// 	// const bool bSuccess = DeletedFolderNum == ScanData.FoldersEmpty.Num();
// 	// const FString Title = FString::Printf(TEXT("Deleted %d of %d folders"), DeletedFolderNum, ScanData.FoldersEmpty.Num());
// 	//
// 	// if (bSuccess)
// 	// {
// 	// 	UProjectCleanerLibNotification::ShowModal(Title, EProjectCleanerModalState::OK, 10.0f);
// 	// }
// 	// else
// 	// {
// 	// 	UProjectCleanerLibNotification::ShowModalOutputLog(Title, EProjectCleanerModalState::Error, 10.0f);
// 	// }
// 	//
// 	// ProjectScan();
// }
// //
// // const FProjectCleanerScanData& UProjectCleanerSubsystem::GetScanData() const
// // {
// // 	return ScanData;
// // }
//
// bool UProjectCleanerSubsystem::CanScanProject() const
// {
// 	return !UProjectCleanerLibAsset::AssetRegistryWorking() && !UProjectCleanerLibEditor::EditorInPlayMode() && !ScanningInProgress() && !CleaningInProgress();
// }
//
// bool UProjectCleanerSubsystem::AssetIsExcluded(const FAssetData& AssetData) const
// {
// 	return AssetExcludedByPath(AssetData) || AssetExcludedByClass(AssetData) || AssetExcludedByObject(AssetData);
// }
//
// bool UProjectCleanerSubsystem::ScanningInProgress() const
// {
// 	return bScanningProject;
// }
//
// bool UProjectCleanerSubsystem::CleaningInProgress() const
// {
// 	return bCleaningProject;
// }
//
// FProjectCleanerDelegateProjectScanned& UProjectCleanerSubsystem::OnProjectScanned()
// {
// 	return DelegateProjectScanned;
// }

// FString UProjectCleanerSubsystem::ScanResultToString(const EProjectCleanerScanResult ScanResult)
// {
// 	switch (ScanResult)
// 	{
// 		case EProjectCleanerScanResult::None: return TEXT("");
// 		case EProjectCleanerScanResult::Success: return TEXT("");
// 		case EProjectCleanerScanResult::AssetRegistryWorking: return TEXT("Cant scan project because AssetRegistry still working");
// 		case EProjectCleanerScanResult::EditorInPlayMode: return TEXT("Cant scan project because Editor is in Play mode");
// 		case EProjectCleanerScanResult::ScanningInProgress: return TEXT("Scanning in progress");
// 		case EProjectCleanerScanResult::CleaningInProgress: return TEXT("Cleaning in progress");
// 		case EProjectCleanerScanResult::FailedToSaveAssets: return TEXT("Cant scan project because failed to save some assets");
// 		default: return TEXT("");
// 	}
// }

// void UProjectCleanerSubsystem::FindAssetsTotal()
// {
// 	// ModuleAssetRegistry->Get().GetAssetsByPath(ProjectCleanerConstants::PathRelRoot, ScanData.AssetsAll, true);
// 	// todo:ashe23 for ue5 make sure we exclude __External*__ folders
// }

// void UProjectCleanerSubsystem::FindAssetsPrimary()
// {
// 	FScopedSlowTask SlowTask{
// 		3.0f,
// 		FText::FromString(TEXT("Scanning for primary assets...")),
// 		GIsEditor && !IsRunningCommandlet()
// 	};
// 	SlowTask.MakeDialog();
//
// 	TArray<FName> PrimaryAssetClasses;
// 	// getting list of primary asset classes that are defined in AssetManager
// 	const auto& AssetManager = UAssetManager::Get();
// 	if (!AssetManager.IsValid()) return;
//
// 	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching assets in AssetManager...")));
//
// 	TArray<FPrimaryAssetTypeInfo> AssetTypeInfos;
// 	AssetManager.Get().GetPrimaryAssetTypeInfoList(AssetTypeInfos);
// 	PrimaryAssetClasses.Reserve(AssetTypeInfos.Num());
//
// 	for (const auto& AssetTypeInfo : AssetTypeInfos)
// 	{
// 		if (!AssetTypeInfo.AssetBaseClassLoaded) continue;
//
// 		PrimaryAssetClasses.AddUnique(AssetTypeInfo.AssetBaseClassLoaded->GetFName());
// 	}
//
// 	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching assets that derived from primary assets...")));
//
// 	// getting list of primary assets classes that are derived from main primary assets
// 	TSet<FName> DerivedFromPrimaryAssets;
// 	{
// 		const TSet<FName> ExcludedClassNames;
// 		ModuleAssetRegistry->Get().GetDerivedClassNames(PrimaryAssetClasses, ExcludedClassNames, DerivedFromPrimaryAssets);
// 	}
//
// 	for (const auto& DerivedClassName : DerivedFromPrimaryAssets)
// 	{
// 		PrimaryAssetClasses.AddUnique(DerivedClassName);
// 	}
//
// 	FARFilter Filter;
// 	Filter.bRecursiveClasses = true;
// 	Filter.bRecursivePaths = true;
// 	Filter.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
//
// 	for (const auto& ClassName : PrimaryAssetClasses)
// 	{
// 		Filter.ClassNames.Add(ClassName);
// 	}
// 	ModuleAssetRegistry->Get().GetAssets(Filter, ScanData.AssetsPrimary);
//
// 	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching blueprint primary assets...")));
//
// 	// getting primary blueprint assets
// 	FARFilter FilterBlueprint;
// 	FilterBlueprint.bRecursivePaths = true;
// 	FilterBlueprint.bRecursiveClasses = true;
// 	FilterBlueprint.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
// 	FilterBlueprint.ClassNames.Add(UBlueprint::StaticClass()->GetFName());
//
// 	TArray<FAssetData> BlueprintAssets;
// 	ModuleAssetRegistry->Get().GetAssets(FilterBlueprint, BlueprintAssets);
//
// 	for (const auto& BlueprintAsset : BlueprintAssets)
// 	{
// 		const FName BlueprintClass = FName{*UProjectCleanerLibAsset::GetAssetClassName(BlueprintAsset)};
// 		if (PrimaryAssetClasses.Contains(BlueprintClass))
// 		{
// 			ScanData.AssetsPrimary.AddUnique(BlueprintAsset);
// 		}
// 	}
// }

// void UProjectCleanerSubsystem::FindAssetsIndirect()
// {
// 	FScopedSlowTask SlowTask{
// 		2.0f,
// 		FText::FromString(TEXT("Scanning for indirect assets...")),
// 		GIsEditor && !IsRunningCommandlet()
// 	};
// 	SlowTask.MakeDialog();
// 	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Preparing scan directories...")));
//
// 	const FString SourceDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + TEXT("Source/"));
// 	const FString ConfigDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + TEXT("Config/"));
// 	const FString PluginsDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + TEXT("Plugins/"));
//
// 	TSet<FString> Files;
// 	Files.Reserve(200); // reserving some space
//
// 	// 1) finding all source files in main project "Source" directory (<yourproject>/Source/*)
// 	TArray<FString> FilesToScan;
// 	PlatformFile->FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".cs"));
// 	PlatformFile->FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".cpp"));
// 	PlatformFile->FindFilesRecursively(FilesToScan, *SourceDir, TEXT(".h"));
// 	PlatformFile->FindFilesRecursively(FilesToScan, *ConfigDir, TEXT(".ini"));
// 	Files.Append(FilesToScan);
//
// 	// 2) we should find all source files in plugins folder (<yourproject>/Plugins/*)
// 	TArray<FString> ProjectPluginsFiles;
// 	// finding all installed plugins in "Plugins" directory
// 	struct FDirectoryVisitor : IPlatformFile::FDirectoryVisitor
// 	{
// 		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
// 		{
// 			if (bIsDirectory)
// 			{
// 				InstalledPlugins.Add(FilenameOrDirectory);
// 			}
//
// 			return true;
// 		}
//
// 		TArray<FString> InstalledPlugins;
// 	};
//
// 	FDirectoryVisitor Visitor;
// 	PlatformFile->IterateDirectory(*PluginsDir, Visitor);
//
// 	// 3) for every installed plugin we scanning only "Source" and "Config" folders
// 	for (const auto& Dir : Visitor.InstalledPlugins)
// 	{
// 		const FString PluginSourcePathDir = Dir + "/Source";
// 		const FString PluginConfigPathDir = Dir + "/Config";
//
// 		PlatformFile->FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".cs"));
// 		PlatformFile->FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".cpp"));
// 		PlatformFile->FindFilesRecursively(ProjectPluginsFiles, *PluginSourcePathDir, TEXT(".h"));
// 		PlatformFile->FindFilesRecursively(ProjectPluginsFiles, *PluginConfigPathDir, TEXT(".ini"));
// 	}
//
// 	Files.Append(ProjectPluginsFiles);
// 	Files.Shrink();
//
// 	FScopedSlowTask SlowTaskFiles{
// 		static_cast<float>(Files.Num()),
// 		FText::FromString(TEXT("Scanning files for indirect asset usages...")),
// 		GIsEditor && !IsRunningCommandlet()
// 	};
// 	SlowTaskFiles.MakeDialog();
//
// 	for (const auto& File : Files)
// 	{
// 		SlowTaskFiles.EnterProgressFrame(1.0f, FText::FromString(FString::Printf(TEXT("Scanning %s"), *File)));
//
// 		if (!PlatformFile->FileExists(*File)) continue;
//
// 		FString FileContent;
// 		FFileHelper::LoadFileToString(FileContent, *File);
//
// 		if (FileContent.IsEmpty()) continue;
//
// 		static FRegexPattern Pattern(TEXT(R"(\/Game([A-Za-z0-9_.\/]+)\b)"));
// 		FRegexMatcher Matcher(Pattern, FileContent);
// 		while (Matcher.FindNext())
// 		{
// 			FString FoundedAssetObjectPath = Matcher.GetCaptureGroup(0);
//
// 			// if ObjectPath ends with "_C" , then its probably blueprint, so we trim that
// 			if (FoundedAssetObjectPath.EndsWith("_C"))
// 			{
// 				FString TrimmedObjectPath = FoundedAssetObjectPath;
// 				TrimmedObjectPath.RemoveFromEnd("_C");
//
// 				FoundedAssetObjectPath = TrimmedObjectPath;
// 			}
//
// 			const FAssetData* AssetData = ScanData.AssetsAll.FindByPredicate([&](const FAssetData& Elem)
// 			{
// 				return
// 					Elem.ObjectPath.ToString() == (FoundedAssetObjectPath) ||
// 					Elem.PackageName.ToString() == (FoundedAssetObjectPath);
// 			});
//
// 			if (!AssetData) continue;
//
// 			// if founded asset is ok, we loading file by lines to determine on what line its used
// 			TArray<FString> Lines;
// 			FFileHelper::LoadFileToStringArray(Lines, *File);
// 			for (int32 i = 0; i < Lines.Num(); ++i)
// 			{
// 				if (!Lines.IsValidIndex(i)) continue;
// 				if (!Lines[i].Contains(FoundedAssetObjectPath)) continue;
//
// 				FProjectCleanerIndirectAssetInfo IndirectAsset;
// 				IndirectAsset.AssetData = *AssetData;
// 				IndirectAsset.FilePath = FPaths::ConvertRelativePathToFull(File);
// 				IndirectAsset.LineNum = i + 1;
// 				ScanData.AssetsIndirectInfo.AddUnique(IndirectAsset);
// 				ScanData.AssetsIndirect.AddUnique(*AssetData);
// 			}
// 		}
// 	}
//
// 	SlowTask.EnterProgressFrame(1.0f);
// }

// void UProjectCleanerSubsystem::FindAssetsExcluded()
// {
// 	// FScopedSlowTask SlowTask{
// 	// 	static_cast<float>(ScanData.AssetsAll.Num()),
// 	// 	FText::FromString(TEXT("Scanning for excluded assets based on specified settings...")),
// 	// 	GIsEditor && !IsRunningCommandlet()
// 	// };
// 	// SlowTask.MakeDialog();
// 	//
// 	// ScanData.AssetsExcluded.Reserve(ScanData.AssetsAll.Num());
// 	//
// 	// for (const auto& Asset : ScanData.AssetsAll)
// 	// {
// 	// 	SlowTask.EnterProgressFrame(1.0f, FText::FromString(FString::Printf(TEXT("%s"), *Asset.AssetName.ToString())));
// 	//
// 	// 	if (AssetIsExcluded(Asset))
// 	// 	{
// 	// 		ScanData.AssetsExcluded.AddUnique(Asset);
// 	// 	}
// 	// }
// 	//
// 	// ScanData.AssetsExcluded.Shrink();
// }

// void UProjectCleanerSubsystem::FindAssetsUsed()
// {
// 	// FScopedSlowTask SlowTask{
// 	// 	3.0f,
// 	// 	FText::FromString(TEXT("Searching for used assets..")),
// 	// 	GIsEditor && !IsRunningCommandlet()
// 	// };
// 	// SlowTask.MakeDialog();
// 	//
// 	// SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching assets with external referencers..")));
// 	//
// 	// TArray<FAssetData> AssetsWithExternalRefs;
// 	// AssetsWithExternalRefs.Reserve(ScanData.AssetsAll.Num());
// 	//
// 	// TArray<FName> Refs;
// 	// for (const auto& Asset : ScanData.AssetsAll)
// 	// {
// 	// 	ModuleAssetRegistry->Get().GetReferencers(Asset.PackageName, Refs);
// 	//
// 	// 	const bool HasExternalRefs = Refs.ContainsByPredicate([](const FName& Ref)
// 	// 	{
// 	// 		return !Ref.ToString().StartsWith(ProjectCleanerConstants::PathRelRoot.ToString());
// 	// 	});
// 	//
// 	// 	if (HasExternalRefs)
// 	// 	{
// 	// 		AssetsWithExternalRefs.AddUnique(Asset);
// 	// 	}
// 	//
// 	// 	Refs.Reset();
// 	// }
// 	//
// 	// AssetsWithExternalRefs.Shrink();
// 	//
// 	// SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching editor or plugin specific assets...")));
// 	//
// 	// TArray<FAssetData> AssetsEditor;
// 	// TArray<FAssetData> AssetsMegascans;
// 	//
// 	// FARFilter FilterEditorAssets;
// 	// FilterEditorAssets.bRecursivePaths = true;
// 	// FilterEditorAssets.bRecursiveClasses = true;
// 	// FilterEditorAssets.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
// 	// FilterEditorAssets.ClassNames.Add(UEditorUtilityWidget::StaticClass()->GetFName());
// 	// FilterEditorAssets.ClassNames.Add(UEditorUtilityBlueprint::StaticClass()->GetFName());
// 	// FilterEditorAssets.ClassNames.Add(UEditorUtilityWidgetBlueprint::StaticClass()->GetFName());
// 	// FilterEditorAssets.ClassNames.Add(UMapBuildDataRegistry::StaticClass()->GetFName());
// 	// ModuleAssetRegistry->Get().GetAssets(FilterEditorAssets, AssetsEditor);
// 	//
// 	// if (FModuleManager::Get().IsModuleLoaded(TEXT("MegascansPlugin")))
// 	// {
// 	// 	ModuleAssetRegistry->Get().GetAssetsByPath(ProjectCleanerConstants::PathRelMSPresets, AssetsMegascans, true);
// 	// }
// 	//
// 	// SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Searching for used assets dependencies...")));
// 	//
// 	// ScanData.AssetsUsed.Reserve(
// 	// 	ScanData.AssetsPrimary.Num() + ScanData.AssetsIndirect.Num() + AssetsWithExternalRefs.Num() + AssetsEditor.Num() + AssetsMegascans.Num() + ScanData.AssetsExcluded.Num()
// 	// );
// 	//
// 	// for (const auto& Asset : ScanData.AssetsPrimary)
// 	// {
// 	// 	ScanData.AssetsUsed.AddUnique(Asset);
// 	// }
// 	//
// 	// for (const auto& Asset : ScanData.AssetsIndirect)
// 	// {
// 	// 	ScanData.AssetsUsed.AddUnique(Asset);
// 	// }
// 	//
// 	// for (const auto& Asset : AssetsWithExternalRefs)
// 	// {
// 	// 	ScanData.AssetsUsed.AddUnique(Asset);
// 	// }
// 	//
// 	// for (const auto& Asset : AssetsEditor)
// 	// {
// 	// 	ScanData.AssetsUsed.AddUnique(Asset);
// 	// }
// 	//
// 	// for (const auto& Asset : AssetsMegascans)
// 	// {
// 	// 	ScanData.AssetsUsed.AddUnique(Asset);
// 	// }
// 	//
// 	// for (const auto& Asset : ScanData.AssetsExcluded)
// 	// {
// 	// 	ScanData.AssetsUsed.AddUnique(Asset);
// 	// }
// 	//
// 	// TArray<FAssetData> AssetsDependencies;
// 	// UProjectCleanerLibAsset::GetAssetsDependencies(ScanData.AssetsUsed, AssetsDependencies);
// 	//
// 	// ScanData.AssetsUsed.Reserve(ScanData.AssetsUsed.Num() + AssetsDependencies.Num());
// 	//
// 	// for (const auto& Asset : AssetsDependencies)
// 	// {
// 	// 	ScanData.AssetsUsed.AddUnique(Asset);
// 	// }
// 	//
// 	// ScanData.AssetsUsed.Shrink();
// }
//
// void UProjectCleanerSubsystem::FindAssetsUnused()
// {
// 	// FScopedSlowTask SlowTask{
// 	// 	static_cast<float>(ScanData.AssetsAll.Num()),
// 	// 	FText::FromString(TEXT("Searching for unused assets...")),
// 	// 	GIsEditor && !IsRunningCommandlet()
// 	// };
// 	// SlowTask.MakeDialog();
// 	//
// 	// ScanData.AssetsUnused.Reserve(ScanData.AssetsAll.Num());
// 	//
// 	// for (const auto& Asset : ScanData.AssetsAll)
// 	// {
// 	// 	SlowTask.EnterProgressFrame(1.0f, FText::FromString(FString::Printf(TEXT("%s"), *Asset.AssetName.ToString())));
// 	//
// 	// 	if (!ScanData.AssetsUsed.Contains(Asset))
// 	// 	{
// 	// 		ScanData.AssetsUnused.AddUnique(Asset);
// 	// 	}
// 	// }
// 	//
// 	// ScanData.AssetsUnused.Shrink();
// }
//
// void UProjectCleanerSubsystem::FindFilesCorrupted()
// {
// 	// TArray<FString> Files;
// 	// IFileManager::Get().FindFilesRecursive(Files, *FPaths::ProjectContentDir(), TEXT("*.*"), true, false);
// 	//
// 	// FScopedSlowTask SlowTask{
// 	// 	static_cast<float>(Files.Num()),
// 	// 	FText::FromString(TEXT("Searching for corrupted files...")),
// 	// 	GIsEditor && !IsRunningCommandlet()
// 	// };
// 	// SlowTask.MakeDialog();
// 	//
// 	// ScanData.FilesCorrupted.Reserve(Files.Num());
// 	//
// 	// for (const auto& File : Files)
// 	// {
// 	// 	SlowTask.EnterProgressFrame(1.0f, FText::FromString(FString::Printf(TEXT("%s"), *File)));
// 	//
// 	// 	const FString FilePathAbs = FPaths::ConvertRelativePathToFull(File);
// 	// 	if (!UProjectCleanerLibFile::FileHasEngineExtension(FilePathAbs)) continue;
// 	// 	if (!UProjectCleanerLibFile::FileIsCorrupted(FilePathAbs)) continue;
// 	//
// 	// 	ScanData.FilesCorrupted.AddUnique(FilePathAbs);
// 	// }
// 	//
// 	// ScanData.FilesCorrupted.Shrink();
// }
//
// void UProjectCleanerSubsystem::FindFilesNonEngine()
// {
// 	// TArray<FString> Files;
// 	// IFileManager::Get().FindFilesRecursive(Files, *FPaths::ProjectContentDir(), TEXT("*.*"), true, false);
// 	//
// 	// FScopedSlowTask SlowTask{
// 	// 	static_cast<float>(Files.Num()),
// 	// 	FText::FromString(TEXT("Searching for non engine files...")),
// 	// 	GIsEditor && !IsRunningCommandlet()
// 	// };
// 	// SlowTask.MakeDialog();
// 	//
// 	// ScanData.FilesNonEngine.Reserve(Files.Num());
// 	//
// 	// for (const auto& File : Files)
// 	// {
// 	// 	SlowTask.EnterProgressFrame(1.0f, FText::FromString(FString::Printf(TEXT("%s"), *File)));
// 	//
// 	// 	const FString FilePathAbs = FPaths::ConvertRelativePathToFull(File);
// 	// 	if (UProjectCleanerLibFile::FileHasEngineExtension(FilePathAbs)) continue;
// 	//
// 	// 	ScanData.FilesNonEngine.AddUnique(FilePathAbs);
// 	// }
// 	//
// 	// ScanData.FilesNonEngine.Shrink();
// }
//
// void UProjectCleanerSubsystem::FindFolders()
// {
// 	// TArray<FString> Folders;
// 	// IFileManager::Get().FindFilesRecursive(Folders, *FPaths::ProjectContentDir(), TEXT("*.*"), false, true);
// 	//
// 	// TArray<FAssetData> Assets;
// 	// for (const auto& Folder : Folders)
// 	// {
// 	// 	const FString FolderPathAbs = FPaths::ConvertRelativePathToFull(Folder);
// 	//
// 	// 	ScanData.FoldersAll.AddUnique(FolderPathAbs);
// 	//
// 	// 	if (
// 	// 		UProjectCleanerLibFile::FolderIsEmpty(FolderPathAbs) &&
// 	// 		!UProjectCleanerLibFile::FolderIsExcluded(FolderPathAbs) &&
// 	// 		!UProjectCleanerLibFile::FolderIsEngineGenerated(FolderPathAbs))
// 	// 	{
// 	// 		ScanData.FoldersEmpty.AddUnique(FolderPathAbs);
// 	// 	}
// 	// }
// }

// void UProjectCleanerSubsystem::ScanContentFolder(FProjectCleanerScanData& ScanData)
// {
// 	// struct FContentFolderVisitor : IPlatformFile::FDirectoryVisitor
// 	// {
// 	// 	explicit FContentFolderVisitor(FProjectCleanerScanData& InScanData) : ScanData(InScanData)
// 	// 	{
// 	// 	}
// 	//
// 	// 	virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
// 	// 	{
// 	// 		const FString PathAbs = UProjectCleanerLibPath::ConvertToAbs(FilenameOrDirectory);
// 	// 		if (PathAbs.IsEmpty()) return true;
// 	//
// 	// 		if (bIsDirectory)
// 	// 		{
// 	// 			ScanData.FoldersAll.Add(PathAbs);
// 	//
// 	// 			// we can remove folders that are not excluded and not engine generated
// 	// 			if (
// 	// 				UProjectCleanerLibFile::FolderIsEmpty(PathAbs) &&
// 	// 				!UProjectCleanerLibFile::FolderIsExcluded(PathAbs) &&
// 	// 				!UProjectCleanerLibFile::FolderIsEngineGenerated(PathAbs)
// 	// 			)
// 	// 			{
// 	// 				ScanData.FoldersEmpty.Add(PathAbs);
// 	// 			}
// 	//
// 	// 			return true;
// 	// 		}
// 	//
// 	// 		if (UProjectCleanerLibFile::FileHasEngineExtension(PathAbs))
// 	// 		{
// 	// 			if (UProjectCleanerLibFile::FileIsCorrupted(PathAbs))
// 	// 			{
// 	// 				ScanData.FilesCorrupted.Add(PathAbs);
// 	// 			}
// 	// 		}
// 	// 		else
// 	// 		{
// 	// 			ScanData.FilesNonEngine.Add(PathAbs);
// 	// 		}
// 	//
// 	// 		return true;
// 	// 	}
// 	//
// 	// 	FProjectCleanerScanData& ScanData;
// 	// };
// 	//
// 	// FContentFolderVisitor Visitor{ScanData};
// 	// FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryRecursively(*UProjectCleanerLibPath::GetFolderContent(), Visitor);
// }

// void UProjectCleanerSubsystem::FindAssetsByScanSettings(const FProjectCleanerScanSettings& ScanSettings, FProjectCleanerScanData& ScanData) const
// {
// 	// todo:ashe23 for ue5 make sure we exclude __External*__ folders
//
// 	FARFilter Filter;
// 	Filter.bRecursivePaths = true;
//
// 	Filter.PackagePaths.Reserve(ScanSettings.ScanPaths.Num());
// 	for (const auto& Path : ScanSettings.ScanPaths)
// 	{
// 		const FString PathAbs = UProjectCleanerLibPath::ConvertToAbs(Path);
// 		if (PathAbs.IsEmpty()) continue;
//
// 		Filter.PackagePaths.Add(FName{*Path});
// 	}
//
// 	if (Filter.PackagePaths.Num() == 0)
// 	{
// 		Filter.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
// 	}
//
// 	ModuleAssetRegistry->Get().GetAssets(Filter, ScanData.AssetsAll);
//
// 	// // excluding assets filtered by path
// 	// {
// 	// 	FARFilter PathExcludeFilter;
// 	// 	PathExcludeFilter.bRecursivePaths = true;
// 	//
// 	// 	PathExcludeFilter.PackagePaths.Reserve(ScanSettings.ExcludePaths.Num());
// 	// 	for (const auto& ExcludePath : ScanSettings.ExcludePaths)
// 	// 	{
// 	// 		const FString PathAbs = UProjectCleanerLibPath::ConvertToAbs(ExcludePath);
// 	// 		if (PathAbs.IsEmpty()) continue;
// 	//
// 	// 		PathExcludeFilter.PackagePaths.Add(FName{*ExcludePath});
// 	// 	}
// 	//
// 	// 	if (PathExcludeFilter.PackagePaths.Num() > 0)
// 	// 	{
// 	// 		ModuleAssetRegistry->Get().UseFilterToExcludeAssets(ScanData.AssetsAll, PathExcludeFilter);
// 	// 	}
// 	// }
// 	//
// 	// // excluding assets filtered by specific assets
// 	// {
// 	// 	FARFilter AssetExcludeFilter;
// 	// 	for (const auto& Asset : ScanSettings.ExcludeAssets)
// 	// 	{
// 	// 		if (!Asset.LoadSynchronous()) continue;
// 	//
// 	// 		AssetExcludeFilter.PackagePaths.Add(Asset.ToSoftObjectPath().GetAssetPathName());
// 	// 	}
// 	//
// 	// 	if (AssetExcludeFilter.PackagePaths.Num() > 0)
// 	// 	{
// 	// 		ModuleAssetRegistry->Get().UseFilterToExcludeAssets(ScanData.AssetsAll, AssetExcludeFilter);
// 	// 	}
// 	// }
//
//
// 	return;
// }
//
// // void UProjectCleanerSubsystem::ScanDataReset()
// // {
// // 	ScanData.ScanResult = EProjectCleanerScanResult::None;
// // 	ScanData.ScanResultMsg.Reset();
// // 	ScanData.AssetsAll.Reset();
// // 	ScanData.AssetsUsed.Reset();
// // 	ScanData.AssetsPrimary.Reset();
// // 	ScanData.AssetsIndirect.Reset();
// // 	ScanData.AssetsIndirectInfo.Reset();
// // 	ScanData.AssetsExcluded.Reset();
// // 	ScanData.AssetsUnused.Reset();
// // 	ScanData.FoldersAll.Reset();
// // 	ScanData.FoldersEmpty.Reset();
// // 	ScanData.FilesCorrupted.Reset();
// // 	ScanData.FilesNonEngine.Reset();
// // }
//
// bool UProjectCleanerSubsystem::AssetExcludedByPath(const FAssetData& AssetData) const
// {
// 	// for (const auto& ExcludedFolder : ScanSettings.ExcludeFolders)
// 	// {
// 	// 	const FString FolderPathAbs = UProjectCleanerLibPath::ConvertToAbs(ExcludedFolder);
// 	// 	const FString AssetPathAbs = UProjectCleanerLibPath::ConvertToAbs(AssetData.PackagePath.ToString());
// 	//
// 	// 	if (FolderPathAbs.IsEmpty() || AssetPathAbs.IsEmpty()) continue;
// 	// 	if (FPaths::IsUnderDirectory(AssetPathAbs, FolderPathAbs))
// 	// 	{
// 	// 		return true;
// 	// 	}
// 	// }
//
// 	return false;
// }
//
// bool UProjectCleanerSubsystem::AssetExcludedByClass(const FAssetData& AssetData) const
// {
// 	const FString AssetClassName = UProjectCleanerLibAsset::GetAssetClassName(AssetData);
//
// 	// for (const auto& ExcludedClass : ScanSettings.ExcludeClasses)
// 	// {
// 	// 	if (!ExcludedClass) continue;
// 	//
// 	// 	const FString ExcludedClassName = ExcludedClass->GetName();
// 	// 	if (AssetClassName.Equals(ExcludedClassName))
// 	// 	{
// 	// 		return true;
// 	// 	}
// 	// }
//
// 	return false;
// }
//
// bool UProjectCleanerSubsystem::AssetExcludedByObject(const FAssetData& AssetData) const
// {
// 	// for (const auto& ExcludedAsset : ScanSettings.ExcludeAssets)
// 	// {
// 	// 	if (!ExcludedAsset.LoadSynchronous()) continue;
// 	//
// 	// 	if (ExcludedAsset.ToSoftObjectPath() == AssetData.ToSoftObjectPath())
// 	// 	{
// 	// 		return true;
// 	// 	}
// 	// }
//
// 	return false;
// }
//
// void UProjectCleanerSubsystem::BucketFill(TArray<FAssetData>& Bucket, const int32 BucketSize)
// {
// 	// Searching Root assets
// 	// int32 Index = 0;
// 	// TArray<FName> Refs;
// 	// while (Bucket.Num() < BucketSize && ScanData.AssetsUnused.IsValidIndex(Index))
// 	// {
// 	// 	const FAssetData CurrentAsset = ScanData.AssetsUnused[Index];
// 	// 	ModuleAssetRegistry->Get().GetReferencers(CurrentAsset.PackageName, Refs);
// 	// 	Refs.RemoveAllSwap([&](const FName& Ref)
// 	// 	{
// 	// 		return !Ref.ToString().StartsWith(ProjectCleanerConstants::PathRelRoot.ToString()) || Ref.IsEqual(CurrentAsset.PackageName);
// 	// 	}, false);
// 	// 	Refs.Shrink();
// 	//
// 	// 	if (Refs.Num() == 0)
// 	// 	{
// 	// 		Bucket.AddUnique(CurrentAsset);
// 	// 		ScanData.AssetsUnused.RemoveAt(Index);
// 	// 	}
// 	//
// 	// 	Refs.Reset();
// 	//
// 	// 	++Index;
// 	// }
// 	//
// 	// if (Bucket.Num() > 0)
// 	// {
// 	// 	return;
// 	// }
// 	//
// 	// // if root assets not found, we deleting assets single by finding its referencers
// 	// if (ScanData.AssetsUnused.Num() == 0)
// 	// {
// 	// 	return;
// 	// }
// 	//
// 	// TArray<FAssetData> Stack;
// 	// Stack.Add(ScanData.AssetsUnused[0]);
// 	//
// 	// while (Stack.Num() > 0)
// 	// {
// 	// 	const FAssetData Current = Stack.Pop(false);
// 	// 	Bucket.AddUnique(Current);
// 	// 	ScanData.AssetsUnused.Remove(Current);
// 	//
// 	// 	ModuleAssetRegistry->Get().GetReferencers(Current.PackageName, Refs);
// 	//
// 	// 	Refs.RemoveAllSwap([&](const FName& Ref)
// 	// 	{
// 	// 		return !Ref.ToString().StartsWith(ProjectCleanerConstants::PathRelRoot.ToString()) || Ref.IsEqual(Current.PackageName);
// 	// 	}, false);
// 	// 	Refs.Shrink();
// 	//
// 	// 	for (const auto& Ref : Refs)
// 	// 	{
// 	// 		const FString ObjectPath = Ref.ToString() + TEXT(".") + FPaths::GetBaseFilename(*Ref.ToString());
// 	// 		const FAssetData AssetData = ModuleAssetRegistry->Get().GetAssetByObjectPath(FName{*ObjectPath});
// 	// 		if (AssetData.IsValid())
// 	// 		{
// 	// 			if (!Bucket.Contains(AssetData))
// 	// 			{
// 	// 				Stack.Add(AssetData);
// 	// 			}
// 	//
// 	// 			Bucket.AddUnique(AssetData);
// 	// 			ScanData.AssetsUnused.Remove(AssetData);
// 	// 		}
// 	// 	}
// 	//
// 	// 	Refs.Reset();
// 	// }
// }
//
// bool UProjectCleanerSubsystem::BucketPrepare(const TArray<FAssetData>& Bucket, TArray<UObject*>& LoadedAssets) const
// {
// 	TArray<FString> ObjectPaths;
// 	ObjectPaths.Reserve(Bucket.Num());
//
// 	for (const auto& Asset : Bucket)
// 	{
// 		if (!Asset.IsValid()) continue;
//
// 		ObjectPaths.Add(Asset.ObjectPath.ToString());
// 	}
//
// 	return AssetViewUtils::LoadAssetsIfNeeded(ObjectPaths, LoadedAssets, false, true);
// }
//
// int32 UProjectCleanerSubsystem::BucketDelete(const TArray<UObject*>& LoadedAssets) const
// {
// 	int32 DeletedAssetsNum = ObjectTools::DeleteObjects(LoadedAssets, false);
//
// 	if (DeletedAssetsNum == 0)
// 	{
// 		DeletedAssetsNum = ObjectTools::ForceDeleteObjects(LoadedAssets, false);
// 	}
//
// 	return DeletedAssetsNum;
// }
