// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcAssetGraph.h"
#include "Slate/SPjcItemUsage.h"
#include "PjcAssetGraph.h"
#include "PjcShim.h"
#include "PjcStyles.h"
#include "PjcTypes.h"
// Engine Headers
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/SlateIconFinder.h"

namespace
{
	// folder opens when its circle gets this big on screen, fully open after fade
	constexpr float FolderOpenPixels = 120.0f;
	constexpr float FolderFadePixels = 90.0f;
	constexpr float MaxZoom = 12.0f;
	constexpr float CullMargin = 60.0f;
	constexpr int32 MaxLabels = 90;
	constexpr int32 MaxForcedNodes = 300;
	// asset circle this big shows class icon
	constexpr float IconPixels = 13.0f;
	// hovered closed folder shows links of its assets when it has not more of them
	constexpr int32 MaxHoverFolderAssets = 20000;

	const FLinearColor BackgroundColor {0.006f, 0.007f, 0.009f, 1.0f};
	const FLinearColor PanelColor {0.0f, 0.0f, 0.0f, 0.75f};
	const FLinearColor LinkColor {0.4f, 0.5f, 0.65f, 1.0f};
	const FLinearColor TextColor {0.85f, 0.87f, 0.9f, 1.0f};
	const FLinearColor HintColor {0.3f, 0.32f, 0.35f, 1.0f};
	const FLinearColor FolderColor {0.55f, 0.6f, 0.7f, 1.0f};

	FLinearColor WithAlpha(const FLinearColor& Color, const float InAlpha) {
		return FLinearColor {Color.R, Color.G, Color.B, Color.A * FMath::Clamp(InAlpha, 0.0f, 1.0f)};
	}

	FPaintGeometry MakePaintGeometry(const FGeometry& Geometry, const FVector2D& Offset, const FVector2D& Size) {
		return Geometry.ToPaintGeometry(Size, FSlateLayoutTransform {Offset});
	}

	/** Unit circle points, few sizes, so circles cost no trigonometry */
	const TArray<FVector2D>& GetUnitCircle(const float RadiusPixels) {
		static const int32 Counts[] = {6, 10, 16, 24, 40, 64, 96};
		static TArray<FVector2D> Tables[UE_ARRAY_COUNT(Counts)];

		int32 Table = 0;
		for (const float Limit : {2.0f, 5.0f, 12.0f, 30.0f, 80.0f, 200.0f}) {
			if (RadiusPixels < Limit) break;
			++Table;
		}

		if (Tables[Table].Num() == 0) {
			for (int32 Step = 0; Step < Counts[Table]; ++Step) {
				const float Angle = 6.2831853f * Step / Counts[Table];
				Tables[Table].Add(FVector2D {FMath::Cos(Angle), FMath::Sin(Angle)});
			}
		}
		return Tables[Table];
	}

	/** Anti aliased shapes in widget space, drawn by one draw call */
	class FPjcMesh
	{
	  public:

		explicit FPjcMesh(const FSlateRenderTransform& InTransform) : Transform(InTransform) {}

		void Disc(const FVector2D& Center, const float Radius, const FLinearColor& Color) {
			if (Color.A < 0.004f || Radius <= 0.0f) return;

			const TArray<FVector2D>& Circle = GetUnitCircle(Radius);
			const int32 Num = Circle.Num();
			const FColor Solid = Color.ToFColor(true);
			const FColor Clear = WithAlpha(Color, 0.0f).ToFColor(true);
			const float Inner = FMath::Max(Radius - 0.5f, 0.0f);
			const float Outer = Radius + 0.5f;

			const SlateIndex First = static_cast<SlateIndex>(Vertices.Num());
			AddVertex(Center, Solid);
			for (const auto& Dir : Circle) {
				AddVertex(Center + Dir * Inner, Solid);
				AddVertex(Center + Dir * Outer, Clear);
			}
			for (int32 Step = 0; Step < Num; ++Step) {
				const SlateIndex InnerA = First + 1 + Step * 2;
				const SlateIndex InnerB = First + 1 + (Step + 1) % Num * 2;
				AddTriangle(First, InnerA, InnerB);
				AddTriangle(InnerA, InnerA + 1, InnerB + 1);
				AddTriangle(InnerA, InnerB + 1, InnerB);
			}
		}

		void Ring(const FVector2D& Center, const float Radius, const float Thickness, const FLinearColor& Color) {
			if (Color.A < 0.004f || Radius <= 0.0f) return;

			const TArray<FVector2D>& Circle = GetUnitCircle(Radius);
			const int32 Num = Circle.Num();
			const FColor Solid = Color.ToFColor(true);
			const FColor Clear = WithAlpha(Color, 0.0f).ToFColor(true);
			const float Half = FMath::Max(Thickness * 0.5f - 0.5f, 0.0f);
			const float Radii[] = {FMath::Max(Radius - Half - 1.0f, 0.0f), Radius - Half, Radius + Half, Radius + Half + 1.0f};
			const FColor Colors[] = {Clear, Solid, Solid, Clear};

			const SlateIndex First = static_cast<SlateIndex>(Vertices.Num());
			for (const auto& Dir : Circle) {
				for (int32 Band = 0; Band < 4; ++Band) {
					AddVertex(Center + Dir * Radii[Band], Colors[Band]);
				}
			}
			for (int32 Step = 0; Step < Num; ++Step) {
				const SlateIndex A = First + Step * 4;
				const SlateIndex B = First + (Step + 1) % Num * 4;
				for (int32 Band = 0; Band < 3; ++Band) {
					AddTriangle(A + Band, A + Band + 1, B + Band + 1);
					AddTriangle(A + Band, B + Band + 1, B + Band);
				}
			}
		}

		/** Part of ring, angles in turns clockwise from top */
		void
		Arc(const FVector2D& Center, const float Radius, const float Thickness, const float Start, const float Sweep, const FLinearColor& Color) {
			if (Color.A < 0.004f || Radius <= 0.0f || Sweep <= 0.0001f) return;

			const int32 Num = FMath::Clamp(static_cast<int32>(Radius * Sweep * 0.8f) + 2, 2, 128);
			const FColor Solid = Color.ToFColor(true);
			const FColor Clear = WithAlpha(Color, 0.0f).ToFColor(true);
			const float Half = FMath::Max(Thickness * 0.5f - 0.5f, 0.0f);
			const float Radii[] = {FMath::Max(Radius - Half - 1.0f, 0.0f), Radius - Half, Radius + Half, Radius + Half + 1.0f};
			const FColor Colors[] = {Clear, Solid, Solid, Clear};

			const SlateIndex First = static_cast<SlateIndex>(Vertices.Num());
			for (int32 Step = 0; Step <= Num; ++Step) {
				const float Angle = (Start + Sweep * Step / Num) * 6.2831853f;
				const FVector2D Dir {FMath::Sin(Angle), -FMath::Cos(Angle)};
				for (int32 Band = 0; Band < 4; ++Band) {
					AddVertex(Center + Dir * Radii[Band], Colors[Band]);
				}
			}
			for (int32 Step = 0; Step < Num; ++Step) {
				const SlateIndex A = First + Step * 4;
				const SlateIndex B = A + 4;
				for (int32 Band = 0; Band < 3; ++Band) {
					AddTriangle(A + Band, A + Band + 1, B + Band + 1);
					AddTriangle(A + Band, B + Band + 1, B + Band);
				}
			}
		}

