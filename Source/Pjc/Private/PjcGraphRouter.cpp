// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcGraphRouter.h"
// Engine Headers
#include "Algo/Reverse.h"

namespace
{
	// unity builds join files: names unique to this file

	// room around a box no line enters
	constexpr float PjcRoutePad = 4.0f;
	// costs of a step: one grid step is 1
	constexpr float PjcRouteTurn = 14.0f;
	constexpr float PjcRouteShare = 5.0f;
	constexpr float PjcRouteCross = 0.6f;
	constexpr float PjcRouteBox = 400.0f;

	// sides of a box, and the direction a line leaves through them
	constexpr int32 PjcRouteRight = 0;
	constexpr int32 PjcRouteLeft = 1;
	constexpr int32 PjcRouteBottom = 2;
	constexpr int32 PjcRouteTop = 3;
	const int32 PjcRouteStepX[4] = {1, -1, 0, 0};
	const int32 PjcRouteStepY[4] = {0, 0, 1, -1};

	struct FPjcRouteBox
	{
		float X = 0.0f;
		float Y = 0.0f;
	};

	// One end of a line on a side of a box
	struct FPjcRouteEnd
	{
		int32 Link = 0;
		bool bFrom = true;
		float Toward = 0.0f;
	};

	struct FPjcRouteLine
	{
		int32 Link = 0;
		int32 SideFrom = 0;
		int32 SideTo = 0;
		// on the side of the box, and the first grid point outside it
		FPjcRouteBox PointFrom;
		FPjcRouteBox StubFrom;
		FPjcRouteBox PointTo;
		FPjcRouteBox StubTo;
	};

	struct FPjcRouteOpen
	{
		float Cost = 0.0f;
		int32 State = 0;
	};

	float PjcRouteSnap(const float Value) {
		return FMath::RoundToFloat(Value / PjcGraphRouter::Grid) * PjcGraphRouter::Grid;
	}
}	// namespace

