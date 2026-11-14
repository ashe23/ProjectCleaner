// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcFixWindow.h"
#include "Slate/PjcWidgets.h"
#include "PjcFormat.h"
#include "PjcShim.h"
#include "PjcStyles.h"
#include "PjcSubsystem.h"
// Engine Headers
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	// unity builds join files: names unique to this file
	constexpr int32 PjcFixMaxRows = 40;
	TWeakPtr<SPjcFixWindow> PjcFixOpen;

	FLinearColor PjcFixHex(const TCHAR* Value) {
		return FLinearColor {FColor::FromHex(Value)};
	}

	const FSlateBrush* PjcFixWhite() {
		return FCoreStyle::Get().GetBrush("WhiteBrush");
	}

	TSharedRef<SWidget> PjcFixLine() {
		// clang-format off
		return SNew(SBox).HeightOverride(1.0f)
			[
				SNew(SImage).Image(PjcFixWhite()).ColorAndOpacity(PjcWidgets::GetColor("ProjectCleaner.Color.Line"))
			];
		// clang-format on
	}

	// a box with a 1 px outline
	TSharedRef<SWidget> PjcFixBox(const FLinearColor& Outline, const FLinearColor& Fill, const FMargin& Padding, const TSharedRef<SWidget>& Inner) {
		// clang-format off
		return SNew(SBorder).BorderImage(PjcFixWhite()).BorderBackgroundColor(Outline).Padding(1.0f)
			[
				SNew(SBorder).BorderImage(PjcFixWhite()).BorderBackgroundColor(Fill).Padding(Padding)
				[
					Inner
				]
			];
		// clang-format on
	}

	TSharedRef<SWidget> PjcFixDot(const FLinearColor& Color, const float Size) {
		// clang-format off
		return SNew(SBox).WidthOverride(Size).HeightOverride(Size).VAlign(VAlign_Center)
			[
				SNew(SImage).Image(FPjcStyles::Get().GetBrush("ProjectCleaner.Dot")).ColorAndOpacity(Color)
			];
		// clang-format on
	}

	// version control of the project: its name, what it means for deleted files, and whether there is one
	bool PjcFixVersionControl(FString& OutName, FString& OutDetail) {
		ISourceControlModule& SourceControl = ISourceControlModule::Get();
		if (SourceControl.IsEnabled() && SourceControl.GetProvider().IsAvailable()) {
			OutName = SourceControl.GetProvider().GetName().ToString();
			const bool bGit = OutName.Contains(TEXT("Git"));
			OutDetail = bGit ? TEXT("deleted files can be restored from it") : TEXT("files are checked out first, deleted ones can be restored");
			return true;
		}
		if (UPjcSubsystem::ProjectHasVersionControl()) {
			OutName = TEXT("Version control");
			OutDetail = TEXT("found next to the project, deleted files can be restored from it");
			return true;
		}
		OutName = TEXT("No version control");
		OutDetail = TEXT("deleted files cannot be restored");
		return false;
	}
}	// namespace

void SPjcFixWindow::Construct(const FArguments& InArgs) {
	Build = InArgs._Build;
	Window = InArgs._Window;
	// clang-format off
	ChildSlot
	[
		SNew(SBorder).BorderImage(PjcFixWhite()).BorderBackgroundColor(PjcWidgets::GetColor("ProjectCleaner.Color.Panel")).Padding(0.0f)
		[
			SAssignNew(Content, SBox).WidthOverride(640.0f).MaxDesiredHeight(780.0f)
		]
	];
	// clang-format on
	Rebuild();
}

void SPjcFixWindow::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) {
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	// a picker calls back from inside its own menu: built again here, after it is done
	if (bRebuild) {
		bRebuild = false;
		Rebuild();
	}
}

