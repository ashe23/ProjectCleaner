// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcOverview.h"
#include "PjcCookInfo.h"
#include "PjcShim.h"
#include "PjcSubsystem.h"
// Engine Headers
#include "HAL/FileManager.h"

FPjcOverview FPjcOverview::Make(const FPjcScanResult& Result) {
	FPjcOverview Overview;
	TMap<EPjcUsageReason, FTotal> Reasons;
	TMap<FName, FClass> Classes;

	for (const auto& Asset : Result.Assets) {
		const int64 Size = Result.GetSize(Asset.PackageName);
		const EPjcAssetStatus Status = Result.GetStatus(Asset.PackageName);
		Overview.All.Add(Size);

		const FName ClassName = PjcShim::GetAssetClassName(Asset);
		FClass& Class = Classes.FindOrAdd(ClassName);
		Class.Name = ClassName;
		Class.Total.Add(Size);

		switch (Status) {
			case EPjcAssetStatus::Used :
				Overview.Used.Add(Size);
				Class.SizeUsed += Size;
				break;
			case EPjcAssetStatus::EditorOnly :
				Overview.EditorOnly.Add(Size);
				Class.SizeEditorOnly += Size;
				break;
			case EPjcAssetStatus::Review :
				Overview.Review.Add(Size);
				Class.SizeReview += Size;
				break;
			case EPjcAssetStatus::Unused :
				Overview.Unused.Add(Size);
				Class.SizeUnused += Size;
				break;
		}

		// reasons tell why assets are used: neither unused nor review ones
		if (Status == EPjcAssetStatus::Used || Status == EPjcAssetStatus::EditorOnly) {
			Reasons.FindOrAdd(GetGroupReason(Result, Asset.PackageName)).Add(Size);
		}
	}

	for (const auto& Pair : Reasons) {
		Overview.Reasons.Add(FReason {Pair.Key, Pair.Value});
	}
	Overview.Reasons.Sort([](const FReason& A, const FReason& B) {
		return A.Total.Size != B.Total.Size ? A.Total.Size > B.Total.Size : A.Total.Num > B.Total.Num;
	});

	Classes.GenerateValueArray(Overview.Classes);
	Overview.Classes.Sort([](const FClass& A, const FClass& B) {
		return A.Total.Size != B.Total.Size ? A.Total.Size > B.Total.Size : A.Name.LexicalLess(B.Name);
	});

	MakeUnusedFolders(Result, Overview);

	for (const auto& Asset : Result.Unused) {
		Overview.LargestUnused.Add(FAsset {Asset.PackageName, PjcShim::GetAssetClassName(Asset), Result.GetSize(Asset.PackageName)});
	}
	Overview.LargestUnused.Sort([](const FAsset& A, const FAsset& B) {
		return A.Size != B.Size ? A.Size > B.Size : A.Package.LexicalLess(B.Package);
	});
	if (Overview.LargestUnused.Num() > MaxLargestUnused) {
		Overview.LargestUnused.SetNum(MaxLargestUnused);
	}

	return Overview;
}

void FPjcOverview::MakeUnusedFolders(const FPjcScanResult& Result, FPjcOverview& Overview) {
	// every folder with what is inside it, subfolders included
	TMap<FString, FFolder> Folders;
	for (const auto& Asset : Result.Assets) {
		const int64 Size = Result.GetSize(Asset.PackageName);
		const bool bUnused = Result.GetStatus(Asset.PackageName) == EPjcAssetStatus::Unused;

		FString Path = Asset.PackagePath.ToString();
		while (!Path.IsEmpty()) {
			FFolder& Folder = Folders.FindOrAdd(Path);
			Folder.Path = Path;
			Folder.All.Add(Size);
			if (bUnused) {
				Folder.Unused.Add(Size);
			}

			int32 Slash = INDEX_NONE;
			Path = Path.FindLastChar(TEXT('/'), Slash) && Slash > 0 ? Path.Left(Slash) : FString {};
		}
	}

	// mostly unused: the topmost such folder stands for its subfolders
	const auto IsMostlyUnused = [](const FFolder& Folder) {
		return Folder.Unused.Size > 0 && Folder.Unused.Size * 2 >= Folder.All.Size;
	};
	for (const auto& Pair : Folders) {
		if (!IsMostlyUnused(Pair.Value)) continue;

		int32 Slash = INDEX_NONE;
		const FFolder* Parent = Pair.Key.FindLastChar(TEXT('/'), Slash) && Slash > 0 ? Folders.Find(Pair.Key.Left(Slash)) : nullptr;
		if (Parent && IsMostlyUnused(*Parent)) continue;

		Overview.UnusedFolders.Add(Pair.Value);
	}
	Overview.UnusedFolders.Sort([](const FFolder& A, const FFolder& B) {
		return A.Unused.Size != B.Unused.Size ? A.Unused.Size > B.Unused.Size : A.Path < B.Path;
	});
}

