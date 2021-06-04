// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved

#include "Graph/AssetsVisualizerGraphNode.h"
#include "EdGraph/EdGraphPin.h"

FText UAssetsVisualizerGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(TEXT("BP_Suzan"));
}

FLinearColor UAssetsVisualizerGraphNode::GetNodeTitleColor() const
{
	return FLinearColor(1.0f, 0.3f, 0.0f);
}

FText UAssetsVisualizerGraphNode::GetTooltipText() const
{
	return FText::FromString(TEXT("BP_Suzan_tooltip"));
}

void UAssetsVisualizerGraphNode::AllocateDefaultPins()
{
	ReferencerPin = CreatePin(EEdGraphPinDirection::EGPD_Input, NAME_None, NAME_None);
	DependencyPin = CreatePin(EEdGraphPinDirection::EGPD_Output, NAME_None, NAME_None);

	ReferencerPin->bHidden = false;
	DependencyPin->bHidden = false;
	ReferencerPin->PinType.PinCategory = FName{ "Test1" };
	DependencyPin->PinType.PinCategory = FName{ "Test2" };
}

void UAssetsVisualizerGraphNode::Setup()
{
	NodePosX = 200.0f;
	NodePosY = 200.0f;

	NodeComment = "Some Test Node Comment";
	AllocateDefaultPins();
}
