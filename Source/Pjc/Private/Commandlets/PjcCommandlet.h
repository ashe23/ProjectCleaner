// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "PjcCommandlet.generated.h"

UCLASS()
class UPjcCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UPjcCommandlet();
	virtual int32 Main(const FString& Params) override;

private:
	void ParseCommandLinesArguments(const FString& Params);

	bool bScanOnly = false;
};
