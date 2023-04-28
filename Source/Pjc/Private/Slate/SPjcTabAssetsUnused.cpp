// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabAssetsUnused.h"
#include "PjcCmds.h"
// Engine Headers
#include "Widgets/Layout/SSeparator.h"

void SPjcTabAssetsUnused::Construct(const FArguments& InArgs)
{
	Cmds = MakeShareable(new FUICommandList);

	Cmds->MapAction(
		FPjcCmds::Get().TabAssetsUnusedBtnScan,
		FExecuteAction::CreateLambda([&]()
		{
			UE_LOG(LogTemp, Warning, TEXT("Scanning assets"));
		})
	);

	Cmds->MapAction(
		FPjcCmds::Get().TabAssetsUnusedBtnClean,
		FExecuteAction::CreateLambda([]()
		{
			UE_LOG(LogTemp, Warning, TEXT("Cleaning project"));
		}),
		FCanExecuteAction::CreateLambda([]()
		{
			return true;
		})
	);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
		[
			CreateToolbar()
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(5.0f)
		[
			SNew(SSeparator).Thickness(5.0f)
		]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5.0f)
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("Tab Unused Assets")))
		]
	];
}

TSharedRef<SWidget> SPjcTabAssetsUnused::CreateToolbar() const
{
	FToolBarBuilder ToolBarBuilder{Cmds, FMultiBoxCustomization::None};
	ToolBarBuilder.BeginSection("PjcTabAssetUnusedActions");
	{
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().TabAssetsUnusedBtnScan);
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().TabAssetsUnusedBtnClean);
	}
	ToolBarBuilder.EndSection();

	return ToolBarBuilder.MakeWidget();
}
