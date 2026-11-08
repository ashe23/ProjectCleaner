// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FTabManager;

enum class EPjcAssetSet : uint8;

// Links between the Project Cleaner tools
namespace PjcTabs
{
	void Open(const TWeakPtr<FTabManager>& Tabs, const FName TabId);

	// Shows the asset in the Inspector
	void Inspect(const TWeakPtr<FTabManager>& Tabs, const FName Package);

	// Shows the folder in the Unused list of Assets
	void ShowFolder(const TWeakPtr<FTabManager>& Tabs, const FString& Path);

	// Shows a list of Assets
	void ShowSet(const TWeakPtr<FTabManager>& Tabs, const EPjcAssetSet Set);

	// Shows a category of Issues, EPjcIssueCategory as number
	void ShowIssues(const TWeakPtr<FTabManager>& Tabs, const uint8 Category);

	// Shows the assets the last delete could not remove in Assets
	void ShowNotDeleted(const TWeakPtr<FTabManager>& Tabs);
}	// namespace PjcTabs
