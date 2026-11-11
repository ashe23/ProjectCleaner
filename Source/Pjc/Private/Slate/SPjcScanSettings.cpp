// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcScanSettings.h"
#include "Slate/PjcWidgets.h"
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
#include "Widgets/Input/SMenuAnchor.h"
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

	// clang-format off
	ChildSlot
	[
		SAssignNew(Body, SBox)
	];
	// clang-format on
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
	NumFolderLabelUnused = Result.FolderLabelUnused.Num();
	SizeFolderLabelUnused = 0;
	for (const auto& Package : Result.FolderLabelUnused) {
		SizeFolderLabelUnused += Result.GetSize(Package);
	}
	for (const auto& Asset : Result.Assets) {
		const FPjcAssetUsage* Usage = Result.Usage.Find(Asset.PackageName);
		if (!Usage) continue;

		NumFolderLabelOnly += Usage->bFolderLabelOnly ? 1 : 0;
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
	const FString UnregisteredKeep = Edited.bUnregisteredPrimaryUsed ? TEXT("Keeps") : TEXT("Would keep");

	const TSharedRef<SVerticalBox> Scope = SNew(SVerticalBox);
	// clang-format off
	Scope->AddSlot().AutoHeight()
	[
		MakeSwitchRow(
			TEXT("Project plugins"),
			&FPjcScanDraft::bScanProjectPlugins,
			Edited.bScanProjectPlugins ? NumProjectPlugins : -1,
			TEXT("Content of project plugins is scanned too, not only /Game."),
			Edited.bScanProjectPlugins ? FString::Printf(TEXT("%s project plugins with content"), *PjcFormat::Count(NumProjectPlugins)) : FString {}
		)
	];
	// clang-format on
	if (Edited.bScanProjectPlugins) {
		// clang-format off
		Scope->AddSlot().AutoHeight()
		[
			MakeListRows(
				TEXT("IgnoredPlugins"),
				TEXT("Ignored plugins"),
				&FPjcScanDraft::IgnoredPlugins,
				TEXT("Plugin name\u2026"),
				TEXT("Plugins left out of the scan. Nothing in them is reported as unused."),
				14.0f
			)
		];
		// clang-format on
	}

	const int32 NumUnregistered = Roots(EPjcUsageReason::PrimaryClass);
	const FString UnregisteredUsed = FString::Printf(TEXT("%s %s assets used"), *UnregisteredKeep, *PjcFormat::Count(NumUnregistered));
	const FString UnregisteredMore = NumUnregistered > 0 ? UnregisteredUsed : TEXT("None in the last scan");
	const FString LabelCount = PjcFormat::Count(NumFolderLabelUnused);
	const FString LabelSize = PjcFormat::Size(SizeFolderLabelUnused);
	const FString LabelText = FString::Printf(TEXT("%s assets in label folders (%s) follow their references"), *LabelCount, *LabelSize);
	const FString LabelUnused = NumFolderLabelUnused > 0 ? LabelText : FString {};
	const TSharedRef<SVerticalBox> Used = SNew(SVerticalBox);
	// clang-format off
	Used->AddSlot().AutoHeight()
	[
		MakeFixedRow(
			TEXT("Primary assets"),
			Roots(EPjcUsageReason::Primary),
			TEXT("primary assets"),
			TEXT("Maps and the assets the Asset Manager registers count as used and start chains, ")
				TEXT("with the assets and collections Primary Asset Labels list.")
		)
	];
	Used->AddSlot().AutoHeight()
	[
		MakeFixedRow(
			TEXT("Editor assets"),
			Roots(EPjcUsageReason::Editor),
			TEXT("editor assets"),
			TEXT("Editor utilities, validators, scriptable tools and tool data such as bookmarks, presets and poses count as used, ")
				TEXT("shown as editor only.")
		)
	];
	Used->AddSlot().AutoHeight()
	[
		MakeFixedRow(
			TEXT("Splash content"),
			Roots(EPjcUsageReason::Splash),
			TEXT("splash assets"),
			TEXT("Assets in Content/Splash count as used, shown as editor only: the editor imports the splash images there ")
				TEXT("as textures, and imports them again when they are deleted.")
		)
	];
	Used->AddSlot().AutoHeight()
	[
		MakeFixedRow(
			TEXT("Source & config"),
			Roots(EPjcUsageReason::Indirect),
			TEXT("assets named"),
			TEXT("Assets named in C++, C#, Python and .ini files count as used, platform configs included.")
		)
	];
	Used->AddSlot().AutoHeight()
	[
		MakeFixedRow(
			TEXT("Config folders"),
			Roots(EPjcUsageReason::ConfigFolder),
			TEXT("assets in config folders"),
			TEXT("Folders set to always cook and Data Registry scan folders keep every asset inside used.")
		)
	];
	Used->AddSlot().AutoHeight()
	[
		MakeFixedRow(
			TEXT("Localized assets"),
			NumLocalized,
			TEXT("localized assets"),
			TEXT("Language copies in L10N folders count as used when their original is used.")
		)
	];
	Used->AddSlot().AutoHeight()
	[
		MakeSwitchRow(
			TEXT("Unregistered primary classes"),
			&FPjcScanDraft::bUnregisteredPrimaryUsed,
			NumUnregistered,
			TEXT("Assets of a primary asset class that the Asset Manager did not register count as used: ")
				TEXT("outside its folders, or in a game feature plugin that is not registered yet."),
			UnregisteredMore
		)
	];
	Used->AddSlot().AutoHeight()
	[
		MakeFixedRow(
			TEXT("Folder labels"),
			NumFolderLabelOnly,
			TEXT("assets in packaged label folders"),
			TEXT("Primary Asset Labels with Label Assets In My Directory count the assets in their folder as used ")
				TEXT("when the label's cook rule packages them: Always Cook or a Development Always rule, on a label type ")
				TEXT("that is not editor only. Other labels only sort assets into chunks, ")
				TEXT("so their assets count as used only if something references them."),
			LabelUnused
		)
	];
	Used->AddSlot().AutoHeight()
	[
		MakeSwitchRow(
			TEXT("Cinematics"),
			&FPjcScanDraft::bCinematicAssetsUsed,
			-1,
			TEXT("Level Sequences, Template Sequences and Movie Render configs count as used."),
			FString {}
		)
	];
	// clang-format on
	const TSharedRef<SVerticalBox> Exclusions = SNew(SVerticalBox);
	// clang-format off
	Exclusions->AddSlot().AutoHeight()
	[
		MakeListRows(
			TEXT("ExcludedFolders"),
			TEXT("Folders"),
			&FPjcScanDraft::ExcludedFolders,
			TEXT("Folder, e.g. /Game/Keep"),
			TEXT("Every asset in these folders is kept.")
		)
	];
	Exclusions->AddSlot().AutoHeight()
	[
		MakeListRows(
			TEXT("ExcludedClasses"),
			TEXT("Classes"),
			&FPjcScanDraft::ExcludedClasses,
			TEXT("Class path, e.g. /Script/Engine.Texture2D"),
			TEXT("Assets of these classes are kept.")
		)
	];
	Exclusions->AddSlot().AutoHeight()
	[
		MakeListRows(
			TEXT("ExcludedAssets"),
			TEXT("Assets"),
			&FPjcScanDraft::ExcludedAssets,
			TEXT("Asset path, e.g. /Game/Maps/L_Main.L_Main"),
			TEXT("These assets count as used and are never deleted.")
		)
	];

	Used->AddSlot().AutoHeight()
	[
		MakeListRows(
			TEXT("UsedClasses"),
			TEXT("Always used classes"),
			&FPjcScanDraft::UsedClasses,
			TEXT("Class path, e.g. /Script/Engine.CurveFloat"),
			TEXT("Assets of these classes, and what they reference, count as used.")
		)
	];

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
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 6.0f})
		[
			MakeCategory(TEXT("Exclusions"), TEXT("Exclusions"), TEXT("Always kept, never deleted"), Exclusions)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {8.0f, 6.0f})
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Shared with the team \u00B7 DefaultEditor.ini")))
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
			FPjcScanDraft::NotifyEdited();
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
#if ENGINE_MAJOR_VERSION < 5
			.ForegroundColor(FSlateColor::UseForeground())
