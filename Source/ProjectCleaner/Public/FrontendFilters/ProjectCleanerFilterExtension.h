// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ContentBrowserFrontEndFilterExtension.h"
#include "ProjectCleanerFilterExtension.generated.h"

UCLASS()
class UProjectCleanerFilterExtension final : public UContentBrowserFrontEndFilterExtension
{
	GENERATED_BODY()
	
public:
	virtual void AddFrontEndFilterExtensions(TSharedPtr<FFrontendFilterCategory> DefaultCategory, TArray<TSharedRef<FFrontendFilter>>& InOutFilterList) const override;
};