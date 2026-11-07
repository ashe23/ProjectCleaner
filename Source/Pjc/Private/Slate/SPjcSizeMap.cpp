// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcSizeMap.h"
#include "PjcFormat.h"
#include "PjcShim.h"
#include "PjcStyles.h"
// Engine Headers
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
	// unity builds join files: names unique to this file
	constexpr int32 PjcMapMaxItems = 150;
	constexpr int32 PjcMapMaxDepth = 9;
	constexpr float PjcMapHeader = 18.0f;

	// Rectangle of the treemap layout
	struct FPjcMapRect
	{
		double X = 0.0;
		double Y = 0.0;
		double W = 0.0;
		double H = 0.0;
	};

	FLinearColor PjcMapColor(const TCHAR* Hex) {
		return FLinearColor {FColor::FromHex(Hex)};
	}

	// Squarified treemap: rectangles for values sorted biggest first, as square as the area allows
	void PjcMapSquarify(const TArray<double>& Values, double X, double Y, double W, double H, TArray<FPjcMapRect>& Out) {
		Out.SetNumZeroed(Values.Num());
		double Total = 0.0;
		for (const double Value : Values) {
			Total += Value;
		}
		if (Total <= 0.0 || W <= 0.0 || H <= 0.0) return;

		TArray<double> Areas;
		Areas.Reserve(Values.Num());
		for (const double Value : Values) {
			Areas.Add(Value * W * H / Total);
		}

		const auto Worst = [&Areas](const int32 First, const int32 Last, const double Side) {
			double Sum = 0.0;
			double Max = 0.0;
			double Min = TNumericLimits<double>::Max();
			for (int32 Index = First; Index <= Last; ++Index) {
				Sum += Areas[Index];
				Max = FMath::Max(Max, Areas[Index]);
				Min = FMath::Min(Min, Areas[Index]);
			}
			if (Sum <= 0.0 || Min <= 0.0) return TNumericLimits<double>::Max();
			return FMath::Max(Side * Side * Max / (Sum * Sum), (Sum * Sum) / (Side * Side * Min));
		};

		int32 First = 0;
		while (First < Areas.Num()) {
			const double Side = FMath::Min(W, H);
			int32 Last = First;
			double Best = Worst(First, Last, Side);
			while (Last + 1 < Areas.Num()) {
				const double Next = Worst(First, Last + 1, Side);
				if (Next > Best) break;
				Best = Next;
				++Last;
			}

			double Sum = 0.0;
			for (int32 Index = First; Index <= Last; ++Index) {
				Sum += Areas[Index];
			}
			if (W >= H) {
				const double ColumnWidth = H > 0.0 ? Sum / H : 0.0;
				double Top = Y;
				for (int32 Index = First; Index <= Last; ++Index) {
					const double CellHeight = ColumnWidth > 0.0 ? Areas[Index] / ColumnWidth : 0.0;
					Out[Index] = FPjcMapRect {X, Top, ColumnWidth, CellHeight};
					Top += CellHeight;
				}
				X += ColumnWidth;
				W -= ColumnWidth;
			}
			else {
				const double RowHeight = W > 0.0 ? Sum / W : 0.0;
				double Left = X;
				for (int32 Index = First; Index <= Last; ++Index) {
					const double CellWidth = RowHeight > 0.0 ? Areas[Index] / RowHeight : 0.0;
					Out[Index] = FPjcMapRect {Left, Y, CellWidth, RowHeight};
					Left += CellWidth;
				}
				Y += RowHeight;
				H -= RowHeight;
			}
			First = Last + 1;
		}
	}
}	// namespace

void SPjcSizeMap::Construct(const FArguments& InArgs) {
	OnFolderClicked = InArgs._OnFolderClicked;
	OnAssetClicked = InArgs._OnAssetClicked;
	SetToolTipText(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SPjcSizeMap::GetHoverText)));
}