#endif
			.ButtonStyle(&PjcShim::GetStyle().GetWidgetStyle<FButtonStyle>(TEXT("NoBorder")))
			.ContentPadding(FMargin {4.0f, 0.0f})
			.ToolTipText(FText::FromString(TEXT("Hide scan settings")))
			.OnClicked_Lambda([this]() {
				OnClose.ExecuteIfBound();
				return FReply::Handled();
			})
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("\u00AB"))).ColorAndOpacity(FSlateColor::UseSubduedForeground())
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
			SNew(STextBlock).Text(FText::FromString(Title)).ToolTipText(FText::FromString(ToolTip)).Font(FPjcStyles::GetFont("Bold", 10))
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

TSharedRef<SWidget>
SPjcScanSettings::MakeFixedRow(const FString& Label, const int32 Num, const FString& What, const FString& Description, const FString& Extra) const {
	const FString Found = Num > 0 ? FString::Printf(TEXT("%s %s"), *PjcFormat::Count(Num), *What) : TEXT("No ") + What;
	const FString ExtraLine = Extra.IsEmpty() ? FString {} : Extra + TEXT("\n");
	const FString More = Found + TEXT(" in the last scan\n") + ExtraLine + TEXT("Always on");
	// clang-format off
	const TSharedRef<SWidget> Lock = SNew(SBox).WidthOverride(12.0f).HeightOverride(12.0f).ToolTipText(FText::FromString(TEXT("Always on")))
		[
			SNew(SImage).Image(PjcShim::GetIconLock()).ColorAndOpacity(FSlateColor::UseSubduedForeground())
		];
	// clang-format on

	return MakeRow(Label, Num > 0 ? PjcFormat::Count(Num) : TEXT("\u2013"), Lock, PjcWidgets::MakeToolTip(Label, Description, More), false, false);
}

