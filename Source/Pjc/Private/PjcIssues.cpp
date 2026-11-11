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
		TArray<FName> Names;
		for (const auto& Redirector : Result.Redirectors) {
			FPjcRedirectorInfo& Info = OutRedirectors.AddDefaulted_GetRef();
			Info.Asset = Redirector;

			// the package it points to is its dependency (a missing one stays recorded)
			Names.Reset();
			AssetRegistry.GetDependencies(Redirector.PackageName, Names);
			Names.Sort(FNameLexicalLess {});
			for (const FName Dependency : Names) {
				if (Dependency.ToString().StartsWith(TEXT("/Script/")) || Dependency == Redirector.PackageName) continue;

				Info.Target = Dependency;
				break;
			}
			Info.bTargetExists = !Info.Target.IsNone() && PjcShim::DoesPackageExist(Info.Target.ToString(), nullptr);

			Names.Reset();
			AssetRegistry.GetReferencers(Redirector.PackageName, Names);
			Names.Sort(FNameLexicalLess {});
			for (const FName Referencer : Names) {
				if (!RedirectorPackages.Contains(Referencer)) {
					Info.UsedBy.Add(Referencer);
				}
			}

			if (!Info.bTargetExists) {
				Info.State = EPjcRedirectorState::Broken;
			}
			else if (Info.UsedBy.Num() > 0) {
				Info.State = EPjcRedirectorState::InUse;
			}
			else if (Result.Status.Contains(Info.Target) && Result.GetStatus(Info.Target) == EPjcAssetStatus::Unused) {
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
		}
		return FString {};
	}
}	// namespace PjcIssues
