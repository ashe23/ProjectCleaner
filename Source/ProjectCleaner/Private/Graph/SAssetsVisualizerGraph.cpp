// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "Graph/SAssetsVisualizerGraph.h"
#include "Graph/AssetsVisualizerGraph.h"
#include "Graph/AssetsVisualizerGraphSchema.h"
#include "Graph/AssetsVisualizerGraphNode.h"
#include "Graph/AssetRelationalMap.h"

void SAssetsVisualizerGraph::Construct(const FArguments& InArgs)
{
	RelationalMap = InArgs._RelationalMap;

	GraphObj = NewObject<UAssetsVisualizerGraph>();
	GraphObj->Schema = UAssetsVisualizerGraphSchema::StaticClass();
	GraphObj->AddToRoot();

	GraphEditorPtr = SNew(SGraphEditor)
		.GraphToEdit(GraphObj);

	// create root node
	// testing
	float RootX = 0.0f;
	float RootY = 0.0f;
	float OffsetX = 0.0f;
	float OffsetY = 0.0f;

	if (RelationalMap)
	{
		for (const auto& Node : RelationalMap->GetNodes())
		{
			// creating root node
			const auto CurrentRootNode = CreateNode(FText::FromName(Node.AssetData.PackageName), RootX + OffsetX, RootY + OffsetY);

			OffsetX = 500.0f;
			const auto Height = Node.LinkedAssetsData.Num() == 0 ? 500.0f : Node.LinkedAssetsData.Num() * 100;
			OffsetY = -Height;
			// creating linked nodes
			for (const auto& LinkedNode : Node.LinkedAssetsData)
			{
				const auto CurrentLinkedNode = CreateNode(FText::FromName(LinkedNode->PackageName), RootX + OffsetX, RootY + OffsetY);
				OffsetY += 100.0f;
				MakeLink(*CurrentRootNode, *CurrentLinkedNode);
			}

			OffsetX = 0;
			OffsetY = 0;
			RootY += Height;
		}
	}


	//if (RootNode && OtherNode)
	//{
	//	const auto RefPin = RootNode->GetRefPin();
	//	const auto DepPin = OtherNode->GetDepPin();
	//	if (RefPin && DepPin)
	//	{
	//		RefPin->MakeLinkTo(DepPin);
	//	}
	//}

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

UAssetsVisualizerGraphNode* SAssetsVisualizerGraph::CreateNode(const FText& Title, const float X, const float Y)
{
	UAssetsVisualizerGraphNode* NewNode = GraphObj->CreateReferenceNode();
	if (!NewNode) return nullptr;

	NewNode->Setup(Title, X, Y);
	GraphObj->Nodes.Add(NewNode);

	return NewNode;
}

void SAssetsVisualizerGraph::MakeLink(UAssetsVisualizerGraphNode& RootNode, UAssetsVisualizerGraphNode& ChildNode)
{
	if (!RootNode.IsValidLowLevelFast() || !ChildNode.IsValidLowLevelFast()) return;

	const auto DepPin = RootNode.GetDepPin();
	const auto RefPin = ChildNode.GetRefPin();

	if (!RefPin || !DepPin) return;

	DepPin->MakeLinkTo(RefPin);
}