TSharedRef<SWidget>
SPjcScanSettings::MakeSwitchRow(const FString& Label, FDraftFlag Flag, const int32 Num, const FString& Description, const FString& More) {
	const bool bValue = FPjcScanDraft::Get().*Flag;
	const bool bDefault = FPjcScanDraft {}.*Flag;
	const bool bChanged = bValue != bDefault;
	const FString MoreLine = More.IsEmpty() ? FString {} : More + TEXT("\n");
	const FString DefaultText = FString::Printf(TEXT("Default: %s"), bDefault ? TEXT("on") : TEXT("off"));
	const FString Lines = MoreLine + DefaultText + (bChanged ? TEXT(" \u00B7 changed") : TEXT(""));

	// clang-format off
	const TSharedRef<SWidget> Switch = SNew(SCheckBox)
		.IsChecked(bValue ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
		.OnCheckStateChanged_Lambda([this, Flag](const ECheckBoxState State) {
			FPjcScanDraft::Get().*Flag = State == ECheckBoxState::Checked;
			FPjcScanDraft::NotifyEdited();
		});
	// clang-format on

	const FString Value = Num < 0 ? FString {} : (Num > 0 ? PjcFormat::Count(Num) : TEXT("\u2013"));
	return MakeRow(Label, Value, Switch, PjcWidgets::MakeToolTip(Label, Description, Lines), bChanged, !bValue);
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

	// + opens the picker, floating over the rows below (above them when there is no room)
	const bool bTypedIsPath = Id == FName {TEXT("ExcludedFolders")} || Id == FName {TEXT("ExcludedAssets")};
	const bool bTypedAllowed = Id != FName {TEXT("IgnoredPlugins")};
	const int32 MaxBeforeTyping = Id == FName {TEXT("ExcludedAssets")} ? 20 : 0;
	const auto MakePicker = [this, Id, List, Placeholder, bTypedIsPath, bTypedAllowed, MaxBeforeTyping]() -> TSharedRef<SWidget> {
		TArray<FPjcPickerItem> PickerItems;
		GetPickerItems(Id, PickerItems);
		// clang-format off
		const TSharedRef<SPjcListPicker> NewPicker = SNew(SPjcListPicker)
			.Items(PickerItems)
			.Added(FPjcScanDraft::Get().*List)
			.Hint(Placeholder)
			.Query(PickerQuery)
			.MaxBeforeTyping(MaxBeforeTyping)
			.bAllowTyped(bTypedAllowed)
			.bTypedIsPath(bTypedIsPath)
			.OnPicked_Lambda([this, List](const FString& Value) {
				// a class name typed by hand is stored as its path
				const bool bClassList = List == &FPjcScanDraft::UsedClasses || List == &FPjcScanDraft::ExcludedClasses;
				const UClass* TypedClass = bClassList && !Value.Contains(TEXT("/")) ? PjcShim::FindClass(Value) : nullptr;
				const TSharedPtr<SPjcListPicker> Open = Picker.Pin();
				PickerQuery = Open.IsValid() ? Open->GetQuery() : FString {};
				(FPjcScanDraft::Get().*List).AddUnique(TypedClass ? TypedClass->GetPathName() : Value);
				FPjcScanDraft::NotifyEdited();
			})
			.OnClose_Lambda([this]() {
				AddingList = NAME_None;
				PickerQuery.Reset();
				RequestRebuild();
			});
		// clang-format on
		Picker = NewPicker;
		return NewPicker;
	};
	TSharedPtr<SMenuAnchor> Anchor;
	// clang-format off
	SAssignNew(Anchor, SMenuAnchor)
		.Placement(MenuPlacement_BelowAnchor)
		.OnGetMenuContent_Lambda(MakePicker)
		.OnMenuOpenChanged_Lambda([this, Id](const bool bOpen) {
			if (!bOpen && AddingList == Id && !bRebuildPending) {
				AddingList = NAME_None;
				PickerQuery.Reset();
			}
		})
		[
			SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
			.ForegroundColor(FSlateColor::UseForeground())
#endif
			.ButtonStyle(&PjcShim::GetStyle().GetWidgetStyle<FButtonStyle>(TEXT("NoBorder")))
			.ContentPadding(FMargin {4.0f, 0.0f})
			.ToolTipText(FText::FromString(TEXT("Add")))
			.OnClicked_Lambda([this, Id]() {
				const bool bOpen = AddingList != Id;
				AddingList = bOpen ? Id : NAME_None;
				PickerQuery.Reset();
				RequestRebuild();
				return FReply::Handled();
			})
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("+"))).Font(FPjcStyles::GetFont("Bold", 11))
			]
		];
	// clang-format on
	const TSharedRef<SWidget> Add = Anchor.ToSharedRef();
	if (AddingList == Id) {
		// open once the rebuilt panel is shown
		const TWeakPtr<SMenuAnchor> WeakAnchor = Anchor;
		const auto OpenPicker = [WeakAnchor](double, float) {
			if (const TSharedPtr<SMenuAnchor> Pinned = WeakAnchor.Pin()) {
				Pinned->SetIsOpen(true);
			}
			return EActiveTimerReturnType::Stop;
		};
		RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateLambda(OpenPicker));
	}
	// clang-format off
	Rows->AddSlot().AutoHeight()
	[
		MakeRow(
			Label,
			Items.Num() > 0 ? PjcFormat::Count(Items.Num()) : FString {},
			Add,
			PjcWidgets::MakeToolTip(Label, Description, FString {}),
			false,
			false,
			Indent
		)
	];
	// clang-format on

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
#if ENGINE_MAJOR_VERSION < 5
					.ForegroundColor(FSlateColor::UseForeground())
