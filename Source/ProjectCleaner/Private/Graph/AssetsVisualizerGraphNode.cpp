// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved

#include "Graph/AssetsVisualizerGraphNode.h"

FText UAssetsVisualizerGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(TEXT("NodeTestTile"));
}

FLinearColor UAssetsVisualizerGraphNode::GetNodeTitleColor() const
{
	return FLinearColor(1.0f, 0.0f, 0.0f);
}

FText UAssetsVisualizerGraphNode::GetTooltipText() const
{
	return FText::FromString(TEXT("NodeTestTooltipText"));
}

void UAssetsVisualizerGraphNode::AllocateDefaultPins()
{
}
