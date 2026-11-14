// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcIssues.h"
#include "PjcShim.h"
#include "PjcSubsystem.h"

namespace PjcIssues
{
	void GetRedirectors(const FPjcScanResult& Result, TArray<FPjcRedirectorInfo>& OutRedirectors) {
		OutRedirectors.Reset(Result.Redirectors.Num());

		TSet<FName> RedirectorPackages;
		for (const auto& Redirector : Result.Redirectors) {
			RedirectorPackages.Add(Redirector.PackageName);
		}

		const IAssetRegistry& AssetRegistry = UPjcSubsystem::GetModuleAssetRegistry().Get();
		// the package each redirector points to (a missing one stays recorded)
		TMap<FName, FName> TargetOf;
		TArray<FName> Names;
		for (const auto& Redirector : Result.Redirectors) {
			Names.Reset();
			AssetRegistry.GetDependencies(Redirector.PackageName, Names);
			Names.Sort(FNameLexicalLess {});
			for (const FName Dependency : Names) {
				if (Dependency.ToString().StartsWith(TEXT("/Script/")) || Dependency == Redirector.PackageName) continue;

				TargetOf.Add(Redirector.PackageName, Dependency);
				break;
			}
		}
#if ENGINE_MAJOR_VERSION < 5
		// UE 4.27: a level's actor files are not in the asset registry
		TSet<FName> ActorFileDependencies;
		if (Result.Redirectors.Num() > 0) {
			UPjcSubsystem::GetExternalActorsDependencies(ActorFileDependencies);
		}
#endif

		for (const auto& Redirector : Result.Redirectors) {
			FPjcRedirectorInfo& Info = OutRedirectors.AddDefaulted_GetRef();
			Info.Asset = Redirector;
			Info.Target = TargetOf.FindRef(Redirector.PackageName);
			// a chain (redirector to redirector) ends at the real asset
			Info.FinalTarget = Info.Target;
			for (int32 Step = 0; Step < 64 && RedirectorPackages.Contains(Info.FinalTarget) && TargetOf.Contains(Info.FinalTarget); ++Step) {
				Info.Via.Add(Info.FinalTarget);
				Info.FinalTarget = TargetOf[Info.FinalTarget];
			}
			Info.bTargetExists = !Info.FinalTarget.IsNone() && PjcShim::DoesPackageExist(Info.FinalTarget.ToString(), nullptr);

			// who points through it: assets of the scan, and content the scan leaves out
			Names.Reset();
			AssetRegistry.GetReferencers(Redirector.PackageName, Names);
			Names.Sort(FNameLexicalLess {});
			FName Outside;
			for (const FName Referencer : Names) {
				if (RedirectorPackages.Contains(Referencer)) continue;
				if (!UPjcSubsystem::PathIsInsideProject(Referencer.ToString())) {
					Outside = Outside.IsNone() ? Referencer : Outside;
					continue;
				}
				Info.UsedBy.Add(Referencer);
			}

			// what needs the old path besides assets: files that name it (C++, Python, config), content outside the scan
			const FPjcAssetUsage* Usage = Result.Usage.Find(Redirector.PackageName);
			const TArray<FPjcSourceRef>* Refs = Result.SourceRefs.Find(Redirector.PackageName);
			const bool bNamedUsage = Usage && (Usage->Reason == EPjcUsageReason::Indirect || Usage->Reason == EPjcUsageReason::ConfigFolder);
			const bool bNamed = (Refs && Refs->Num() > 0) || bNamedUsage;
			bool bOutside = !Outside.IsNone() || (Usage && Usage->Reason == EPjcUsageReason::EngineReferenced);
#if ENGINE_MAJOR_VERSION < 5
			bOutside = bOutside || ActorFileDependencies.Contains(Redirector.PackageName);
#endif
			if (bNamed) {
				const bool bLine = Refs && Refs->Num() > 0;
				const FString FromUsage = Usage ? UPjcSubsystem::GetUsageReasonText(*Usage) : FString {};
				Info.KeptBy = bLine ? FString::Printf(TEXT("Named in %s, line %d"), *(*Refs)[0].File, (*Refs)[0].Line) : FromUsage;
			}
			if (bOutside) {
				const FString FromUsage = Usage ? UPjcSubsystem::GetUsageReasonText(*Usage) : FString {TEXT("Used by a level's actor files")};
				Info.KeptBy = Outside.IsNone() ? FromUsage : TEXT("Used by ") + Outside.ToString() + TEXT(", which the scan leaves out");
			}

			const bool bUsersUnused = Info.UsedBy.Num() > 0 && !Info.UsedBy.ContainsByPredicate([&Result](const FName User) {
				return !Result.Status.Contains(User) || Result.GetStatus(User) != EPjcAssetStatus::Unused;
			});
			const bool bTargetUnused = Result.Status.Contains(Info.FinalTarget) && Result.GetStatus(Info.FinalTarget) == EPjcAssetStatus::Unused;
			if (!Info.bTargetExists) {
				Info.State = EPjcRedirectorState::Broken;
			}
			else if (bOutside) {
				Info.State = EPjcRedirectorState::UsedOutside;
			}
			else if (bNamed) {
				Info.State = EPjcRedirectorState::NamedInFiles;
			}
			else if (bUsersUnused) {
				Info.State = EPjcRedirectorState::UsedByUnused;
			}
			else if (Info.UsedBy.Num() > 0) {
				Info.State = EPjcRedirectorState::InUse;
			}
			else if (bTargetUnused) {
				Info.State = EPjcRedirectorState::TargetUnused;
			}
			else {
				Info.State = EPjcRedirectorState::NotReferenced;
			}
		}

		OutRedirectors.Sort([](const FPjcRedirectorInfo& A, const FPjcRedirectorInfo& B) {
			if (A.State != B.State) {
				return static_cast<uint8>(A.State) < static_cast<uint8>(B.State);
			}
			return A.Asset.PackageName.LexicalLess(B.Asset.PackageName);
		});
	}

