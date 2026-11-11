// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcInspectorGraph.h"
#include "Slate/PjcWidgets.h"
#include "PjcFormat.h"
#include "PjcShim.h"
#include "PjcStyles.h"
#include "PjcSubsystem.h"
// Engine Headers
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/PackageName.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
	// unity builds join files: names unique to this file
	constexpr float PjcGraphPad = 24.0f;
	constexpr float PjcGraphGapX = 70.0f;
	// unused group: boxes of a row stand close, the lines between rows need the height
	constexpr float PjcGraphGapXGroup = 22.0f;
	constexpr float PjcGraphGapY = 74.0f;
	constexpr float PjcGraphReadable = 0.65f;
	constexpr float PjcGraphTextZoom = 0.3f;

	FLinearColor PjcGraphColor(const TCHAR* Hex) {
		return FLinearColor {FColor::FromHex(Hex)};
	}

	// Who keeps a root used, the title of its source box
	const TCHAR* PjcGraphSourceTitle(const EPjcUsageReason Reason) {
		switch (Reason) {
			case EPjcUsageReason::Primary :
			case EPjcUsageReason::ConfigFolder :	 return TEXT("Asset Manager");
			case EPjcUsageReason::PrimaryClass :	 return TEXT("Primary class");
			case EPjcUsageReason::EngineReferenced : return TEXT("Outside scanned content");
			case EPjcUsageReason::Indirect :		 return TEXT("Code or config");
			case EPjcUsageReason::Excluded :		 return TEXT("Exclusion");
			case EPjcUsageReason::Cinematic :		 return TEXT("Cinematics");
			case EPjcUsageReason::Editor :			 return TEXT("Editor");
			case EPjcUsageReason::Splash :			 return TEXT("Splash folder");
			case EPjcUsageReason::GameplayCue :		 return TEXT("GameplayCue");
			case EPjcUsageReason::Megascans :		 return TEXT("Megascans");
			case EPjcUsageReason::Localized :		 return TEXT("Localization");
			case EPjcUsageReason::FolderLabelAsset :
			case EPjcUsageReason::FolderLabel :		 return TEXT("Folder label");
			default :								 return TEXT("Root");
		}
	}

	FAssetData PjcGraphAssetOf(const FName Package) {
		TArray<FAssetData> Assets;
		UPjcSubsystem::GetModuleAssetRegistry().Get().GetAssetsByPackageName(Package, Assets);
		return Assets.Num() > 0 ? Assets[0] : FAssetData {};
	}

	// Point on a cubic curve
	FVector2D PjcGraphBezier(const FVector2D& P0, const FVector2D& P1, const FVector2D& P2, const FVector2D& P3, const float T) {
		const float U = 1.0f - T;
		return P0 * (U * U * U) + P1 * (3.0f * U * U * T) + P2 * (3.0f * U * T * T) + P3 * (T * T * T);
	}
}	// namespace

void SPjcInspectorGraph::Construct(const FArguments& InArgs) {
	OnNodeClicked = InArgs._OnNodeClicked;
	OnNodeDoubleClicked = InArgs._OnNodeDoubleClicked;
	OnViewChanged = InArgs._OnViewChanged;
	SetClipping(EWidgetClipping::ClipToBounds);
	SetToolTipText(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SPjcInspectorGraph::GetHoverText)));
}

void SPjcInspectorGraph::SetGraph(const FPjcScanResult* InResult, const FPjcInspectorGraph& InGraph) {
	Result = InResult;
	Graph = InGraph;
	Locked = INDEX_NONE;
	Selected = INDEX_NONE;
	Highlighted = INDEX_NONE;
	Hovered = INDEX_NONE;
	Pressed = INDEX_NONE;
	UpdateLayout();
	bFitPending = true;
	Invalidate(EInvalidateWidgetReason::Paint);
}

const FPjcInspectorGraph& SPjcInspectorGraph::GetGraph() const {
	return Graph;
}