		void Line(const FVector2D& Start, const FVector2D& End, const float Thickness, const FLinearColor& ColorStart, const FLinearColor& ColorEnd) {
			if (ColorStart.A < 0.004f && ColorEnd.A < 0.004f) return;

			const FVector2D Delta = End - Start;
			const float Length = static_cast<float>(Delta.Size());
			if (Length < 0.5f) return;

			const FVector2D Normal = FVector2D {-Delta.Y, Delta.X} / Length;
			const float Half = FMath::Max(Thickness * 0.5f - 0.5f, 0.0f);
			const float Offsets[] = {Half + 1.0f, Half, -Half, -Half - 1.0f};

			const SlateIndex First = static_cast<SlateIndex>(Vertices.Num());
			for (int32 Side = 0; Side < 2; ++Side) {
				const FVector2D& Point = Side == 0 ? Start : End;
				const FLinearColor& Color = Side == 0 ? ColorStart : ColorEnd;
				const FColor Solid = Color.ToFColor(true);
				const FColor Clear = WithAlpha(Color, 0.0f).ToFColor(true);
				for (int32 Band = 0; Band < 4; ++Band) {
					AddVertex(Point + Normal * Offsets[Band], Band == 0 || Band == 3 ? Clear : Solid);
				}
			}
			for (int32 Band = 0; Band < 3; ++Band) {
				AddTriangle(First + Band, First + Band + 1, First + Band + 5);
				AddTriangle(First + Band, First + Band + 5, First + Band + 4);
			}
		}

		void Triangle(const FVector2D& A, const FVector2D& B, const FVector2D& C, const FLinearColor& Color) {
			if (Color.A < 0.004f) return;

			const FColor Solid = Color.ToFColor(true);
			const SlateIndex First = static_cast<SlateIndex>(Vertices.Num());
			AddVertex(A, Solid);
			AddVertex(B, Solid);
			AddVertex(C, Solid);
			AddTriangle(First, First + 1, First + 2);
		}

		void Draw(FSlateWindowElementList& OutDrawElements, const int32 Layer, const FSlateResourceHandle& Handle) const {
			if (Indices.Num() == 0) return;

			FSlateDrawElement::MakeCustomVerts(OutDrawElements, Layer, Handle, Vertices, Indices, nullptr, 0, 0);
		}

	  private:

		void AddVertex(const FVector2D& Position, const FColor& Color) {
#if ENGINE_MAJOR_VERSION >= 5
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, FVector2f {Position}, FVector2f {0.0f, 0.0f}, Color));
#else
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, Position, FVector2D {0.0f, 0.0f}, Color));
#endif
		}

		void AddTriangle(const SlateIndex A, const SlateIndex B, const SlateIndex C) {
			Indices.Add(A);
			Indices.Add(B);
			Indices.Add(C);
		}

		FSlateRenderTransform Transform;
		TArray<FSlateVertex> Vertices;
		TArray<SlateIndex> Indices;
	};
}	// namespace

void SPjcAssetGraph::Construct(const FArguments& InArgs) {
	OnSelectionChanged = InArgs._OnSelectionChanged;
	OnAssetDoubleClicked = InArgs._OnAssetDoubleClicked;

	SetClipping(EWidgetClipping::ClipToBounds);
}

void SPjcAssetGraph::SetGraph(const TSharedPtr<FPjcAssetGraph>& InGraph) {
	Graph = InGraph;

	const int32 NumNodes = Graph.IsValid() ? Graph->GetNodes().Num() : 0;
	const int32 NumFolders = Graph.IsValid() ? Graph->GetFolders().Num() : 0;
	Selected = INDEX_NONE;
	Hovered = INDEX_NONE;
	HoveredFolder = INDEX_NONE;
	Focus.Init(EFocus::None, NumNodes);
	FocusLinks.Reset();
	Emphasized.Init(true, NumNodes);
	FolderEmphasized.Init(0, NumFolders);
	bEmphasisActive = false;
	NumEmphasized = NumNodes;
	DrawnNodes.Reset();
	DrawnFolders.Reset();

	// class icons, shown inside circles big enough
	Icons.Reset(NumNodes);
	for (int32 Index = 0; Index < NumNodes; ++Index) {
		const FAssetData& Asset = Graph->GetNodes()[Index].Item->Asset;
		Icons.Add(Asset.IsValid() ? FSlateIconFinder::FindIconBrushForClass(Asset.GetClass()) : nullptr);
	}

	bViewMoving = false;
	bZoomAnchored = false;
	bFocusFitPending = false;
	bKeepSelectedPending = false;
	bFitPending = true;
	StartTimer();
}

void SPjcAssetGraph::SetSelection(const FName& PackageName, const bool bMoveView) {
	const int32 Node = Graph.IsValid() && !PackageName.IsNone() ? Graph->Find(PackageName) : INDEX_NONE;
	SelectNode(Node, false);
	if (Node == INDEX_NONE || !bMoveView) return;

	bFocusFitPending = true;
	FocusFitPaint = NumPaints;
	bFitPending = false;
	StartTimer();
}

void SPjcAssetGraph::SetEmphasis(const TSet<FName>& Packages, const bool bActive) {
	if (!Graph.IsValid()) return;

	const TArray<FPjcGraphNode>& Nodes = Graph->GetNodes();
	const TArray<FPjcGraphFolder>& Folders = Graph->GetFolders();
	bEmphasisActive = bActive;
	Emphasized.Init(!bActive, Nodes.Num());
	FolderEmphasized.Init(0, Folders.Num());
	NumEmphasized = bActive ? 0 : Emphasized.Num();
	if (!bActive) return;

	for (const auto& Package : Packages) {
		const int32 Node = Graph->Find(Package);
		if (Node == INDEX_NONE) continue;

		Emphasized[Node] = true;
		++NumEmphasized;
		for (int32 Folder = Nodes[Node].Folder; Folder != INDEX_NONE; Folder = Folders[Folder].Parent) {
			++FolderEmphasized[Folder];
		}
	}
}

void SPjcAssetGraph::ZoomToFit() {
	if (!Graph.IsValid() || Graph->GetNodes().Num() == 0) return;

	if (Selected != INDEX_NONE) {
		FitFocus();
		return;
	}

	const FBox2D Box = Graph->GetBounds();
	MoveView(Box.GetCenter(), GetFitZoom(Box));
}

