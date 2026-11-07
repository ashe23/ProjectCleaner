// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcScanResult.h"

// What a redirector is to the project
enum class EPjcRedirectorState : uint8
{
	// assets still point through it: fixing resaves them, then deletes it
	InUse,
	// nothing points through it: fixing deletes it
	NotReferenced,
	// its target is unused: cleanup deletes it with the target
	TargetUnused,
	// its target is missing: assets that use it need a repair by hand
	Broken,
};

// Redirector of the scanned content, for the Issues tool
struct FPjcRedirectorInfo
{
	FAssetData Asset;
	// Package it points to, None when not known
	FName Target;
	bool bTargetExists = true;
	// Assets that point through it (redirectors left out)
	TArray<FName> UsedBy;
	EPjcRedirectorState State = EPjcRedirectorState::NotReferenced;

	// Broken and used: nothing to resave it to
	bool CanFix() const {
		return !(State == EPjcRedirectorState::Broken && UsedBy.Num() > 0);
	}
};

namespace PjcIssues
{
	// Redirectors of a scan with what they point to, who uses them and their state
	void GetRedirectors(const FPjcScanResult& Result, TArray<FPjcRedirectorInfo>& OutRedirectors);

	FString GetStateName(const EPjcRedirectorState State);
	FString GetStateDescription(const EPjcRedirectorState State);
}	// namespace PjcIssues