void SPjcInspectorGraph::SetSelected(const int32 Node) {
	Selected = Graph.Nodes.IsValidIndex(Node) ? Node : INDEX_NONE;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SPjcInspectorGraph::SetHighlighted(const int32 Node) {
	Highlighted = Graph.Nodes.IsValidIndex(Node) ? Node : INDEX_NONE;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SPjcInspectorGraph::SetLocked(const int32 Node) {
	Locked = Graph.Nodes.IsValidIndex(Node) && Graph.Nodes[Node].Kind != EPjcInspectorNodeKind::Source ? Node : INDEX_NONE;
	UpdateFlow();
}

int32 SPjcInspectorGraph::GetLocked() const {
	return Locked;
}

void SPjcInspectorGraph::FitAll() {
	if (ViewSize.X <= 0.0f || ViewSize.Y <= 0.0f) {
		bFitPending = true;
		return;
	}
	bFitPending = false;
	FitAll(ViewSize);
	Invalidate(EInvalidateWidgetReason::Paint);
	OnViewChanged.ExecuteIfBound();
}

void SPjcInspectorGraph::FrameSelected() {
	const int32 Node = Selected != INDEX_NONE ? Selected : Graph.FindNode(Graph.Focus);
	if (Node == INDEX_NONE || ViewSize.X <= 0.0f) return;

	bFitPending = false;
	Frame(Node, ViewSize);
	Invalidate(EInvalidateWidgetReason::Paint);
	OnViewChanged.ExecuteIfBound();
}

void SPjcInspectorGraph::ZoomBy(const float Factor) {
	// around the middle of the view
	const FVector2D Center = ViewSize * 0.5f;
	const FVector2D GraphCenter = (Center - Offset) / Zoom;
	Zoom = FMath::Clamp(Zoom * Factor, ZoomMin, ZoomMax);
	Offset = Center - GraphCenter * Zoom;
	bFitPending = false;
	Invalidate(EInvalidateWidgetReason::Paint);
	OnViewChanged.ExecuteIfBound();
}

float SPjcInspectorGraph::GetZoom() const {
	return Zoom;
}

FVector2D SPjcInspectorGraph::ComputeDesiredSize(float LayoutScaleMultiplier) const {
	return FVector2D {300.0f, 200.0f};
}

void SPjcInspectorGraph::UpdateLayout() {
	Shown.Reset();
	Shown.SetNum(Graph.Nodes.Num());
	GraphSize = FVector2D::ZeroVector;
	if (Graph.Nodes.Num() == 0) return;

	if (Graph.bUnusedGroup) {
		// the model placed every box: rows from 0, centered
		float MaxColumn = 0.0f;
		int32 LastRow = 0;
		bool bShared = false;
		for (int32 Index = 0; Index < Graph.Nodes.Num(); ++Index) {
			const FPjcInspectorNode& Node = Graph.Nodes[Index];
			Shown[Index].Position
				= FVector2D {PjcGraphPad + Node.Column * (NodeWidth + PjcGraphGapXGroup), PjcGraphPad + Node.Layer * (NodeHeight + PjcGraphGapY)};
			MaxColumn = FMath::Max(MaxColumn, Node.Column);
			LastRow = FMath::Max(LastRow, Node.Layer);
			bShared |= Node.bHub;
		}
		GraphSize = FVector2D {
		  PjcGraphPad * 2.0f + MaxColumn * (NodeWidth + PjcGraphGapXGroup) + NodeWidth + 50.0f,
		  PjcGraphPad * 2.0f + LastRow * (NodeHeight + PjcGraphGapY) + NodeHeight
		};
	}

	// rows from 0, nodes of a row in column order without overlaps
	int32 MinLayer = TNumericLimits<int32>::Max();
	for (const FPjcInspectorNode& Node : Graph.Nodes) {
		MinLayer = FMath::Min(MinLayer, Node.Layer);
	}
	TMap<int32, TArray<int32>> Rows;
	for (int32 Index = 0; Index < Graph.Nodes.Num(); ++Index) {
		Rows.FindOrAdd(Graph.Nodes[Index].Layer - MinLayer).Add(Index);
	}
	int32 MaxSlot = 0;
	int32 MaxRow = 0;
	for (auto& Pair : Rows) {
		if (Graph.bUnusedGroup) break;

		Pair.Value.Sort([this](const int32 A, const int32 B) {
			if (Graph.Nodes[A].Column != Graph.Nodes[B].Column) return Graph.Nodes[A].Column < Graph.Nodes[B].Column;
			return A < B;
		});
		int32 Slot = -1;
		for (const int32 Index : Pair.Value) {
			Slot = FMath::Max(FMath::RoundToInt(Graph.Nodes[Index].Column), Slot + 1);
			Shown[Index].Position = FVector2D {PjcGraphPad + Slot * (NodeWidth + PjcGraphGapX), PjcGraphPad + Pair.Key * (NodeHeight + PjcGraphGapY)};
			MaxSlot = FMath::Max(MaxSlot, Slot);
		}
		MaxRow = FMath::Max(MaxRow, Pair.Key);
	}
	if (Graph.NumHidden > 0) {
		++MaxSlot;
		MorePosition = FVector2D {PjcGraphPad + MaxSlot * (NodeWidth + PjcGraphGapX), PjcGraphPad};
	}
	if (!Graph.bUnusedGroup) {
		GraphSize = FVector2D {
		  PjcGraphPad * 2.0f + MaxSlot * (NodeWidth + PjcGraphGapX) + NodeWidth + 120.0f,
		  PjcGraphPad * 2.0f + MaxRow * (NodeHeight + PjcGraphGapY) + NodeHeight
		};
	}

	// text of the nodes
	const FLinearColor Muted = PjcGraphColor(TEXT("#8A9099"));
	const FLinearColor Gold = PjcGraphColor(TEXT("#FFCB77"));
	const FLinearColor Editor = PjcGraphColor(TEXT("#7FB2FF"));
	for (int32 Index = 0; Index < Graph.Nodes.Num(); ++Index) {
		const FPjcInspectorNode& Node = Graph.Nodes[Index];
		FShownNode& Item = Shown[Index];

		if (Node.Kind == EPjcInspectorNodeKind::Source) {
			// the box it points to: a root or a group of roots
			const FPjcInspectorEdge* Edge = Graph.Edges.FindByPredicate([Index](const FPjcInspectorEdge& Existing) {
				return Existing.From == Index;
			});
			const FPjcInspectorNode* Target = Edge ? &Graph.Nodes[Edge->To] : nullptr;
			const bool bGroup = Target && Target->Kind == EPjcInspectorNodeKind::Group;
			const FName Root = bGroup && Target->Roots.Num() > 0 ? Target->Roots[0] : (Target ? Target->Package : Node.Package);
			const FPjcAssetUsage* Usage = Result ? Result->Usage.Find(Root) : nullptr;
			Item.Title = PjcGraphSourceTitle(Usage ? Usage->Reason : EPjcUsageReason::None);
			Item.Line = bGroup ? FString::Printf(TEXT("%d \u00D7 %s"), Target->Roots.Num(), *Node.Label) : Node.Label;
			Item.LineColor = PjcGraphColor(TEXT("#D9C9A3"));
			Item.ToolTip = Item.Title + TEXT("\n") + Item.Line;
			continue;
		}

		if (Node.Kind == EPjcInspectorNodeKind::Group) {
			const FAssetData Asset = Node.Roots.Num() > 0 ? PjcGraphAssetOf(Node.Roots[0]) : FAssetData {};
			const PjcWidgets::FAssetType& Type = PjcWidgets::GetAssetType(Asset);
			int64 Size = 0;
			for (const FName Root : Node.Roots) {
				Size += Result ? Result->GetSize(Root) : 0;
			}
			Item.Title = FString::Printf(TEXT("%d \u00D7 %s"), Node.Roots.Num(), *Type.Label);
			Item.Line = FString::Printf(TEXT("roots \u00B7 %s \u00B7 click to list"), *PjcFormat::Size(Size));
			Item.LineColor = Gold;
			Item.Icon = Type.Icon;
			Item.IconColor = Type.Color;
			Item.Stripe = PjcGraphColor(TEXT("#17C3B2"));
			Item.ToolTip = FString::Printf(TEXT("%d %s roots, drawn as one box\nClick to list them"), Node.Roots.Num(), *Type.Label);
			continue;
		}

		if (Node.Kind == EPjcInspectorNodeKind::Stack) {
			// unused assets with the same links, as one box
			const FAssetData Asset = Node.Roots.Num() > 0 ? PjcGraphAssetOf(Node.Roots[0]) : FAssetData {};
			const PjcWidgets::FAssetType& Type = PjcWidgets::GetAssetType(Asset);
			int64 Size = 0;
			for (const FName Member : Node.Roots) {
				Size += Result ? Result->GetSize(Member) : 0;
			}
			Item.Title = FString::Printf(TEXT("%d \u00D7 %s"), Node.Roots.Num(), *Type.Label);
			Item.Line = FString::Printf(TEXT("%s \u00B7 same links \u00B7 click to list"), *PjcFormat::Size(Size));
			Item.LineColor = Muted;
			Item.Icon = Type.Icon;
			Item.IconColor = Type.Color;
			Item.bUsed = false;
			Item.Stripe = PjcGraphColor(TEXT("#FE6D73"));
			Item.ToolTip = FString::Printf(TEXT("%d %s assets with the same links, drawn as one box"), Node.Roots.Num(), *Type.Label);
			for (int32 Member = 0; Member < Node.Roots.Num() && Member < 12; ++Member) {
				Item.ToolTip += TEXT("\n") + FPackageName::GetShortName(Node.Roots[Member]);
			}
			if (Node.Roots.Num() > 12) {
				Item.ToolTip += FString::Printf(TEXT("\n+%d more"), Node.Roots.Num() - 12);
			}
			continue;
		}

		const FAssetData Asset = PjcGraphAssetOf(Node.Package);
		const PjcWidgets::FAssetType& Type = PjcWidgets::GetAssetType(Asset);
		const FPjcAssetUsage* Usage = Result ? Result->Usage.Find(Node.Package) : nullptr;
		const EPjcAssetStatus Status = Result ? Result->GetStatus(Node.Package) : EPjcAssetStatus::Used;
		const bool bScanned = Usage != nullptr;
		const bool bRoot = Usage && Usage->bUsed && Usage->Parent.IsNone();
		const bool bEditorOnly = bScanned && Status == EPjcAssetStatus::EditorOnly;

		Item.Title = FPackageName::GetShortName(Node.Package);
		Item.Line.Reset();
		if (!bScanned) {
			// outside the scanned content: say whose it is
			const FString Path = Node.Package.ToString();
			const bool bEngine = Path.StartsWith(TEXT("/Engine/"));
			const bool bGame = Path.StartsWith(TEXT("/Game/"));
			Item.Line += bEngine ? TEXT("engine content \u00B7 ") : (bGame ? TEXT("not scanned \u00B7 ") : TEXT("plugin content \u00B7 "));
		}
		if (bRoot) Item.Line += TEXT("root \u00B7 ");
		if (bEditorOnly && !bRoot) Item.Line += TEXT("editor only \u00B7 ");
		Item.Line += Type.Label;
		if (bScanned) Item.Line += TEXT(" \u00B7 ") + PjcFormat::Size(Result->GetSize(Node.Package));
		Item.LineColor = bRoot ? Gold : (bEditorOnly ? Editor : Muted);
		Item.Icon = Type.Icon;
		Item.IconColor = Type.Color;
		Item.bUsed = !bScanned || Usage->bUsed;
		Item.Stripe = Item.bUsed ? (bEditorOnly ? Editor : PjcGraphColor(TEXT("#17C3B2"))) : PjcGraphColor(TEXT("#FE6D73"));
		Item.ToolTip = Node.Package.ToString();
		if (Usage) {
			Item.ToolTip += TEXT("\n") + UPjcSubsystem::GetUsageReasonText(*Usage);
		}
		if (Node.bHub) {
			Item.Line += FString::Printf(TEXT(" \u00B7 used by %d of %d"), Node.NumUsers, Graph.NumGroup);
		}
		if (Node.Package != Graph.Focus) {
			Item.ToolTip += TEXT("\nDouble-click to inspect it");
		}
	}
	// a dot for every shared asset a box uses, in that asset's color
	for (int32 Index = 0; Index < Graph.Nodes.Num(); ++Index) {
		for (const int32 Hub : Graph.Nodes[Index].Hubs) {
			if (Shown.IsValidIndex(Hub)) Shown[Index].Dots.Add(Shown[Hub].IconColor);
		}
	}
}

int32 SPjcInspectorGraph::GetLit() const {
	if (Shown.IsValidIndex(Locked)) return Locked;

	return Shown.IsValidIndex(Hovered) && Graph.Nodes[Hovered].Kind != EPjcInspectorNodeKind::Source ? Hovered : INDEX_NONE;
}

void SPjcInspectorGraph::UpdateFlow() {
	Invalidate(EInvalidateWidgetReason::Paint);
	if (GetLit() != INDEX_NONE && !FlowTimer.IsValid()) {
		FlowTimer = RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateSP(this, &SPjcInspectorGraph::TickFlow));
	}
}

EActiveTimerReturnType SPjcInspectorGraph::TickFlow(double CurrentTime, float DeltaTime) {
	Invalidate(EInvalidateWidgetReason::Paint);
	if (GetLit() != INDEX_NONE) return EActiveTimerReturnType::Continue;

	FlowTimer.Reset();
	return EActiveTimerReturnType::Stop;
}

FVector2D SPjcInspectorGraph::ToLocal(const FVector2D& GraphPosition) const {
	return Offset + GraphPosition * Zoom;
}

void SPjcInspectorGraph::FitAll(const FVector2D& InViewSize) const {
	if (GraphSize.X <= 0.0f || GraphSize.Y <= 0.0f) return;

	const float ViewWidth = static_cast<float>(InViewSize.X);
	const float ViewHeight = static_cast<float>(InViewSize.Y);
	const float Width = static_cast<float>(GraphSize.X);
	const float Height = static_cast<float>(GraphSize.Y);
	Zoom = FMath::Clamp(FMath::Min3(1.0f, (ViewWidth - 20.0f) / Width, (ViewHeight - 20.0f) / Height), ZoomMin, ZoomMax);
	Offset = FVector2D {(ViewWidth - Width * Zoom) * 0.5f, FMath::Max(10.0f, (ViewHeight - Height * Zoom) * 0.5f)};
}

void SPjcInspectorGraph::FitAuto(const FVector2D& InViewSize) const {
	// the whole graph while it stays readable, else a readable zoom with the asset in view at the bottom
	const int32 Focus = Graph.FindNode(Graph.Focus);
	const float ViewWidth = static_cast<float>(InViewSize.X);
	const float ViewHeight = static_cast<float>(InViewSize.Y);
	const float FitX = (ViewWidth - 20.0f) / static_cast<float>(GraphSize.X);
	const float FitY = (ViewHeight - 20.0f) / static_cast<float>(GraphSize.Y);
	const float Fit = FMath::Min3(1.0f, FitX, FitY);
	// the unused group is one picture: always whole, the wheel zooms in
	if (Fit >= PjcGraphReadable || Focus == INDEX_NONE || Graph.bUnusedGroup) {
		FitAll(InViewSize);
		return;
	}
	const float Left = static_cast<float>(Shown[Focus].Position.X);
	const float Top = static_cast<float>(Shown[Focus].Position.Y);
	Zoom = PjcGraphReadable;
	Offset = FVector2D {FMath::Min(10.0f, ViewWidth * 0.5f - (Left + NodeWidth * 0.5f) * Zoom), ViewHeight - 40.0f - (Top + NodeHeight) * Zoom};
}

void SPjcInspectorGraph::Frame(const int32 Node, const FVector2D& InViewSize) const {
	if (!Shown.IsValidIndex(Node)) return;

	const FVector2D Position = Shown[Node].Position;
	Zoom = FMath::Max(Zoom, 1.0f);
	Offset = InViewSize * 0.5f - (Position + FVector2D {NodeWidth, NodeHeight} * 0.5f) * Zoom;
}

int32 SPjcInspectorGraph::FindNodeAt(const FVector2D& LocalPosition) const {
	const FVector2D Point = (LocalPosition - Offset) / Zoom;
	for (int32 Index = 0; Index < Shown.Num(); ++Index) {
		const FVector2D& Position = Shown[Index].Position;
		if (Point.X >= Position.X && Point.Y >= Position.Y && Point.X < Position.X + NodeWidth && Point.Y < Position.Y + NodeHeight) {
			return Index;
		}
	}
	return INDEX_NONE;
}

int32 SPjcInspectorGraph::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled
) const {
	ViewSize = FVector2D {AllottedGeometry.GetLocalSize()};
	if (bFitPending && ViewSize.X > 0.0f && ViewSize.Y > 0.0f && Shown.Num() > 0) {
		bFitPending = false;
		FitAuto(ViewSize);
	}

	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
#if ENGINE_MAJOR_VERSION >= 5
	const FSlateBrush* BrushNode = FPjcStyles::Get().GetBrush("ProjectCleaner.Graph.Node");
	const FSlateBrush* BrushPill = FPjcStyles::Get().GetBrush("ProjectCleaner.Graph.Pill");
#else
	const FSlateBrush* BrushNode = White;
	const FSlateBrush* BrushPill = White;
#endif
	const FSlateBrush* BrushDisc = FPjcStyles::Get().GetBrush("ProjectCleaner.Graph.Disc");
	const FSlateBrush* BrushLoop = FPjcStyles::Get().GetBrush("ProjectCleaner.Graph.Loop");
	const FSlateBrush* BrushPair = FPjcStyles::Get().GetBrush("ProjectCleaner.Graph.Pair");
	const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FSlateFontInfo FontTitle = FPjcStyles::GetFont("Bold", 9);
	const FSlateFontInfo FontLine = FPjcStyles::GetFont("Regular", 8);
	const FSlateFontInfo FontLabel = PjcWidgets::GetFontMono(7);

	const FLinearColor Background = PjcGraphColor(TEXT("#18191C"));
	const FLinearColor Line = PjcGraphColor(TEXT("#34383E"));
	const FLinearColor Fill = PjcGraphColor(TEXT("#26292D"));
	const FLinearColor FillHovered = PjcGraphColor(TEXT("#2D3136"));
	const FLinearColor SelectedColor = PjcGraphColor(TEXT("#4D9BFF"));
	const FLinearColor Foreground = PjcGraphColor(TEXT("#D6D9DD"));
	const FLinearColor SourceFill = PjcGraphColor(TEXT("#2E2716"));
	const FLinearColor SourceLine = PjcGraphColor(TEXT("#6B5626"));
	const FLinearColor Gold = PjcGraphColor(TEXT("#FFCB77"));

	const int32 LayerEdges = LayerId + 1;
	const int32 LayerLabels = LayerId + 2;
	const int32 LayerRing = LayerId + 3;
	const int32 LayerBody = LayerId + 5;
	const int32 LayerContent = LayerId + 8;

	// graph space box, scaled with the view
	const auto Box = [&](const int32 Layer, const FVector2D& Position, const FVector2D& Size, const FSlateBrush* Brush, const FLinearColor& Color) {
		const FPaintGeometry BoxGeometry = AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform {Zoom, ToLocal(Position)});
		FSlateDrawElement::MakeBox(OutDrawElements, Layer, BoxGeometry, Brush, ESlateDrawEffect::None, Color);
	};
	const auto Text = [&](const int32 Layer,
						  const FVector2D& Position,
						  const float MaxWidth,
						  const FString& String,
						  const FSlateFontInfo& Font,
						  const FLinearColor& Color) {
		if (Zoom < PjcGraphTextZoom || String.IsEmpty() || MaxWidth <= 4.0f) return FVector2D::ZeroVector;

		FString Cut = String;
		if (FontMeasure->Measure(Cut, Font).X > MaxWidth) {
			const int32 Fits = FontMeasure->FindLastWholeCharacterIndexBeforeOffset(Cut, Font, FMath::Max(1, FMath::FloorToInt(MaxWidth - 8.0f)));
			Cut = Fits > 0 ? Cut.Left(Fits) + TEXT("\u2026") : FString {};
		}
		const FVector2D Size {FontMeasure->Measure(Cut, Font)};
		const FPaintGeometry TextGeometry = AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform {Zoom, ToLocal(Position)});
		FSlateDrawElement::MakeText(OutDrawElements, Layer, TextGeometry, Cut, Font, ESlateDrawEffect::None, Color);
		return Size;
	};
	const auto Ring = [&](const FVector2D& Position, const FSlateBrush* Brush, const FLinearColor& Color) {
		Box(LayerRing, Position - FVector2D {4.0f, 4.0f}, FVector2D {NodeWidth + 8.0f, NodeHeight + 8.0f}, Brush, Color);
		Box(LayerRing + 1, Position - FVector2D {2.0f, 2.0f}, FVector2D {NodeWidth + 4.0f, NodeHeight + 4.0f}, Brush, Background);
	};

	FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), White, ESlateDrawEffect::None, Background);

	// what is lit: the box a click keeps lit or the one under the pointer, its links and the boxes on their other end
	const int32 Lit = GetLit();
	const bool bDim = Shown.IsValidIndex(Lit);
	TArray<bool> LitNodes;
	LitNodes.Init(false, Shown.Num());
	if (bDim) {
		LitNodes[Lit] = true;
		for (const FPjcInspectorEdge& Edge : Graph.Edges) {
			if (Edge.From == Lit && Shown.IsValidIndex(Edge.To)) LitNodes[Edge.To] = true;
			if (Edge.To == Lit && Shown.IsValidIndex(Edge.From)) LitNodes[Edge.From] = true;
		}
		for (const int32 Hub : Graph.Nodes[Lit].Hubs) {
			if (Shown.IsValidIndex(Hub)) LitNodes[Hub] = true;
		}
		for (int32 Index = 0; Index < Shown.Num(); ++Index) {
			if (Graph.Nodes[Index].Hubs.Contains(Lit)) LitNodes[Index] = true;
		}
	}
	const auto Faded = [&Background, bDim](const FLinearColor& Color, const bool bLit) {
		return bDim && !bLit ? FMath::Lerp(Background, Color, 0.28f) : Color;
	};
	const FLinearColor Amber = PjcGraphColor(TEXT("#D2A640"));
	// circular links have a colour of their own: the root sources are gold already
	const FLinearColor Circular = PjcGraphColor(TEXT("#FF8A3D"));
	// lit links flow: their dashes move this far along, in graph units
	constexpr float FlowOn = 10.0f;
	constexpr float FlowOff = 7.0f;
	// in doubles: the time is a big number, and UE 4 has Fmod for floats only, in which the dashes stand still
	const double FlowPeriod = static_cast<double>(FlowOn + FlowOff);
	const double FlowTravel = FSlateApplication::Get().GetCurrentTime() * 26.0;
	const float Flow = static_cast<float>(FlowTravel - FMath::FloorToDouble(FlowTravel / FlowPeriod) * FlowPeriod);

	// references: no arrowheads, a box uses what is below it. Two boxes that use each other share one line with a mark;
	// a reference that leads back up goes over the row or round the side
	TArray<FVector2D> Points;
	TArray<FVector2D> Dash;
	TArray<float> Lengths;
	Marks.Reset();
	for (const FPjcInspectorEdge& Edge : Graph.Edges) {
		if (!Shown.IsValidIndex(Edge.From) || !Shown.IsValidIndex(Edge.To)) continue;

		const FPjcInspectorNode& NodeFrom = Graph.Nodes[Edge.From];
		const FPjcInspectorNode& NodeTo = Graph.Nodes[Edge.To];
		const bool bSource = NodeFrom.Kind == EPjcInspectorNodeKind::Source;
		const FPjcInspectorEdge* Back = bSource ? nullptr : Graph.Edges.FindByPredicate([&Edge](const FPjcInspectorEdge& Other) {
			return Other.From == Edge.To && Other.To == Edge.From;
		});
		const bool bUp = !bSource && NodeTo.Layer <= NodeFrom.Layer;
		const bool bBackUp = NodeFrom.Layer <= NodeTo.Layer;
		if (Back && bUp && (!bBackUp || Edge.From > Edge.To)) continue;

		const bool bLit = bDim && (Edge.From == Lit || Edge.To == Lit);
		FLinearColor Color = PjcGraphColor(TEXT("#2C9A8F"));
		float Width = 2.2f;
		float DashOn = 0.0f;
		float DashOff = 0.0f;
		if (bSource) {
			Color = PjcGraphColor(TEXT("#8A7440"));
			Width = 2.5f;
		}
		else if (Back || (bUp && Edge.bCircle)) {
			Color = Circular;
		}
		else if (Edge.Kind == EPjcLinkKind::EditorOnly) {
			Color = PjcGraphColor(TEXT("#7FB2FF"));
		}
		else if (Graph.bUnusedGroup) {
			Color = PjcGraphColor(TEXT("#A8565A"));
			Width = 1.8f;
		}
		if (!bSource && Edge.Kind == EPjcLinkKind::Soft) {
			DashOn = 6.0f;
			DashOff = 4.0f;
		}
		else if (!bSource && Edge.Kind == EPjcLinkKind::EditorOnly) {
			DashOn = 2.0f;
			DashOff = 4.0f;
		}
		if (bLit) {
			Width = 3.2f;
			DashOn = FlowOn;
			DashOff = FlowOff;
		}
		// links that are not lit go further back than the boxes
		Color = bDim && !bLit ? FMath::Lerp(Background, Color, 0.18f) : Color;

		const FVector2D& From = Shown[Edge.From].Position;
		const FVector2D& To = Shown[Edge.To].Position;
		FVector2D Start {From.X + NodeWidth * 0.5f, From.Y + NodeHeight};
		FVector2D End {To.X + NodeWidth * 0.5f, To.Y - 2.0f};
		FVector2D C1 {Start.X, End.Y - 30.0f};
		FVector2D C2 {End.X, Start.Y + 30.0f};
		if (bUp && NodeFrom.Layer == NodeTo.Layer) {
			// neighbours in a row: over their tops
			Start = FVector2D {From.X + NodeWidth * 0.5f, From.Y};
			C1 = FVector2D {Start.X, Start.Y - 34.0f};
			C2 = FVector2D {End.X, End.Y - 34.0f};
		}
		else if (bUp) {
			// back up: round the right side
			Start = FVector2D {From.X + NodeWidth, From.Y + NodeHeight * 0.5f};
			End = FVector2D {To.X + NodeWidth + 3.0f, To.Y + NodeHeight * 0.5f};
			const float Out = static_cast<float>(FMath::Max(Start.X, End.X)) + 44.0f;
			C1 = FVector2D {Out, Start.Y};
			C2 = FVector2D {Out, End.Y};
		}

		Points.Reset();
		constexpr int32 Steps = 32;
		for (int32 Step = 0; Step <= Steps; ++Step) {
			Points.Add(ToLocal(PjcGraphBezier(Start, C1, C2, End, static_cast<float>(Step) / Steps)));
		}
		const float Thickness = FMath::Max(1.0f, Width * Zoom);
		const FPaintGeometry EdgeGeometry = AllottedGeometry.ToPaintGeometry();
		if (DashOn <= 0.0f) {
			FSlateDrawElement::MakeLines(OutDrawElements, LayerEdges, EdgeGeometry, Points, ESlateDrawEffect::None, Color, true, Thickness);
		}
		else {
			// dashes along the curve, in graph units; on a lit link they move from the asset to what it uses. A dash is
			// one line over the bends of the curve: cut at every bend, its cuts stand still while the dash moves
			const float On = DashOn * Zoom;
			const float Period = (DashOn + DashOff) * Zoom;
			Lengths.Reset();
			Lengths.Add(0.0f);
			for (int32 Index = 1; Index < Points.Num(); ++Index) {
				Lengths.Add(Lengths.Last() + static_cast<float>(FVector2D::Distance(Points[Index - 1], Points[Index])));
			}
			const float Total = Lengths.Last();
			const auto PointAt = [&Points, &Lengths](const int32 Piece, const float At) {
				const float Length = Lengths[Piece + 1] - Lengths[Piece];
				return Points[Piece] + (Points[Piece + 1] - Points[Piece]) * (Length > 0.0f ? (At - Lengths[Piece]) / Length : 0.0f);
			};
			int32 Piece = 0;
			for (float Begin = bLit ? Flow * Zoom - Period : 0.0f; Begin < Total; Begin += Period) {
				const float DashFrom = FMath::Max(Begin, 0.0f);
				const float DashTo = FMath::Min(Begin + On, Total);
				if (DashTo - DashFrom <= 0.01f) continue;

				while (Piece < Points.Num() - 2 && Lengths[Piece + 1] <= DashFrom) {
					++Piece;
				}
				Dash.Reset();
				Dash.Add(PointAt(Piece, DashFrom));
				int32 Last = Piece;
				while (Last < Points.Num() - 2 && Lengths[Last + 1] < DashTo) {
					++Last;
					Dash.Add(Points[Last]);
				}
				Dash.Add(PointAt(Last, DashTo));
				FSlateDrawElement::MakeLines(OutDrawElements, LayerEdges, EdgeGeometry, Dash, ESlateDrawEffect::None, Color, true, Thickness);
			}
		}

		// mark on a line two boxes share (they use each other) or that closes a circle of references
		if (!bSource && (Back || Edge.bCircle) && Zoom >= PjcGraphTextZoom) {
			const FVector2D Middle = PjcGraphBezier(Start, C1, C2, End, 0.5f);
			const FString NameFrom = Shown[Edge.From].Title;
			const FString NameTo = Shown[Edge.To].Title;
			Marks.Add(
				FMark {
				  ToLocal(Middle),
				  Back ? FString::Printf(TEXT("%s and %s use each other"), *NameFrom, *NameTo)
					   : FString::Printf(TEXT("%s uses %s, which leads back to it: a circle of references"), *NameFrom, *NameTo)
				}
			);
			// a round badge in the colour of the link: two arrows for both ways, a loop for a circle. Images, not
			// glyphs: they look the same with every font and engine
			const FLinearColor MarkColor = Faded(Circular, bLit);
			Box(LayerLabels, Middle - FVector2D {10.0f, 10.0f}, FVector2D {20.0f, 20.0f}, BrushDisc, MarkColor);
			Box(LayerLabels + 1, Middle - FVector2D {8.5f, 8.5f}, FVector2D {17.0f, 17.0f}, BrushDisc, PjcGraphColor(TEXT("#17191C")));
			Box(LayerRing, Middle - FVector2D {6.5f, 6.5f}, FVector2D {13.0f, 13.0f}, Back ? BrushPair : BrushLoop, MarkColor);
		}

		// what the link is, when not a plain hard reference (the unused group says it with the line alone)
		FString Label;
		if (NodeFrom.Kind == EPjcInspectorNodeKind::Group) {
			Label = FString::Printf(TEXT("%d references"), Edge.NumLinks);
		}
		else if (!bSource && Edge.Kind == EPjcLinkKind::Soft) {
			Label = TEXT("soft");
		}
		else if (!bSource && Edge.Kind == EPjcLinkKind::EditorOnly) {
			Label = TEXT("editor only");
		}
		if (!Label.IsEmpty() && !Graph.bUnusedGroup && Zoom >= PjcGraphTextZoom) {
			const bool bSameColumn = FMath::Abs(Start.X - End.X) < 1.0;
			const FVector2D Position = bSameColumn ? FVector2D {End.X + 10.0f, End.Y - 30.0f} : FVector2D {Start.X + 10.0f, Start.Y + 8.0f};
			const FVector2D Size {FontMeasure->Measure(Label, FontLabel)};
			Box(LayerLabels, Position, Size + FVector2D {10.0f, 4.0f}, BrushNode, Background);
			const FVector2D LabelPosition = Position + FVector2D {5.0f, 2.0f};
			Text(LayerLabels + 1, LabelPosition, static_cast<float>(Size.X) + 1.0f, Label, FontLabel, Faded(Foreground, bLit));
		}
	}

	// boxes
	const int32 Focus = Graph.FindNode(Graph.Focus);
	for (int32 Index = 0; Index < Shown.Num(); ++Index) {
		const FPjcInspectorNode& Node = Graph.Nodes[Index];
		const FShownNode& Item = Shown[Index];
		const FVector2D& Position = Item.Position;
		const FVector2D Size {NodeWidth, NodeHeight};
		const bool bLit = LitNodes[Index];

		if (Node.Kind == EPjcInspectorNodeKind::Source) {
			Box(LayerBody, Position, Size, BrushPill, Faded(SourceLine, bLit));
			Box(LayerBody + 1, Position + FVector2D {1.0f, 1.0f}, Size - FVector2D {2.0f, 2.0f}, BrushPill, Faded(SourceFill, bLit));
			Text(LayerContent, Position + FVector2D {16.0f, 7.0f}, NodeWidth - 30.0f, Item.Title, FontTitle, Faded(Gold, bLit));
			Text(LayerContent, Position + FVector2D {16.0f, 24.0f}, NodeWidth - 30.0f, Item.Line, FontLine, Faded(Item.LineColor, bLit));
			continue;
		}

		const bool bFocus = Index == Focus;
		if (Index == Locked) {
			// kept lit by a click
			Ring(Position, BrushNode, Amber);
		}
		else if (Index == Selected) {
			Ring(Position, BrushNode, SelectedColor);
		}
		else if (Index == Highlighted) {
			Ring(Position, BrushNode, SelectedColor * FLinearColor {1.0f, 1.0f, 1.0f, 0.55f});
		}
		else if (Node.Kind == EPjcInspectorNodeKind::Stack) {
			// a second box behind it: a stack
			const FVector2D Behind = Position + FVector2D {5.0f, -5.0f};
			Box(LayerRing, Behind, Size, BrushNode, Faded(Line, bLit));
			Box(
				LayerRing + 1, Behind + FVector2D {1.0f, 1.0f}, Size - FVector2D {2.0f, 2.0f}, BrushNode, Faded(PjcGraphColor(TEXT("#202327")), bLit)
			);
		}

		FLinearColor Body = Index == Hovered ? FillHovered : Fill;
		if (bFocus) {
			Body = Item.bUsed ? PjcGraphColor(TEXT("#20302F")) : PjcGraphColor(TEXT("#33232A"));
		}
		Box(LayerBody, Position, Size, BrushNode, Faded(bFocus ? Item.Stripe : Line, bLit));
		Box(LayerBody + 1, Position, FVector2D {6.0f, NodeHeight}, BrushNode, Faded(Item.Stripe, bLit));
		Box(LayerBody + 2, Position + FVector2D {3.0f, 1.0f}, Size - FVector2D {4.0f, 2.0f}, BrushNode, Faded(Body, bLit));

		float TextLeft = 12.0f;
		if (Item.Icon && Zoom >= PjcGraphTextZoom) {
			const FLinearColor IconTint = Item.IconColor * FLinearColor {1.0f, 1.0f, 1.0f, bDim && !bLit ? 0.3f : 1.0f};
			Box(LayerContent, Position + FVector2D {11.0f, 13.0f}, FVector2D {20.0f, 20.0f}, Item.Icon, IconTint);
			TextLeft = 39.0f;
		}
		const float DotsWidth = Item.Dots.Num() * 10.0f;
		Text(
			LayerContent,
			Position + FVector2D {TextLeft, 6.0f},
			NodeWidth - TextLeft - 8.0f - DotsWidth,
			Item.Title,
			FontTitle,
			Faded(Foreground, bLit)
		);
		Text(LayerContent, Position + FVector2D {TextLeft, 24.0f}, NodeWidth - TextLeft - 8.0f, Item.Line, FontLine, Faded(Item.LineColor, bLit));
		for (int32 Dot = 0; Dot < Item.Dots.Num(); ++Dot) {
			Box(LayerContent,
				Position + FVector2D {NodeWidth - 13.0f - Dot * 10.0f, 7.0f},
				FVector2D {7.0f, 7.0f},
				BrushPill,
				Faded(Item.Dots[Dot], bLit));
		}
	}

	// text above some rows of the unused group
	for (const FPjcInspectorCaption& Caption : Graph.Captions) {
		const FVector2D At {PjcGraphPad, PjcGraphPad + Caption.Layer * (NodeHeight + PjcGraphGapY) - 22.0f};
		Text(LayerContent, At, 600.0f, Caption.Text, FontLine, Faded(PjcGraphColor(TEXT("#8A9099")), false));
	}

	// root paths not drawn
	if (Graph.NumHidden > 0) {
		const FVector2D Size {NodeWidth, NodeHeight};
		Box(LayerBody, MorePosition, Size, BrushPill, SourceLine);
		Box(LayerBody + 1, MorePosition + FVector2D {1.0f, 1.0f}, Size - FVector2D {2.0f, 2.0f}, BrushPill, Background);
		const FString More = FString::Printf(TEXT("+%s more roots"), *PjcFormat::Count(Graph.NumHidden));
		const float Width = static_cast<float>(FontMeasure->Measure(More, FontLine).X);
		const FVector2D MoreAt = MorePosition + FVector2D {(NodeWidth - Width) * 0.5f, 15.0f};
		Text(LayerContent, MoreAt, Width + 1.0f, More, FontLine, PjcGraphColor(TEXT("#D9C9A3")));
	}

	return LayerContent + 1;
}