void SPjcSizeMap::SetFolder(const FPjcScanResult* InResult, const TSharedPtr<FPjcUnusedFolder>& InFolder, const bool bInAreaAll) {
	Result = InResult;
	Folder = InFolder;
	bAreaAll = bInAreaAll;
	bLayoutDirty = true;
	Hovered = INDEX_NONE;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SPjcSizeMap::SetSelected(const TSet<int32>& InSelected) {
	Selected = InSelected;
	Invalidate(EInvalidateWidgetReason::Paint);
}

FVector2D SPjcSizeMap::ComputeDesiredSize(float LayoutScaleMultiplier) const {
	return FVector2D {200.0f, 200.0f};
}

void SPjcSizeMap::GetItems(const FPjcUnusedFolder& InFolder, TArray<FItem>& OutItems) const {
	OutItems.Reset();
	for (const auto& Child : InFolder.Children) {
		const int64 Value = bAreaAll ? Child->All.Size : Child->Unused.Size;
		if (Value > 0) {
			FItem& Item = OutItems.AddDefaulted_GetRef();
			Item.Kind = ECellKind::Folder;
			Item.Folder = Child;
			Item.Value = Value;
		}
	}
	for (const int32 Index : InFolder.Assets) {
		const FName Package = Result->Assets[Index].PackageName;
		const bool bUnused = Result->GetStatus(Package) == EPjcAssetStatus::Unused;
		const int64 Value = Result->GetSize(Package);
		if ((!bAreaAll && !bUnused) || Value <= 0) continue;

		FItem& Item = OutItems.AddDefaulted_GetRef();
		Item.Kind = ECellKind::Asset;
		Item.Asset = Index;
		Item.bUnused = bUnused;
		Item.Value = Value;
	}
	OutItems.Sort([](const FItem& A, const FItem& B) {
		return A.Value > B.Value;
	});

	// the smallest share one box
	if (OutItems.Num() > PjcMapMaxItems) {
		FItem More;
		More.Kind = ECellKind::More;
		for (int32 Index = PjcMapMaxItems; Index < OutItems.Num(); ++Index) {
			More.Value += OutItems[Index].Value;
			++More.NumMore;
		}
		OutItems.SetNum(PjcMapMaxItems);
		OutItems.Add(More);
	}
}

void SPjcSizeMap::LayoutFolder(const FPjcUnusedFolder& InFolder, const FVector2D& Position, const FVector2D& Size, const int32 Depth) const {
	TArray<FItem> Items;
	GetItems(InFolder, Items);

	TArray<double> Values;
	for (const auto& Item : Items) {
		Values.Add(static_cast<double>(Item.Value));
	}
	TArray<FPjcMapRect> Rects;
	PjcMapSquarify(Values, Position.X, Position.Y, Size.X, Size.Y, Rects);

	for (int32 Index = 0; Index < Items.Num(); ++Index) {
		const FVector2D CellPosition {static_cast<float>(Rects[Index].X), static_cast<float>(Rects[Index].Y)};
		const FVector2D CellSize {static_cast<float>(Rects[Index].W), static_cast<float>(Rects[Index].H)};
		if (CellSize.X < 2.0f || CellSize.Y < 2.0f) continue;

		const FItem& Item = Items[Index];
		FCell& Cell = Cells.AddDefaulted_GetRef();
		Cell.Position = CellPosition;
		Cell.Size = CellSize;
		Cell.Kind = Item.Kind;
		Cell.Depth = Depth;
		Cell.Asset = Item.Asset;
		Cell.bUnused = Item.bUnused;
		Cell.Value = Item.Value;
		Cell.NumMore = Item.NumMore;
		if (Item.Kind != ECellKind::Folder) continue;

		// a folder whose only content is one folder is one box
		TSharedPtr<FPjcUnusedFolder> Shown = Item.Folder;
		FString Label = Shown->Name;
		TArray<FItem> Inner;
		for (GetItems(*Shown, Inner); Inner.Num() == 1 && Inner[0].Kind == ECellKind::Folder; GetItems(*Shown, Inner)) {
			Shown = Inner[0].Folder;
			Label += TEXT(" / ") + Shown->Name;
		}
		Cell.Folder = Shown;
		Cell.Label = Label;

		if (Depth < PjcMapMaxDepth && CellSize.X > 24.0f && CellSize.Y > 24.0f) {
			const float Top = CellSize.X > 46.0f && CellSize.Y > 22.0f ? PjcMapHeader + 1.0f : 2.0f;
			LayoutFolder(*Shown, CellPosition + FVector2D {2.0f, Top}, CellSize - FVector2D {4.0f, Top + 2.0f}, Depth + 1);
		}
	}
}

void SPjcSizeMap::UpdateLayout(const FVector2D& Size) const {
	if (!bLayoutDirty && LayoutSize.Equals(Size, 0.5f)) return;

	Cells.Reset();
	LayoutSize = Size;
	bLayoutDirty = false;
	if (Result && Folder.IsValid()) {
		LayoutFolder(*Folder, FVector2D::ZeroVector, Size, 0);
	}
}

int32 SPjcSizeMap::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled
) const {
	UpdateLayout(FVector2D {AllottedGeometry.GetLocalSize()});

	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FSlateFontInfo Font = FPjcStyles::GetFont("Regular", 8);
	const FSlateFontInfo FontBold = FPjcStyles::GetFont("Bold", 8);
	const FSlateFontInfo FontSmall = FCoreStyle::GetDefaultFontStyle("Mono", 7);

	const auto Box = [&](const int32 Layer, const FVector2D& Position, const FVector2D& Size, const FLinearColor& Color) {
		FSlateDrawElement::MakeBox(
			OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform {Position}), White, ESlateDrawEffect::None, Color
		);
	};
	const auto Text = [&](const int32 Layer,
						  const FVector2D& Position,
						  const float MaxWidth,
						  const FString& String,
						  const FSlateFontInfo& TextFont,
						  const FLinearColor& Color) {
		if (MaxWidth <= 4.0f || String.IsEmpty()) return;

		// cut to the width
		FString Shown = String;
		if (FontMeasure->Measure(Shown, TextFont).X > MaxWidth) {
			const int32 Offset = FMath::Max(1, FMath::FloorToInt(MaxWidth - 6.0f));
			const int32 Fits = FontMeasure->FindLastWholeCharacterIndexBeforeOffset(Shown, TextFont, Offset);
			Shown = Fits > 0 ? Shown.Left(Fits) + TEXT("\u2026") : FString {};
		}
		const FVector2D Size {FontMeasure->Measure(Shown, TextFont)};
		FSlateDrawElement::MakeText(
			OutDrawElements,
			Layer,
			AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform {Position}),
			Shown,
			TextFont,
			ESlateDrawEffect::None,
			Color
		);
	};
	const auto Outline = [&](const int32 Layer, const FVector2D& Position, const FVector2D& Size, const float Width, const FLinearColor& Color) {
		Box(Layer, Position, FVector2D {Size.X, Width}, Color);
		Box(Layer, Position + FVector2D {0.0f, Size.Y - Width}, FVector2D {Size.X, Width}, Color);
		Box(Layer, Position, FVector2D {Width, Size.Y}, Color);
		Box(Layer, Position + FVector2D {Size.X - Width, 0.0f}, FVector2D {Width, Size.Y}, Color);
	};

	const FLinearColor FolderFill = PjcMapColor(TEXT("#24272B"));
	const FLinearColor FolderLine = PjcMapColor(TEXT("#3C4148"));
	const FLinearColor FolderTint = PjcMapColor(TEXT("#8E3A3F"));
	const FLinearColor HeaderFill = PjcMapColor(TEXT("#2B2F34"));
	const FLinearColor UnusedFill = PjcMapColor(TEXT("#7A3236"));
	const FLinearColor UsedFill = PjcMapColor(TEXT("#2A3B3A"));
	const FLinearColor Muted = PjcMapColor(TEXT("#8A9099"));

	for (int32 Index = 0; Index < Cells.Num(); ++Index) {
		const FCell& Cell = Cells[Index];
		const int32 Layer = LayerId + Cell.Depth * 3;
		const bool bHovered = Index == Hovered;

		if (Cell.Kind == ECellKind::Folder) {
			const double FolderSize = static_cast<double>(Cell.Folder->All.Size);
			const float Share = FolderSize > 0.0 ? static_cast<float>(static_cast<double>(Cell.Folder->Unused.Size) / FolderSize) : 0.0f;
			const FLinearColor Fill = bAreaAll ? FLinearColor::LerpUsingHSV(FolderFill, FolderTint, Share) : FolderFill;
			Box(Layer, Cell.Position, Cell.Size, bHovered ? PjcMapColor(TEXT("#6A7078")) : FolderLine);
			Box(Layer, Cell.Position + FVector2D {1.0f, 1.0f}, Cell.Size - FVector2D {2.0f, 2.0f}, Fill);
			if (Cell.Size.X > 46.0f && Cell.Size.Y > 22.0f) {
				Box(Layer + 1, Cell.Position + FVector2D {1.0f, 1.0f}, FVector2D {Cell.Size.X - 2.0f, PjcMapHeader}, HeaderFill);
				const FString Value = PjcFormat::Size(Cell.Value);
				const float ValueWidth = FontMeasure->Measure(Value, FontSmall).X;
				const float NameWidth = FMath::Min(FontMeasure->Measure(Cell.Label, FontBold).X, Cell.Size.X - ValueWidth - 22.0f);
				Text(Layer + 2, Cell.Position + FVector2D {6.0f, 2.0f}, NameWidth + 1.0f, Cell.Label, FontBold, FLinearColor {0.85f, 0.86f, 0.88f});
				if (NameWidth + ValueWidth + 22.0f < Cell.Size.X) {
					Text(Layer + 2, Cell.Position + FVector2D {12.0f + NameWidth, 4.0f}, ValueWidth + 1.0f, Value, FontSmall, Muted);
				}
			}
			continue;
		}

		if (Cell.Kind == ECellKind::More) {
			Box(Layer, Cell.Position, Cell.Size, FolderLine);
			Box(Layer, Cell.Position + FVector2D {1.0f, 1.0f}, Cell.Size - FVector2D {2.0f, 2.0f}, PjcMapColor(TEXT("#2A2D31")));
			if (Cell.Size.X > 60.0f && Cell.Size.Y > 30.0f) {
				Text(Layer + 2, Cell.Position + FVector2D {5.0f, 3.0f}, Cell.Size.X - 9.0f, TEXT("+") + PjcFormat::Count(Cell.NumMore), Font, Muted);
				Text(Layer + 2, Cell.Position + FVector2D {5.0f, 17.0f}, Cell.Size.X - 9.0f, PjcFormat::Size(Cell.Value), FontSmall, Muted);
			}
			continue;
		}

		FLinearColor Fill = Cell.bUnused ? UnusedFill : UsedFill;
		if (bHovered) {
			Fill = Fill * 1.25f;
			Fill.A = 1.0f;
		}
		Box(Layer, Cell.Position, Cell.Size, FLinearColor {0.0f, 0.0f, 0.0f, 0.45f});
		Box(Layer, Cell.Position + FVector2D {1.0f, 1.0f}, Cell.Size - FVector2D {2.0f, 2.0f}, Fill);
		if (Selected.Contains(Cell.Asset)) {
			Outline(Layer + 1, Cell.Position, Cell.Size, 2.0f, FLinearColor::White);
		}
		if (Cell.Size.X > 54.0f && Cell.Size.Y > 30.0f) {
			const FLinearColor Label = Cell.bUnused ? PjcMapColor(TEXT("#FFE1E2")) : PjcMapColor(TEXT("#BFE3DF"));
			Text(Layer + 2, Cell.Position + FVector2D {5.0f, 3.0f}, Cell.Size.X - 9.0f, Result->Assets[Cell.Asset].AssetName.ToString(), Font, Label);
			Text(Layer + 2, Cell.Position + FVector2D {5.0f, 17.0f}, Cell.Size.X - 9.0f, PjcFormat::Size(Cell.Value), FontSmall, Label * 0.85f);
		}
	}

	return LayerId + (PjcMapMaxDepth + 2) * 3;
}

