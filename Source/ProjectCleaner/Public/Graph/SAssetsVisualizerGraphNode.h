// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "SGraphNode.h"

class SAssetsVisualizerGraphNode : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SAssetsVisualizerGraphNode) {}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, class UAssetsVisualizerGraphNode* InNode);

	// SGraphNode implementation
	virtual void UpdateGraphNode() override;
	virtual bool IsNodeEditable() const override { return false; }	
	// End SGraphNode implementation
};