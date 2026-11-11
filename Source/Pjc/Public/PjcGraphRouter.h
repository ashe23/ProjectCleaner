// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// One line to lay: from the box that uses to the box it uses, indices into the boxes
struct FPjcRouteLink
{
	int32 From = 0;
	int32 To = 0;
};

/**
 * Lays the lines of a graph of boxes at right angles, like the wiring of a plan: on a grid around the boxes, a turn
 * costs and so does a lane another line already uses, so lines that run side by side keep apart and none goes through
 * a box. A box uses what is below it: a line leaves at the bottom and arrives at the top; boxes in one row are joined
 * side to side. Ends on one side of a box are spread along it.
 */
namespace PjcGraphRouter
{
	// Distance between two lanes
	constexpr float Grid = 10.0f;

	// Boxes: top left corners of boxes of one size. OutRoutes: for every link the points where its line starts, turns
	// and ends
	void Route(const TArray<FVector2D>& Boxes, const FVector2D& BoxSize, const TArray<FPjcRouteLink>& Links, TArray<TArray<FVector2D>>& OutRoutes);

	// The same line with round corners, as short straight pieces
	TArray<FVector2D> Round(const TArray<FVector2D>& Corners, const float Radius);

	// Point half way along a line
	FVector2D Middle(const TArray<FVector2D>& Points);
}	// namespace PjcGraphRouter
