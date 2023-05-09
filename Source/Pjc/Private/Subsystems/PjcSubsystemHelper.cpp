// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Subsystems/PjcSubsystemHelper.h"
#include "PjcConstants.h"
#include "PjcTypes.h"
// Engine Headers
#include "EditorTutorial.h"
#include "EditorUtilityBlueprint.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "Engine/AssetManager.h"

void UPjcHelperSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UPjcHelperSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UPjcHelperSubsystem::GetClassNamesPrimary(TSet<FName>& OutClassNames)
{
	// getting list of primary asset classes that are defined in AssetManager
	const auto& AssetManager = UAssetManager::Get();
	if (!AssetManager.IsValid()) return;

	TSet<FName> ClassNamesPrimaryBase;
	TArray<FPrimaryAssetTypeInfo> AssetTypeInfos;
	AssetManager.Get().GetPrimaryAssetTypeInfoList(AssetTypeInfos);
	ClassNamesPrimaryBase.Reserve(AssetTypeInfos.Num());

	for (const auto& AssetTypeInfo : AssetTypeInfos)
	{
		if (!AssetTypeInfo.AssetBaseClassLoaded) continue;

		ClassNamesPrimaryBase.Emplace(AssetTypeInfo.AssetBaseClassLoaded->GetFName());
	}

	// getting list of primary assets classes that are derived from main primary assets
	OutClassNames.Empty();
	GetModuleAssetRegistry().Get().GetDerivedClassNames(ClassNamesPrimaryBase.Array(), TSet<FName>{}, OutClassNames);
}

void UPjcHelperSubsystem::GetClassNamesEditor(TSet<FName>& OutClassNames)
{
	const TArray<FName> ClassNamesEditorBase{
		UEditorUtilityWidget::StaticClass()->GetFName(),
		UEditorUtilityBlueprint::StaticClass()->GetFName(),
		UEditorUtilityWidgetBlueprint::StaticClass()->GetFName(),
		UEditorTutorial::StaticClass()->GetFName()
	};

	OutClassNames.Empty();
	GetModuleAssetRegistry().Get().GetDerivedClassNames(ClassNamesEditorBase, TSet<FName>{}, OutClassNames);
}

void UPjcHelperSubsystem::GetAssetsDependencies(TSet<FAssetData>& Assets)
{
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

			GetModuleAssetRegistry().Get().GetDependencies(CurrentPackageName, Deps);

			Deps.RemoveAllSwap([&](const FName& Dep)
			{
				return !Dep.ToString().StartsWith(*PjcConstants::PathRoot.ToString());
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

	for (const auto& Dep : UsedAssetsDeps)
	{
		Filter.PackageNames.Add(Dep);
	}

	TArray<FAssetData> TempContainer;
	GetModuleAssetRegistry().Get().GetAssets(Filter, TempContainer);

	Assets.Append(TempContainer);
}

FString UPjcHelperSubsystem::PathNormalize(const FString& InPath)
{
	if (InPath.IsEmpty()) return {};

	// Ensure the path dont starts with a slash or a disk drive letter
	if (!(InPath.StartsWith(TEXT("/")) || InPath.StartsWith(TEXT("\\")) || (InPath.Len() > 2 && InPath[1] == ':')))
	{
		return {};
	}

	FString Path = FPaths::ConvertRelativePathToFull(InPath).TrimStartAndEnd();
	FPaths::RemoveDuplicateSlashes(Path);

	// Collapse any ".." or "." references in the path
	FPaths::CollapseRelativeDirectories(Path);

	if (FPaths::GetExtension(Path).IsEmpty())
	{
		FPaths::NormalizeDirectoryName(Path);
	}
	else
	{
		FPaths::NormalizeFilename(Path);
	}

	// Ensure the path does not end with a trailing slash
	if (Path.EndsWith(TEXT("/")) || Path.EndsWith(TEXT("\\")))
	{
		Path = Path.LeftChop(1);
	}

	return Path;
}

FString UPjcHelperSubsystem::PathConvertToAbsolute(const FString& InPath)
{
	const FString PathNormalized = PathNormalize(InPath);
	const FString PathProjectContent = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()).LeftChop(1);

	if (PathNormalized.IsEmpty()) return {};
	if (PathNormalized.StartsWith(PathProjectContent)) return PathNormalized;
	if (PathNormalized.StartsWith(PjcConstants::PathRoot.ToString()))
	{
		FString Path = PathNormalized;
		Path.RemoveFromStart(PjcConstants::PathRoot.ToString());

		return Path.IsEmpty() ? PathProjectContent : PathProjectContent / Path;
	}

	return {};
}

FString UPjcHelperSubsystem::PathConvertToRelative(const FString& InPath)
{
	const FString PathNormalized = PathNormalize(InPath);
	const FString PathProjectContent = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()).LeftChop(1);

	if (PathNormalized.IsEmpty()) return {};
	if (PathNormalized.StartsWith(PjcConstants::PathRoot.ToString())) return PathNormalized;
	if (PathNormalized.StartsWith(PathProjectContent))
	{
		FString Path = PathNormalized;
		Path.RemoveFromStart(PathProjectContent);

		return Path.IsEmpty() ? PjcConstants::PathRoot.ToString() : PjcConstants::PathRoot.ToString() / Path;
	}

	return {};
}

