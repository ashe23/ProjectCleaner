// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "Graph/SAssetsVisualizerGraph.h"
#include "Graph/AssetsVisualizerGraph.h"
#include "Graph/AssetsVisualizerGraphSchema.h"

void SAssetsVisualizerGraph::Construct(const FArguments& InArgs)
{
	GraphObj = NewObject<UAssetsVisualizerGraph>();
	GraphObj->Schema = UAssetsVisualizerGraphSchema::StaticClass();
	GraphObj->AddToRoot();

	GraphEditorPtr = SNew(SGraphEditor)
		.GraphToEdit(GraphObj);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				GraphEditorPtr.ToSharedRef()
			]
		]
	];
}
