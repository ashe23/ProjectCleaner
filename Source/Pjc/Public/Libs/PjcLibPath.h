// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FPjcLibPath
{
	static FString Normalize(const FString& InPath);
	static FString ToAbsolute(const FString& InPath);
	static FString ToContentPath(const FString& InPath);
	static FString ToObjectPath(const FString& InPath);
	static bool IsPathEmpty(const FString& InPath);
	static bool IsPathExcluded(const FString& InPath);
};
