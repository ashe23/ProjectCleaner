// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "Graph/SAssetsVisualizerGraph.h"
#include "Graph/AssetsVisualizerGraph.h"
#include "Graph/AssetsVisualizerGraphSchema.h"
#include "Graph/AssetsVisualizerGraphNode.h"

void SAssetsVisualizerGraph::Construct(const FArguments& InArgs)
{
	GraphObj = NewObject<UAssetsVisualizerGraph>();
	GraphObj->Schema = UAssetsVisualizerGraphSchema::StaticClass();
	GraphObj->AddToRoot();

	GraphEditorPtr = SNew(SGraphEditor)
		.GraphToEdit(GraphObj);

	// create root node
	UAssetsVisualizerGraphNode* RootNode = GraphObj->CreateReferenceNode();
	if (RootNode)
	{
		RootNode->Setup();
		
		GraphEditorPtr->SetNodeSelection(RootNode, true);
	}

	UAssetsVisualizerGraphNode* OtherNode = GraphObj->CreateReferenceNode();
	if (OtherNode)
	{
		OtherNode->Setup();
		GraphObj->Nodes.Add(OtherNode);
	}

	if (RootNode && OtherNode)
	{
		const auto RefPin = RootNode->GetRefPin();
		const auto DepPin = OtherNode->GetDepPin();
		if (RefPin && DepPin)
		{
			RefPin->MakeLinkTo(DepPin);
		}
	}

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
