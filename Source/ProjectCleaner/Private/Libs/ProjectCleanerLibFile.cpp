// // Copyright Ashot Barkhudaryan. All Rights Reserved.
//
// #include "Libs/ProjectCleanerLibFile.h"
// #include "Libs/ProjectCleanerLibPath.h"
// // Engine Headers
// #include "AssetRegistry/AssetRegistryModule.h"
//
// int64 UProjectCleanerLibFile::GetFilesTotalSize(const TArray<FString>& Files)
// {
// 	if (Files.Num() == 0) return 0;
//
// 	int64 TotalSize = 0;
// 	for (const auto& File : Files)
// 	{
// 		if (File.IsEmpty() || !FPaths::FileExists(File)) continue;
//
// 		TotalSize += IFileManager::Get().FileSize(*File);
// 	}
//
// 	return TotalSize;
// }
//
// void UProjectCleanerLibFile::GetFilesNonEngine(TArray<FString>& FilesNonEngine)
// {
// 	TArray<FString> FilesAll;
// 	// IFileManager::Get().FindFilesRecursive(FilesAll, *UProjectCleanerLibPath::GetFolderContent(), TEXT("*.*"), true, false);
//
// 	FilesNonEngine.Empty();
// 	FilesNonEngine.Reserve(FilesAll.Num());
//
// 	for (const auto& File : FilesAll)
// 	{
// 		if (FileHasEngineExtension(File)) continue;
//
// 		FilesNonEngine.AddUnique(File);
// 	}
//
// 	FilesNonEngine.Shrink();
// }
//
// void UProjectCleanerLibFile::GetFilesCorrupted(TArray<FString>& FilesCorrupted)
// {
// 	TArray<FString> FilesAll;
// 	// IFileManager::Get().FindFilesRecursive(FilesAll, *UProjectCleanerLibPath::GetFolderContent(), TEXT("*.*"), true, false);
//
// 	FilesCorrupted.Empty();
// 	FilesCorrupted.Reserve(FilesAll.Num());
//
// 	for (const auto& File : FilesAll)
// 	{
// 		if (!FileHasEngineExtension(File)) continue;
// 		if (!FileIsCorrupted(File)) continue;
//
// 		FilesCorrupted.AddUnique(File);
// 	}
//
// 	FilesCorrupted.Shrink();
// }
//
// void UProjectCleanerLibFile::GetFolders(const FString& InPath, TArray<FString>& Folders, const bool bRecursive)
// {
// 	Folders.Empty();
//
// 	if (InPath.IsEmpty()) return;
// 	// if (UProjectCleanerLibPath::ConvertToAbs(InPath).IsEmpty()) return;
//
// 	if (bRecursive)
// 	{
// 		IFileManager::Get().FindFilesRecursive(Folders, *InPath, TEXT("*.*"), false, true);
// 	}
// 	else
// 	{
// 		TArray<FString> FoldersAll;
// 		IFileManager::Get().FindFiles(FoldersAll, *(InPath / TEXT("*")), false, true);
//
// 		Folders.Reserve(FoldersAll.Num());
// 		for (const auto& Folder : FoldersAll)
// 		{
// 			Folders.AddUnique(InPath / Folder);
// 		}
// 	}
// }
//
// void UProjectCleanerLibFile::GetFoldersEmpty(TArray<FString>& FoldersEmpty)
// {
// 	TArray<FString> FoldersAll;
// 	// IFileManager::Get().FindFilesRecursive(FoldersAll, *UProjectCleanerLibPath::GetFolderContent(), TEXT("*.*"), false, true);
//
// 	FoldersEmpty.Empty();
// 	FoldersEmpty.Reserve(FoldersAll.Num());
//
// 	for (const auto& Folder : FoldersAll)
// 	{
// 		if (FolderIsEmpty(Folder))
// 		{
// 			FoldersEmpty.AddUnique(Folder);
// 		}
// 	}
//
// 	FoldersEmpty.Shrink();
// }
//
// bool UProjectCleanerLibFile::FolderIsEmpty(const FString& InPath)
// {
// 	if (InPath.IsEmpty()) return false;
// 	if (!FPaths::DirectoryExists(InPath)) return false;
//
// 	TArray<FString> Files;
// 	IFileManager::Get().FindFilesRecursive(Files, *InPath, TEXT("*.*"), true, false);
//
// 	return Files.Num() == 0;
// }
//
// bool UProjectCleanerLibFile::FolderIsExcluded(const FString& InPath)
// {
// 	return false; // todo:ashe23
// }
//
// // bool UProjectCleanerLibFile::FileIsCorrupted(const FString& FilePathAbs)
// // {
// // 	if (!FPaths::FileExists(FilePathAbs)) return false;
// //
// // 	const FString RelativePath = UProjectCleanerLibPath::ConvertToRel(FilePathAbs);
// //
// // 	// here we got absolute path "C:/MyProject/Content/material.uasset"
// // 	// we must first convert that path to In Engine Internal Path like "/Game/material.uasset"
// // 	if (RelativePath.IsEmpty()) return false;
// //
// // 	// Converting file path to object path (This is for searching in AssetRegistry)
// // 	// example "/Game/Name.uasset" => "/Game/Name.Name"
// // 	FString ObjectPath = RelativePath;
// // 	ObjectPath.RemoveFromEnd(FPaths::GetExtension(RelativePath, true));
// // 	ObjectPath.Append(TEXT(".") + FPaths::GetBaseFilename(RelativePath));
// //
// // 	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
// // 	const FAssetData AssetData = ModuleAssetRegistry.Get().GetAssetByObjectPath(FName{*ObjectPath});
// //
// // 	// if its does not exist in asset registry, then something wrong with asset
// // 	return !AssetData.IsValid();
// // }
//
// bool UProjectCleanerLibFile::FileHasEngineExtension(const FString& FilePathAbs)
// {
// 	const FString Extension = FPaths::GetExtension(FilePathAbs).ToLower();
//
// 	TSet<FString> EngineExtensions;
// 	EngineExtensions.Reserve(3);
// 	EngineExtensions.Add(TEXT("uasset"));
// 	EngineExtensions.Add(TEXT("umap"));
// 	EngineExtensions.Add(TEXT("collection"));
//
// 	return EngineExtensions.Contains(Extension);
// }
//
// bool UProjectCleanerLibFile::FolderIsEngineGenerated(const FString& InPath)
// {
// 	return false;
// 	// TSet<FString> Folders;
// 	// Folders.Reserve(4);
// 	// Folders.Add(UProjectCleanerLibPath::GetFolderCollections());
// 	// Folders.Add(UProjectCleanerLibPath::GetFolderDevelopers());
// 	// Folders.Add(UProjectCleanerLibPath::GetFolderDevelopersUser());
// 	// Folders.Add(UProjectCleanerLibPath::GetFolderCollectionsUser());
// 	//
// 	// return Folders.Contains(InPath);
// }
