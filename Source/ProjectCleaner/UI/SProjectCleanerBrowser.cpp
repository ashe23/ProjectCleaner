// Fill out your copyright notice in the Description page of Project Settings.


#include "SProjectCleanerBrowser.h"

void SProjectCleanerBrowser::Construct(const FArguments& InArgs)
{
	Options.Add(MakeShareable(new FString("Option1")));
	Options.Add(MakeShareable(new FString("Option2")));
	Options.Add(MakeShareable(new FString("LastOption")));

	CurrentItem = Options[0];
	
	ChildSlot
	[
		SNew(SComboBox<FComboItemType>)
		.OptionsSource(&Options)
		.OnSelectionChanged(this, &SProjectCleanerBrowser::OnSelectionChanged)
		.OnGenerateWidget(this, &SProjectCleanerBrowser::MakeWidgetForOption)
		.InitiallySelectedItem(CurrentItem)
		[
			SNew(STextBlock)
			.Text(this, &SProjectCleanerBrowser::GetCurrentItemLabel)
		]
	];
}

TSharedRef<SWidget> SProjectCleanerBrowser::MakeWidgetForOption(FComboItemType InOption)
{
	return SNew(STextBlock).Text(FText::FromString(*InOption));
}

void SProjectCleanerBrowser::OnSelectionChanged(FComboItemType NewValue, ESelectInfo::Type)
{
	CurrentItem = NewValue;
}

FText SProjectCleanerBrowser::GetCurrentItemLabel() const
{
	if(CurrentItem.IsValid())
	{
		return FText::FromString(*CurrentItem);
	}

	return FText::FromString("<<Invalid Option>>");
}