bool SPjcFixWindow::Show(const FString& WindowTitle, TFunction<FPjcFixView()> InBuild) {
	// clang-format off
	const TSharedRef<SWindow> FixWindow = SNew(SWindow)
		.Title(FText::FromString(WindowTitle))
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.SizingRule(ESizingRule::Autosized);
	// clang-format on
	const TSharedRef<SPjcFixWindow> Widget = SNew(SPjcFixWindow).Build(MoveTemp(InBuild)).Window(FixWindow);
	FixWindow->SetContent(Widget);
	PjcFixOpen = Widget;
	FSlateApplication::Get().AddModalWindow(FixWindow, FSlateApplication::Get().GetActiveTopLevelWindow());
	PjcFixOpen.Reset();

	if (Widget->AfterClose) {
		const TFunction<void()> Then = MoveTemp(Widget->AfterClose);
		Then();
		return false;
	}
	return Widget->bConfirmed;
}

void SPjcFixWindow::RefreshOpen() {
	if (const TSharedPtr<SPjcFixWindow> Open = PjcFixOpen.Pin()) {
		Open->bRebuild = true;
	}
}

void SPjcFixWindow::Rebuild() {
	if (Build && Content.IsValid()) {
		Content->SetContent(MakeContent(Build()));
	}
}

void SPjcFixWindow::Close(const bool bConfirm) {
	bConfirmed = bConfirm;
	if (const TSharedPtr<SWindow> Owner = Window.Pin()) {
		Owner->RequestDestroyWindow();
	}
}

