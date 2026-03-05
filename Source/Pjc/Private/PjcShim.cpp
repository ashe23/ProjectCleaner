// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcShim.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/MultiBox/MultiBox.h"

namespace PjcShim
{
	const ISlateStyle& GetStyle() {
#if ENGINE_MAJOR_VERSION == 5
		return FAppStyle::Get();
#else
		return FEditorStyle::Get();
#endif
	}

	const FSlateBrush* GetBrush(const FName PropertyName, const ANSICHAR* Specifier) {
#if ENGINE_MAJOR_VERSION == 5
		return FAppStyle::GetBrush(PropertyName, Specifier);
#else
		return FEditorStyle::GetBrush(PropertyName, Specifier);
#endif
	}

	FSlateColor GetSlateColor(const FName PropertyName, const ANSICHAR* Specifier) {
#if ENGINE_MAJOR_VERSION == 5
		return FAppStyle::GetSlateColor(PropertyName, Specifier);
#else
		return FEditorStyle::GetSlateColor(PropertyName, Specifier);
#endif
	}

	void SetTabManagerMenuMultiBox(const TSharedPtr<FTabManager>& InTabManager, const TSharedRef<FMultiBox>& InMultiBox) {
		if (!InTabManager.IsValid()) return;

#if ENGINE_MAJOR_VERSION == 5
		InTabManager->SetMenuMultiBox(InMultiBox, nullptr);
#else
		InTabManager->SetMenuMultiBox(InMultiBox);
#endif
	}
}	// namespace PjcShim
