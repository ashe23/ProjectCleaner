// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "Graph/SAssetsVisualizerGraphNode.h"

void SAssetsVisualizerGraphNode::Construct(const FArguments& InArgs)
{
	SetCursor(EMouseCursor::CardinalCross);
	UpdateGraphNode();
}

void SAssetsVisualizerGraphNode::UpdateGraphNode()
{
	TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);

	GetOrAddSlot(ENodeZone::Center)
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.Text(FText::FromString("TEXT TEST"))
	];

	CreatePinWidgets();
}