int32 SPjcAssetGraph::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled
) const {
	ViewSize = AllottedGeometry.GetLocalSize();
	DrawnNodes.Reset();
	DrawnFolders.Reset();
	++NumPaints;

	const FSlateBrush* WhiteBrush = PjcShim::GetBrush(TEXT("WhiteBrush"));
	const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FSlateResourceHandle Handle = FSlateApplication::Get().GetRenderer()->GetResourceHandle(*WhiteBrush);
	const FSlateRenderTransform& Transform = AllottedGeometry.GetAccumulatedRenderTransform();

	const int32 LayerNodes = LayerId + 1;
	const int32 LayerIcons = LayerId + 2;
	const int32 LayerFocusLinks = LayerId + 3;
	const int32 LayerFocusNodes = LayerId + 4;
	const int32 LayerLabels = LayerId + 5;
	const int32 LayerPanels = LayerId + 6;
	const int32 LayerPanelContent = LayerId + 7;

	const FSlateFontInfo FontRegular = FPjcStyles::GetFont(TEXT("Regular"), 8);
	const FSlateFontInfo FontBold = FPjcStyles::GetFont(TEXT("Bold"), 9);
	const FSlateFontInfo FontTitle = FPjcStyles::GetFont(TEXT("Bold"), 10);
	const FSlateFontInfo FontFolder = FPjcStyles::GetFont(TEXT("Bold"), 11);

	const FLinearColor ColorUsed = FPjcStyles::Get().GetColor("ProjectCleaner.Color.Green");
	const FLinearColor ColorUnused = FPjcStyles::Get().GetColor("ProjectCleaner.Color.Red");
	const FLinearColor ColorNotDeleted = FPjcStyles::Get().GetColor("ProjectCleaner.Color.Yellow");

	const auto MeasureText = [&](const FString& Text, const FSlateFontInfo& Font) {
		return FVector2D {FontMeasure->Measure(Text, Font)};
	};
	const auto DrawText
		= [&](const FString& Text, const FVector2D& Position, const FSlateFontInfo& Font, const FLinearColor& Color, const int32 Layer) {
			  const FVector2D Size = MeasureText(Text, Font);
			  const FLinearColor Shadow {0.0f, 0.0f, 0.0f, Color.A * 0.9f};
			  FSlateDrawElement::MakeText(
				  OutDrawElements,
				  Layer,
				  MakePaintGeometry(AllottedGeometry, Position + FVector2D {1.0f, 1.0f}, Size),
				  Text,
				  Font,
				  ESlateDrawEffect::None,
				  Shadow
			  );
			  FSlateDrawElement::MakeText(
				  OutDrawElements, Layer, MakePaintGeometry(AllottedGeometry, Position, Size), Text, Font, ESlateDrawEffect::None, Color
			  );
		  };
	const auto DrawPanel = [&](const FVector2D& Position, const FVector2D& Size, const FLinearColor& Color) {
		FSlateDrawElement::MakeBox(
			OutDrawElements, LayerPanels, MakePaintGeometry(AllottedGeometry, Position, Size), WhiteBrush, ESlateDrawEffect::None, Color
		);
	};
	const auto DrawIcon = [&](const int32 Node, const FVector2D& Center, const float RadiusPixels, const float InAlpha, const int32 Layer) {
		if (!Icons.IsValidIndex(Node) || !Icons[Node] || RadiusPixels < IconPixels || InAlpha < 0.3f) return;

		const float Size = FMath::Min(RadiusPixels * 1.1f, 40.0f);
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			Layer,
			MakePaintGeometry(AllottedGeometry, Center - FVector2D {Size, Size} * 0.5f, FVector2D {Size, Size}),
			Icons[Node],
			ESlateDrawEffect::None,
			FLinearColor {0.0f, 0.0f, 0.0f, 0.55f * InAlpha}
		);
	};

	FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), WhiteBrush, ESlateDrawEffect::None, BackgroundColor);

	if (!Graph.IsValid() || Graph->GetNodes().Num() == 0) {
		const FString Text = Graph.IsValid() ? TEXT("No assets to show") : TEXT("Press Scan to see project assets");
		DrawText(Text, (ViewSize - MeasureText(Text, FontTitle)) * 0.5f, FontTitle, HintColor, LayerLabels);
		return LayerPanelContent;
	}

	const TArray<FPjcGraphNode>& Nodes = Graph->GetNodes();
	const TArray<FPjcGraphFolder>& Folders = Graph->GetFolders();
	const bool bFocusActive = Selected != INDEX_NONE;
	const auto IsOnScreen = [this](const FVector2D& Center, const float Radius) {
		return Center.X + Radius > -CullMargin && Center.Y + Radius > -CullMargin && Center.X - Radius < ViewSize.X + CullMargin
			&& Center.Y - Radius < ViewSize.Y + CullMargin;
	};

	// level of detail: folder opens while it grows on screen, what it holds fades in
	FolderOpenness.SetNumUninitialized(Folders.Num());
	FolderAlpha.SetNumUninitialized(Folders.Num());
	FolderShown.SetNumUninitialized(Folders.Num());
	for (int32 Index = 0; Index < Folders.Num(); ++Index) {
		const FPjcGraphFolder& Folder = Folders[Index];
		const float Open = FMath::Clamp((Folder.Radius * Zoom - FolderOpenPixels) / FolderFadePixels, 0.0f, 1.0f);
		const int32 Parent = Folder.Parent;
		const int32 ParentShown = Parent == INDEX_NONE ? INDEX_NONE : FolderShown[Parent];
		FolderOpenness[Index] = Open;
		FolderAlpha[Index] = Parent == INDEX_NONE ? 1.0f : FolderAlpha[Parent] * FolderOpenness[Parent];
		FolderShown[Index] = ParentShown != INDEX_NONE ? ParentShown : (Open < 0.5f ? Index : INDEX_NONE);
	}

	// folders with what they hold, closed folders end there
	FPjcMesh NodeMesh {Transform};
	TArray<int32> Stack = Graph->GetRootFolders();
	while (Stack.Num() > 0) {
		const int32 Index = PjcShim::PopArray(Stack);
		const FPjcGraphFolder& Folder = Folders[Index];
		const FVector2D Center = GraphToLocal(Folder.Center);
		const float RadiusPixels = Folder.Radius * Zoom;
		if (!IsOnScreen(Center, RadiusPixels + 6.0f)) continue;

		const float Open = FolderOpenness[Index];
		const float FolderDim = bFocusActive ? 0.1f : (bEmphasisActive && FolderEmphasized[Index] == 0 ? 0.3f : 1.0f);
		const float Alpha = FolderAlpha[Index] * FolderDim;

		// mostly unused folders turn red, ring outside folder: red part is unused share of its size
		const float Unused = Folder.Size > 0 ? static_cast<float>(static_cast<double>(Folder.SizeUnused) / static_cast<double>(Folder.Size)) : 0.0f;
		const FLinearColor Fill = FMath::Lerp(FolderColor, ColorUnused, Unused * 0.7f);
		NodeMesh.Disc(Center, RadiusPixels, WithAlpha(Fill, (0.12f * (1.0f - Open) + 0.03f * Open) * Alpha));
		NodeMesh.Ring(Center, RadiusPixels, 1.2f, WithAlpha(FolderColor, (0.45f * (1.0f - Open) + 0.18f * Open) * Alpha));

		if (Folder.Size > 0 && RadiusPixels >= 6.0f) {
			const float GaugeRadius = RadiusPixels + 3.5f;
			const float Thickness = Open < 0.5f ? 3.0f : 2.0f;
			NodeMesh.Arc(Center, GaugeRadius, Thickness, Unused, 1.0f - Unused, WithAlpha(ColorUsed, 0.45f * Alpha));
			NodeMesh.Arc(Center, GaugeRadius, Thickness, 0.0f, Unused, WithAlpha(ColorUnused, 0.95f * Alpha));
		}

		if (Open < 0.5f) {
			DrawnFolders.Add(FDrawnItem {Index, Center, RadiusPixels});
		}
		if (Open <= 0.0f) continue;

		Stack.Append(Folder.Folders);
		for (const int32 Asset : Folder.Assets) {
			const FPjcGraphNode& Node = Nodes[Asset];
			const FVector2D NodeCenter = GraphToLocal(Node.Position);
			const float NodeRadius = FMath::Max(Node.Radius * Zoom, 0.8f);
			if (!IsOnScreen(NodeCenter, NodeRadius)) continue;

			const float NodeAlpha = FolderAlpha[Index] * Open * GetDim(Asset);
			NodeMesh.Disc(NodeCenter, NodeRadius, WithAlpha(SPjcItemUsage::GetStatusColor(*Node.Item), NodeAlpha));
			if (FPjcAssetGraph::IsUsageRoot(Node) && NodeRadius >= 2.0f) {
				NodeMesh.Ring(NodeCenter, NodeRadius + 3.0f, 1.5f, WithAlpha(FLinearColor::White, 0.85f * NodeAlpha));
			}
			DrawIcon(Asset, NodeCenter, NodeRadius, NodeAlpha, LayerIcons);

			if (FolderAlpha[Index] * Open >= 0.5f) {
				DrawnNodes.Add(FDrawnItem {Asset, NodeCenter, FMath::Max(NodeRadius, 4.0f)});
			}
		}
	}
	NodeMesh.Draw(OutDrawElements, LayerNodes, Handle);

	// links: of selected asset (usage chain and direct links), hovered asset, or hovered closed folder
	FPjcMesh FocusLinkMesh {Transform};
	FPjcMesh FocusNodeMesh {Transform};
	const auto DrawArrow =
		[&](
			const FVector2D& From, const float RadiusFrom, const FVector2D& To, const float RadiusTo, const float Thickness, const FLinearColor& Color
		) {
			const FVector2D Delta = To - From;
			const float Length = static_cast<float>(Delta.Size());
			if (Length <= RadiusFrom + RadiusTo + 8.0f) return;

			const FVector2D Dir = Delta / Length;
			const FVector2D Normal {-Dir.Y, Dir.X};
			const FVector2D Tip = To - Dir * RadiusTo;
			FocusLinkMesh.Line(From + Dir * RadiusFrom, Tip - Dir * 7.0f, Thickness, WithAlpha(Color, Color.A * 0.45f), Color);
			FocusLinkMesh.Triangle(Tip, Tip - Dir * 9.0f + Normal * 4.5f, Tip - Dir * 9.0f - Normal * 4.5f, Color);
		};

	if (bFocusActive) {
		for (const auto& Link : FocusLinks) {
			const FPjcGraphNode& From = Nodes[Link.From];
			const FPjcGraphNode& To = Nodes[Link.To];
			const FLinearColor Color = Link.bChain ? FLinearColor::White : SPjcItemUsage::GetStatusColor(*(Link.From == Selected ? To : From).Item);
			DrawArrow(
				GraphToLocal(From.Position),
				FMath::Max(From.Radius * Zoom, 3.5f) + 2.0f,
				GraphToLocal(To.Position),
				FMath::Max(To.Radius * Zoom, 3.5f) + 2.0f,
				Link.bChain ? 2.5f : 1.5f,
				WithAlpha(Color, 0.9f)
			);
		}
	}
	else if (!bDragging && Nodes.IsValidIndex(Hovered) && GetShown(Hovered) == Hovered) {
		// assets in closed folders are linked through them
		TMap<int32, bool> Ends;
		for (const int32 Use : Nodes[Hovered].Uses) {
			Ends.Add(GetShown(Use), true);
		}
		for (const int32 User : Nodes[Hovered].UsedBy) {
			if (!Ends.Contains(GetShown(User))) {
				Ends.Add(GetShown(User), false);
			}
		}

		const FVector2D HoveredCenter = GraphToLocal(Nodes[Hovered].Position);
		const float HoveredRadius = Nodes[Hovered].Radius * Zoom + 2.0f;
		for (const auto& End : Ends) {
			if (End.Key == Hovered) continue;

			const FVector2D OtherCenter = GraphToLocal(GetShownCenter(End.Key));
			const float OtherRadius = GetShownRadius(End.Key) * Zoom + 2.0f;
			const FLinearColor Color = WithAlpha(FLinearColor::White, 0.7f);
			if (End.Value) {
				DrawArrow(HoveredCenter, HoveredRadius, OtherCenter, OtherRadius, 1.5f, Color);
			}
			else {
				DrawArrow(OtherCenter, OtherRadius, HoveredCenter, HoveredRadius, 1.5f, Color);
			}
		}
	}
	else if (!bDragging && Folders.IsValidIndex(HoveredFolder) && Folders[HoveredFolder].NumAssets <= MaxHoverFolderAssets) {
		// what assets of folder use outside of it, line width tells how many links
		TMap<int32, int32> Uses;
		TArray<int32> FolderStack {HoveredFolder};
		while (FolderStack.Num() > 0) {
			const FPjcGraphFolder& Folder = Folders[PjcShim::PopArray(FolderStack)];
			FolderStack.Append(Folder.Folders);
			for (const int32 Asset : Folder.Assets) {
				for (const int32 Use : Nodes[Asset].Uses) {
					const int32 Shown = GetShown(Use);
					if (Shown != Nodes.Num() + HoveredFolder && !IsInsideFolder(Use, HoveredFolder)) {
						++Uses.FindOrAdd(Shown);
					}
				}
			}
		}

		const FPjcGraphFolder& Source = Folders[HoveredFolder];
		const FVector2D From = GraphToLocal(Source.Center);
		for (const auto& Use : Uses) {
			const float Weight = FMath::Loge(static_cast<float>(Use.Value));
			DrawArrow(
				From,
				Source.Radius * Zoom + 4.0f,
				GraphToLocal(GetShownCenter(Use.Key)),
				GetShownRadius(Use.Key) * Zoom + 4.0f,
				1.0f + FMath::Min(3.0f, Weight * 0.6f),
				WithAlpha(FLinearColor::White, 0.35f + FMath::Min(0.45f, Weight * 0.1f))
			);
		}
	}

	// selected asset with its chain and links, or few search results, drawn over everything wherever they are
	const bool bForceEmphasized = !bFocusActive && bEmphasisActive && NumEmphasized <= MaxForcedNodes;
	for (int32 Index = 0; Index < Nodes.Num() && (bFocusActive || bForceEmphasized); ++Index) {
		if (bFocusActive ? Focus[Index] == EFocus::None : !Emphasized[Index]) continue;

		const FPjcGraphNode& Node = Nodes[Index];
		const FVector2D Center = GraphToLocal(Node.Position);
		const float RadiusPixels = FMath::Max(Node.Radius * Zoom, 3.5f);
		if (!IsOnScreen(Center, RadiusPixels + 10.0f)) continue;

		FocusNodeMesh.Disc(Center, RadiusPixels + 1.5f, BackgroundColor);
		FocusNodeMesh.Disc(Center, RadiusPixels, SPjcItemUsage::GetStatusColor(*Node.Item));
		if (FPjcAssetGraph::IsUsageRoot(Node)) {
			FocusNodeMesh.Ring(Center, RadiusPixels + 3.0f, 1.5f, WithAlpha(FLinearColor::White, 0.85f));
		}
		if (Index == Selected) {
			FocusNodeMesh.Ring(Center, RadiusPixels + 6.0f, 2.0f, FLinearColor::White);
			FocusNodeMesh.Ring(Center, RadiusPixels + 10.0f, 6.0f, WithAlpha(FLinearColor::White, 0.12f));
		}
		DrawIcon(Index, Center, RadiusPixels, 1.0f, LayerLabels);
		DrawnNodes.Add(FDrawnItem {Index, Center, FMath::Max(RadiusPixels, 6.0f)});
	}
	if (!bDragging) {
		for (const auto& DrawnNode : DrawnNodes) {
			if (DrawnNode.Item == Hovered) {
				FocusNodeMesh.Ring(DrawnNode.Center, DrawnNode.Radius + 2.0f, 1.5f, WithAlpha(FLinearColor::White, 0.6f));
				break;
			}
		}
		for (const auto& DrawnFolder : DrawnFolders) {
			if (DrawnFolder.Item == HoveredFolder) {
				FocusNodeMesh.Ring(DrawnFolder.Center, DrawnFolder.Radius + 8.0f, 1.5f, WithAlpha(FLinearColor::White, 0.5f));
			}
		}
	}
	FocusLinkMesh.Draw(OutDrawElements, LayerFocusLinks, Handle);
	FocusNodeMesh.Draw(OutDrawElements, LayerFocusNodes, Handle);

	// labels: folders first, then important assets, then biggest circles, never on top of each other
	TArray<FSlateRect> Placed;
	const auto MakeRect = [](const FVector2D& Position, const FVector2D& Size) {
		return FSlateRect(
			static_cast<float>(Position.X - 2.0f),
			static_cast<float>(Position.Y - 1.0f),
			static_cast<float>(Position.X + Size.X + 2.0f),
			static_cast<float>(Position.Y + Size.Y + 1.0f)
		);
	};
	const auto IsFree = [&Placed](const FSlateRect& Rect) {
		return !Placed.ContainsByPredicate([&Rect](const FSlateRect& Other) {
			return FSlateRect::DoRectanglesIntersect(Rect, Other);
		});
	};

	struct FLine
	{
		FString Text;
		const FSlateFontInfo* Font;
		FLinearColor Color;
	};
	const auto GetBlockSize = [&](const TArray<FLine>& Lines) {
		FVector2D Size {0.0f, 0.0f};
		for (const auto& Line : Lines) {
			const FVector2D LineSize = MeasureText(Line.Text, *Line.Font);
			Size.X = FMath::Max(Size.X, LineSize.X);
			Size.Y += LineSize.Y;
		}
		return Size;
	};
	const auto DrawBlock = [&](const TArray<FLine>& Lines, const FVector2D& Position, const float Width) {
		float Y = static_cast<float>(Position.Y);
		for (const auto& Line : Lines) {
			const FVector2D LineSize = MeasureText(Line.Text, *Line.Font);
			DrawText(Line.Text, FVector2D {Position.X + (Width - LineSize.X) * 0.5f, Y}, *Line.Font, Line.Color, LayerLabels);
			Y += static_cast<float>(LineSize.Y);
		}
	};
	const auto GetFolderLines = [&](const int32 Index) {
		const FPjcGraphFolder& Folder = Folders[Index];
		TArray<FLine> Lines;
		Lines.Add(FLine {Folder.Name, &FontFolder, FLinearColor::White});
		Lines.Add(
			FLine {FString::Printf(TEXT("%d assets, %s"), Folder.NumAssets, *FText::AsMemory(Folder.Size, IEC).ToString()), &FontRegular, TextColor}
		);
		if (Folder.NumUnused > 0) {
			Lines.Add(
				FLine {
				  FString::Printf(TEXT("%d unused, %s"), Folder.NumUnused, *FText::AsMemory(Folder.SizeUnused, IEC).ToString()),
				  &FontRegular,
				  ColorUnused
				}
			);
		}
		if (bEmphasisActive) {
			Lines.Add(FLine {FString::Printf(TEXT("%d matching"), FolderEmphasized[Index]), &FontRegular, ColorNotDeleted});
		}
		return Lines;
	};

	if (!bFocusActive) {
		// closed folders, biggest first
		TArray<FDrawnItem> ClosedFolders = DrawnFolders;
		ClosedFolders.Sort([](const FDrawnItem& A, const FDrawnItem& B) {
			return A.Radius > B.Radius;
		});
		for (const auto& DrawnFolder : ClosedFolders) {
			if (DrawnFolder.Radius < 30.0f && DrawnFolder.Item != HoveredFolder) continue;

			const TArray<FLine> Lines = GetFolderLines(DrawnFolder.Item);
			const FVector2D BlockSize = GetBlockSize(Lines);
			const FVector2D Position = DrawnFolder.Center - BlockSize * 0.5f;
			const FSlateRect Rect = MakeRect(Position, BlockSize);
			if (!IsFree(Rect) && DrawnFolder.Item != HoveredFolder) continue;

			Placed.Add(Rect);
			DrawBlock(Lines, Position, static_cast<float>(BlockSize.X));
		}

		// open folders keep their name at the top
		for (int32 Index = 0; Index < Folders.Num(); ++Index) {
			if (FolderOpenness[Index] < 0.5f || FolderAlpha[Index] < 0.5f) continue;

			const FVector2D Center = GraphToLocal(Folders[Index].Center);
			const float RadiusPixels = Folders[Index].Radius * Zoom;
			const FVector2D NameSize = MeasureText(Folders[Index].Name, FontFolder);
			const FVector2D Position {Center.X - NameSize.X * 0.5f, Center.Y - RadiusPixels - NameSize.Y - 7.0f};
			if (!IsOnScreen(Position + NameSize * 0.5f, static_cast<float>(NameSize.X))) continue;

			const FSlateRect Rect = MakeRect(Position, NameSize);
			if (!IsFree(Rect)) continue;

			Placed.Add(Rect);
			DrawText(Folders[Index].Name, Position, FontFolder, WithAlpha(TextColor, FolderOpenness[Index] * FolderAlpha[Index]), LayerLabels);
		}
	}

	struct FLabel
	{
		int32 Node = INDEX_NONE;
		FVector2D Center = FVector2D::ZeroVector;
		float Radius = 0.0f;
		int32 Priority = 0;
	};
	TMap<int32, FLabel> LabelsByNode;
	for (const auto& DrawnNode : DrawnNodes) {
		int32 Priority = 0;
		if (DrawnNode.Item == Selected) {
			Priority = 4;
		}
		else if (DrawnNode.Item == Hovered) {
			Priority = 3;
		}
		else if (bFocusActive && Focus[DrawnNode.Item] == EFocus::Chain) {
			Priority = 2;
		}
		else if ((bFocusActive && Focus[DrawnNode.Item] != EFocus::None) || (bForceEmphasized && Emphasized[DrawnNode.Item])) {
			Priority = 1;
		}
		else if (GetDim(DrawnNode.Item) < 1.0f || DrawnNode.Radius < 12.0f) {
			continue;
		}

		FLabel& Label = LabelsByNode.FindOrAdd(DrawnNode.Item);
		if (Label.Node == INDEX_NONE || Priority >= Label.Priority) {
			Label = FLabel {DrawnNode.Item, DrawnNode.Center, DrawnNode.Radius, Priority};
		}
	}

	TArray<FLabel> Labels;
	LabelsByNode.GenerateValueArray(Labels);
	Labels.Sort([](const FLabel& A, const FLabel& B) {
		return A.Priority != B.Priority ? A.Priority > B.Priority : A.Radius > B.Radius;
	});

	int32 NumLabels = 0;
	for (const auto& Label : Labels) {
		if (NumLabels >= MaxLabels && Label.Priority == 0) break;

		const FSlateFontInfo& Font = Label.Priority >= 2 ? FontBold : FontRegular;
		const FString Name = SPjcItemUsage::GetItemName(*Nodes[Label.Node].Item);
		const FVector2D BlockSize = MeasureText(Name, Font);

		// below, above or right of circle, whichever is free
		const float Gap = Label.Radius + (Label.Node == Selected ? 12.0f : 3.0f);
		const FVector2D Spots[] = {
		  FVector2D {Label.Center.X - BlockSize.X * 0.5f,				  Label.Center.Y + Gap},
		  FVector2D {Label.Center.X - BlockSize.X * 0.5f,  Label.Center.Y - Gap - BlockSize.Y},
		  FVector2D {		 Label.Center.X + Gap + 2.0f, Label.Center.Y - BlockSize.Y * 0.5f},
		};

		int32 FreeSpot = INDEX_NONE;
		for (int32 Spot = 0; Spot < UE_ARRAY_COUNT(Spots) && FreeSpot == INDEX_NONE; ++Spot) {
			if (IsFree(MakeRect(Spots[Spot], BlockSize))) {
				FreeSpot = Spot;
			}
		}
		if (FreeSpot == INDEX_NONE && Label.Priority == 0) continue;

		const FVector2D Position = Spots[FreeSpot == INDEX_NONE ? 0 : FreeSpot];
		Placed.Add(MakeRect(Position, BlockSize));
		DrawText(Name, Position, Font, Label.Priority > 0 ? FLinearColor::White : TextColor, LayerLabels);
		++NumLabels;
	}

	// how to read and use it
	DrawText(
		TEXT("Scroll to zoom, drag to move. Click a folder to open it, an asset for details. F fits view, Esc clears selection."),
		FVector2D {10.0f, 8.0f},
		FontRegular,
		WithAlpha(TextColor, 0.45f),
		LayerPanelContent
	);

	{
		FPjcMesh LegendMesh {Transform};
		const FVector2D LegendPos {10.0f, ViewSize.Y - 64.0f};
		DrawPanel(LegendPos, FVector2D {440.0f, 56.0f}, PanelColor);

		float X = static_cast<float>(LegendPos.X) + 12.0f;
		const float Y = static_cast<float>(LegendPos.Y) + 14.0f;
		const auto AddEntry = [&](const FLinearColor& Color, const bool bRing, const TCHAR* Text) {
			LegendMesh.Disc(FVector2D {X, Y}, 5.0f, Color);
			if (bRing) {
				LegendMesh.Ring(FVector2D {X, Y}, 8.0f, 1.5f, WithAlpha(FLinearColor::White, 0.85f));
			}
			DrawText(Text, FVector2D {X + 12.0f, Y - 7.0f}, FontRegular, TextColor, LayerPanelContent);
			X += 24.0f + static_cast<float>(MeasureText(Text, FontRegular).X);
		};
		AddEntry(ColorUsed, false, TEXT("Used"));
		AddEntry(ColorUnused, false, TEXT("Unused"));
		AddEntry(ColorNotDeleted, false, TEXT("Not deleted"));
		AddEntry(ColorUsed, true, TEXT("Used for its own reason"));
		DrawText(
			TEXT("Circle area is size on disk. Red part of folder ring is unused share of its size."),
			FVector2D {LegendPos.X + 8.0f, LegendPos.Y + 30.0f},
			FontRegular,
			WithAlpha(TextColor, 0.7f),
			LayerPanelContent
		);
		LegendMesh.Draw(OutDrawElements, LayerPanelContent, Handle);
	}

	// hovered asset or folder card
	TArray<FLine> Card;
	FLinearColor CardTint = TextColor;
	if (Hovered != INDEX_NONE && Nodes.IsValidIndex(Hovered) && !bDragging) {
		const FPjcGraphNode& Node = Nodes[Hovered];
		const FPjcUsageItem& Item = *Node.Item;
		Card.Add(FLine {SPjcItemUsage::GetItemName(Item), &FontTitle, FLinearColor::White});
		Card.Add(FLine {FString::Printf(TEXT("%s, %s"), *Item.ClassName, *FText::AsMemory(Item.Size, IEC).ToString()), &FontRegular, TextColor});
		if (Node.NumOwned > 0) {
			Card.Add(
				FLine {
				  FString::Printf(TEXT("Only it uses %d asset(s), %s with them"), Node.NumOwned, *FText::AsMemory(Node.SizeOwned, IEC).ToString()),
				  &FontRegular,
				  TextColor
				}
			);
		}
		Card.Add(FLine {SPjcItemUsage::GetStatusText(Item).ToString() + TEXT(": ") + Item.ReasonText, &FontRegular, TextColor});
		CardTint = SPjcItemUsage::GetStatusColor(Item);
	}
	else if (HoveredFolder != INDEX_NONE && Folders.IsValidIndex(HoveredFolder) && !bDragging) {
		const FPjcGraphFolder& Folder = Folders[HoveredFolder];
		const int32 NumUsed = Folder.NumAssets - Folder.NumUnused;
		Card.Add(FLine {Folder.Name, &FontTitle, FLinearColor::White});
		Card.Add(FLine {Folder.Path, &FontRegular, TextColor});
		Card.Add(
			FLine {FString::Printf(TEXT("%d assets, %s"), Folder.NumAssets, *FText::AsMemory(Folder.Size, IEC).ToString()), &FontRegular, TextColor}
		);
		Card.Add(
			FLine {
			  FString::Printf(TEXT("Used: %d, %s"), NumUsed, *FText::AsMemory(Folder.Size - Folder.SizeUnused, IEC).ToString()),
			  &FontRegular,
			  ColorUsed
			}
		);
		Card.Add(
			FLine {
			  FString::Printf(TEXT("Unused: %d, %s"), Folder.NumUnused, *FText::AsMemory(Folder.SizeUnused, IEC).ToString()),
			  &FontRegular,
			  ColorUnused
			}
		);
		if (Folder.NumNotDeleted > 0) {
			Card.Add(FLine {FString::Printf(TEXT("Not deleted by last cleanup: %d"), Folder.NumNotDeleted), &FontRegular, ColorNotDeleted});
		}
		Card.Add(FLine {TEXT("Arrows: what it uses outside. Click or scroll to open"), &FontRegular, WithAlpha(TextColor, 0.7f)});
		CardTint = FolderColor;
	}

	if (Card.Num() > 0) {
		FVector2D CardSize {0.0f, 0.0f};
		for (const auto& Line : Card) {
			const FVector2D LineSize = MeasureText(Line.Text, *Line.Font);
			CardSize.X = FMath::Max(CardSize.X, LineSize.X);
			CardSize.Y += LineSize.Y + 2.0f;
		}
		CardSize += FVector2D {20.0f, 14.0f};

		FVector2D CardPos = MouseLocal + FVector2D {16.0f, 18.0f};
		CardPos.X = FMath::Min(CardPos.X, ViewSize.X - CardSize.X - 4.0f);
		CardPos.Y = CardPos.Y + CardSize.Y > ViewSize.Y - 4.0f ? MouseLocal.Y - CardSize.Y - 8.0f : CardPos.Y;
		DrawPanel(CardPos - FVector2D {1.0f, 1.0f}, CardSize + FVector2D {2.0f, 2.0f}, WithAlpha(CardTint, 0.8f));
		DrawPanel(CardPos, CardSize, FLinearColor {0.012f, 0.013f, 0.016f, 0.97f});

		float LineY = static_cast<float>(CardPos.Y) + 7.0f;
		for (const auto& Line : Card) {
			DrawText(Line.Text, FVector2D {CardPos.X + 10.0f, LineY}, *Line.Font, Line.Color, LayerPanelContent);
			LineY += static_cast<float>(MeasureText(Line.Text, *Line.Font).Y) + 2.0f;
		}
	}

	return LayerPanelContent + 1;
}

