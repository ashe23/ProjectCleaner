// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "EdGraph/EdGraph.h"
#include "AssetsVisualizerGraph.generated.h"

class UAssetsVisualizerGraphNode;

UCLASS()
class UAssetsVisualizerGraph : public UEdGraph
{
	GENERATED_BODY()

public:
	void RebuildGraph();
private:
	UAssetsVisualizerGraphNode* CreateReferenceNode();
	FIntPoint CurrentGraphRootOrigin;
};