// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphEditor.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class UAssetsVisualizerGraph;

class SAssetsVisualizerGraph : public SCompoundWidget
{
public:
	
	SLATE_BEGIN_ARGS(SAssetsVisualizerGraph) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
private:
	UAssetsVisualizerGraph* GraphObj = nullptr;
	TSharedPtr<SGraphEditor> GraphEditorPtr;
};