int32 SPjcSizeMap::FindCell(const FVector2D& LocalPosition) const {
	// topmost: children come after their folder
	for (int32 Index = Cells.Num() - 1; Index >= 0; --Index) {
		const FCell& Cell = Cells[Index];
		const auto CellEnd = Cell.Position + Cell.Size;
		if (LocalPosition.X >= Cell.Position.X && LocalPosition.Y >= Cell.Position.Y && LocalPosition.X < CellEnd.X && LocalPosition.Y < CellEnd.Y) {
			return Index;
		}
	}
	return INDEX_NONE;
}

FReply SPjcSizeMap::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) {
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();

	Pressed = FindCell(FVector2D {MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition())});
	return FReply::Handled();
}

FReply SPjcSizeMap::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) {
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();

	const int32 Index = FindCell(FVector2D {MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition())});
	if (Index == INDEX_NONE || Index != Pressed) return FReply::Handled();

	const FCell& Cell = Cells[Index];
	if (Cell.Kind == ECellKind::Folder) {
		OnFolderClicked.ExecuteIfBound(Cell.Folder->Path);
	}
	else if (Cell.Kind == ECellKind::Asset && Cell.bUnused) {
		OnAssetClicked.ExecuteIfBound(Cell.Asset);
	}
	return FReply::Handled();
}