#endif
					.ButtonStyle(&PjcShim::GetStyle().GetWidgetStyle<FButtonStyle>(TEXT("NoBorder")))
					.ContentPadding(FMargin {4.0f, 0.0f})
					.ToolTipText(FText::FromString(TEXT("Remove")))
					.OnClicked_Lambda([this, List, Item]() {
						(FPjcScanDraft::Get().*List).Remove(Item);
						FPjcScanDraft::NotifyEdited();
						return FReply::Handled();
					})
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("\u00D7"))).ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
				]
			]
		];
		// clang-format on
	}

	// a list of two or more can be emptied at once (Undo and Defaults still work)
	if (Items.Num() >= 2) {
		const FString Entries = PjcFormat::Count(Items.Num());
		const FString ClearTip = FString::Printf(TEXT("Removes all %s entries of %s. Other settings stay. Undo brings them back."), *Entries, *Label);
		// clang-format off
		Rows->AddSlot().AutoHeight().HAlign(HAlign_Left).Padding(FMargin {14.0f + Indent, 1.0f, 6.0f, 2.0f})
		[
			SNew(SHyperlink)
			.Text(FText::FromString(TEXT("Clear all")))
			.ToolTipText(FText::FromString(ClearTip))
			.OnNavigate_Lambda([List]() {
				(FPjcScanDraft::Get().*List).Reset();
				FPjcScanDraft::NotifyEdited();
			})
		];
		// clang-format on
	}

	return Rows;
}

