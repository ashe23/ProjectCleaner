// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/PjcTabs.h"
#include "Slate/SPjcTabInspector.h"
#include "Slate/SPjcTabIssues.h"
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
		const TSharedPtr<SDockTab> Tab = Manager.IsValid() ? Manager->TryInvokeTab(FTabId {PjcConstants::TabInspector}) : nullptr;
		if (Tab.IsValid() && Tab->GetContent()->GetType() == TEXT("SPjcTabInspector")) {
			StaticCastSharedRef<SPjcTabInspector>(Tab->GetContent())->Inspect(Package);
		}
	}

	void ShowFolder(const TWeakPtr<FTabManager>& Tabs, const FString& Path) {
		const TSharedPtr<FTabManager> Manager = Tabs.Pin();
		const TSharedPtr<SDockTab> Tab = Manager.IsValid() ? Manager->TryInvokeTab(FTabId {PjcConstants::TabAssetsUnused}) : nullptr;
		if (Tab.IsValid() && Tab->GetContent()->GetType() == TEXT("SPjcTabUnused")) {
			const TSharedRef<SPjcTabUnused> Assets = StaticCastSharedRef<SPjcTabUnused>(Tab->GetContent());
			Assets->ShowSet(EPjcAssetSet::Unused);
			Assets->SelectFolder(Path);
		}
	}

	void ShowSet(const TWeakPtr<FTabManager>& Tabs, const EPjcAssetSet Set) {
		const TSharedPtr<FTabManager> Manager = Tabs.Pin();
		const TSharedPtr<SDockTab> Tab = Manager.IsValid() ? Manager->TryInvokeTab(FTabId {PjcConstants::TabAssetsUnused}) : nullptr;
		if (Tab.IsValid() && Tab->GetContent()->GetType() == TEXT("SPjcTabUnused")) {
			StaticCastSharedRef<SPjcTabUnused>(Tab->GetContent())->ShowSet(Set);
		}
	}

	void ShowIssues(const TWeakPtr<FTabManager>& Tabs, const uint8 Category) {
		const TSharedPtr<FTabManager> Manager = Tabs.Pin();
		const TSharedPtr<SDockTab> Tab = Manager.IsValid() ? Manager->TryInvokeTab(FTabId {PjcConstants::TabIssues}) : nullptr;
		if (Tab.IsValid() && Tab->GetContent()->GetType() == TEXT("SPjcTabIssues")) {
			StaticCastSharedRef<SPjcTabIssues>(Tab->GetContent())->ShowCategory(static_cast<EPjcIssueCategory>(Category));
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
