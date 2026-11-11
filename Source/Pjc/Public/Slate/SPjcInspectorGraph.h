// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcInspector.h"
#include "Widgets/SLeafWidget.h"

DECLARE_DELEGATE_OneParam(FPjcOnInspectorNode, int32 /* Index into FPjcInspectorGraph::Nodes */);

/**
 * Draws an Inspector graph top to bottom: sources, roots, the paths down to the asset; for an unused asset the group
 * it is deleted with. Links have no arrowheads: a box uses what is below it. The box under the pointer is lit with its
 * links and the boxes on their other end, the rest dims; a lit link flows from the asset to what it uses. A click
 * keeps a box lit (again or Esc: release). Drag to pan, wheel to zoom, F frames the selected node, Home fits the graph.
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
	// Keeps a box lit with its links, as a click on it does; INDEX_NONE releases
	void SetLocked(const int32 Node);
	int32 GetLocked() const;

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
		// Colors of the shared assets it uses, drawn as dots
		TArray<FLinearColor> Dots;
	};

	void UpdateLayout();
	void FitAuto(const FVector2D& ViewSize) const;
	void FitAll(const FVector2D& ViewSize) const;
	void Frame(const int32 Node, const FVector2D& ViewSize) const;
	int32 FindNodeAt(const FVector2D& LocalPosition) const;
	// The box that is lit: the one a click keeps lit, else the one under the pointer
	int32 GetLit() const;
	// Lit links flow: repaints every frame while a box is lit
	void UpdateFlow();
	EActiveTimerReturnType TickFlow(double CurrentTime, float DeltaTime);
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
	// Box a click keeps lit
	int32 Locked = INDEX_NONE;
	TSharedPtr<FActiveTimerHandle> FlowTimer;

	// view: local = Offset + graph * Zoom
	mutable FVector2D Offset = FVector2D::ZeroVector;
	mutable float Zoom = 1.0f;
	mutable FVector2D ViewSize = FVector2D::ZeroVector;
	mutable bool bFitPending = false;

	FVector2D MouseDown = FVector2D::ZeroVector;
	// Where the mouse is, and the marks painted on links (two assets that use each other, a circle of references):
	// a mark under the mouse says what it means
	struct FMark
	{
		FVector2D Position = FVector2D::ZeroVector;
		FString ToolTip;
	};
	FVector2D MouseAt = FVector2D::ZeroVector;
	mutable TArray<FMark> Marks;
	int32 Pressed = INDEX_NONE;
	bool bPanning = false;
};
