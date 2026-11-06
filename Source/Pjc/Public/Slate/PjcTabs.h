// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FTabManager;

/** Links between the Project Cleaner tools */
namespace PjcTabs
{
	void Open(const TWeakPtr<FTabManager>& Tabs, const FName TabId);

	/** Shows the asset in Asset Usage */
	void Inspect(const TWeakPtr<FTabManager>& Tabs, const FName Package);

	/** Shows the folder in Assets Unused */
	void ShowFolder(const TWeakPtr<FTabManager>& Tabs, const FString& Path);
}	// namespace PjcTabs
