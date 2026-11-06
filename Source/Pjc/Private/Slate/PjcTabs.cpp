// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/PjcTabs.h"
#include "Slate/SPjcTabAssetsUsage.h"
#include "Slate/SPjcTabUnused.h"
#include "PjcConstants.h"
// Engine Headers
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"

namespace PjcTabs
{
	void Open(const TWeakPtr<FTabManager>& Tabs, const FName TabId) {
		if (const TSharedPtr<FTabManager> Manager = Tabs.Pin()) {
			Manager->TryInvokeTab(FTabId {TabId});
		}
	}

	void Inspect(const TWeakPtr<FTabManager>& Tabs, const FName Package) {
		const TSharedPtr<FTabManager> Manager = Tabs.Pin();
		const TSharedPtr<SDockTab> Tab = Manager.IsValid() ? Manager->TryInvokeTab(FTabId {PjcConstants::TabAssetsUsage}) : nullptr;
		if (Tab.IsValid() && Tab->GetContent()->GetType() == TEXT("SPjcTabAssetsUsage")) {
			StaticCastSharedRef<SPjcTabAssetsUsage>(Tab->GetContent())->Inspect(Package);
		}
	}

	void ShowFolder(const TWeakPtr<FTabManager>& Tabs, const FString& Path) {
		const TSharedPtr<FTabManager> Manager = Tabs.Pin();
		const TSharedPtr<SDockTab> Tab = Manager.IsValid() ? Manager->TryInvokeTab(FTabId {PjcConstants::TabAssetsUnused}) : nullptr;
		if (Tab.IsValid() && Tab->GetContent()->GetType() == TEXT("SPjcTabUnused")) {
			StaticCastSharedRef<SPjcTabUnused>(Tab->GetContent())->SelectFolder(Path);
		}
	}

	void ShowNotDeleted(const TWeakPtr<FTabManager>& Tabs) {
		const TSharedPtr<FTabManager> Manager = Tabs.Pin();
		const TSharedPtr<SDockTab> Tab = Manager.IsValid() ? Manager->TryInvokeTab(FTabId {PjcConstants::TabAssetsUnused}) : nullptr;
		if (Tab.IsValid() && Tab->GetContent()->GetType() == TEXT("SPjcTabUnused")) {
			StaticCastSharedRef<SPjcTabUnused>(Tab->GetContent())->ShowNotDeleted();
		}
	}
}	// namespace PjcTabs
