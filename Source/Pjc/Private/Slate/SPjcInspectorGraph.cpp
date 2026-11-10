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
		Pair.Value.Sort([this](const int32 A, const int32 B) {
			if (Graph.Nodes[A].Column != Graph.Nodes[B].Column) return Graph.Nodes[A].Column < Graph.Nodes[B].Column;
			return A < B;
		});
		int32 Slot = -1;
		for (const int32 Index : Pair.Value) {
			Slot = FMath::Max(Graph.Nodes[Index].Column, Slot + 1);
			Shown[Index].Position = FVector2D {PjcGraphPad + Slot * (NodeWidth + PjcGraphGapX), PjcGraphPad + Pair.Key * (NodeHeight + PjcGraphGapY)};
			MaxSlot = FMath::Max(MaxSlot, Slot);
		}
		MaxRow = FMath::Max(MaxRow, Pair.Key);
	}
	if (Graph.NumHidden > 0) {
		++MaxSlot;
		MorePosition = FVector2D {PjcGraphPad + MaxSlot * (NodeWidth + PjcGraphGapX), PjcGraphPad};
	}
	GraphSize = FVector2D {
	  PjcGraphPad * 2.0f + MaxSlot * (NodeWidth + PjcGraphGapX) + NodeWidth + 120.0f,
	  PjcGraphPad * 2.0f + MaxRow * (NodeHeight + PjcGraphGapY) + NodeHeight
	};

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
		if (Node.Package != Graph.Focus) {
			Item.ToolTip += TEXT("\nDouble-click to inspect it");
		}
	}
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
	if (Fit >= PjcGraphReadable || Focus == INDEX_NONE) {
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

	// references: curves from the bottom of one box to the top of the next
	TArray<FVector2D> Points;
	TArray<FVector2D> Dash;
	for (const FPjcInspectorEdge& Edge : Graph.Edges) {
		if (!Shown.IsValidIndex(Edge.From) || !Shown.IsValidIndex(Edge.To)) continue;

		const bool bSource = Graph.Nodes[Edge.From].Kind == EPjcInspectorNodeKind::Source;
		const bool bTouched = !bSource && Selected != INDEX_NONE && (Edge.From == Selected || Edge.To == Selected);
		FLinearColor Color = PjcGraphColor(TEXT("#2C9A8F"));
		float Width = 2.2f;
		float DashOn = 0.0f;
		float DashOff = 0.0f;
		if (bSource) {
			Color = PjcGraphColor(TEXT("#8A7440"));
			Width = 2.5f;
		}
		else if (Graph.bUnusedGroup) {
			Color = PjcGraphColor(TEXT("#A8565A"));
		}
		else if (Edge.Kind == EPjcLinkKind::EditorOnly) {
			Color = PjcGraphColor(TEXT("#7FB2FF"));
		}
		if (!bSource && Edge.Kind == EPjcLinkKind::Soft) {
			DashOn = 6.0f;
			DashOff = 4.0f;
		}
		else if (!bSource && Edge.Kind == EPjcLinkKind::EditorOnly) {
			DashOn = 2.0f;
			DashOff = 4.0f;
		}
		if (bTouched) {
			Color = SelectedColor;
			Width = 3.0f;
		}

		const FVector2D& From = Shown[Edge.From].Position;
		const FVector2D& To = Shown[Edge.To].Position;
		const FVector2D Start {From.X + NodeWidth * 0.5f, From.Y + NodeHeight};
		const FVector2D Tip {To.X + NodeWidth * 0.5f, To.Y - 1.0f};
		const FVector2D End {Tip.X, Tip.Y - 8.0f};
		const bool bDown = End.Y > Start.Y;
		const FVector2D C1 = bDown ? FVector2D {Start.X, End.Y - 30.0f} : FVector2D {Start.X + 60.0f, Start.Y + 40.0f};
		const FVector2D C2 = bDown ? FVector2D {End.X, Start.Y + 30.0f} : FVector2D {End.X + 60.0f, End.Y - 40.0f};

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
			// dashes along the curve, in graph units
			const float On = DashOn * Zoom;
			const float Period = (DashOn + DashOff) * Zoom;
			float Walked = 0.0f;
			for (int32 Index = 1; Index < Points.Num(); ++Index) {
				const FVector2D A = Points[Index - 1];
				const FVector2D B = Points[Index];
				const float Length = static_cast<float>(FVector2D::Distance(A, B));
				float Along = 0.0f;
				while (Along < Length) {
					const float Phase = FMath::Fmod(Walked + Along, Period);
					const bool bOn = Phase < On;
					const float Span = FMath::Min(Length - Along, bOn ? On - Phase : Period - Phase);
					if (bOn && Span > 0.01f) {
						Dash.Reset();
						Dash.Add(A + (B - A) * (Along / Length));
						Dash.Add(A + (B - A) * ((Along + Span) / Length));
						FSlateDrawElement::MakeLines(OutDrawElements, LayerEdges, EdgeGeometry, Dash, ESlateDrawEffect::None, Color, true, Thickness);
					}
					Along += FMath::Max(Span, 0.01f);
				}
				Walked += Length;
			}
		}

		// arrow head
		const FVector2D Direction = (Tip - C2).GetSafeNormal();
		const FVector2D Side {-Direction.Y, Direction.X};
		const FVector2D Base = Tip - Direction * 9.0f;
		Dash.Reset();
		Dash.Add(ToLocal(Base + Side * 4.5f));
		Dash.Add(ToLocal(Tip));
		Dash.Add(ToLocal(Base - Side * 4.5f));
		FSlateDrawElement::MakeLines(OutDrawElements, LayerEdges, EdgeGeometry, Dash, ESlateDrawEffect::None, Color, true, Thickness);

		// what the link is, when not a plain hard reference
		FString Label;
		if (Graph.Nodes[Edge.From].Kind == EPjcInspectorNodeKind::Group) {
			Label = FString::Printf(TEXT("%d references"), Edge.NumLinks);
		}
		else if (!bSource && Edge.Kind == EPjcLinkKind::Soft) {
			Label = TEXT("soft");
		}
		else if (!bSource && Edge.Kind == EPjcLinkKind::EditorOnly) {
			Label = TEXT("editor only");
		}
		if (!Label.IsEmpty() && Zoom >= PjcGraphTextZoom) {
			const bool bSameColumn = FMath::Abs(Start.X - End.X) < 1.0;
			const FVector2D Position = bSameColumn ? FVector2D {End.X + 10.0f, End.Y - 30.0f} : FVector2D {Start.X + 10.0f, Start.Y + 8.0f};
			const FVector2D Size {FontMeasure->Measure(Label, FontLabel)};
			Box(LayerLabels, Position, Size + FVector2D {10.0f, 4.0f}, BrushNode, Background);
			const FVector2D LabelPosition = Position + FVector2D {5.0f, 2.0f};
			Text(LayerLabels + 1, LabelPosition, static_cast<float>(Size.X) + 1.0f, Label, FontLabel, bTouched ? SelectedColor : Foreground);
		}
	}

	// boxes
	const int32 Focus = Graph.FindNode(Graph.Focus);
	for (int32 Index = 0; Index < Shown.Num(); ++Index) {
		const FPjcInspectorNode& Node = Graph.Nodes[Index];
		const FShownNode& Item = Shown[Index];
		const FVector2D& Position = Item.Position;
		const FVector2D Size {NodeWidth, NodeHeight};

		if (Node.Kind == EPjcInspectorNodeKind::Source) {
			Box(LayerBody, Position, Size, BrushPill, SourceLine);
			Box(LayerBody + 1, Position + FVector2D {1.0f, 1.0f}, Size - FVector2D {2.0f, 2.0f}, BrushPill, SourceFill);
			Text(LayerContent, Position + FVector2D {16.0f, 7.0f}, NodeWidth - 30.0f, Item.Title, FontTitle, Gold);
			Text(LayerContent, Position + FVector2D {16.0f, 24.0f}, NodeWidth - 30.0f, Item.Line, FontLine, Item.LineColor);
			continue;
		}

		const bool bFocus = Index == Focus;
		if (Index == Selected) {
			Ring(Position, BrushNode, SelectedColor);
		}
		else if (Index == Highlighted) {
			Ring(Position, BrushNode, SelectedColor * FLinearColor {1.0f, 1.0f, 1.0f, 0.55f});
		}

		FLinearColor Body = Index == Hovered ? FillHovered : Fill;
		if (bFocus) {
			Body = Item.bUsed ? PjcGraphColor(TEXT("#20302F")) : PjcGraphColor(TEXT("#33232A"));
		}
		Box(LayerBody, Position, Size, BrushNode, bFocus ? Item.Stripe : Line);
		Box(LayerBody + 1, Position, FVector2D {6.0f, NodeHeight}, BrushNode, Item.Stripe);
		Box(LayerBody + 2, Position + FVector2D {3.0f, 1.0f}, Size - FVector2D {4.0f, 2.0f}, BrushNode, Body);

		float TextLeft = 12.0f;
		if (Item.Icon && Zoom >= PjcGraphTextZoom) {
			Box(LayerContent, Position + FVector2D {11.0f, 13.0f}, FVector2D {20.0f, 20.0f}, Item.Icon, Item.IconColor);
			TextLeft = 39.0f;
		}
		Text(LayerContent, Position + FVector2D {TextLeft, 6.0f}, NodeWidth - TextLeft - 8.0f, Item.Title, FontTitle, Foreground);
		Text(LayerContent, Position + FVector2D {TextLeft, 24.0f}, NodeWidth - TextLeft - 8.0f, Item.Line, FontLine, Item.LineColor);
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

	const int32 Node = FindNodeAt(Local);
	if (Node != Hovered) {
		Hovered = Node;
		Invalidate(EInvalidateWidgetReason::Paint);
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
	return FReply::Unhandled();
}

FCursorReply SPjcInspectorGraph::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const {
	if (bPanning) return FCursorReply::Cursor(EMouseCursor::GrabHandClosed);
	if (Hovered != INDEX_NONE && Graph.Nodes[Hovered].Kind != EPjcInspectorNodeKind::Source) return FCursorReply::Cursor(EMouseCursor::Hand);
	return FCursorReply::Cursor(EMouseCursor::Default);
}

FText SPjcInspectorGraph::GetHoverText() const {
	if (bPanning || !Shown.IsValidIndex(Hovered)) return FText::GetEmpty();

	return FText::FromString(Shown[Hovered].ToolTip);
}
