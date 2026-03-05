// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
// #include "Styling/SlateStyle.h"
#include "Styling/SlateColor.h"
#include "Runtime/Launch/Resources/Version.h"

#if ENGINE_MAJOR_VERSION == 5
#include "Styling/AppStyle.h"
#else
#include "EditorStyleSet.h"
#endif

// This is compatibility layer for handling different engine api versions
namespace PjcShim
{
	const ISlateStyle& GetStyle();
	const FSlateBrush* GetBrush(const FName PropertyName, const ANSICHAR* Specifier = nullptr);
	FSlateColor GetSlateColor(const FName PropertyName, const ANSICHAR* Specifier = nullptr);
}	// namespace PjcShim