	FString GetStateName(const EPjcRedirectorState State) {
		switch (State) {
			case EPjcRedirectorState::InUse :		  return TEXT("In use");
			case EPjcRedirectorState::NotReferenced : return TEXT("Not referenced");
			case EPjcRedirectorState::TargetUnused :  return TEXT("Target unused");
			case EPjcRedirectorState::Broken :		  return TEXT("Broken");
			case EPjcRedirectorState::NamedInFiles :  return TEXT("Named in files");
			case EPjcRedirectorState::UsedOutside :	  return TEXT("Used outside the scan");
			case EPjcRedirectorState::UsedByUnused :  return TEXT("Used only by unused");
		}
		return FString {};
	}

	FString GetStateDescription(const EPjcRedirectorState State) {
		switch (State) {
			case EPjcRedirectorState::InUse :		  return TEXT("Assets still point through it. Fix resaves them, then deletes the redirector.");
			case EPjcRedirectorState::NotReferenced : return TEXT("Nothing points through it. Fix deletes it.");
			case EPjcRedirectorState::TargetUnused :
				return TEXT("Its target is unused: Clean project deletes it with the target. Fix deletes it now.");
			case EPjcRedirectorState::Broken :
				return TEXT("Its target is missing. Assets that use it have broken references and need a repair by hand.");
			case EPjcRedirectorState::NamedInFiles :
				return TEXT(
					"C++, Python or config files name the old path. Deleting it breaks them: Fix keeps it, unless it adds a redirect for the "
				) TEXT("old path to Config/DefaultEngine.ini.");
			case EPjcRedirectorState::UsedOutside :
				return TEXT("Content the scan leaves out uses it (engine, other plugins, Developers folders, a level's actor files in UE 4.27). ")
					TEXT("Fix cannot resave that content: the redirector stays.");
			case EPjcRedirectorState::UsedByUnused :
				return TEXT("Only unused assets point through it: Clean project deletes it with them. Fix resaves them and deletes it now.");
		}
		return FString {};
	}
}	// namespace PjcIssues
