// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabFilesExternal.h"
#include "PjcCmds.h"
#include "PjcStyles.h"
#include "PjcSubsystem.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"

void SPjcTabFilesExternal::Construct(const FArguments& InArgs)
{
	Cmds = MakeShareable(new FUICommandList);

	Cmds->MapAction(
		FPjcCmds::Get().FilesScan,
		FExecuteAction::CreateLambda([]() {})
	);
	Cmds->MapAction(
		FPjcCmds::Get().FilesDelete,
		FExecuteAction::CreateLambda([]() {})
	);
	Cmds->MapAction(
		FPjcCmds::Get().FilesExclude,
		FExecuteAction::CreateLambda([]() {})
	);
	Cmds->MapAction(
		FPjcCmds::Get().FilesExcludeByExt,
		FExecuteAction::CreateLambda([]() {})
	);
	Cmds->MapAction(
		FPjcCmds::Get().FilesExcludeByPath,
		FExecuteAction::CreateLambda([]() {})
	);
	Cmds->MapAction(
		FPjcCmds::Get().ClearSelection,
		FExecuteAction::CreateLambda([]() {})
	);

	FPropertyEditorModule& PropertyEditor = UPjcSubsystem::GetModulePropertyEditor();
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bShowOptions = true;
	DetailsViewArgs.bAllowFavoriteSystem = false;
	DetailsViewArgs.bShowPropertyMatrixButton = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.ViewIdentifier = "PjcEditorFileExcludeSettings";

	const auto SettingsProperty = PropertyEditor.CreateDetailView(DetailsViewArgs);
	SettingsProperty->SetObject(GetMutableDefault<UPjcFileExcludeSettings>());

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
			SNew(SSplitter)
			.PhysicalSplitterHandleSize(3.0f)
			.Style(FEditorStyle::Get(), "DetailsView.Splitter")
			+ SSplitter::Slot().Value(0.4f)
			[
				SNew(SBox)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Justification(ETextJustify::Center)
							.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Gray"))
							.ShadowOffset(FVector2D{1.5f, 1.5f})
							.ShadowColorAndOpacity(FLinearColor::Black)
							.Font(FPjcStyles::GetFont("Bold", 15))
							.Text(FText::FromString(TEXT("Summary")))
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("Stats here")))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(3.0f)
					[
						SNew(SSeparator).Thickness(3.0f)
					]
					+ SVerticalBox::Slot().FillHeight(1.0f).Padding(3.0f)
					[
						SNew(SScrollBox)
						.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
						.AnimateWheelScrolling(true)
						.AllowOverscroll(EAllowOverscroll::No)
						+ SScrollBox::Slot()
						[
							SettingsProperty
						]
					]
				]
			]
			+ SSplitter::Slot().Value(0.4f)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("List here")))
			]
		]
	];
}

TSharedRef<SWidget> SPjcTabFilesExternal::CreateToolbar() const
{
	FToolBarBuilder ToolBarBuilder{Cmds, FMultiBoxCustomization::None};
	ToolBarBuilder.BeginSection("PjcSectionActionsFilesExternal");
	{
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().FilesScan);
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().FilesDelete);
		ToolBarBuilder.AddSeparator();
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().FilesExclude);
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().FilesExcludeByExt);
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().FilesExcludeByPath);
		ToolBarBuilder.AddSeparator();
		ToolBarBuilder.AddToolBarButton(FPjcCmds::Get().ClearSelection);
	}
	ToolBarBuilder.EndSection();

	return ToolBarBuilder.MakeWidget();
}