void PjcGraphRouter::Route(
	const TArray<FVector2D>& Boxes,
	const FVector2D& BoxSize,
	const TArray<FPjcRouteLink>& Links,
	TArray<TArray<FVector2D>>& OutRoutes
) {
	OutRoutes.Reset();
	OutRoutes.SetNum(Links.Num());
	if (Boxes.Num() == 0 || Links.Num() == 0) return;

	const float Width = static_cast<float>(BoxSize.X);
	const float Height = static_cast<float>(BoxSize.Y);

	// middles of the boxes and the grid around them all
	TArray<FPjcRouteBox> Middles;
	float MinX = TNumericLimits<float>::Max();
	float MinY = TNumericLimits<float>::Max();
	float MaxX = TNumericLimits<float>::Lowest();
	float MaxY = TNumericLimits<float>::Lowest();
	for (const FVector2D& Box : Boxes) {
		const float Left = static_cast<float>(Box.X);
		const float Top = static_cast<float>(Box.Y);
		Middles.Add(FPjcRouteBox {Left + Width * 0.5f, Top + Height * 0.5f});
		MinX = FMath::Min(MinX, Left);
		MinY = FMath::Min(MinY, Top);
		MaxX = FMath::Max(MaxX, Left + Width);
		MaxY = FMath::Max(MaxY, Top + Height);
	}
	const float X0 = PjcRouteSnap(MinX) - 8.0f * Grid;
	const float Y0 = PjcRouteSnap(MinY) - 8.0f * Grid;
	const int32 Cols = FMath::CeilToInt((MaxX - MinX) / Grid) + 17;
	const int32 Rows = FMath::CeilToInt((MaxY - MinY) / Grid) + 17;
	const int32 Num = Cols * Rows;

	TArray<bool> Blocked;
	Blocked.Init(false, Num);
	for (const FPjcRouteBox& Box : Middles) {
		const int32 C0 = FMath::Max(0, FMath::FloorToInt((Box.X - Width * 0.5f - PjcRoutePad - X0) / Grid));
		const int32 C1 = FMath::Min(Cols - 1, FMath::CeilToInt((Box.X + Width * 0.5f + PjcRoutePad - X0) / Grid));
		const int32 R0 = FMath::Max(0, FMath::FloorToInt((Box.Y - Height * 0.5f - PjcRoutePad - Y0) / Grid));
		const int32 R1 = FMath::Min(Rows - 1, FMath::CeilToInt((Box.Y + Height * 0.5f + PjcRoutePad - Y0) / Grid));
		for (int32 Row = R0; Row <= R1; ++Row) {
			for (int32 Col = C0; Col <= C1; ++Col) {
				Blocked[Row * Cols + Col] = true;
			}
		}
	}

	// sides: below, bottom to top. Above, top to bottom. In one row, side to side
	TArray<FPjcRouteLine> Lines;
	TMap<int32, TArray<FPjcRouteEnd>> Ends;
	for (int32 Index = 0; Index < Links.Num(); ++Index) {
		const FPjcRouteLink& Link = Links[Index];
		if (!Middles.IsValidIndex(Link.From) || !Middles.IsValidIndex(Link.To) || Link.From == Link.To) continue;

		const FPjcRouteBox& From = Middles[Link.From];
		const FPjcRouteBox& To = Middles[Link.To];
		FPjcRouteLine Line;
		Line.Link = Index;
		if (To.Y - From.Y > Height * 0.75f) {
			Line.SideFrom = PjcRouteBottom;
			Line.SideTo = PjcRouteTop;
		}
		else if (From.Y - To.Y > Height * 0.75f) {
			Line.SideFrom = PjcRouteTop;
			Line.SideTo = PjcRouteBottom;
		}
		else {
			Line.SideFrom = To.X >= From.X ? PjcRouteRight : PjcRouteLeft;
			Line.SideTo = To.X >= From.X ? PjcRouteLeft : PjcRouteRight;
		}
		const bool bSideways = Line.SideFrom < PjcRouteBottom;
		Ends.FindOrAdd(Link.From * 4 + Line.SideFrom).Add(FPjcRouteEnd {Lines.Num(), true, bSideways ? To.Y : To.X});
		Ends.FindOrAdd(Link.To * 4 + Line.SideTo).Add(FPjcRouteEnd {Lines.Num(), false, bSideways ? From.Y : From.X});
		Lines.Add(Line);
	}

	// ends on one side of a box: spread along it, in the order of where they lead
	for (auto& Pair : Ends) {
		const FPjcRouteBox& Box = Middles[Pair.Key / 4];
		const int32 Side = Pair.Key % 4;
		const bool bSideways = Side < PjcRouteBottom;
		TArray<FPjcRouteEnd>& List = Pair.Value;
		List.StableSort([](const FPjcRouteEnd& A, const FPjcRouteEnd& B) {
			return A.Toward < B.Toward;
		});
		const float Room = (bSideways ? Height : Width) - 2.0f * Grid;
		const float Fits = FMath::FloorToFloat(Room / FMath::Max(1, List.Num() - 1) / Grid) * Grid;
		const float Step = FMath::Clamp(Fits, bSideways ? Grid * 0.5f : Grid, 2.0f * Grid);
		const float Out = FMath::CeilToFloat(((bSideways ? Width : Height) * 0.5f + PjcRoutePad) / Grid) * Grid + Grid;
		for (int32 Index = 0; Index < List.Num(); ++Index) {
			const float Offset = FMath::Clamp((Index - (List.Num() - 1) * 0.5f) * Step, -Room * 0.5f, Room * 0.5f);
			const float Along = PjcRouteSnap((bSideways ? Box.Y : Box.X) + Offset);
			const float Sign = static_cast<float>(bSideways ? PjcRouteStepX[Side] : PjcRouteStepY[Side]);
			FPjcRouteBox Point;
			FPjcRouteBox Stub;
			if (bSideways) {
				Point = FPjcRouteBox {Box.X + Sign * Width * 0.5f, Along};
				Stub = FPjcRouteBox {PjcRouteSnap(Box.X) + Sign * Out, Along};
			}
			else {
				Point = FPjcRouteBox {Along, Box.Y + Sign * Height * 0.5f};
				Stub = FPjcRouteBox {Along, PjcRouteSnap(Box.Y) + Sign * Out};
			}
			FPjcRouteLine& Line = Lines[List[Index].Link];
			(List[Index].bFrom ? Line.PointFrom : Line.PointTo) = Point;
			(List[Index].bFrom ? Line.StubFrom : Line.StubTo) = Stub;
		}
	}

	// short lines first: they get the straight lanes
	const auto Span = [](const FPjcRouteLine& Line) {
		return FMath::Abs(Line.StubFrom.X - Line.StubTo.X) + FMath::Abs(Line.StubFrom.Y - Line.StubTo.Y);
	};
	TArray<int32> Order;
	for (int32 Index = 0; Index < Lines.Num(); ++Index) {
		Order.Add(Index);
	}
	Order.StableSort([&Lines, &Span](const int32 A, const int32 B) {
		return Span(Lines[A]) < Span(Lines[B]);
	});

	const auto CellOf = [X0, Y0, Cols, Rows](const FPjcRouteBox& Point) {
		const int32 Col = FMath::Clamp(FMath::RoundToInt((Point.X - X0) / Grid), 0, Cols - 1);
		const int32 Row = FMath::Clamp(FMath::RoundToInt((Point.Y - Y0) / Grid), 0, Rows - 1);
		return Row * Cols + Col;
	};
	const auto Cheaper = [](const FPjcRouteOpen& A, const FPjcRouteOpen& B) {
		return A.Cost < B.Cost;
	};

	// how much each cell is used by lines that run along or across it
	TArray<float> UsedAlong;
	TArray<float> UsedDown;
	UsedAlong.Init(0.0f, Num);
	UsedDown.Init(0.0f, Num);
	// A* over (cell, direction it was entered in); Seen says which line wrote a state, so nothing is cleared per line
	TArray<float> Costs;
	TArray<int32> Came;
	TArray<int32> Seen;
	Costs.Init(0.0f, Num * 4);
	Came.Init(INDEX_NONE, Num * 4);
	Seen.Init(INDEX_NONE, Num * 4);
	TArray<FPjcRouteOpen> Open;
	TArray<int32> Cells;

	for (const int32 LineIndex : Order) {
		const FPjcRouteLine& Line = Lines[LineIndex];
		const int32 Start = CellOf(Line.StubFrom);
		const int32 Target = CellOf(Line.StubTo);
		const int32 TargetCol = Target % Cols;
		const int32 TargetRow = Target / Cols;
		// the line arrives going into the box: against the side it enters through
		const int32 ArriveDir = Line.SideTo ^ 1;

		Open.Reset();
		const int32 First = Start * 4 + Line.SideFrom;
		Costs[First] = 0.0f;
		Came[First] = INDEX_NONE;
		Seen[First] = LineIndex;
		Open.HeapPush(FPjcRouteOpen {0.0f, First}, Cheaper);
		int32 Goal = INDEX_NONE;
		while (Open.Num() > 0) {
			FPjcRouteOpen Top;
			Open.HeapPop(Top, Cheaper);
			const int32 Cell = Top.State / 4;
			const int32 Dir = Top.State % 4;
			if (Cell == Target) {
				Goal = Top.State;
				break;
			}
			const float Cost = Costs[Top.State];
			const int32 Col = Cell % Cols;
			const int32 Row = Cell / Cols;
			for (int32 NextDir = 0; NextDir < 4; ++NextDir) {
				// no way back
				if ((NextDir ^ 1) == Dir) continue;

				const int32 NextCol = Col + PjcRouteStepX[NextDir];
				const int32 NextRow = Row + PjcRouteStepY[NextDir];
				if (NextCol < 0 || NextRow < 0 || NextCol >= Cols || NextRow >= Rows) continue;

				const int32 Next = NextRow * Cols + NextCol;
				const bool bAlong = NextDir < PjcRouteBottom;
				float Step = 1.0f;
				if (Blocked[Next] && Next != Target && Next != Start) Step += PjcRouteBox;
				if (NextDir != Dir) Step += PjcRouteTurn;
				Step += bAlong ? UsedAlong[Next] * PjcRouteShare + UsedDown[Next] * PjcRouteCross
							   : UsedDown[Next] * PjcRouteShare + UsedAlong[Next] * PjcRouteCross;
				if (Next == Target && NextDir != ArriveDir) Step += PjcRouteTurn;

				const int32 NextState = Next * 4 + NextDir;
				const float NextCost = Cost + Step;
				if (Seen[NextState] == LineIndex && Costs[NextState] <= NextCost) continue;

				Seen[NextState] = LineIndex;
				Costs[NextState] = NextCost;
				Came[NextState] = Top.State;
				const float Left = static_cast<float>(FMath::Abs(NextCol - TargetCol) + FMath::Abs(NextRow - TargetRow));
				Open.HeapPush(FPjcRouteOpen {NextCost + Left, NextState}, Cheaper);
			}
		}

		TArray<FVector2D> Points;
		Points.Add(FVector2D {Line.PointFrom.X, Line.PointFrom.Y});
		if (Goal == INDEX_NONE) {
			Points.Add(FVector2D {Line.StubFrom.X, Line.StubFrom.Y});
			Points.Add(FVector2D {Line.StubTo.X, Line.StubTo.Y});
		}
		else {
			Cells.Reset();
			for (int32 State = Goal; State != INDEX_NONE; State = Came[State]) {
				Cells.Add(State / 4);
			}
			Algo::Reverse(Cells);
			// the lane is taken, and a little of the lanes next to it: later lines keep away
			for (int32 Index = 1; Index < Cells.Num(); ++Index) {
				const bool bAlong = FMath::Abs(Cells[Index] - Cells[Index - 1]) == 1;
				TArray<float>& Used = bAlong ? UsedAlong : UsedDown;
				const int32 Beside = bAlong ? Cols : 1;
				for (const int32 Cell : {Cells[Index - 1], Cells[Index]}) {
					Used[Cell] += 1.0f;
					if (Cell - Beside >= 0) Used[Cell - Beside] += 0.35f;
					if (Cell + Beside < Num) Used[Cell + Beside] += 0.35f;
				}
			}
			for (const int32 Cell : Cells) {
				Points.Add(FVector2D {X0 + (Cell % Cols) * Grid, Y0 + (Cell / Cols) * Grid});
			}
		}
		Points.Add(FVector2D {Line.PointTo.X, Line.PointTo.Y});

		// only the points where the line turns
		TArray<FVector2D>& Corners = OutRoutes[Line.Link];
		Corners.Add(Points[0]);
		for (int32 Index = 1; Index < Points.Num() - 1; ++Index) {
			const FVector2D& Before = Corners.Last();
			const FVector2D& Here = Points[Index];
			const FVector2D& After = Points[Index + 1];
			const auto Same = [](const double A, const double B) {
				return FMath::Abs(A - B) < 0.01;
			};
			const bool bStraight = (Same(Before.X, Here.X) && Same(Here.X, After.X)) || (Same(Before.Y, Here.Y) && Same(Here.Y, After.Y));
			if (!bStraight) Corners.Add(Here);
		}
		Corners.Add(Points.Last());
	}
}

