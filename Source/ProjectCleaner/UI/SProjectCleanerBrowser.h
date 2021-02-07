// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// #include "CoreMinimal.h"
#include "SCompoundWidget.h"
#include "SlateBasics.h"
#include "SlateExtras.h"

/**
 * 
 */
class SProjectCleanerBrowser : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectCleanerBrowser)
	{}
	SLATE_END_ARGS()

	typedef TSharedPtr<FString> FComboItemType;

	void Construct(const FArguments& InArgs);

	TSharedRef<SWidget> MakeWidgetForOption(FComboItemType InOption);
	void OnSelectionChanged(FComboItemType NewValue, ESelectInfo::Type);
	FText GetCurrentItemLabel() const;
	
	
	FComboItemType CurrentItem;
	TArray<FComboItemType> Options;
};