FReply SPjcSizeMap::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) {
	const int32 Index = FindCell(FVector2D {MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition())});
	if (Index != Hovered) {
		Hovered = Index;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
	return FReply::Unhandled();
}

void SPjcSizeMap::OnMouseLeave(const FPointerEvent& MouseEvent) {
	Hovered = INDEX_NONE;
	Invalidate(EInvalidateWidgetReason::Paint);
}

FText SPjcSizeMap::GetHoverText() const {
	if (!Cells.IsValidIndex(Hovered) || !Result) return FText::GetEmpty();

	const FCell& Cell = Cells[Hovered];
	if (Cell.Kind == ECellKind::Folder) {
		const FPjcUnusedFolder& Shown = *Cell.Folder;
		return FText::FromString(
			FString::Printf(
				TEXT("%s\nUnused %s of %s \u00B7 %s of %s assets\nClick to open"),
				*Shown.Path,
				*PjcFormat::Size(Shown.Unused.Size),
				*PjcFormat::Size(Shown.All.Size),
				*PjcFormat::Count(Shown.Unused.Num),
				*PjcFormat::Count(Shown.All.Num)
			)
		);
	}
	if (Cell.Kind == ECellKind::More) {
		return FText::FromString(FString::Printf(TEXT("%s smaller items \u00B7 %s"), *PjcFormat::Count(Cell.NumMore), *PjcFormat::Size(Cell.Value)));
	}

	const FAssetData& Asset = Result->Assets[Cell.Asset];
	return FText::FromString(
		FString::Printf(
			TEXT("%s\n%s \u00B7 %s%s"),
			*Asset.PackageName.ToString(),
			*PjcShim::GetAssetClassName(Asset).ToString(),
			*PjcFormat::Size(Cell.Value),
			Cell.bUnused ? TEXT(" \u00B7 unused, click to select it") : TEXT(" \u00B7 used, listed in the Content Browser")
		)
	);
}
