// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcUnused.h"
#include "PjcSubsystem.h"
// Engine Headers
#include "Engine/PrimaryAssetLabel.h"

FPjcUnusedTree FPjcUnusedTree::Make(const FPjcScanResult& Result, const EPjcAssetSet InSet) {
	FPjcUnusedTree Tree;
	Tree.Set = InSet;

	// folder and its parents, made when first needed
	TFunction<TSharedPtr<FPjcUnusedFolder>(const FString&)> Ensure;
	Ensure = [&Tree, &Ensure](const FString& Path) -> TSharedPtr<FPjcUnusedFolder> {
		if (const TSharedPtr<FPjcUnusedFolder>* Found = Tree.Folders.Find(Path)) return *Found;

		const TSharedPtr<FPjcUnusedFolder> Folder = MakeShared<FPjcUnusedFolder>();
		Folder->Path = Path;
		int32 Slash = INDEX_NONE;
		Path.FindLastChar(TEXT('/'), Slash);
		Folder->Name = Path.Mid(Slash + 1);
		Tree.Folders.Add(Path, Folder);

		if (Slash > 0) {
			const TSharedPtr<FPjcUnusedFolder> Parent = Ensure(Path.Left(Slash));
			Folder->Parent = Parent;
			Parent->Children.Add(Folder);
		}
		else {
			Tree.Roots.Add(Folder);
		}
		return Folder;
	};

	for (int32 Index = 0; Index < Result.Assets.Num(); ++Index) {
		const FAssetData& Asset = Result.Assets[Index];
		const int64 Size = Result.GetSize(Asset.PackageName);
		const bool bListed = IsInSet(Result, Asset.PackageName, InSet);

		TSharedPtr<FPjcUnusedFolder> Folder = Ensure(Asset.PackagePath.ToString());
		Folder->Assets.Add(Index);
		for (; Folder.IsValid(); Folder = Folder->Parent.Pin()) {
			Folder->All.Add(Size);
			if (bListed) {
				Folder->Listed.Add(Size);
			}
		}
	}

	for (const auto& EmptyFolder : Result.FoldersEmpty) {
		const FString Path = UPjcSubsystem::PathConvertToRelative(EmptyFolder);
		if (Path.StartsWith(TEXT("/"))) {
			Ensure(Path)->bEmpty = true;
		}
	}

	Tree.Roots.Sort([](const TSharedPtr<FPjcUnusedFolder>& A, const TSharedPtr<FPjcUnusedFolder>& B) {
		if (A->Path.Equals(TEXT("/Game")) != B->Path.Equals(TEXT("/Game"))) return A->Path.Equals(TEXT("/Game"));
		return A->Listed.Size != B->Listed.Size ? A->Listed.Size > B->Listed.Size : A->Path < B->Path;
	});

	return Tree;
}

bool FPjcUnusedTree::IsInSet(const FPjcScanResult& Result, const FName Package, const EPjcAssetSet InSet) {
	const EPjcAssetStatus Status = Result.GetStatus(Package);
	switch (InSet) {
		case EPjcAssetSet::Excluded : return Result.Excluded.Contains(Package);
		case EPjcAssetSet::Used :	  return Status == EPjcAssetStatus::Used || Status == EPjcAssetStatus::EditorOnly;
		case EPjcAssetSet::Review :	  return Status == EPjcAssetStatus::Review;
		case EPjcAssetSet::Unused :
		default :					  return Status == EPjcAssetStatus::Unused;
	}
}

void FPjcUnusedTree::GetDeleteGroup(const FPjcScanResult& Result, const TArray<FName>& Packages, TArray<FName>& OutGroup) {
	OutGroup.Reset();

	TSet<FName> Seen {Packages};
	TArray<FName> Stack = Packages;
	TArray<FName> Referencers;
	while (Stack.Num() > 0) {
		const FName Package = Stack.Pop();
		Referencers.Reset();
		UPjcSubsystem::GetModuleAssetRegistry().Get().GetReferencers(Package, Referencers);
		Referencers.Sort(FNameLexicalLess {});

		for (const FName Referencer : Referencers) {
			// unused: nothing used reaches it (redirectors included, usage has them). Review assets are kept only for review
			const FPjcAssetUsage* Usage = Result.Usage.Find(Referencer);
			const bool bReview = Result.Status.Contains(Referencer) && Result.GetStatus(Referencer) == EPjcAssetStatus::Review;
			if (!Usage || (Usage->bUsed && !bReview) || Seen.Contains(Referencer)) continue;

			Seen.Add(Referencer);
			OutGroup.Add(Referencer);
			Stack.Add(Referencer);
		}
	}
}

void FPjcUnusedTree::GetUsedHolders(const FPjcScanResult& Result, const TArray<FName>& Packages, TArray<FName>& OutHolders) {
	OutHolders.Reset();

	TArray<FName> Group;
	GetDeleteGroup(Result, Packages, Group);
	TSet<FName> Deleted {Packages};
	Deleted.Append(Group);

	TSet<FName> Labels;
	for (const auto& Asset : Result.Assets) {
		const UClass* AssetClass = Asset.GetClass();
		if (AssetClass && AssetClass->IsChildOf(UPrimaryAssetLabel::StaticClass())) {
			Labels.Add(Asset.PackageName);
		}
	}

	TSet<FName> Holders;
	TArray<FName> Referencers;
	for (const auto& Package : Deleted) {
		Referencers.Reset();
		UPjcSubsystem::GetModuleAssetRegistry().Get().GetReferencers(Package, Referencers);
		for (const FName Referencer : Referencers) {
			if (Deleted.Contains(Referencer) || Labels.Contains(Referencer) || !Result.Status.Contains(Referencer)) continue;
			if (Result.GetStatus(Referencer) == EPjcAssetStatus::Used || Result.GetStatus(Referencer) == EPjcAssetStatus::EditorOnly) {
				Holders.Add(Referencer);
			}
		}
	}
	OutHolders = Holders.Array();
	OutHolders.Sort(FNameLexicalLess {});
}

void FPjcUnusedTree::GetListed(const FPjcScanResult& Result, const FString& Path, const bool bSubfolders, TArray<int32>& OutAssets) const {
	OutAssets.Reset();

	TArray<TSharedPtr<FPjcUnusedFolder>> Stack;
	if (const TSharedPtr<FPjcUnusedFolder> Folder = Find(Path)) {
		Stack.Add(Folder);
	}
	while (Stack.Num() > 0) {
		const TSharedPtr<FPjcUnusedFolder> Folder = Stack.Pop();
		for (const int32 Index : Folder->Assets) {
			if (IsInSet(Result, Result.Assets[Index].PackageName, Set)) {
				OutAssets.Add(Index);
			}
		}
		if (bSubfolders) {
			Stack.Append(Folder->Children);
		}
	}
}