TSharedRef<SWidget> SPjcFixWindow::MakeContent(const FPjcFixView& View) {
	const FSlateColor Muted = FSlateColor::UseSubduedForeground();

	// header: title, what happens, the counts
	const TSharedRef<SVerticalBox> Head = SNew(SVerticalBox);
	// clang-format off
	Head->AddSlot().AutoHeight()
	[
		SNew(STextBlock).Text(FText::FromString(View.Title)).Font(FPjcStyles::GetFont("Bold", 12))
	];
	// clang-format on
	if (!View.Lead.IsEmpty()) {
		// clang-format off
		Head->AddSlot().AutoHeight().Padding(FMargin {0.0f, 6.0f, 0.0f, 0.0f})
		[
			SNew(STextBlock).Text(FText::FromString(View.Lead)).AutoWrapText(true).ColorAndOpacity(Muted)
		];
		// clang-format on
	}
	if (View.Stats.Num() > 0) {
		const TSharedRef<SHorizontalBox> Stats = SNew(SHorizontalBox);
		for (int32 Index = 0; Index < View.Stats.Num(); ++Index) {
			const FPjcFixStat& Stat = View.Stats[Index];
			const FLinearColor NumColor = Stat.Num > 0 ? PjcWidgets::GetColor(Stat.Color) : PjcFixHex(TEXT("#5D636B"));
			// clang-format off
			Stats->AddSlot().FillWidth(1.0f).Padding(FMargin {Index > 0 ? 6.0f : 0.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(SBorder).BorderImage(PjcFixWhite()).BorderBackgroundColor(PjcWidgets::GetColor("ProjectCleaner.Color.Header")).Padding(FMargin {10.0f, 5.0f})
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 6.0f, 0.0f})
					[
						SNew(STextBlock).Text(FText::FromString(PjcFormat::Count(Stat.Num))).Font(PjcWidgets::GetFontMono(12)).ColorAndOpacity(NumColor)
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(Stat.Label)).ColorAndOpacity(Muted).Clipping(EWidgetClipping::ClipToBounds)
					]
				]
			];
			// clang-format on
		}
		// clang-format off
		Head->AddSlot().AutoHeight().Padding(FMargin {0.0f, 10.0f, 0.0f, 0.0f})
		[
			Stats
		];
		// clang-format on
	}

	// body: the sections, then the choices, the options, the warning and a tip
	const TSharedRef<SVerticalBox> Body = SNew(SVerticalBox);
	const auto AddBody = [&Body](const TSharedRef<SWidget>& Widget) {
		// clang-format off
		Body->AddSlot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 12.0f})
		[
			Widget
		];
		// clang-format on
	};
	for (const auto& Section : View.Sections) {
		AddBody(MakeSection(Section));
	}

	if (View.Choice && View.Choices.Num() > 0) {
		int32* Choice = View.Choice;
		const TSharedRef<SVerticalBox> Choices = SNew(SVerticalBox);
		for (int32 Index = 0; Index < View.Choices.Num(); ++Index) {
			const FPjcFixChoice& Item = View.Choices[Index];
			const bool bOn = *Choice == Index;
			const FLinearColor DangerOutline = PjcFixHex(TEXT("#6B3034"));
			const FLinearColor OnOutline = Item.bDanger ? DangerOutline : PjcWidgets::GetColor("ProjectCleaner.Color.Accent");
			const FLinearColor DangerFill = PjcFixHex(TEXT("#2C1C1E"));
			const FLinearColor OnFill = Item.bDanger ? DangerFill : PjcWidgets::GetColor("ProjectCleaner.Color.Marked");
			const FLinearColor Outline = bOn ? OnOutline : PjcWidgets::GetColor("ProjectCleaner.Color.Line");
			const FLinearColor Fill = bOn ? OnFill : PjcWidgets::GetColor("ProjectCleaner.Color.Header");
			const FSlateColor TitleColor
				= Item.bDanger ? FSlateColor {PjcWidgets::GetColor("ProjectCleaner.Color.DangerText")} : FSlateColor::UseForeground();

			const TSharedRef<SVerticalBox> Text = SNew(SVerticalBox);
			// clang-format off
			Text->AddSlot().AutoHeight()
			[
				SNew(STextBlock).Text(FText::FromString(Item.Title)).ColorAndOpacity(TitleColor)
			];
			Text->AddSlot().AutoHeight().Padding(FMargin {0.0f, 2.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock).Text(FText::FromString(Item.Description)).AutoWrapText(true).Font(FPjcStyles::GetFont("Regular", 8)).ColorAndOpacity(Muted)
			];
			// clang-format on
			if (bOn && Item.Extra.IsValid()) {
				// clang-format off
				Text->AddSlot().AutoHeight().Padding(FMargin {0.0f, 6.0f, 0.0f, 0.0f})
				[
					Item.Extra.ToSharedRef()
				];
				// clang-format on
			}
			// clang-format off
			Choices->AddSlot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 6.0f})
			[
				PjcFixBox(Outline, Fill, FMargin {10.0f, 8.0f},
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(FMargin {0.0f, 1.0f, 8.0f, 0.0f})
					[
						SNew(SCheckBox)
						.Style(&FCoreStyle::Get().GetWidgetStyle<FCheckBoxStyle>("RadioButton"))
						.IsChecked_Lambda([Choice, Index]() { return *Choice == Index ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this, Choice, Index](const ECheckBoxState) {
							*Choice = Index;
							bRebuild = true;
						})
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[
						Text
					])
			];
			// clang-format on
		}
		AddBody(Choices);
	}

	if (View.Options.Num() > 0) {
		const TSharedRef<SVerticalBox> Options = SNew(SVerticalBox);
		// clang-format off
		Options->AddSlot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 6.0f})
		[
			PjcWidgets::MakeCardTitle(TEXT("Options"))
		];
		// clang-format on
		for (const auto& Option : View.Options) {
			bool* Value = Option.Value;
			if (!Value) continue;
			// clang-format off
			Options->AddSlot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 6.0f})
			[
				SNew(SCheckBox)
				.Style(&PjcShim::GetCheckBoxStyle())
				.IsChecked_Lambda([Value]() { return *Value ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this, Value](const ECheckBoxState State) {
					*Value = State == ECheckBoxState::Checked;
					bRebuild = true;
				})
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(FText::FromString(Option.Label))
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(FText::FromString(Option.Description)).AutoWrapText(true).Font(FPjcStyles::GetFont("Regular", 8)).ColorAndOpacity(Muted)
					]
				]
			];
			// clang-format on
		}
		AddBody(Options);
	}

	if (!View.Warning.IsEmpty()) {
		const FLinearColor Yellow = PjcWidgets::GetColor("ProjectCleaner.Color.Yellow");
		// clang-format off
		AddBody(PjcFixBox(PjcFixHex(TEXT("#4F4122")), PjcFixHex(TEXT("#2A2416")), FMargin {10.0f, 7.0f},
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin {0.0f, 0.0f, 8.0f, 0.0f})
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("\u26A0"))).ColorAndOpacity(Yellow)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(FText::FromString(View.WarningTitle)).Font(FPjcStyles::GetFont("Bold", 9)).ColorAndOpacity(Yellow)
					.Visibility(View.WarningTitle.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(FText::FromString(View.Warning)).AutoWrapText(true).ColorAndOpacity(PjcFixHex(TEXT("#E8C88A")))
				]
			]));
		// clang-format on
	}
	if (!View.Tip.IsEmpty()) {
		AddBody(SNew(STextBlock).Text(FText::FromString(View.Tip)).AutoWrapText(true).ColorAndOpacity(Muted));
	}

	// clang-format off
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {16.0f, 14.0f, 16.0f, 12.0f})
		[
			Head
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			PjcFixLine()
		]
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot().Padding(FMargin {16.0f, 12.0f, 16.0f, 0.0f})
			[
				Body
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			PjcFixLine()
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {16.0f, 10.0f, 16.0f, 12.0f})
		[
			MakeFooter(View)
		];
	// clang-format on
}