FPjcOverview::FShipped FPjcOverview::MakeShipped(const FPjcScanResult& Result, const FPjcCookInfo& Cook) {
	FShipped Shipped;
	TSet<FName> Scanned;
	TMap<EPjcUsageReason, FTotal> Reasons;

	for (const auto& Asset : Result.Assets) {
		const FName Package = Asset.PackageName;
		Scanned.Add(Package);

		const int64 Size = Result.GetSize(Package);
		const bool bUnused = Result.GetStatus(Package) == EPjcAssetStatus::Unused;
		if (const int64* CookedSize = Cook.Cooked.Find(Package)) {
			(bUnused ? Shipped.UnusedShipped : Shipped.Shipped).Add(Size);
			(bUnused ? Shipped.UnusedShippedCooked : Shipped.ShippedCooked) += *CookedSize;
		}
		else if (bUnused) {
			Shipped.Unused.Add(Size);
		}
		else {
			Shipped.UsedNotShipped.Add(Size);
			Reasons.FindOrAdd(GetGroupReason(Result, Package)).Add(Size);
		}

		// assets the cook did not see matter when the game uses them: they are new since (what reaches them changed
		// too, or they are a root such as a map)
		const bool bSeen = Cook.SavedDigests.Contains(Package);
		if (bSeen || Result.GetStatus(Package) == EPjcAssetStatus::Used) {
			Shipped.NumChanged += Cook.IsSavedSince(Package) ? 1 : 0;
		}
	}
	for (const auto& Redirector : Result.Redirectors) {
		Scanned.Add(Redirector.PackageName);
	}

	for (const auto& Pair : Reasons) {
		Shipped.UsedNotShippedReasons.Add(FReason {Pair.Key, Pair.Value});
	}
	Shipped.UsedNotShippedReasons.Sort([](const FReason& A, const FReason& B) {
		return A.Total.Num != B.Total.Num ? A.Total.Num > B.Total.Num : A.Total.Size > B.Total.Size;
	});

	// cooked packages the scan does not have: generated by the cook, outside the scan, or deleted since
	const TArray<FString> RootPaths = UPjcSubsystem::GetProjectRootPaths();
	TMap<FString, int32> Roots;
	for (const auto& Pair : Cook.Cooked) {
		if (Scanned.Contains(Pair.Key)) continue;

		const FString Path = Pair.Key.ToString();
		if (Path.Contains(TEXT("/_Generated_/"))) {
			++Shipped.NumGenerated;
			continue;
		}

		const bool bInScan = RootPaths.ContainsByPredicate([&Path](const FString& RootPath) {
			return Path.Equals(RootPath) || Path.StartsWith(RootPath + TEXT("/"));
		});
		if (bInScan && !PjcShim::DoesPackageExist(Path, nullptr)) {
			++Shipped.NumDeleted;
			continue;
		}

		++Shipped.NumOutside;
		Shipped.SizeOutside += Pair.Value;
		// top folder: /Engine, /SomePlugin
		const int32 Slash = Path.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 1);
		Roots.FindOrAdd(Slash > 0 ? Path.Left(Slash) : Path)++;
	}
	for (const auto& Pair : Roots) {
		Shipped.OutsideRoots.Emplace(Pair.Key, Pair.Value);
	}
	Shipped.OutsideRoots.Sort([](const TPair<FString, int32>& A, const TPair<FString, int32>& B) {
		return A.Value != B.Value ? A.Value > B.Value : A.Key < B.Key;
	});

	return Shipped;
}

EPjcUsageReason FPjcOverview::GetGroupReason(const FPjcScanResult& Result, const FName Package) {
	const EPjcUsageReason Reason = Result.GetChainReason(Package);
	return Reason == EPjcUsageReason::FolderLabelAsset ? EPjcUsageReason::Primary : Reason;
}
