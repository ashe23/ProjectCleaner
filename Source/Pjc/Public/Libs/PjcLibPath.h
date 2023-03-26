// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FPjcLibPath
{
	static TOptional<FString> Normalize(const FString& InPath);
};