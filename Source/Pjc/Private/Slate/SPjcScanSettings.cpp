// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcScanSettings.h"
#include "PjcFormat.h"
#include "PjcShim.h"
#include "PjcStyles.h"
#include "PjcSubsystem.h"
// Engine Headers
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	// folded categories, shared by the panels of every tool
	TSet<FName> CollapsedCategories;

	FSlateColor ColorChanged() {
		return FSlateColor {FPjcStyles::Get().GetColor("ProjectCleaner.Color.Yellow")};
	}

	FSlateFontInfo FontMono() {
		return FCoreStyle::GetDefaultFontStyle("Mono", 9);
	}
}	// namespace

void SPjcScanSettings::Construct(const FArguments& InArgs) {
	OnClose = InArgs._OnClose;
	ScanFinishedHandle = UPjcSubsystem::OnScanFinished().AddSP(this, &SPjcScanSettings::OnScanFinished);
	DraftEditedHandle = FPjcScanDraft::OnEdited().AddSP(this, &SPjcScanSettings::RequestRebuild);

	ChildSlot[SAssignNew(Body, SBox)];
	OnScanFinished();
}

SPjcScanSettings::~SPjcScanSettings() {
	UPjcSubsystem::OnScanFinished().Remove(ScanFinishedHandle);
	FPjcScanDraft::OnEdited().Remove(DraftEditedHandle);
}

void SPjcScanSettings::OnScanFinished() {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();

	// assets that start chains, by their own reason
	NumRoots.Reset();
	NumLocalized = 0;
	NumFolderLabelOnly = 0;
	SizeFolderLabelOnly = 0;
	for (const auto& Asset : Result.Assets) {
		const FPjcAssetUsage* Usage = Result.Usage.Find(Asset.PackageName);
		if (!Usage) continue;

		if (Usage->bFolderLabelOnly) {
			++NumFolderLabelOnly;
			SizeFolderLabelOnly += Result.GetSize(Asset.PackageName);
		}
		if (Usage->Reason == EPjcUsageReason::Localized && Usage->bUsed) {
			++NumLocalized;
		}
		if (Usage->bUsed && Usage->Parent.IsNone()) {
			NumRoots.FindOrAdd(Usage->Reason)++;
		}
	}

	NumProjectPlugins = 0;
	for (const auto& Plugin : IPluginManager::Get().GetEnabledPluginsWithContent()) {
		NumProjectPlugins += Plugin->GetLoadedFrom() == EPluginLoadedFrom::Project ? 1 : 0;
	}

	Rebuild();
}

void SPjcScanSettings::RequestRebuild() {
	// next tick: the widget whose event asks for it is still running
	if (bRebuildPending) return;

	bRebuildPending = true;
	RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateLambda([this](double, float) {
							bRebuildPending = false;
							Rebuild();
							return EActiveTimerReturnType::Stop;
						}));
}