TSharedRef<SWidget> SPjcFixWindow::MakeSection(const FPjcFixSection& Section) {
	const TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	// clang-format off
	Box->AddSlot().AutoHeight()
	[
		SNew(SBorder).BorderImage(PjcFixWhite()).BorderBackgroundColor(PjcWidgets::GetColor("ProjectCleaner.Color.Recess")).Padding(FMargin {10.0f, 5.0f})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 8.0f, 0.0f})
			[
				PjcFixDot(PjcWidgets::GetColor(Section.Color), 7.0f)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				PjcWidgets::MakeCardTitle(Section.Title)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(PjcFormat::Count(Section.Rows.Num()))).Font(PjcWidgets::GetFontMono(8)).ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		]
	];
	Box->AddSlot().AutoHeight()
	[
		PjcFixLine()
	];
	// clang-format on
	if (!Section.Note.IsEmpty()) {
		// clang-format off
		Box->AddSlot().AutoHeight().Padding(FMargin {10.0f, 6.0f})
		[
			SNew(STextBlock).Text(FText::FromString(Section.Note)).AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())
		];
		Box->AddSlot().AutoHeight()
		[
			PjcFixLine()
		];
		// clang-format on
	}

	const TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	for (int32 Index = 0; Index < Section.Rows.Num() && Index < PjcFixMaxRows; ++Index) {
		if (Index > 0) {
			// clang-format off
			Rows->AddSlot().AutoHeight()
			[
				SNew(SBox).HeightOverride(1.0f)
				[
					SNew(SImage).Image(PjcFixWhite()).ColorAndOpacity(PjcFixHex(TEXT("#2B2B2B")))
				]
			];
			// clang-format on
		}
		// clang-format off
		Rows->AddSlot().AutoHeight()
		[
			MakeRow(Section.Rows[Index])
		];
		// clang-format on
	}
	if (Section.Rows.Num() > PjcFixMaxRows) {
		const FString More = FString::Printf(TEXT("and %s more"), *PjcFormat::Count(Section.Rows.Num() - PjcFixMaxRows));
		// clang-format off
		Rows->AddSlot().AutoHeight().Padding(FMargin {10.0f, 5.0f})
		[
			SNew(STextBlock).Text(FText::FromString(More)).ColorAndOpacity(FSlateColor::UseSubduedForeground())
		];
		// clang-format on
	}
	// clang-format off
	Box->AddSlot().AutoHeight()
	[
		SNew(SBox).MaxDesiredHeight(200.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				Rows
			]
		]
	];
	// clang-format on
	return PjcFixBox(PjcWidgets::GetColor("ProjectCleaner.Color.Line"), PjcFixHex(TEXT("#1F1F1F")), FMargin {0.0f}, Box);
}

