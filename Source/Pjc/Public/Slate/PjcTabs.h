// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FTabManager;

enum class EPjcAssetSet : uint8;

// Links between the Project Cleaner tools
namespace PjcTabs
{
	void Open(const TWeakPtr<FTabManager>& Tabs, const FName TabId);

	// Tabs of the Project Cleaner window, for links from windows outside the tools (Clean project)
	void SetMain(const TSharedPtr<FTabManager>& Tabs);
	TWeakPtr<FTabManager> GetMain();

	// A Scan or Clean project window is open: the tool content behind it is greyed
	void SetWindowOpen(const bool bOpen);
	bool IsWindowOpen();

	// Shows the asset in the Inspector
	void Inspect(const TWeakPtr<FTabManager>& Tabs, const FName Package);

	// Shows the folder in the Unused list of Assets
	void ShowFolder(const TWeakPtr<FTabManager>& Tabs, const FString& Path);

	// Shows a list of Assets, only the assets of one reason when given
	void ShowSet(const TWeakPtr<FTabManager>& Tabs, const EPjcAssetSet Set, const FString& Reason = FString {});

	// Shows a category of Issues, EPjcIssueCategory as number; for assets to review only one reason when given
	void ShowIssues(const TWeakPtr<FTabManager>& Tabs, const uint8 Category, const FString& Reason = FString {});

	// Shows the assets the last delete could not remove in Assets
	void ShowNotDeleted(const TWeakPtr<FTabManager>& Tabs);
}	// namespace PjcTabs