FVector2D SPjcAssetGraph::ComputeDesiredSize(float LayoutScaleMultiplier) const {
	return FVector2D {400.0f, 300.0f};
}

bool SPjcAssetGraph::SupportsKeyboardFocus() const {
	return true;
}

FReply SPjcAssetGraph::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) {
	const FKey Button = MouseEvent.GetEffectingButton();
	if (Button != EKeys::LeftMouseButton && Button != EKeys::RightMouseButton && Button != EKeys::MiddleMouseButton) {
		return FReply::Unhandled();
	}

	MouseDownLocal = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	bDragging = false;

	return FReply::Handled().CaptureMouse(SharedThis(this)).SetUserFocus(SharedThis(this), EFocusCause::Mouse);
}

FReply SPjcAssetGraph::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) {
	if (!HasMouseCapture()) return FReply::Unhandled();

	// click: asset selects it, closed folder opens, empty space clears selection
	if (!bDragging && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) {
		const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		const int32 Node = FindNodeAt(Local);
		const int32 Folder = Node == INDEX_NONE ? FindFolderAt(Local) : INDEX_NONE;
		if (Folder != INDEX_NONE) {
			OpenFolder(Folder);
		}
		else {
			SelectNode(Node, true);
		}
	}
	bDragging = false;

	return FReply::Handled().ReleaseMouseCapture();
}

