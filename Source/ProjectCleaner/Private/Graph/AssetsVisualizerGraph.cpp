// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "Graph/AssetsVisualizerGraph.h"
#include "Graph/AssetsVisualizerGraphNode.h"

UAssetsVisualizerGraphNode* UAssetsVisualizerGraph::CreateReferenceNode()
{
	return Cast<UAssetsVisualizerGraphNode>(CreateNode(UAssetsVisualizerGraphNode::StaticClass(), false));
}
