// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "Graph/SAssetsVisualizerGraphNode.h"
#include "Graph/AssetsVisualizerGraphNode.h"

void SAssetsVisualizerGraphNode::Construct(const FArguments& InArgs, UAssetsVisualizerGraphNode* InNode)
{
	GraphNode = InNode;
	SetCursor(EMouseCursor::CardinalCross);
	UpdateGraphNode();
}

void SAssetsVisualizerGraphNode::UpdateGraphNode()
{	
	TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);
	TSharedPtr<SVerticalBox> MainVerticalBox;

	GetOrAddSlot(ENodeZone::Center)
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	[
		SAssignNew(MainVerticalBox, SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("Border"))
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.Padding(FMargin(0, 3))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					// LEFT
					SNew(SBox)
					.WidthOverride(40)
					[
						SAssignNew(LeftNodeBox, SVerticalBox)
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString("AAA"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					// RIGHT
					SNew(SBox)
					.WidthOverride(40)
					[
						SAssignNew(RightNodeBox, SVerticalBox)
					]
				]
			]
		]
	];

	CreateBelowWidgetControls(MainVerticalBox);
	CreatePinWidgets();
}