FString UPjcHelperSubsystem::PathConvertToObjectPath(const FString& InPath)
{
	if (FPaths::FileExists(InPath))
	{
		const FString FileName = FPaths::GetBaseFilename(InPath);
		const FString AssetPath = PathConvertToRelative(FPaths::GetPath(InPath));

		return FString::Printf(TEXT("%s/%s.%s"), *AssetPath, *FileName, *FileName);
	}

	const FString ObjectPath = FPackageName::ExportTextPathToObjectPath(InPath);

	if (!ObjectPath.StartsWith(PjcConstants::PathRoot.ToString())) return {};

	TArray<FString> Parts;
	ObjectPath.ParseIntoArray(Parts, TEXT("/"), true);

	if (Parts.Num() > 0)
	{
		FString Left;
		FString Right;
		Parts.Last().Split(TEXT("."), &Left, &Right);

		if (!Left.IsEmpty() && !Right.IsEmpty() && Left.Equals(*Right))
		{
			return ObjectPath;
		}
	}

	return {};
}

bool UPjcHelperSubsystem::EditorIsInPlayMode()
{
	return GEditor && GEditor->PlayWorld || GIsPlayInEditorWorld;
}

bool UPjcHelperSubsystem::ProjectContainsRedirectors()
{
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Emplace(PjcConstants::PathRoot);
	Filter.ClassNames.Emplace(UObjectRedirector::StaticClass()->GetFName());

	TArray<FAssetData> Redirectors;
	GetModuleAssetRegistry().Get().GetAssets(Filter, Redirectors);

	return Redirectors.Num() > 0;
}

bool UPjcHelperSubsystem::AssetIsBlueprint(const FAssetData& InAssetData)
{
	if (!InAssetData.IsValid()) return false;

	const UClass* AssetClass = InAssetData.GetClass();
	if (!AssetClass) return false;

	return AssetClass->IsChildOf(UBlueprint::StaticClass());
}

bool UPjcHelperSubsystem::AssetIsExtReferenced(const FAssetData& InAssetData)
{
	if (!InAssetData.IsValid()) return false;

	TArray<FName> Refs;
	GetModuleAssetRegistry().Get().GetReferencers(InAssetData.PackageName, Refs);

	return Refs.ContainsByPredicate([](const FName& Ref)
	{
		return !Ref.ToString().StartsWith(PjcConstants::PathRoot.ToString());
	});
}

