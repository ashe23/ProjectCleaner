// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcUnused.h"
#include "PjcSubsystem.h"

FPjcUnusedTree FPjcUnusedTree::Make(const FPjcScanResult& Result) {
	FPjcUnusedTree Tree;

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
		const bool bUnused = Result.GetStatus(Asset.PackageName) == EPjcAssetStatus::Unused;

		TSharedPtr<FPjcUnusedFolder> Folder = Ensure(Asset.PackagePath.ToString());
		Folder->Assets.Add(Index);
		for (; Folder.IsValid(); Folder = Folder->Parent.Pin()) {
			Folder->All.Add(Size);
			if (bUnused) {
				Folder->Unused.Add(Size);
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
		return A->Unused.Size != B->Unused.Size ? A->Unused.Size > B->Unused.Size : A->Path < B->Path;
	});

	return Tree;
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
			// unused: nothing used reaches it (redirectors included, usage has them)
			const FPjcAssetUsage* Usage = Result.Usage.Find(Referencer);
			if (!Usage || Usage->bUsed || Seen.Contains(Referencer)) continue;

			Seen.Add(Referencer);
			OutGroup.Add(Referencer);
			Stack.Add(Referencer);
		}
	}
}

void FPjcUnusedTree::GetUnused(const FPjcScanResult& Result, const FString& Path, const bool bSubfolders, TArray<int32>& OutAssets) const {
	OutAssets.Reset();

	TArray<TSharedPtr<FPjcUnusedFolder>> Stack;
	if (const TSharedPtr<FPjcUnusedFolder> Folder = Find(Path)) {
		Stack.Add(Folder);
	}
	while (Stack.Num() > 0) {
		const TSharedPtr<FPjcUnusedFolder> Folder = Stack.Pop();
		for (const int32 Index : Folder->Assets) {
			if (Result.GetStatus(Result.Assets[Index].PackageName) == EPjcAssetStatus::Unused) {
				OutAssets.Add(Index);
			}
		}
		if (bSubfolders) {
			Stack.Append(Folder->Children);
		}
	}
}