FReply SPjcAssetGraph::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) {
	MouseLocal = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

	if (HasMouseCapture()) {
		if (!bDragging && FVector2D::Distance(MouseLocal, MouseDownLocal) > 4.0f) {
			bDragging = true;
			Hovered = INDEX_NONE;
			HoveredFolder = INDEX_NONE;
		}
		if (bDragging) {
			const FVector2D CursorDelta = MouseEvent.GetCursorDelta();
			ViewCenter -= CursorDelta / MyGeometry.Scale / Zoom;
			bViewMoving = false;
			bZoomAnchored = false;
			bFitPending = false;
			bFocusFitPending = false;
			bKeepSelectedPending = false;
		}
		return FReply::Handled();
	}

	UpdateHovered();
	return FReply::Unhandled();
}

FReply SPjcAssetGraph::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) {
	if (!Graph.IsValid()) return FReply::Unhandled();

	const float From = bViewMoving ? ZoomTarget : Zoom;
	ZoomTarget = FMath::Clamp(From * FMath::Pow(1.25f, MouseEvent.GetWheelDelta()), GetMinZoom(), MaxZoom);
	AnchorLocal = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	AnchorGraph = LocalToGraph(AnchorLocal);
	bZoomAnchored = true;
	bViewMoving = true;
	bFitPending = false;
	bFocusFitPending = false;
	bKeepSelectedPending = false;
	StartTimer();

	return FReply::Handled();
}