void SPjcScanSettings::Rebuild() {
	const FPjcScanDraft& Edited = FPjcScanDraft::Get();
	const auto Roots = [this](const EPjcUsageReason Reason) {
		const int32* Num = NumRoots.Find(Reason);
		return Num ? *Num : 0;
	};
	const FString LabelsKeep = Edited.bFolderLabelsUsed ? TEXT("Keeps") : TEXT("Would keep");
	const FString UnregisteredKeep = Edited.bUnregisteredPrimaryUsed ? TEXT("Keeps") : TEXT("Would keep");

	const TSharedRef<SVerticalBox> Scope = SNew(SVerticalBox);
	Scope->AddSlot().AutoHeight()[MakeSwitchRow(
		TEXT("Project plugins"),
		&FPjcScanDraft::bScanProjectPlugins,
		Edited.bScanProjectPlugins ? NumProjectPlugins : -1,
		TEXT("Content of project plugins is scanned too, not only /Game."),
		Edited.bScanProjectPlugins ? FString::Printf(TEXT("%s project plugins with content"), *PjcFormat::Count(NumProjectPlugins)) : FString {}
	)];
	if (Edited.bScanProjectPlugins) {
		Scope->AddSlot().AutoHeight()[MakeListRows(
			TEXT("IgnoredPlugins"),
			TEXT("Ignored plugins"),
			&FPjcScanDraft::IgnoredPlugins,
			TEXT("Plugin name…"),
			TEXT("Plugins left out of the scan. Nothing in them is reported as unused."),
			14.0f
		)];
	}

	const int32 NumUnregistered = Roots(EPjcUsageReason::PrimaryClass);
	const TSharedRef<SVerticalBox> Used = SNew(SVerticalBox);
	Used->AddSlot().AutoHeight()[MakeFixedRow(
		TEXT("Primary assets"),
		Roots(EPjcUsageReason::Primary),
		TEXT("primary assets"),
		TEXT("Maps and the assets the Asset Manager registers count as used and start chains, ")
			TEXT("with the assets and collections Primary Asset Labels list.")
	)];
	Used->AddSlot().AutoHeight()[MakeFixedRow(
		TEXT("Editor assets"),
		Roots(EPjcUsageReason::Editor),
		TEXT("editor assets"),
		TEXT("Editor utilities, validators, scriptable tools and tool data such as bookmarks, presets and poses count as used, shown as editor only.")
	)];
	Used->AddSlot().AutoHeight()[MakeFixedRow(
		TEXT("Source & config"),
		Roots(EPjcUsageReason::Indirect),
		TEXT("assets named"),
		TEXT("Assets named in C++, Python and .ini files count as used, platform configs included.")
	)];
	Used->AddSlot().AutoHeight()[MakeFixedRow(
		TEXT("Config folders"),
		Roots(EPjcUsageReason::ConfigFolder),
		TEXT("assets in config folders"),
		TEXT("Folders set to always cook and Data Registry scan folders keep every asset inside used.")
	)];
	Used->AddSlot().AutoHeight()[MakeFixedRow(
		TEXT("Localized assets"),
		NumLocalized,
		TEXT("localized assets"),
		TEXT("Language copies in L10N folders count as used when their original is used.")
	)];
	Used->AddSlot().AutoHeight()[MakeSwitchRow(
		TEXT("Unregistered primary classes"),
		&FPjcScanDraft::bUnregisteredPrimaryUsed,
		NumUnregistered,
		TEXT("Assets of a primary asset class that the Asset Manager did not register count as used: ")
			TEXT("outside its folders, or in a game feature plugin that is not registered yet."),
		NumUnregistered > 0 ? FString::Printf(TEXT("%s %s assets used"), *UnregisteredKeep, *PjcFormat::Count(NumUnregistered))
							: TEXT("None in the last scan")
	)];
	Used->AddSlot().AutoHeight()[MakeSwitchRow(
		TEXT("Folder labels"),
		&FPjcScanDraft::bFolderLabelsUsed,
		NumFolderLabelOnly,
		TEXT("Primary Asset Labels with Label Assets In My Directory keep every asset in their folder used."),
		FString::Printf(TEXT("%s %s assets used (%s)"), *LabelsKeep, *PjcFormat::Count(NumFolderLabelOnly), *PjcFormat::Size(SizeFolderLabelOnly))
	)];
	Used->AddSlot().AutoHeight()[MakeSwitchRow(
		TEXT("Cinematics"),
		&FPjcScanDraft::bCinematicAssetsUsed,
		-1,
		TEXT("Level Sequences and Movie Render Queue configs count as used."),
		FString {}
	)];
	Used->AddSlot().AutoHeight()[MakeListRows(
		TEXT("UsedClasses"),
		TEXT("Always used classes"),
		&FPjcScanDraft::UsedClasses,
		TEXT("Class path, e.g. /Script/Engine.CurveFloat"),
		TEXT("Assets of these classes, and what they reference, count as used.")
	)];

	// clang-format off
	Body->SetContent(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {6.0f, 6.0f, 4.0f, 8.0f})
		[
			MakeHeader()
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 6.0f})
		[
			MakeCategory(TEXT("Scope"), TEXT("Scan scope"), TEXT("What is scanned"), Scope)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 6.0f})
		[
			MakeCategory(TEXT("Used"), TEXT("Counted as used"), TEXT("Rules that keep assets used. Locked rules are always on."), Used)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {8.0f, 6.0f})
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Shared with the team · DefaultEditor.ini")))
			.ToolTipText(FText::FromString(TEXT("Settings are saved in Config/DefaultEditor.ini, so the team shares them through source control")))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			.Font(FPjcStyles::GetFont("Regular", 8))
		]
	);
	// clang-format on
}

TSharedRef<SWidget> SPjcScanSettings::MakeHeader() {
	const auto Edit = [this](TFunction<void()> Change) {
		return [this, Change]() {
			Change();
			FPjcScanDraft::OnEdited().Broadcast();
		};
	};

	// clang-format off
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("SCAN SETTINGS")))
			.Font(FPjcStyles::GetFont("Bold", 9))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 10.0f, 0.0f})
		[
			SNew(SHyperlink)
			.Text(FText::FromString(TEXT("Undo")))
			.ToolTipText(FText::FromString(TEXT("Back to the settings of the last scan")))
			.Visibility_Lambda([]() { return FPjcScanDraft::IsChanged() ? EVisibility::Visible : EVisibility::Collapsed; })
			.OnNavigate_Lambda(Edit([]() { FPjcScanDraft::Revert(); }))
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 8.0f, 0.0f})
		[
			SNew(SHyperlink)
			.Text(FText::FromString(TEXT("Defaults")))
			.ToolTipText(FText::FromString(TEXT("Back to the plugin defaults")))
			.OnNavigate_Lambda(Edit([]() { FPjcScanDraft::Get() = FPjcScanDraft {}; }))
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SButton)
			.ButtonStyle(&PjcShim::GetStyle().GetWidgetStyle<FButtonStyle>(TEXT("NoBorder")))
			.ContentPadding(FMargin {4.0f, 0.0f})
			.ToolTipText(FText::FromString(TEXT("Hide scan settings")))
			.OnClicked_Lambda([this]() {
				OnClose.ExecuteIfBound();
				return FReply::Handled();
			})
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("«"))).ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		];
	// clang-format on
}

