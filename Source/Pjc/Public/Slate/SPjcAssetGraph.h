// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

class FPjcAssetGraph;
class FActiveTimerHandle;
struct FSlateBrush;

DECLARE_DELEGATE_OneParam(FPjcOnGraphAsset, const FName&);

/**
 * Draws asset graph: circle area is size on disk, color is status, lines are dependencies. Zoomed out, every folder region
 * is one bubble, zooming in opens it, then every group (asset with everything only it uses) opens the same way. Selected
 * asset is shown with its usage chain and links, everything else fades.
 */
class SPjcAssetGraph final : public SLeafWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcAssetGraph) {}
	/** Asset clicked, None when selection is cleared */
	SLATE_EVENT(FPjcOnGraphAsset, OnSelectionChanged)
	SLATE_EVENT(FPjcOnGraphAsset, OnAssetDoubleClicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Shows new graph, view fits it */
	void SetGraph(const TSharedPtr<FPjcAssetGraph>& InGraph);
	/** Selects asset without notifying, None or asset not in graph clears selection */
	void SetSelection(const FName& PackageName, const bool bMoveView);
	/** When active, only given assets are shown in full, others fade */
	void SetEmphasis(const TSet<FName>& Packages, const bool bActive);
	/** Fits selected asset with its links, or whole graph */
	void ZoomToFit();

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
	virtual bool SupportsKeyboardFocus() const override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;

  private:

	enum class EFocus : uint8
	{
		None,
		Neighbor,
		Chain,
		Selected,
	};

	struct FFocusLink
	{
		int32 From = INDEX_NONE;
		int32 To = INDEX_NONE;
		bool bChain = false;
	};

	/** Circle drawn last paint, Item is node index, or region index for regions */
	struct FDrawnItem
	{
		int32 Item = INDEX_NONE;
		FVector2D Center = FVector2D::ZeroVector;
		float Radius = 0.0f;
	};

	/** Link between shown circles. Ends are node indices, regions follow nodes (node count + region index) */
	struct FDrawnLink
	{
		int32 From = INDEX_NONE;
		int32 To = INDEX_NONE;
		int32 Count = 0;
	};

	EActiveTimerReturnType OnTimer(double InCurrentTime, float InDeltaTime);
	void StartTimer();
	void SelectNode(const int32 Node, const bool bNotify);
	void UpdateFocus();
	void FitFocus();
	void OpenRegion(const int32 Region);
	void MoveView(const FVector2D& Center, const float InZoom);
	void UpdateHovered();
	int32 FindNodeAt(const FVector2D& LocalPosition) const;
	int32 FindRegionAt(const FVector2D& LocalPosition) const;
	FVector2D GraphToLocal(const FVector2D& GraphPosition) const;
	FVector2D LocalToGraph(const FVector2D& LocalPosition) const;
	float GetFitZoom(const FBox2D& Box) const;
	float GetMinZoom() const;
	/** Shown circle of link end: region bubble, group bubble while group is closed, otherwise asset itself */
	float GetShownRadius(const int32 End) const;
	FVector2D GetShownCenter(const int32 End) const;
	float GetDim(const int32 Node) const;

	TSharedPtr<FPjcAssetGraph> Graph;
	FPjcOnGraphAsset OnSelectionChanged;
	FPjcOnGraphAsset OnAssetDoubleClicked;
	TWeakPtr<FActiveTimerHandle> TimerHandle;
	TArray<const FSlateBrush*> Icons;

	FVector2D ViewCenter = FVector2D::ZeroVector;
	float Zoom = 1.0f;
	FVector2D ViewCenterTarget = FVector2D::ZeroVector;
	float ZoomTarget = 1.0f;
	bool bViewMoving = false;
	/** wheel zoom keeps graph point under cursor in place */
	bool bZoomAnchored = false;
	FVector2D AnchorGraph = FVector2D::ZeroVector;
	FVector2D AnchorLocal = FVector2D::ZeroVector;
	bool bFitPending = false;
	/** selection is framed after next paint, when view has its new size (details panel opens next to it) */
	bool bFocusFitPending = false;
	/** clicked asset is kept on screen after next paint, details panel narrows the view */
	bool bKeepSelectedPending = false;
	mutable int32 NumPaints = 0;
	int32 FocusFitPaint = 0;

	int32 Selected = INDEX_NONE;
	int32 Hovered = INDEX_NONE;
	int32 HoveredRegion = INDEX_NONE;
	TArray<EFocus> Focus;
	TArray<FFocusLink> FocusLinks;
	TArray<bool> Emphasized;
	bool bEmphasisActive = false;
	int32 NumEmphasized = 0;

	FVector2D MouseLocal = FVector2D::ZeroVector;
	FVector2D MouseDownLocal = FVector2D::ZeroVector;
	bool bDragging = false;

	// paint results, used by input handling and next paint
	mutable FVector2D ViewSize = FVector2D {1.0, 1.0};
	mutable TArray<FDrawnItem> DrawnNodes;
	mutable TArray<FDrawnItem> DrawnRegions;
	mutable TArray<float> RegionOpenness;
	mutable TArray<float> Openness;
	mutable TArray<float> Alpha;
	mutable TArray<int32> Reps;
	mutable TArray<int32> LinkReps;
	mutable TArray<FDrawnLink> Links;
};