TSharedRef<SWidget> SPjcFixWindow::MakeRow(const FPjcFixRow& Row) {
	const FSlateColor Muted = FSlateColor::UseSubduedForeground();
	const FSlateFontInfo Small = FPjcStyles::GetFont("Regular", 8);

	const TSharedRef<SHorizontalBox> NameLine = SNew(SHorizontalBox);
	// clang-format off
	NameLine->AddSlot().AutoWidth()
	[
		SNew(STextBlock).Text(FText::FromString(Row.Name)).Font(FPjcStyles::GetFont("Bold", 9))
	];
	// clang-format on
	if (Row.bUnsaved) {
		// clang-format off
		NameLine->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {6.0f, 0.0f, 0.0f, 0.0f})
		[
			PjcFixDot(PjcWidgets::GetColor("ProjectCleaner.Color.Yellow"), 6.0f)
		];
		// clang-format on
	}
	if (!Row.Target.IsEmpty()) {
		const FSlateColor TargetColor
			= Row.bTargetMissing ? FSlateColor {PjcWidgets::GetColor("ProjectCleaner.Color.Red")} : FSlateColor::UseForeground();
		// clang-format off
		NameLine->AddSlot().AutoWidth().Padding(FMargin {6.0f, 0.0f})
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("\u2192"))).ColorAndOpacity(Muted)
		];
		NameLine->AddSlot().AutoWidth()
		[
			SNew(STextBlock).Text(FText::FromString(Row.Target)).ColorAndOpacity(TargetColor)
		];
		// clang-format on
	}

	const TSharedRef<SVerticalBox> Lines = SNew(SVerticalBox);
	// clang-format off
	Lines->AddSlot().AutoHeight()
	[
		NameLine
	];
	// clang-format on
	if (!Row.Detail.IsEmpty()) {
		const FSlateColor DetailColor = Row.DetailColor.IsNone() ? Muted : FSlateColor {PjcWidgets::GetColor(Row.DetailColor)};
		// clang-format off
		Lines->AddSlot().AutoHeight().Padding(FMargin {0.0f, 1.0f, 0.0f, 0.0f})
		[
			SNew(STextBlock).Text(FText::FromString(Row.Detail)).AutoWrapText(true).Font(Small).ColorAndOpacity(DetailColor)
		];
		// clang-format on
	}
	if (!Row.Hint.IsEmpty()) {
		// clang-format off
		Lines->AddSlot().AutoHeight().Padding(FMargin {0.0f, 1.0f, 0.0f, 0.0f})
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("\u2192 ") + Row.Hint)).AutoWrapText(true).Font(Small).ColorAndOpacity(PjcFixHex(TEXT("#9AA0A8")))
		];
		// clang-format on
	}

	const TSharedRef<SHorizontalBox> Right = SNew(SHorizontalBox);
	if (!Row.Side.IsEmpty()) {
		// clang-format off
		Right->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {8.0f, 0.0f, 0.0f, 0.0f})
		[
			SNew(STextBlock).Text(FText::FromString(Row.Side)).Font(Small).ColorAndOpacity(Muted)
		];
		// clang-format on
	}
	if (!Row.Pill.IsEmpty()) {
		const FLinearColor PillColor = PjcWidgets::GetColor(Row.PillColor);
		// clang-format off
		Right->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {8.0f, 0.0f, 0.0f, 0.0f})
		[
			SNew(SBorder)
			.BorderImage(FPjcStyles::Get().GetBrush("ProjectCleaner.Pill"))
			.BorderBackgroundColor(PillColor.CopyWithNewOpacity(0.16f))
			.Padding(FMargin {8.0f, 1.0f})
			[
				SNew(STextBlock).Text(FText::FromString(Row.Pill)).Font(Small).ColorAndOpacity(PillColor)
			]
		];
		// clang-format on
	}
	if (!Row.Button.IsEmpty() && Row.OnButton) {
		const TFunction<void()> OnButton = Row.OnButton;
		const bool bCloses = Row.bButtonCloses;
		// clang-format off
		Right->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {8.0f, 0.0f, 0.0f, 0.0f})
		[
			SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
			.ButtonStyle(&PjcShim::GetButtonStyle427(false))
			.ForegroundColor(FSlateColor::UseForeground())
#endif
			.ContentPadding(FMargin {8.0f, 0.0f})
			.OnClicked_Lambda([this, OnButton, bCloses]() {
				if (bCloses) {
					AfterClose = OnButton;
					Close(false);
				}
				else {
					OnButton();
				}
				return FReply::Handled();
			})
			[
				SNew(STextBlock).Text(FText::FromString(Row.Button)).Font(Small)
			]
		];
		// clang-format on
	}

	// clang-format off
	return SNew(SBox).Padding(FMargin {10.0f, 5.0f})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				Lines
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				Right
			]
		];
	// clang-format on
}

