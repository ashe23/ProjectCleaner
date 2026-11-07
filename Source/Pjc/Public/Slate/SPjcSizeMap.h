// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcUnused.h"
#include "Widgets/SLeafWidget.h"

DECLARE_DELEGATE_OneParam(FPjcOnMapFolder, const FString& /* Path */);
DECLARE_DELEGATE_OneParam(FPjcOnMapAsset, int32 /* Index into FPjcScanResult::Assets */);

/**
 * Size map of a folder: nested boxes as big as their content, down to the assets. Area Unused sizes folders by their
 * unused assets and shows only those; area All shows every asset, folders tinted by their unused share.
 * A folder whose only content is one folder is one box ("Characters / Heroes").
 */
class SPjcSizeMap final : public SLeafWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcSizeMap) {}
	SLATE_EVENT(FPjcOnMapFolder, OnFolderClicked)
	SLATE_EVENT(FPjcOnMapAsset, OnAssetClicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Folder to show; Result must stay alive while shown (the shared scan result)
	void SetFolder(const FPjcScanResult* InResult, const TSharedPtr<FPjcUnusedFolder>& InFolder, const bool bInAreaAll);
	void SetSelected(const TSet<int32>& InSelected);

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled
	) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;

  private:

	enum class ECellKind : uint8
	{
		Folder,
		Asset,
		More,
	};

	struct FCell
	{
		FVector2D Position = FVector2D::ZeroVector;
		FVector2D Size = FVector2D::ZeroVector;
		ECellKind Kind = ECellKind::Asset;
		int32 Depth = 0;
		// Folder cell: the deepest folder of a one-child chain
		TSharedPtr<FPjcUnusedFolder> Folder;
		FString Label;
		int32 Asset = INDEX_NONE;
		bool bUnused = false;
		int64 Value = 0;
		int32 NumMore = 0;
	};

	struct FItem
	{
		ECellKind Kind = ECellKind::Asset;
		TSharedPtr<FPjcUnusedFolder> Folder;
		int32 Asset = INDEX_NONE;
		bool bUnused = false;
		int64 Value = 0;
		int32 NumMore = 0;
	};

	void GetItems(const FPjcUnusedFolder& Folder, TArray<FItem>& OutItems) const;
	void LayoutFolder(const FPjcUnusedFolder& Folder, const FVector2D& Position, const FVector2D& Size, const int32 Depth) const;
	void UpdateLayout(const FVector2D& Size) const;
	int32 FindCell(const FVector2D& LocalPosition) const;
	FText GetHoverText() const;

	FPjcOnMapFolder OnFolderClicked;
	FPjcOnMapAsset OnAssetClicked;

	const FPjcScanResult* Result = nullptr;
	TSharedPtr<FPjcUnusedFolder> Folder;
	bool bAreaAll = false;
	TSet<int32> Selected;

	mutable TArray<FCell> Cells;
	mutable FVector2D LayoutSize = FVector2D::ZeroVector;
	mutable bool bLayoutDirty = true;
	int32 Hovered = INDEX_NONE;
	int32 Pressed = INDEX_NONE;
};
