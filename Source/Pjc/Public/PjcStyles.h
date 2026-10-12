// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"
#include "Textures/SlateIcon.h"

class FPjcStyles
{
  public:

	static void Initialize();
	static void Shutdown();
	static void ReloadTextures();
	static const ISlateStyle& Get();
	static FName GetStyleSetName();
	static FSlateFontInfo GetFont(const FString& FontType, const uint32 FontSize);
	static FSlateIcon GetIcon(const FString& IconSpecifier);

  private:

	static TSharedRef<FSlateStyleSet> Create();
	static TSharedPtr<FSlateStyleSet> StyleInstance;
};