TArray<FVector2D> PjcGraphRouter::Round(const TArray<FVector2D>& Corners, const float Radius) {
	if (Corners.Num() < 3) return Corners;

	TArray<FVector2D> Points;
	Points.Add(Corners[0]);
	for (int32 Index = 1; Index < Corners.Num() - 1; ++Index) {
		const FVector2D& Before = Corners[Index - 1];
		const FVector2D& Here = Corners[Index];
		const FVector2D& After = Corners[Index + 1];
		const float LengthIn = static_cast<float>(FVector2D::Distance(Before, Here));
		const float LengthOut = static_cast<float>(FVector2D::Distance(Here, After));
		const float Cut = FMath::Min3(Radius, LengthIn * 0.5f, LengthOut * 0.5f);
		if (Cut < 0.5f) {
			Points.Add(Here);
			continue;
		}
		const FVector2D Enter = Here + (Before - Here) * (Cut / LengthIn);
		const FVector2D Leave = Here + (After - Here) * (Cut / LengthOut);
		// a quarter turn as a curve bent toward the corner
		constexpr int32 Pieces = 4;
		for (int32 Piece = 0; Piece <= Pieces; ++Piece) {
			const float T = static_cast<float>(Piece) / Pieces;
			const float U = 1.0f - T;
			Points.Add(Enter * (U * U) + Here * (2.0f * U * T) + Leave * (T * T));
		}
	}
	Points.Add(Corners.Last());
	return Points;
}

FVector2D PjcGraphRouter::Middle(const TArray<FVector2D>& Points) {
	if (Points.Num() == 0) return FVector2D::ZeroVector;

	float Total = 0.0f;
	for (int32 Index = 1; Index < Points.Num(); ++Index) {
		Total += static_cast<float>(FVector2D::Distance(Points[Index - 1], Points[Index]));
	}
	float Walked = 0.0f;
	for (int32 Index = 1; Index < Points.Num(); ++Index) {
		const float Length = static_cast<float>(FVector2D::Distance(Points[Index - 1], Points[Index]));
		if (Walked + Length >= Total * 0.5f && Length > 0.0f) {
			return Points[Index - 1] + (Points[Index] - Points[Index - 1]) * ((Total * 0.5f - Walked) / Length);
		}
		Walked += Length;
	}
	return Points.Last();
}
