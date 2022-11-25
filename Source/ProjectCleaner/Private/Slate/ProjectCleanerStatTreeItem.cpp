// // Copyright Ashot Barkhudaryan. All Rights Reserved.
//
// #include "Slate/ProjectCleanerStatTreeItem.h"
// #include "ProjectCleanerTypes.h"
// #include "AssetRegistry/AssetRegistryModule.h"
//
// // modified version of
// // Engine/Source/Runtime/Core/Private/GenericPlatform/GenericPlatformFile.cpp::FFindFilesVisitor
// // to find directories instead of files
// class FFindDirectoriesVisitor : public IPlatformFile::FDirectoryVisitor
// {
// public:
// 	IPlatformFile&   PlatformFile;
// 	FRWLock          FoundFilesLock;
// 	TArray<FString>& FoundFiles;
// 	FFindDirectoriesVisitor(IPlatformFile& InPlatformFile, TArray<FString>& InFoundFiles)
// 		: IPlatformFile::FDirectoryVisitor(EDirectoryVisitorFlags::ThreadSafe)
// 		, PlatformFile(InPlatformFile)
// 		, FoundFiles(InFoundFiles)
// 	{
// 	}
// 	virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
// 	{
// 		if (bIsDirectory)
// 		{
// 			FString FileName(FilenameOrDirectory);
// 			FRWScopeLock ScopeLock(FoundFilesLock, SLT_Write);
// 			FoundFiles.Emplace(MoveTemp(FileName));
// 		}
// 		return true;
// 	}
// };
//
// void SProjectCleanerStats::Construct(const FArguments& Args)
// {
// 	UpdateTreeItems();
//
// 	ChildSlot
// 	[
// 		SNew(STreeView<TWeakObjectPtr<UProjectCleanerStatTreeItem>>)
// 		.TreeItemsSource(&TreeItems)
// 		.OnGenerateRow(this, &SProjectCleanerStats::OnGenerateRow)
// 		.OnGetChildren(this, &SProjectCleanerStats::OnGetChildren)
// 		.HeaderRow(
// 			                                                            SNew(SHeaderRow)
// 			                                                            + SHeaderRow::Column(FName{TEXT("Path")})
// 			                                                            [
// 				                                                            SNew(STextBlock)
// 				                                                            .Text(FText::FromString(TEXT("Path")))
// 			                                                            ]
// 			                                                            + SHeaderRow::Column(FName{TEXT("Size")})
// 			                                                            [
// 				                                                            SNew(STextBlock)
// 				                                                            .Text(FText::FromString(TEXT("Size")))
// 			                                                            ]
// 			                                                            + SHeaderRow::Column(FName{TEXT("Files")})
// 																		[
// 																			SNew(STextBlock)
// 																			.Text(FText::FromString(TEXT("Assets")))
// 																		]
// 																	   + SHeaderRow::Column(FName{TEXT("Unused")})
// 																	   [
// 																		   SNew(STextBlock)
// 																		   .Text(FText::FromString(TEXT("Unused Assets")))
// 																	   ]
// 																		+ SHeaderRow::Column(FName{TEXT("Folders")})
// 																	   [
// 																		   SNew(STextBlock)
// 																		   .Text(FText::FromString(TEXT("Folders")))
// 																	   ]
// 																	   + SHeaderRow::Column(FName{TEXT("Empty")})
// 																	   [
// 																		   SNew(STextBlock)
// 																		   .Text(FText::FromString(TEXT("Empty Folders")))
// 																	   ]
// 																	   + SHeaderRow::Column(FName{TEXT("Progress")})
// 																	   [
// 																		   SNew(STextBlock)
// 																		   .Text(FText::FromString(TEXT("% of Parent (Size)")))
// 																	   ]
// 		                                                            )
// 	];
// }
//
// void SProjectCleanerStats::UpdateTreeItems()
// {
// 	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
// 	
// 	TreeItems.Reset();
//
// 	TArray<TWeakObjectPtr<UProjectCleanerStatTreeItem>> Stack;
// 	
// 	const FString RootDir = FPaths::ProjectContentDir();
// 	
// 	const auto RootItem = NewObject<UProjectCleanerStatTreeItem>();
// 	RootItem->Path = TEXT("Content");
// 	RootItem->PathFull = RootDir;
//
// 	Stack.Push(RootItem);
//
// 	while (Stack.Num() > 0)
// 	{
// 		const auto CurrentNode = Stack.Pop();
//
// 		TArray<FString> SubDirs;
// 		FFindDirectoriesVisitor FindDirectoriesVisitor(PlatformFile, SubDirs);
// 		PlatformFile.IterateDirectory(*CurrentNode->PathFull, FindDirectoriesVisitor);
// 		
//
// 		for (const auto& SubDir : SubDirs)
// 		{
// 			const auto SubDirItem = NewObject<UProjectCleanerStatTreeItem>();
// 			SubDirItem->PathFull = SubDir;
// 			SubDirItem->Path = FPaths::GetBaseFilename(SubDir);
// 			CurrentNode->SubDirs.Add(SubDirItem);
// 			Stack.Push(SubDirItem);
// 		}
// 	}
//
// 	TreeItems.Add(RootItem);
// 	
// 	if (TreeView.IsValid())
// 	{
// 		TreeView->RequestTreeRefresh();
// 	}
// 	
// }
//
// TSharedRef<ITableRow> SProjectCleanerStats::OnGenerateRow(TWeakObjectPtr<UProjectCleanerStatTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
// {
// 	return SNew(SProjectCleanerStatTreeItem, OwnerTable).ListItem(Item);
// }
//
// void SProjectCleanerStats::OnGetChildren(TWeakObjectPtr<UProjectCleanerStatTreeItem> Item, TArray<TWeakObjectPtr<UProjectCleanerStatTreeItem>>& OutChildren)
// {
// 	OutChildren.Append(Item->SubDirs);
// }