TSharedRef<SWidget>
SPjcScanSettings::MakeCategory(const FName Id, const FString& Title, const FString& ToolTip, const TSharedRef<SWidget>& Content) const {
	// clang-format off
	return SNew(SExpandableArea)
		.BorderImage(PjcShim::GetBrush(TEXT("DetailsView.CategoryTop")))
		.BodyBorderImage(FCoreStyle::Get().GetBrush("NoBrush"))
		.HeaderPadding(FMargin {4.0f, 3.0f})
		.Padding(FMargin {0.0f, 2.0f, 0.0f, 0.0f})
		.InitiallyCollapsed(CollapsedCategories.Contains(Id))
		.OnAreaExpansionChanged_Lambda([Id](const bool bExpanded) {
			if (bExpanded) {
				CollapsedCategories.Remove(Id);
			}
			else {
				CollapsedCategories.Add(Id);
			}
		})
		.HeaderContent()
		[
			SNew(STextBlock)
			.Text(FText::FromString(Title))
			.ToolTipText(FText::FromString(ToolTip))
			.Font(FPjcStyles::GetFont("Bold", 10))
		]
		.BodyContent()
		[
			Content
		];
	// clang-format on
}

TSharedRef<SWidget> SPjcScanSettings::MakeRow(
	const FString& Label,
	const FString& Value,
	const TSharedRef<SWidget>& Control,
	const TSharedRef<SToolTip>& ToolTip,
	const bool bChanged,
	const bool bDim,
	const float Indent
) const {
	// clang-format off
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("NoBrush"))
		.Padding(0.0f)
		.ToolTip(ToolTip)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				// changed from the default
				SNew(SBox).WidthOverride(2.0f)
				[
					SNew(SImage)
					.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.ColorAndOpacity(bChanged ? ColorChanged() : FSlateColor {FLinearColor::Transparent})
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(FMargin {6.0f + Indent, 4.0f, 6.0f, 4.0f})
			[
				SNew(STextBlock).Text(FText::FromString(Label))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {4.0f, 0.0f})
			[
				SNew(STextBlock)
				.Text(FText::FromString(Value))
				.Font(FontMono())
				.ColorAndOpacity(bDim ? FSlateColor::UseSubduedForeground() : FSlateColor::UseForeground())
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(32.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					Control
				]
			]
		];
	// clang-format on
}

TSharedRef<SWidget> SPjcScanSettings::MakeFixedRow(const FString& Label, const int32 Num, const FString& What, const FString& Description) const {
	const FString More
		= (Num > 0 ? FString::Printf(TEXT("%s %s"), *PjcFormat::Count(Num), *What) : TEXT("No ") + What) + TEXT(" in the last scan\nAlways on");
	const TSharedRef<SWidget> Lock = SNew(SBox).WidthOverride(12.0f).HeightOverride(12.0f).ToolTipText(
		FText::FromString(TEXT("Always on"))
	)[SNew(SImage).Image(PjcShim::GetIconLock()).ColorAndOpacity(FSlateColor::UseSubduedForeground())];

	return MakeRow(Label, Num > 0 ? PjcFormat::Count(Num) : TEXT("–"), Lock, MakeToolTip(Label, Description, More), false, false);
}

