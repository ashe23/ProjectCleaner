// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcInspector.h"
#include "Widgets/SLeafWidget.h"

DECLARE_DELEGATE_OneParam(FPjcOnInspectorNode, int32 /* Index into FPjcInspectorGraph::Nodes */);

/**
 * Draws an Inspector graph top to bottom: sources, roots, the paths down to the asset. Drag to pan, wheel to zoom,
 * F frames the selected node, Home fits the graph.
 */
class SPjcInspectorGraph final : public SLeafWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcInspectorGraph) {}
	SLATE_EVENT(FPjcOnInspectorNode, OnNodeClicked)
	SLATE_EVENT(FPjcOnInspectorNode, OnNodeDoubleClicked)
	SLATE_EVENT(FSimpleDelegate, OnViewChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Graph to show; Result must stay alive while shown (the shared scan result). The view fits it on the next paint.
	void SetGraph(const FPjcScanResult* InResult, const FPjcInspectorGraph& InGraph);
	const FPjcInspectorGraph& GetGraph() const;
	void SetSelected(const int32 Node);
	// Node marked while the pointer is over it somewhere else (the details lists)
	void SetHighlighted(const int32 Node);

	void FitAll();
	void FrameSelected();
	void ZoomBy(const float Factor);
	float GetZoom() const;

	static constexpr float NodeWidth = 214.0f;
	static constexpr float NodeHeight = 46.0f;
	static constexpr float ZoomMin = 0.2f;
	static constexpr float ZoomMax = 2.0f;

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
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;

  private:

	// Node as drawn: graph space position and its text
	struct FShownNode
	{
		FVector2D Position = FVector2D::ZeroVector;
		FString Title;
		FString Line;
		FLinearColor LineColor = FLinearColor::White;
		FLinearColor Stripe = FLinearColor::White;
		const FSlateBrush* Icon = nullptr;
		FLinearColor IconColor = FLinearColor::White;
		bool bUsed = true;
		FString ToolTip;
	};

	void UpdateLayout();
	void FitAuto(const FVector2D& ViewSize) const;
	void FitAll(const FVector2D& ViewSize) const;
	void Frame(const int32 Node, const FVector2D& ViewSize) const;
	int32 FindNodeAt(const FVector2D& LocalPosition) const;
	FVector2D ToLocal(const FVector2D& GraphPosition) const;
	FText GetHoverText() const;

	FPjcOnInspectorNode OnNodeClicked;
	FPjcOnInspectorNode OnNodeDoubleClicked;
	FSimpleDelegate OnViewChanged;

	const FPjcScanResult* Result = nullptr;
	FPjcInspectorGraph Graph;
	TArray<FShownNode> Shown;
	FVector2D GraphSize = FVector2D::ZeroVector;
	// "+N more roots" box, when root paths are hidden
	FVector2D MorePosition = FVector2D::ZeroVector;
	int32 Selected = INDEX_NONE;
	int32 Highlighted = INDEX_NONE;
	int32 Hovered = INDEX_NONE;

	// view: local = Offset + graph * Zoom
	mutable FVector2D Offset = FVector2D::ZeroVector;
	mutable float Zoom = 1.0f;
	mutable FVector2D ViewSize = FVector2D::ZeroVector;
	mutable bool bFitPending = false;

	FVector2D MouseDown = FVector2D::ZeroVector;
	int32 Pressed = INDEX_NONE;
	bool bPanning = false;
};
