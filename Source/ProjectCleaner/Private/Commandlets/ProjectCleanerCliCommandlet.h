// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "ProjectCleanerCliCommandlet.generated.h"

UCLASS()
class PROJECTCLEANER_API UProjectCleanerCliCommandlet final : public UCommandlet
{
	GENERATED_BODY()

public:
	UProjectCleanerCliCommandlet();
	virtual int32 Main(const FString& Params) override;
};