FReply SPjcAssetGraph::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) {
	if (!Graph.IsValid() || MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();

	// asset opens its editor, closed folder opens, empty space fits view
	const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const int32 Node = FindNodeAt(Local);
	if (Node != INDEX_NONE) {
		OnAssetDoubleClicked.ExecuteIfBound(Graph->GetNodes()[Node].Item->PackageName);
		return FReply::Handled();
	}

	const int32 Folder = FindFolderAt(Local);
	if (Folder != INDEX_NONE) {
		OpenFolder(Folder);
	}
	else {
		ZoomToFit();
	}
	return FReply::Handled();
}

void SPjcAssetGraph::OnMouseLeave(const FPointerEvent& MouseEvent) {
	SLeafWidget::OnMouseLeave(MouseEvent);

	Hovered = INDEX_NONE;
	HoveredFolder = INDEX_NONE;
}

FReply SPjcAssetGraph::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) {
	if (InKeyEvent.GetKey() == EKeys::Escape && Selected != INDEX_NONE) {
		SelectNode(INDEX_NONE, true);
		return FReply::Handled();
	}
	if (InKeyEvent.GetKey() == EKeys::F || InKeyEvent.GetKey() == EKeys::Home) {
		ZoomToFit();
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FCursorReply SPjcAssetGraph::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const {
	if (bDragging) return FCursorReply::Cursor(EMouseCursor::GrabHandClosed);
	if (Hovered != INDEX_NONE || HoveredFolder != INDEX_NONE) return FCursorReply::Cursor(EMouseCursor::Hand);

	return FCursorReply::Unhandled();
}

EActiveTimerReturnType SPjcAssetGraph::OnTimer(double InCurrentTime, float InDeltaTime) {
	bool bContinue = false;

	// new graph is fitted once view knows its size
	if (bFitPending && Graph.IsValid()) {
		if (ViewSize.X > 1.0f && ViewSize.Y > 1.0f) {
			const FBox2D Bounds = Graph->GetBounds();
			if (Bounds.bIsValid) {
				ViewCenter = ViewCenterTarget = Bounds.GetCenter();
				Zoom = ZoomTarget = GetFitZoom(Bounds);
			}
			bFitPending = false;
		}
		else {
			bContinue = true;
		}
	}

	if (bFocusFitPending) {
		if (NumPaints > FocusFitPaint) {
			bFocusFitPending = false;
			FitFocus();
		}
		bContinue = true;
	}

	if (bKeepSelectedPending) {
		if (NumPaints > FocusFitPaint) {
			bKeepSelectedPending = false;
			if (Graph.IsValid() && Graph->GetNodes().IsValidIndex(Selected)) {
				const FVector2D Position = Graph->GetNodes()[Selected].Position;
				const FVector2D Local = GraphToLocal(Position);
				if (Local.X < 60.0f || Local.Y < 60.0f || Local.X > ViewSize.X - 60.0f || Local.Y > ViewSize.Y - 60.0f) {
					MoveView(Position, bViewMoving ? ZoomTarget : Zoom);
				}
			}
		}
		bContinue = true;
	}

	if (bViewMoving) {
		const float Blend = 1.0f - FMath::Exp(-InDeltaTime * 12.0f);
		Zoom = FMath::Exp(FMath::Lerp(FMath::Loge(Zoom), FMath::Loge(ZoomTarget), Blend));
		if (!bZoomAnchored) {
			ViewCenter = FMath::Lerp(ViewCenter, ViewCenterTarget, Blend);
		}

		const bool bZoomDone = FMath::Abs(FMath::Loge(Zoom / ZoomTarget)) < 0.002f;
		const bool bCenterDone = bZoomAnchored || FVector2D::Distance(ViewCenter, ViewCenterTarget) * Zoom < 0.5;
		if (bZoomDone && bCenterDone) {
			Zoom = ZoomTarget;
			ViewCenter = bZoomAnchored ? ViewCenter : ViewCenterTarget;
			bViewMoving = false;
		}
		if (bZoomAnchored) {
			ViewCenter = AnchorGraph - (AnchorLocal - ViewSize * 0.5f) / Zoom;
			bZoomAnchored = bViewMoving;
		}
		bContinue = bContinue || bViewMoving;
	}

	return bContinue ? EActiveTimerReturnType::Continue : EActiveTimerReturnType::Stop;
}

void SPjcAssetGraph::StartTimer() {
	if (TimerHandle.IsValid()) return;

	TimerHandle = RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateSP(this, &SPjcAssetGraph::OnTimer));
}

void SPjcAssetGraph::SelectNode(const int32 Node, const bool bNotify) {
	Selected = Graph.IsValid() && Graph->GetNodes().IsValidIndex(Node) ? Node : INDEX_NONE;
	UpdateFocus();

	if (bNotify) {
		bKeepSelectedPending = Selected != INDEX_NONE;
		FocusFitPaint = NumPaints;
		StartTimer();
		OnSelectionChanged.ExecuteIfBound(Selected != INDEX_NONE ? Graph->GetNodes()[Selected].Item->PackageName : FName {NAME_None});
	}
}

void SPjcAssetGraph::UpdateFocus() {
	FocusLinks.Reset();
	if (!Graph.IsValid()) return;

	const TArray<FPjcGraphNode>& Nodes = Graph->GetNodes();
	Focus.Init(EFocus::None, Nodes.Num());
	if (Selected == INDEX_NONE) return;

	Focus[Selected] = EFocus::Selected;

	// used asset: chain up to asset used for its own reason. Unused asset: assets it is deleted with
	const FPjcGraphNode& SelectedNode = Nodes[Selected];
	int32 Step = Selected;
	for (int32 Guard = 0; Guard < 64; ++Guard) {
		const FPjcGraphNode& StepNode = Nodes[Step];
		const int32 Next = SelectedNode.Item->Usage.bUsed ? Graph->Find(StepNode.Item->Usage.Parent) : StepNode.Owner;
		if (Next == INDEX_NONE || Focus[Next] != EFocus::None) break;

		Focus[Next] = EFocus::Chain;
		if (SelectedNode.Item->Usage.bUsed) {
			FocusLinks.Add(FFocusLink {Next, Step, true});
		}
		Step = Next;
	}

	for (const int32 Use : SelectedNode.Uses) {
		Focus[Use] = Focus[Use] == EFocus::None ? EFocus::Neighbor : Focus[Use];
		FocusLinks.Add(FFocusLink {Selected, Use, false});
	}
	for (const int32 User : SelectedNode.UsedBy) {
		const bool bChainLink = FocusLinks.ContainsByPredicate([&](const FFocusLink& Link) {
			return Link.bChain && Link.From == User && Link.To == Selected;
		});
		Focus[User] = Focus[User] == EFocus::None ? EFocus::Neighbor : Focus[User];
		if (!bChainLink) {
			FocusLinks.Add(FFocusLink {User, Selected, false});
		}
	}
}

void SPjcAssetGraph::FitFocus() {
	if (!Graph.IsValid() || Selected == INDEX_NONE) return;

	// selected asset with its chain and links, they are drawn even inside closed folders
	FBox2D Box {ForceInit};
	const TArray<FPjcGraphNode>& Nodes = Graph->GetNodes();
	for (int32 Index = 0; Index < Nodes.Num(); ++Index) {
		if (Focus[Index] == EFocus::None) continue;

		Box += Nodes[Index].Position - FVector2D {Nodes[Index].Radius, Nodes[Index].Radius};
		Box += Nodes[Index].Position + FVector2D {Nodes[Index].Radius, Nodes[Index].Radius};
	}
	MoveView(Box.GetCenter(), FMath::Min(GetFitZoom(Box), 2.0f));
}

void SPjcAssetGraph::OpenFolder(const int32 Folder) {
	if (!Graph.IsValid() || !Graph->GetFolders().IsValidIndex(Folder)) return;

	const FPjcGraphFolder& GraphFolder = Graph->GetFolders()[Folder];
	const FVector2D Extent {GraphFolder.Radius, GraphFolder.Radius};
	const float FitZoom = GetFitZoom(FBox2D {GraphFolder.Center - Extent, GraphFolder.Center + Extent});
	MoveView(GraphFolder.Center, FMath::Max(FitZoom, (FolderOpenPixels + FolderFadePixels) * 1.15f / GraphFolder.Radius));
}

void SPjcAssetGraph::MoveView(const FVector2D& Center, const float InZoom) {
	ViewCenterTarget = Center;
	ZoomTarget = FMath::Clamp(InZoom, GetMinZoom(), MaxZoom);
	bViewMoving = true;
	bZoomAnchored = false;
	bFitPending = false;
	StartTimer();
}

void SPjcAssetGraph::UpdateHovered() {
	Hovered = FindNodeAt(MouseLocal);
	HoveredFolder = Hovered == INDEX_NONE ? FindFolderAt(MouseLocal) : INDEX_NONE;
}

int32 SPjcAssetGraph::FindNodeAt(const FVector2D& LocalPosition) const {
	for (int32 Index = DrawnNodes.Num() - 1; Index >= 0; --Index) {
		if (FVector2D::DistSquared(DrawnNodes[Index].Center, LocalPosition) <= FMath::Square(DrawnNodes[Index].Radius)) {
			return DrawnNodes[Index].Item;
		}
	}
	return INDEX_NONE;
}

int32 SPjcAssetGraph::FindFolderAt(const FVector2D& LocalPosition) const {
	for (int32 Index = DrawnFolders.Num() - 1; Index >= 0; --Index) {
		if (FVector2D::DistSquared(DrawnFolders[Index].Center, LocalPosition) <= FMath::Square(DrawnFolders[Index].Radius)) {
			return DrawnFolders[Index].Item;
		}
	}
	return INDEX_NONE;
}

FVector2D SPjcAssetGraph::GraphToLocal(const FVector2D& GraphPosition) const {
	return (GraphPosition - ViewCenter) * Zoom + ViewSize * 0.5f;
}

FVector2D SPjcAssetGraph::LocalToGraph(const FVector2D& LocalPosition) const {
	return (LocalPosition - ViewSize * 0.5f) / Zoom + ViewCenter;
}

float SPjcAssetGraph::GetFitZoom(const FBox2D& Box) const {
	// room for labels at sides, hint above and legend below
	const FVector2D Size = Box.GetSize();
	const float ZoomX = FMath::Max(static_cast<float>(ViewSize.X) - 200.0f, 100.0f) / FMath::Max(static_cast<float>(Size.X), 1.0f);
	const float ZoomY = FMath::Max(static_cast<float>(ViewSize.Y) - 220.0f, 100.0f) / FMath::Max(static_cast<float>(Size.Y), 1.0f);
	return FMath::Clamp(FMath::Min(ZoomX, ZoomY), 0.0001f, MaxZoom);
}

float SPjcAssetGraph::GetMinZoom() const {
	if (!Graph.IsValid()) return 0.0001f;

	const FBox2D Bounds = Graph->GetBounds();
	return Bounds.bIsValid ? GetFitZoom(Bounds) * 0.25f : 0.0001f;
}

int32 SPjcAssetGraph::GetShown(const int32 Node) const {
	const int32 Folder = Graph->GetNodes()[Node].Folder;
	const int32 Shown = FolderShown.IsValidIndex(Folder) ? FolderShown[Folder] : INDEX_NONE;
	return Shown == INDEX_NONE ? Node : Graph->GetNodes().Num() + Shown;
}

float SPjcAssetGraph::GetShownRadius(const int32 Shown) const {
	const TArray<FPjcGraphNode>& Nodes = Graph->GetNodes();
	return Shown < Nodes.Num() ? Nodes[Shown].Radius : Graph->GetFolders()[Shown - Nodes.Num()].Radius;
}

FVector2D SPjcAssetGraph::GetShownCenter(const int32 Shown) const {
	const TArray<FPjcGraphNode>& Nodes = Graph->GetNodes();
	return Shown < Nodes.Num() ? Nodes[Shown].Position : Graph->GetFolders()[Shown - Nodes.Num()].Center;
}

float SPjcAssetGraph::GetDim(const int32 Node) const {
	if (Selected != INDEX_NONE) return Focus[Node] != EFocus::None ? 1.0f : 0.1f;
	if (bEmphasisActive) return Emphasized[Node] ? 1.0f : 0.15f;

	return 1.0f;
}

bool SPjcAssetGraph::IsInsideFolder(const int32 Node, const int32 Folder) const {
	const TArray<FPjcGraphFolder>& Folders = Graph->GetFolders();
	for (int32 Step = Graph->GetNodes()[Node].Folder; Step != INDEX_NONE; Step = Folders[Step].Parent) {
		if (Step == Folder) return true;
	}
	return false;
}