void SPjcScanSettings::GetPickerItems(const FName Id, TArray<FPjcPickerItem>& OutItems) {
	OutItems.Reset();
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();

	// project plugins with content, with their assets
	if (Id == FName {TEXT("IgnoredPlugins")}) {
		for (const auto& Plugin : IPluginManager::Get().GetEnabledPluginsWithContent()) {
			if (Plugin->GetLoadedFrom() != EPluginLoadedFrom::Project) continue;

			TArray<FAssetData> PluginAssets;
			UPjcSubsystem::GetModuleAssetRegistry().Get().GetAssetsByPath(FName {*(TEXT("/") + Plugin->GetName())}, PluginAssets, true);
			FPjcPickerItem& Item = OutItems.AddDefaulted_GetRef();
			Item.Value = Plugin->GetName();
			Item.Meta = PjcFormat::Count(PluginAssets.Num()) + TEXT(" assets");
			Item.Sort = PluginAssets.Num();
		}
		return;
	}

	// classes with assets in the project, most assets first
	if (Id == FName {TEXT("UsedClasses")} || Id == FName {TEXT("ExcludedClasses")}) {
		TMap<FName, TPair<FString, int32>> Classes;
		for (const auto& Asset : Result.Assets) {
			const FName ClassName = PjcShim::GetAssetClassName(Asset);
			TPair<FString, int32>* Found = Classes.Find(ClassName);
			if (!Found) {
				const UClass* AssetClass = Asset.GetClass();
				Found = &Classes.Add(ClassName, TPair<FString, int32> {AssetClass ? AssetClass->GetPathName() : ClassName.ToString(), 0});
			}
			++Found->Value;
		}
		for (const auto& Pair : Classes) {
			FPjcPickerItem& Item = OutItems.AddDefaulted_GetRef();
			Item.Value = Pair.Value.Key;
			Item.Label = FSoftObjectPath {Pair.Value.Key}.GetAssetName();
			Item.Meta = PjcFormat::Count(Pair.Value.Value) + TEXT(" assets");
			Item.Sort = Pair.Value.Value;
		}
		OutItems.Sort([](const FPjcPickerItem& A, const FPjcPickerItem& B) {
			return A.Sort > B.Sort;
		});
		return;
	}

	// every folder with its assets, by path
	if (Id == FName {TEXT("ExcludedFolders")}) {
		TMap<FString, int32> Folders;
		for (const auto& Asset : Result.Assets) {
			FString Path = Asset.PackagePath.ToString();
			while (!Path.IsEmpty()) {
				++Folders.FindOrAdd(Path);
				int32 Slash = INDEX_NONE;
				if (!Path.FindLastChar(TEXT('/'), Slash) || Slash == 0) break;
				Path.LeftInline(Slash);
			}
		}
		Folders.KeySort([](const FString& A, const FString& B) {
			return A < B;
		});
		for (const auto& Pair : Folders) {
			FPjcPickerItem& Item = OutItems.AddDefaulted_GetRef();
			Item.Value = Pair.Key;
			Item.Meta = PjcFormat::Count(Pair.Value) + TEXT(" assets");
			Item.Sort = Pair.Value;
		}
		return;
	}

	// assets by name, with their class and size
	for (const auto& Asset : Result.Assets) {
		const int64 Size = Result.GetSize(Asset.PackageName);
		FPjcPickerItem& Item = OutItems.AddDefaulted_GetRef();
		Item.Value = PjcShim::GetObjectPathString(Asset);
		Item.Label = Asset.AssetName.ToString();
		Item.Meta = FString::Printf(TEXT("%s \u00B7 %s"), *PjcWidgets::GetAssetType(Asset).Label, *PjcFormat::Size(Size));
		Item.Sort = Size;
	}
}