bool UPjcHelperSubsystem::AssetIsCircular(const FAssetData& InAssetData)
{
	if (!InAssetData.IsValid()) return false;

	TArray<FName> Refs;
	TArray<FName> Deps;

	GetModuleAssetRegistry().Get().GetReferencers(InAssetData.PackageName, Refs);
	GetModuleAssetRegistry().Get().GetDependencies(InAssetData.PackageName, Deps);

	for (const auto& Ref : Refs)
	{
		if (Deps.Contains(Ref))
		{
			return true;
		}
	}

	return false;
}

FName UPjcHelperSubsystem::GetAssetExactClassName(const FAssetData& InAssetData)
{
	if (!InAssetData.IsValid()) return NAME_None;

	if (AssetIsBlueprint(InAssetData))
	{
		const FString GeneratedClassName = InAssetData.TagsAndValues.FindTag(TEXT("GeneratedClass")).GetValue();
		const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(*GeneratedClassName);
		return FName{*FPackageName::ObjectPathToObjectName(ClassObjectPath)};
	}

	return InAssetData.AssetClass;
}

void UPjcHelperSubsystem::GetAssetsByFilter(const FPjcAssetSearchFilter& InSearchFilter, TArray<FAssetData>& OutAssets)
{
	if (InSearchFilter.IsEmpty()) return;

	TArray<FAssetData> AssetsByPackagePaths;
	TArray<FAssetData> AssetsByClassNames;
	TArray<FAssetData> AssetsByObjectPaths;

	// loading assets by package paths
	{
		FARFilter Filter;
		Filter.bRecursivePaths = InSearchFilter.bRecursivePaths;

		Filter.PackagePaths.Reserve(InSearchFilter.PackagePaths.Num());

		for (const auto& PackagePath : InSearchFilter.PackagePaths)
		{
			const FName RelativePath = FName{*PathConvertToRelative(PackagePath.ToString())};
			if (!RelativePath.IsValid()) continue;

			Filter.PackagePaths.Emplace(RelativePath);
		}

		GetModuleAssetRegistry().Get().GetAssets(Filter, AssetsByPackagePaths);
	}

	// loading assets by class names
	{
		TArray<FAssetData> AssetsAll;
		GetModuleAssetRegistry().Get().GetAssetsByPath(PjcConstants::PathRoot, AssetsAll, true);

		TSet<FName> ClassNames;

		if (InSearchFilter.bRecursiveClasses)
		{
			GetModuleAssetRegistry().Get().GetDerivedClassNames(InSearchFilter.ClassNames, TSet<FName>{}, ClassNames);
		}
		else
		{
			ClassNames.Append(InSearchFilter.ClassNames);
		}

		AssetsByClassNames.Reserve(AssetsAll.Num());

		for (const auto& Asset : AssetsAll)
		{
			if (ClassNames.Contains(Asset.AssetClass) || ClassNames.Contains(GetAssetExactClassName(Asset)))
			{
				AssetsByClassNames.Emplace(Asset);
			}
		}

		AssetsByClassNames.Shrink();
	}

	// loading assets by object paths
	{
		FARFilter Filter;
		Filter.ObjectPaths.Reserve(InSearchFilter.ObjectPaths.Num());

		for (const auto& Path : InSearchFilter.ObjectPaths)
		{
			const FName ObjectPath = FName{*PathConvertToObjectPath(Path.ToString())};
			if (!ObjectPath.IsValid()) continue;

			Filter.ObjectPaths.Emplace(ObjectPath);
		}

		GetModuleAssetRegistry().Get().GetAssets(Filter, AssetsByObjectPaths);
	}

	TSet<FAssetData> AssetsAll;
	AssetsAll.Append(AssetsByPackagePaths);
	AssetsAll.Append(AssetsByClassNames);
	AssetsAll.Append(AssetsByObjectPaths);

	OutAssets.Reset();
	OutAssets = AssetsAll.Array();
}

