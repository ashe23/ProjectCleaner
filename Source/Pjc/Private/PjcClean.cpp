// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcClean.h"
#include "PjcIssues.h"
#include "PjcShim.h"
#include "PjcSubsystem.h"
// Engine Headers
#include "Engine/PrimaryAssetLabel.h"

FPjcCleanPlan FPjcCleanPlan::Make(const FPjcScanResult& Result) {
	FPjcCleanPlan Plan;
	Plan.Assets = Result.Unused;
	Plan.Assets.Sort([&Result](const FAssetData& A, const FAssetData& B) {
		return Result.GetSize(A.PackageName) > Result.GetSize(B.PackageName);
	});

	TSet<FName> Deleted;
	TMap<FString, FPjcOverview::FTotal> Folders;
	TMap<FName, FPjcOverview::FTotal> Classes;
	for (const auto& Asset : Plan.Assets) {
		const int64 Size = Result.GetSize(Asset.PackageName);
		Plan.Size += Size;
		Deleted.Add(Asset.PackageName);

		// two levels: /Game/Characters
		const FString Path = Asset.PackagePath.ToString();
		const int32 Second = Path.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 1);
		const int32 Third = Second > 0 ? Path.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Second + 1) : INDEX_NONE;
		Folders.FindOrAdd(Third > 0 ? Path.Left(Third) : Path).Add(Size);
		Classes.FindOrAdd(PjcShim::GetAssetClassName(Asset)).Add(Size);
	}
	for (const auto& Pair : Folders) {
		Plan.Folders.Emplace(Pair.Key, Pair.Value);
	}
	for (const auto& Pair : Classes) {
		Plan.Classes.Emplace(Pair.Key, Pair.Value);
	}
	Plan.Folders.Sort([](const TPair<FString, FPjcOverview::FTotal>& A, const TPair<FString, FPjcOverview::FTotal>& B) {
		return A.Value.Size != B.Value.Size ? A.Value.Size > B.Value.Size : A.Key < B.Key;
	});
	Plan.Classes.Sort([](const TPair<FName, FPjcOverview::FTotal>& A, const TPair<FName, FPjcOverview::FTotal>& B) {
		return A.Value.Size != B.Value.Size ? A.Value.Size > B.Value.Size : A.Key.LexicalLess(B.Key);
	});

	Plan.NumFoldersEmpty = CountFoldersEmptyAfter(Result, Deleted);

	// the redirectors Clean project removes: those that go with the deleted assets (chains, used only by unused assets too)
	TArray<FAssetData> RedirectorsStale;
	UPjcSubsystem::GetRedirectorsStale(Deleted, RedirectorsStale);
	Plan.NumRedirectors = RedirectorsStale.Num();
	Plan.NumRedirectorsLeft = FMath::Max(0, Result.Redirectors.Num() - Plan.NumRedirectors);

	Plan.NumExcluded = Result.Excluded.Num();
	for (const auto& Pair : Result.Status) {
		Plan.NumReview += Pair.Value == EPjcAssetStatus::Review ? 1 : 0;
	}
	return Plan;
}

int32 FPjcCleanPlan::CountFoldersEmptyAfter(const FPjcScanResult& Result, const TSet<FName>& Deleted) {
	// every folder below a content root with its assets, and how many of them go
	TMap<FString, TPair<int32, int32>> Folders;
	for (const auto& Asset : Result.Assets) {
		const bool bDeleted = Deleted.Contains(Asset.PackageName);
		FString Path = Asset.PackagePath.ToString();
		while (Path.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 1) > 0) {
			TPair<int32, int32>& Counts = Folders.FindOrAdd(Path);
			++Counts.Key;
			Counts.Value += bDeleted ? 1 : 0;
			int32 Slash = INDEX_NONE;
			Path.FindLastChar(TEXT('/'), Slash);
			Path.LeftInline(Slash);
		}
	}

	int32 NumEmpty = 0;
	for (const auto& Pair : Folders) {
		NumEmpty += Pair.Value.Key > 0 && Pair.Value.Key == Pair.Value.Value ? 1 : 0;
	}
	return NumEmpty + Result.FoldersEmpty.Num();
}

void FPjcCleanPlan::GetBrokenReferencers(const TArray<FName>& Deleted, TArray<FName>& OutReferencers) {
	OutReferencers.Reset();

	const TSet<FName> DeletedSet {Deleted};
	TSet<FName> Broken;
	TArray<FName> Referencers;
	for (const FName Package : Deleted) {
		if (FPackageName::DoesPackageExist(Package.ToString())) continue;

		Referencers.Reset();
		UPjcSubsystem::GetModuleAssetRegistry().Get().GetReferencers(Package, Referencers);
		for (const FName Referencer : Referencers) {
			if (DeletedSet.Contains(Referencer) || !UPjcSubsystem::PathIsInsideProject(Referencer.ToString())) continue;
			if (!FPackageName::DoesPackageExist(Referencer.ToString())) continue;

			TArray<FAssetData> ReferencerAssets;
			UPjcSubsystem::GetModuleAssetRegistry().Get().GetAssetsByPackageName(Referencer, ReferencerAssets);
			const bool bLabel = ReferencerAssets.ContainsByPredicate([](const FAssetData& Asset) {
				const UClass* AssetClass = Asset.GetClass();
				return AssetClass && AssetClass->IsChildOf(UPrimaryAssetLabel::StaticClass());
			});
			if (!bLabel) {
				Broken.Add(Referencer);
			}
		}
	}
	OutReferencers = Broken.Array();
	OutReferencers.Sort(FNameLexicalLess {});
}
