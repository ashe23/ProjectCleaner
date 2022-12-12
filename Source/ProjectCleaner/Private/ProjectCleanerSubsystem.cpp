// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerSubsystem.h"
#include "ProjectCleanerConstants.h"
// Engine Headers
#include "AssetRegistry/AssetRegistryModule.h"
#include "Internationalization/Regex.h"
#include "Libs/ProjectCleanerLibPath.h"
#include "Misc/FileHelper.h"

void UProjectCleanerSubsystem::GetFilesNonEngine(TSet<FString>& FilesNonEngine)
{
	FilesNonEngine.Reset();

	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(
		Files,
		*UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute),
		TEXT("*.*"),
		true,
		false
	);

	for (const auto& File : Files)
	{
		const FString FilePath = FPaths::ConvertRelativePathToFull(File);
		const FString FileExtension = FPaths::GetExtension(File);

		if (UProjectCleanerLibPath::FileHasEngineExtension(FileExtension)) continue;

		FilesNonEngine.Add(FilePath);
	}
}

void UProjectCleanerSubsystem::GetFilesCorrupted(TSet<FString>& FilesCorrupted)
{
	FilesCorrupted.Reset();

	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(
		Files,
		*UProjectCleanerLibPath::FolderContent(EProjectCleanerPathType::Absolute),
		TEXT("*.*"),
		true,
		false
	);

	for (const auto& File : Files)
	{
		const FString FilePath = FPaths::ConvertRelativePathToFull(File);
		const FString FileExtension = FPaths::GetExtension(File);

		if (!UProjectCleanerLibPath::FileHasEngineExtension(FileExtension)) continue;
		if (!UProjectCleanerLibPath::FileIsCorrupted(FileExtension)) continue;

		FilesCorrupted.Add(FilePath);
	}
}

void UProjectCleanerSubsystem::GetAssetsIndirect(TArray<FProjectCleanerIndirectAsset>& AssetsIndirect)
{
	AssetsIndirect.Reset();

	TArray<FAssetData> AllAssets;
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	ModuleAssetRegistry.Get().GetAssetsByPath(ProjectCleanerConstants::PathRelRoot, AllAssets, true);

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

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
	struct FDirectoryVisitor : IPlatformFile::FDirectoryVisitor
	{
		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (bIsDirectory)
			{
				InstalledPlugins.Add(FPaths::ConvertRelativePathToFull(FilenameOrDirectory));
			}

			return true;
		}

		TArray<FString> InstalledPlugins;
	};

	FDirectoryVisitor Visitor;
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

	for (const auto& File : Files)
	{
		if (!PlatformFile.FileExists(*File)) continue;

		FString FileContent;
		FFileHelper::LoadFileToString(FileContent, *File);

		if (!UProjectCleanerLibPath::FileContainsIndirectAssets(FileContent)) continue;

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