void UPjcHelperSubsystem::GetFilesInPath(const FString& InSearchPath, const bool bSearchRecursive, TSet<FString>& OutFiles)
{
	OutFiles.Empty();

	struct FFindFilesVisitor : IPlatformFile::FDirectoryVisitor
	{
		TSet<FString>& Files;

		explicit FFindFilesVisitor(TSet<FString>& InFiles) : Files(InFiles) { }

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (!bIsDirectory)
			{
				Files.Emplace(FPaths::ConvertRelativePathToFull(FilenameOrDirectory));
			}

			return true;
		}
	};

	FFindFilesVisitor FindFilesVisitor{OutFiles};

	if (bSearchRecursive)
	{
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryRecursively(*InSearchPath, FindFilesVisitor);
	}
	else
	{
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*InSearchPath, FindFilesVisitor);
	}
}

void UPjcHelperSubsystem::GetFilesInPathByExt(const FString& InSearchPath, const bool bSearchRecursive, const bool bExtSearchInvert, const TSet<FString>& InExtensions, TSet<FString>& OutFiles)
{
	OutFiles.Empty();

	struct FFindFilesVisitor : IPlatformFile::FDirectoryVisitor
	{
		const bool bSearchInvert;
		TSet<FString>& Files;
		const TSet<FString>& Extensions;

		explicit FFindFilesVisitor(const bool bInSearchInvert, TSet<FString>& InFiles, const TSet<FString>& InExtensions)
			: bSearchInvert(bInSearchInvert),
			  Files(InFiles),
			  Extensions(InExtensions) { }

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (!bIsDirectory)
			{
				const FString FullPath = FPaths::ConvertRelativePathToFull(FilenameOrDirectory);

				if (Extensions.Num() == 0)
				{
					Files.Emplace(FullPath);
					return true;
				}

				const FString Ext = FPaths::GetExtension(FullPath, false);
				const bool bExistsInSearchList = Extensions.Contains(Ext);

				if (
					bExistsInSearchList && !bSearchInvert ||
					!bExistsInSearchList && bSearchInvert
				)
				{
					Files.Emplace(FullPath);
				}
			}

			return true;
		}
	};

	TSet<FString> ExtensionsNormalized;
	ExtensionsNormalized.Reserve(InExtensions.Num());

	for (const auto& Ext : InExtensions)
	{
		const FString ExtNormalized = Ext.Replace(TEXT("."), TEXT(""));
		ExtensionsNormalized.Emplace(ExtNormalized);
	}

	FFindFilesVisitor FindFilesVisitor{bExtSearchInvert, OutFiles, ExtensionsNormalized};
	if (bSearchRecursive)
	{
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryRecursively(*InSearchPath, FindFilesVisitor);
	}
	else
	{
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*InSearchPath, FindFilesVisitor);
	}
}

void UPjcHelperSubsystem::GetFoldersInPath(const FString& InSearchPath, const bool bSearchRecursive, TSet<FString>& OutFolders)
{
	OutFolders.Empty();

	struct FFindFoldersVisitor : IPlatformFile::FDirectoryVisitor
	{
		TSet<FString>& Folders;

		explicit FFindFoldersVisitor(TSet<FString>& InFolders) : Folders(InFolders) { }

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (bIsDirectory)
			{
				Folders.Emplace(FPaths::ConvertRelativePathToFull(FilenameOrDirectory));
			}

			return true;
		}
	};

	FFindFoldersVisitor FindFoldersVisitor{OutFolders};
	if (bSearchRecursive)
	{
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryRecursively(*InSearchPath, FindFoldersVisitor);
	}
	else
	{
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*InSearchPath, FindFoldersVisitor);
	}
}

FAssetRegistryModule& UPjcHelperSubsystem::GetModuleAssetRegistry()
{
	return FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistry);
}

FAssetToolsModule& UPjcHelperSubsystem::GetModuleAssetTools()
{
	return FModuleManager::LoadModuleChecked<FAssetToolsModule>(PjcConstants::ModuleAssetTools);
}