bool SPjcInspectorGraph::SupportsKeyboardFocus() const {
	return true;
}

FReply SPjcInspectorGraph::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) {
	const FKey Button = MouseEvent.GetEffectingButton();
	if (Button != EKeys::LeftMouseButton && Button != EKeys::RightMouseButton && Button != EKeys::MiddleMouseButton) return FReply::Unhandled();

	MouseDown = FVector2D {MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition())};
	Pressed = Button == EKeys::LeftMouseButton ? FindNodeAt(MouseDown) : INDEX_NONE;
	bPanning = false;
	return FReply::Handled().CaptureMouse(SharedThis(this)).SetUserFocus(SharedThis(this), EFocusCause::Mouse);
}

FReply SPjcInspectorGraph::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) {
	if (!HasMouseCapture()) return FReply::Unhandled();

	const FVector2D Local {MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition())};
	if (!bPanning && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) {
		const int32 Node = FindNodeAt(Local);
		if (Node != INDEX_NONE && Node == Pressed) {
			// a click keeps the box lit with its links, a second one releases it
			if (Graph.Nodes[Node].Kind != EPjcInspectorNodeKind::Source) {
				Locked = Locked == Node ? INDEX_NONE : Node;
				UpdateFlow();
			}
			OnNodeClicked.ExecuteIfBound(Node);
		}
	}
	bPanning = false;
	Pressed = INDEX_NONE;
	return FReply::Handled().ReleaseMouseCapture();
}

