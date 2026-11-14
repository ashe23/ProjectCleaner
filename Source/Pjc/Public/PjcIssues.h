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
	// C++, Python or config files name its old path: deleting it breaks them, unless a redirect keeps the path working
	NamedInFiles,
	// content the scan leaves out uses it (engine, other plugins, Developers, UE 4.27 actor files): fixing cannot resave that
	UsedOutside,
	// only unused assets point through it: Clean project deletes it with them
	UsedByUnused,
};

// Redirector of the scanned content, for the Issues tool
struct FPjcRedirectorInfo
{
	FAssetData Asset;
	// Package it points to, None when not known
	FName Target;
	// The real asset at the end of a chain (redirector to redirector), and the redirectors on the way
	FName FinalTarget;
	TArray<FName> Via;
	bool bTargetExists = true;
	// Assets of the scan that point through it (redirectors left out)
	TArray<FName> UsedBy;
	EPjcRedirectorState State = EPjcRedirectorState::NotReferenced;
	// Why it must stay: the file and line that name it, or what outside the scan uses it
	FString KeptBy;

	// Broken and used: nothing to resave it to. Used outside the scan: fixing cannot resave that content
	bool CanFix() const {
		return !(State == EPjcRedirectorState::Broken && UsedBy.Num() > 0) && State != EPjcRedirectorState::UsedOutside;
	}
};

namespace PjcIssues
{
	// Redirectors of a scan with what they point to, who uses them and their state
	void GetRedirectors(const FPjcScanResult& Result, TArray<FPjcRedirectorInfo>& OutRedirectors);

	FString GetStateName(const EPjcRedirectorState State);
	FString GetStateDescription(const EPjcRedirectorState State);
}	// namespace PjcIssues
