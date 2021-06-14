// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "ProjectCleanerExcludeOptionsUI.generated.h"

UCLASS(Transient)
class UExcludeOptions : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(DisplayName = "Paths", EditAnywhere, Category = "ExcludeOptions", meta = (ContentDir))
	TArray<FDirectoryPath> Paths;

	UPROPERTY(DisplayName = "Classes", EditAnywhere, Category = "ExcludeOptions")
	TArray<UClass*> Classes;
};

/**
 * @brief Shows Exclude Options UI
 */
class SProjectCleanerExcludeOptionsUI : public SCompoundWidget
{
public:
	
	SLATE_BEGIN_ARGS(SProjectCleanerExcludeOptionsUI) {}
		SLATE_ARGUMENT(UExcludeOptions*, ExcludeOptions)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
private:

	/** Data **/
	TSharedPtr<IDetailsView> ExcludeOptionsProperty;
	UExcludeOptions* ExcludeOptions = nullptr;
};