FReply SPjcInspectorGraph::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) {
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();

	const int32 Node = FindNodeAt(FVector2D {MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition())});
	if (Node != INDEX_NONE) {
		OnNodeDoubleClicked.ExecuteIfBound(Node);
	}
	return FReply::Handled();
}

FReply SPjcInspectorGraph::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) {
	const FVector2D Local {MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition())};
	if (HasMouseCapture()) {
		if (!bPanning && FVector2D::Distance(Local, MouseDown) > 4.0f) {
			bPanning = true;
			Hovered = INDEX_NONE;
		}
		if (bPanning) {
			Offset += FVector2D {MouseEvent.GetCursorDelta()} / MyGeometry.Scale;
			bFitPending = false;
			Invalidate(EInvalidateWidgetReason::Paint);
			OnViewChanged.ExecuteIfBound();
		}
		return FReply::Handled();
	}

	MouseAt = Local;
	const int32 Node = FindNodeAt(Local);
	if (Node != Hovered) {
		Hovered = Node;
		UpdateFlow();
	}
	return FReply::Unhandled();
}

FReply SPjcInspectorGraph::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) {
	if (Shown.Num() == 0) return FReply::Unhandled();

	const FVector2D Local {MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition())};
	const FVector2D GraphPoint = (Local - Offset) / Zoom;
	Zoom = FMath::Clamp(Zoom * FMath::Pow(1.15f, MouseEvent.GetWheelDelta()), ZoomMin, ZoomMax);
	Offset = Local - GraphPoint * Zoom;
	bFitPending = false;
	Invalidate(EInvalidateWidgetReason::Paint);
	OnViewChanged.ExecuteIfBound();
	return FReply::Handled();
}

