// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphEditor.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class UAssetsVisualizerGraph;
class AssetRelationalMap;

class SAssetsVisualizerGraph : public SCompoundWidget
{
public:
	
	SLATE_BEGIN_ARGS(SAssetsVisualizerGraph) {}
		SLATE_ARGUMENT(AssetRelationalMap*, RelationalMap)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
private:

	UAssetsVisualizerGraphNode* CreateNode(const FText& Title, const float X, const float Y);
	void MakeLink(UAssetsVisualizerGraphNode& RootNode, UAssetsVisualizerGraphNode& ChildNode);

	UAssetsVisualizerGraph* GraphObj = nullptr;
	TSharedPtr<SGraphEditor> GraphEditorPtr;
	AssetRelationalMap* RelationalMap = nullptr;
};