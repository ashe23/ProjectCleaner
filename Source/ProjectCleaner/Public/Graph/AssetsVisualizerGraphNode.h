// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "EdGraph/EdGraphNode.h"
#include "AssetsVisualizerGraphNode.generated.h"

UCLASS()
class UAssetsVisualizerGraphNode : public UEdGraphNode
{
	GENERATED_BODY()

	// UEdGraphNode implementation
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual void AllocateDefaultPins() override;
	// End UEdGraphNode implementation
};