// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "Widgets/SCompoundWidget.h"

class SWindow;

// Asks before deleting assets: the selection, the unused assets that go with it, and whether version control can restore them
class SPjcDeleteDialog final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcDeleteDialog) {}
	SLATE_ARGUMENT(TArray<FAssetData>, Selected)
	SLATE_ARGUMENT(TArray<FAssetData>, With)
	// Packages the last delete could not remove, and what held them
	SLATE_ARGUMENT(TArray<FString>, Held)
	SLATE_ARGUMENT(TSharedPtr<SWindow>, Window)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Shows the dialog as a modal window, true when the user confirms
	static bool Confirm(const TArray<FAssetData>& Selected, const TArray<FAssetData>& With, const TArray<FString>& Held);

	// Title the dialog window gets, e.g. "Delete 3 assets"
	static FText GetTitle(const int32 NumAssets);

  private:

	static TSharedRef<SWidget> MakeList(const TArray<FAssetData>& Assets);

	TWeakPtr<SWindow> Window;
	bool bConfirmed = false;
};