TSharedRef<SWidget>
SPjcScanSettings::MakeSwitchRow(const FString& Label, FDraftFlag Flag, const int32 Num, const FString& Description, const FString& More) {
	const bool bValue = FPjcScanDraft::Get().*Flag;
	const bool bDefault = FPjcScanDraft {}.*Flag;
	const bool bChanged = bValue != bDefault;
	const FString Lines = (More.IsEmpty() ? FString {} : More + TEXT("\n"))
		+ FString::Printf(TEXT("Default: %s"), bDefault ? TEXT("on") : TEXT("off")) + (bChanged ? TEXT(" · changed") : TEXT(""));

	const TSharedRef<SWidget> Switch = SNew(SCheckBox)
										   .IsChecked(bValue ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
										   .OnCheckStateChanged_Lambda([this, Flag](const ECheckBoxState State) {
											   FPjcScanDraft::Get().*Flag = State == ECheckBoxState::Checked;
											   FPjcScanDraft::OnEdited().Broadcast();
										   });

	const FString Value = Num < 0 ? FString {} : (Num > 0 ? PjcFormat::Count(Num) : TEXT("–"));
	return MakeRow(Label, Value, Switch, MakeToolTip(Label, Description, Lines), bChanged, !bValue);
}

TSharedRef<SWidget> SPjcScanSettings::MakeListRows(
	const FName Id,
	const FString& Label,
	FDraftList List,
	const FString& Placeholder,
	const FString& Description,
	const float Indent
) {
	const TArray<FString>& Items = FPjcScanDraft::Get().*List;
	const TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);

	// clang-format off
	const TSharedRef<SWidget> Add = SNew(SButton)
		.ButtonStyle(&PjcShim::GetStyle().GetWidgetStyle<FButtonStyle>(TEXT("NoBorder")))
		.ContentPadding(FMargin {4.0f, 0.0f})
		.ToolTipText(FText::FromString(TEXT("Add")))
		.OnClicked_Lambda([this, Id]() {
			AddingList = AddingList == Id ? NAME_None : Id;
			RequestRebuild();
			return FReply::Handled();
		})
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("+"))).Font(FPjcStyles::GetFont("Bold", 11))
		];
	// clang-format on

	Rows->AddSlot().AutoHeight()[MakeRow(
		Label, Items.Num() > 0 ? PjcFormat::Count(Items.Num()) : FString {}, Add, MakeToolTip(Label, Description, FString {}), false, false, Indent
	)];

	for (int32 Index = 0; Index < Items.Num(); ++Index) {
		const FString Item = Items[Index];
		// clang-format off
		Rows->AddSlot().AutoHeight().HAlign(HAlign_Left).Padding(FMargin {14.0f + Indent, 1.0f, 6.0f, 1.0f})
		[
			SNew(SBorder)
			.BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.DarkGroupBorder")))
			.Padding(FMargin {6.0f, 1.0f, 2.0f, 1.0f})
			.ToolTipText(FText::FromString(Item))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).MaxDesiredWidth(220.0f)
					[
						SNew(STextBlock).Text(FText::FromString(Item)).Font(FontMono())
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(&PjcShim::GetStyle().GetWidgetStyle<FButtonStyle>(TEXT("NoBorder")))
					.ContentPadding(FMargin {4.0f, 0.0f})
					.ToolTipText(FText::FromString(TEXT("Remove")))
					.OnClicked_Lambda([this, List, Item]() {
						(FPjcScanDraft::Get().*List).Remove(Item);
						FPjcScanDraft::OnEdited().Broadcast();
						return FReply::Handled();
					})
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("×"))).ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
				]
			]
		];
		// clang-format on
	}

	if (AddingList == Id) {
		TSharedPtr<SEditableTextBox> Input;
		// clang-format off
		Rows->AddSlot().AutoHeight().Padding(FMargin {14.0f + Indent, 2.0f, 6.0f, 4.0f})
		[
			SAssignNew(Input, SEditableTextBox)
			.HintText(FText::FromString(Placeholder))
			.OnTextCommitted_Lambda([this, List](const FText& Text, const ETextCommit::Type CommitType) {
				const FString Value = Text.ToString().TrimStartAndEnd();
				if (CommitType == ETextCommit::OnEnter && !Value.IsEmpty()) {
					(FPjcScanDraft::Get().*List).AddUnique(Value);
					FPjcScanDraft::OnEdited().Broadcast();
				}
				else if (Value.IsEmpty() || CommitType == ETextCommit::OnCleared) {
					AddingList = NAME_None;
				}
				else {
					return;
				}
				RequestRebuild();
			})
		];
		// clang-format on
		FSlateApplication::Get().SetKeyboardFocus(Input, EFocusCause::SetDirectly);
	}

	return Rows;
}

TSharedRef<SToolTip> SPjcScanSettings::MakeToolTip(const FString& Title, const FString& Text, const FString& More) {
	const TSharedRef<SVerticalBox> Lines = SNew(SVerticalBox);
	Lines->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Title)).Font(FPjcStyles::GetFont("Bold", 9))];
	Lines->AddSlot().AutoHeight().Padding(FMargin {0.0f, 3.0f, 0.0f, 0.0f})[SNew(STextBlock).Text(FText::FromString(Text)).WrapTextAt(280.0f)];
	if (!More.IsEmpty()) {
		Lines->AddSlot().AutoHeight().Padding(
			FMargin {0.0f, 5.0f, 0.0f, 0.0f}
		)[SNew(STextBlock).Text(FText::FromString(More)).ColorAndOpacity(FSlateColor::UseSubduedForeground())];
	}
	return SNew(SToolTip)[Lines];
}