void SPjcInspectorGraph::OnMouseLeave(const FPointerEvent& MouseEvent) {
	if (Hovered != INDEX_NONE) {
		Hovered = INDEX_NONE;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

FReply SPjcInspectorGraph::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) {
	if (InKeyEvent.GetKey() == EKeys::F) {
		FrameSelected();
		return FReply::Handled();
	}
	if (InKeyEvent.GetKey() == EKeys::Home) {
		FitAll();
		return FReply::Handled();
	}
	if (InKeyEvent.GetKey() == EKeys::Escape && Locked != INDEX_NONE) {
		Locked = INDEX_NONE;
		UpdateFlow();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FCursorReply SPjcInspectorGraph::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const {
	if (bPanning) return FCursorReply::Cursor(EMouseCursor::GrabHandClosed);
	if (Hovered != INDEX_NONE && Graph.Nodes[Hovered].Kind != EPjcInspectorNodeKind::Source) return FCursorReply::Cursor(EMouseCursor::Hand);
	return FCursorReply::Cursor(EMouseCursor::Default);
}

FText SPjcInspectorGraph::GetHoverText() const {
	if (bPanning) return FText::GetEmpty();
	if (!Shown.IsValidIndex(Hovered)) {
		for (const FMark& Mark : Marks) {
			if (FVector2D::Distance(Mark.Position, MouseAt) <= 11.0f) return FText::FromString(Mark.ToolTip);
		}
		return FText::GetEmpty();
	}

	return FText::FromString(Shown[Hovered].ToolTip);
}