TSharedRef<SWidget> SPjcFixWindow::MakeFooter(const FPjcFixView& View) {
	FString VcsName;
	FString VcsDetail;
	const bool bVcs = PjcFixVersionControl(VcsName, VcsDetail);

	const TSharedRef<SHorizontalBox> Footer = SNew(SHorizontalBox);
	// clang-format off
	Footer->AddSlot().FillWidth(1.0f).VAlign(VAlign_Center)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 6.0f, 0.0f})
		[
			PjcFixDot(PjcWidgets::GetColor(bVcs ? "ProjectCleaner.Color.Ok" : "ProjectCleaner.Color.Yellow"), 8.0f)
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(FText::FromString(VcsName)).Font(FPjcStyles::GetFont("Bold", 9))
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(FText::FromString(TEXT(" \u00B7 ") + VcsDetail)).ColorAndOpacity(FSlateColor::UseSubduedForeground()).Clipping(EWidgetClipping::ClipToBounds)
		]
	];
	// clang-format on
	if (View.bCancel) {
		// clang-format off
		Footer->AddSlot().AutoWidth().Padding(FMargin {8.0f, 0.0f, 0.0f, 0.0f})
		[
			SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
			.ButtonStyle(&PjcShim::GetButtonStyle427(false))
			.ForegroundColor(FSlateColor::UseForeground())
#endif
			.ContentPadding(FMargin {14.0f, 3.0f})
			.OnClicked_Lambda([this]() {
				Close(false);
				return FReply::Handled();
			})
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Cancel")))
			]
		];
		// clang-format on
	}
	const FString Confirm = View.Confirm.IsEmpty() ? FString {TEXT("OK")} : View.Confirm;
	const bool bEnabled = View.bConfirmEnabled;
	// clang-format off
	Footer->AddSlot().AutoWidth().Padding(FMargin {8.0f, 0.0f, 0.0f, 0.0f})
	[
		SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
		.ForegroundColor(FSlateColor::UseForeground())
#endif
		.ButtonStyle(View.bConfirmDanger ? &PjcShim::GetDangerButtonStyle() : &PjcShim::GetPrimaryButtonStyle())
		.ContentPadding(FMargin {14.0f, 3.0f})
		.IsEnabled(bEnabled)
		.OnClicked_Lambda([this]() {
			Close(true);
			return FReply::Handled();
		})
		[
			SNew(STextBlock).Text(FText::FromString(Confirm)).ColorAndOpacity(FLinearColor::White)
		]
	];
	// clang-format on
	return Footer;
}
