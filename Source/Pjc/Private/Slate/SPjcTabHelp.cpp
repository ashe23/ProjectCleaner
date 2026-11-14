// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabHelp.h"
#include "Slate/PjcWidgets.h"
#include "PjcConstants.h"
#include "PjcFormat.h"
#include "PjcScanDraft.h"
#include "PjcShim.h"
#include "PjcStyles.h"
#include "PjcSubsystem.h"
// Engine Headers
#include "DesktopPlatformModule.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformApplicationMisc.h"
#include "HAL/PlatformOutputDevices.h"
#include "Interfaces/IPluginManager.h"
#include "Internationalization/Regex.h"
#include "Misc/App.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	// unity builds join files: names unique to this file

	struct FPjcHelpLink
	{
		const TCHAR* Icon;
		const TCHAR* Title;
		const TCHAR* Tip;
		FString Url;
		// opens Url when unbound
		TFunction<void()> OnOpen;
	};

	// longer new issue links are cut by browsers: the report goes to the clipboard instead
	constexpr int32 PjcHelpMaxUrl = 6000;

	// percent encoding of the UTF-8 bytes, for a query value
	FString PjcHelpUrlEncode(const FString& Text) {
		const FTCHARToUTF8 Utf8 {*Text};
		FString Encoded;
		for (int32 Index = 0; Index < Utf8.Length(); ++Index) {
			const uint8 Byte = static_cast<uint8>(Utf8.Get()[Index]);
			const bool bPlain = FChar::IsAlnum(Byte) || Byte == '-' || Byte == '_' || Byte == '.' || Byte == '~';
			Encoded += bPlain ? FString::Chr(static_cast<TCHAR>(Byte)) : FString::Printf(TEXT("%%%02X"), Byte);
		}
		return Encoded;
	}

	FString PjcHelpJoined(const TArray<FString>& Items) {
		return Items.Num() > 0 ? FString::Join(Items, TEXT(", ")) : FString {TEXT("(none)")};
	}

	const TCHAR* PjcHelpOnOff(const bool bOn) {
		return bOn ? TEXT("on") : TEXT("off");
	}

	TSharedRef<SWidget> PjcHelpCard(const FPjcHelpLink& Link) {
		const FString Url = Link.Url;
		const TFunction<void()> OnOpen = Link.OnOpen;
		const FString Address = Url.Replace(TEXT("https://"), TEXT("")).Replace(TEXT("www."), TEXT(""));
		const FString Shown = Address.Len() > 33 ? Address.Left(32) + TEXT("\u2026") : Address;
		const FLinearColor Accent = PjcWidgets::GetColor("ProjectCleaner.Color.Accent");
		const FButtonStyle& ButtonStyle = FPjcStyles::Get().GetWidgetStyle<FButtonStyle>("ProjectCleaner.Button.Row");
		const auto OnClicked = [Url, OnOpen]() {
			if (OnOpen) {
				OnOpen();
			}
			else {
				FPlatformProcess::LaunchURL(*Url, nullptr, nullptr);
			}
			return FReply::Handled();
		};
		// clang-format off
		return SNew(SBox).WidthOverride(250.0f)
			[
				SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
				.ForegroundColor(FSlateColor::UseForeground())
#endif
				.ButtonStyle(&ButtonStyle)
				.ContentPadding(FMargin {0.0f})
				.ToolTip(PjcWidgets::MakeToolTip(Link.Title, Link.Tip, Url))
				.OnClicked_Lambda(OnClicked)
				[
					PjcWidgets::MakeCard(
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin {0.0f, 0.0f, 8.0f, 0.0f})
							[
								SNew(STextBlock).Text(FText::FromString(Link.Icon)).Font(PjcWidgets::GetFontMono(11)).ColorAndOpacity(Accent)
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f)
							[
								SNew(STextBlock).Text(FText::FromString(Link.Title)).Font(FPjcStyles::GetFont("Bold", 10))
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 4.0f})
						[
							SNew(STextBlock)
							.Text(FText::FromString(Shown))
							.ColorAndOpacity(FSlateColor::UseSubduedForeground())
							.Clipping(EWidgetClipping::ClipToBounds)
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("Open \u203A"))).ColorAndOpacity(Accent)
						],
						FMargin {14.0f, 10.0f}
					)
				]
			];
		// clang-format on
	}
}	// namespace

void SPjcTabHelp::Construct(const FArguments& InArgs) {
	const TFunction<void()> OpenReport = []() {
		SPjcReportWindow::Open();
	};
	const TCHAR* ReportTip = TEXT("Collects what helps, shows it, and you choose what goes into the GitHub issue");
	// clang-format off
	const FPjcHelpLink Links[] = {
		{TEXT("?"), TEXT("Documentation"), TEXT("Guides for every tab and setting"), PjcConstants::UrlDocs, {}},
		{TEXT("!"), TEXT("Report a bug"), ReportTip, PjcConstants::UrlIssueTracker + TEXT("/new"), OpenReport},
		{TEXT("@"), TEXT("Contact"), TEXT("Questions and feedback for the author"), PjcConstants::UrlAuthor, {}},
		{TEXT("\u2197"), TEXT("Release notes"), TEXT("What changed in each version"), PjcConstants::UrlGithub + TEXT("/releases"), {}},
		{TEXT("</>"), TEXT("Source code"), TEXT("The plugin's GitHub repository"), PjcConstants::UrlGithub, {}},
		{TEXT("\u2605"), TEXT("Fab page"), TEXT("Rate the plugin or download other versions"), PjcConstants::UrlFab, {}},
	};
	// clang-format on
	const TSharedRef<SWrapBox> Cards = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D {10.0f, 10.0f});
	for (const auto& Link : Links) {
		Cards->AddSlot()[PjcHelpCard(Link)];
	}

	// clang-format off

	ChildSlot
	[
		SNew(SBorder).BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.GroupBorder"))).Padding(0.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot().Padding(FMargin {20.0f, 16.0f})
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 6.0f})
				[
					PjcWidgets::MakeCardTitle(TEXT("Links"))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					Cards
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 18.0f, 0.0f, 6.0f})
				[
					PjcWidgets::MakeCardTitle(TEXT("System info"))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					PjcWidgets::MakeCard(
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text_Lambda([]() { return FText::FromString(GetSystemInfo()); }).Font(PjcWidgets::GetFontMono(9))
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(FMargin {0.0f, 10.0f, 0.0f, 0.0f})
						[
							SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
							.ButtonStyle(&PjcShim::GetButtonStyle427(false))
							.ForegroundColor(FSlateColor::UseForeground())
#endif
							.ContentPadding(FMargin {12.0f, 2.0f})
							.ToolTipText(FText::FromString(TEXT("Copies the lines above, to paste into a bug report")))
							.OnClicked_Lambda([]() {
								FPlatformApplicationMisc::ClipboardCopy(*GetSystemInfo());
								UPjcSubsystem::ShowNotification(TEXT("Copied \u00B7 paste it into the bug report"), SNotificationItem::CS_Success, 3.0f);
								return FReply::Handled();
							})
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("Copy")))
							]
						],
						FMargin {14.0f, 12.0f}
					)
				]
			]
		]
	];
	// clang-format on
}

FString SPjcTabHelp::GetPluginVersion() {
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(PjcConstants::ModulePjcName.ToString());
	return Plugin.IsValid() ? Plugin->GetDescriptor().VersionName : FString {TEXT("?")};
}

FString SPjcTabHelp::GetSystemInfo() {
	FString OsLabel;
	FString OsVersion;
	FPlatformMisc::GetOSVersions(OsLabel, OsVersion);
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	const FString Assets = PjcFormat::Count(Result.Assets.Num());
	const FString Scanned = FString::Printf(TEXT("%s assets \u00B7 last scan %.1f s"), *Assets, Result.Seconds);
	const FString ScanLine = Result.IsValid() ? Scanned : FString {TEXT("not scanned yet")};
	const FPjcScanDraft Settings = FPjcScanDraft::FromSettings();
	const int32 NumExclusions = Settings.ExcludedFolders.Num() + Settings.ExcludedClasses.Num() + Settings.ExcludedAssets.Num();

	TArray<FString> Lines;
	Lines.Add(TEXT("Project Cleaner ") + GetPluginVersion());
	Lines.Add(TEXT("Unreal Engine ") + FEngineVersion::Current().ToString());
	Lines.Add(OsVersion.IsEmpty() ? OsLabel : FString::Printf(TEXT("%s (%s)"), *OsLabel, *OsVersion));
	Lines.Add(FString::Printf(TEXT("Project: %s \u00B7 %s"), FApp::GetProjectName(), *ScanLine));
	const TCHAR* Plugins = PjcHelpOnOff(Settings.bScanProjectPlugins);
	const TCHAR* Primary = PjcHelpOnOff(Settings.bUnregisteredPrimaryUsed);
	const TCHAR* Cinematics = PjcHelpOnOff(Settings.bCinematicAssetsUsed);
	const FString Rules = FString::Printf(TEXT("project plugins %s \u00B7 unregistered primary classes %s"), Plugins, Primary);
	Lines.Add(FString::Printf(TEXT("Scan settings: %s \u00B7 cinematics %s \u00B7 %d exclusions"), *Rules, Cinematics, NumExclusions));
	return FString::Join(Lines, TEXT("\n"));
}

void SPjcReportWindow::Construct(const FArguments& InArgs) {
	Window = InArgs._Window;
	SystemText = SPjcTabHelp::GetSystemInfo();
	SettingsText = GetScanSettingsText();
	ScanText = GetLastScanText();
	LogText = GetLogText();
	CrashText = GetLastCrashText();

	const auto Line = [](const FString& Text) {
		int32 Lines = 1;
		for (const TCHAR Char : Text) {
			Lines += Char == TEXT('\n') ? 1 : 0;
		}
		return Text.IsEmpty() ? 0 : Lines;
	};
	// a part of the report: its line count, off and greyed when there is nothing to include
	const auto Option = [&Line](bool* Flag, const FString& Label, const FString& Detail, const FString& Text, const bool bPart) {
		const int32 NumLines = Line(Text);
		const FString Counted = FString::Printf(TEXT("%d %s"), NumLines, NumLines == 1 ? TEXT("line") : TEXT("lines"));
		const FString Lines = Text.IsEmpty() ? FString {TEXT("none")} : Counted;
		const FString Count = bPart ? Lines : FString {};
		if (bPart && Text.IsEmpty()) {
			*Flag = false;
		}
		// clang-format off
		return SNew(SCheckBox)
			.IsEnabled(!bPart || !Text.IsEmpty())
			.IsChecked_Lambda([Flag]() { return *Flag ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([Flag](const ECheckBoxState State) { *Flag = State == ECheckBoxState::Checked; })
			.ToolTipText(FText::FromString(Detail))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(STextBlock).Text(FText::FromString(Label))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin {8.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(STextBlock).Text(FText::FromString(Count)).Font(PjcWidgets::GetFontMono(8)).ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			];
		// clang-format on
	};

	const TSharedRef<SVerticalBox> Include = SNew(SVerticalBox);
	const auto AddOption = [&Include](const TSharedRef<SWidget>& Widget) {
		// clang-format off
		Include->AddSlot().AutoHeight().Padding(FMargin {0.0f, 2.0f})
		[
			Widget
		];
		// clang-format on
	};
	AddOption(PjcWidgets::MakeCardTitle(TEXT("Include")));
	const FString LogTip = TEXT("Only its own lines from this editor's log");
	const FString CrashTip = TEXT("Call stack of the newest crash, only when Project Cleaner is in it. No memory dump.");
	const FString UserTip = TEXT("Your user and machine name become <user> and <machine>");
	AddOption(Option(&bSystem, TEXT("System info"), TEXT("Plugin, engine and OS versions"), SystemText, true));
	AddOption(Option(&bSettings, TEXT("Scan settings"), TEXT("Rules and exclusions: they name folders and classes"), SettingsText, true));
	AddOption(Option(&bScan, TEXT("Last scan and cleanup"), TEXT("Numbers only"), ScanText, true));
	AddOption(Option(&bLog, TEXT("Project Cleaner log lines"), LogTip, LogText, true));
	AddOption(Option(&bCrash, TEXT("Last crash"), CrashTip, CrashText, true));
	AddOption(PjcWidgets::MakeCardTitle(TEXT("Privacy")));
	AddOption(Option(&bHideUser, TEXT("Hide user and machine name"), UserTip, FString {}, false));
	AddOption(Option(&bHidePaths, TEXT("Hide asset paths"), TEXT("/Game/... paths become <asset>"), FString {}, false));

	// clang-format off
	ChildSlot
	[
		SNew(SBorder).BorderImage(PjcShim::GetBrushPage()).Padding(0.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(FMargin {16.0f, 12.0f})
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("Title")))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 2.0f, 0.0f, 8.0f})
				[
					SNew(SEditableTextBox)
#if ENGINE_MAJOR_VERSION < 5
					.Style(&PjcShim::GetTextBoxStyle427())
#endif
					.HintText(FText::FromString(TEXT("Short summary")))
					.OnTextChanged_Lambda([this](const FText& Text) { Title = Text.ToString(); })
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("What happened")))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 2.0f, 0.0f, 8.0f})
				[
					SNew(SBox).HeightOverride(64.0f)
					[
						SNew(SMultiLineEditableTextBox)
#if ENGINE_MAJOR_VERSION < 5
						.Style(&PjcShim::GetTextBoxStyle427())
#endif
						.HintText(FText::FromString(TEXT("What you did, what you expected, what happened")))
						.OnTextChanged_Lambda([this](const FText& Text) { WhatHappened = Text.ToString(); })
					]
				]
				+ SVerticalBox::Slot().FillHeight(1.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin {0.0f, 0.0f, 14.0f, 0.0f})
					[
						SNew(SBox).WidthOverride(240.0f)
						[
							Include
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("Preview \u00B7 %s characters"), *PjcFormat::Count(MakeReport().Len()))); })
							.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						]
						+ SVerticalBox::Slot().FillHeight(1.0f).Padding(FMargin {0.0f, 2.0f})
						[
							SNew(SMultiLineEditableTextBox)
#if ENGINE_MAJOR_VERSION < 5
							.Style(&PjcShim::GetTextBoxStyle427())
#endif
							.IsReadOnly(true)
							.Font(PjcWidgets::GetFontMono(8))
							.Text_Lambda([this]() { return FText::FromString(MakeReport()); })
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 8.0f, 0.0f, 0.0f})
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Project Cleaner sends nothing. You post the report yourself, and GitHub issues are public: check the preview first.")))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					.AutoWrapText(true)
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBorder).BorderImage(PjcShim::GetBrush(TEXT("ToolPanel.DarkGroupBorder"))).Padding(FMargin {16.0f, 8.0f})
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
						.ButtonStyle(&PjcShim::GetButtonStyle427(false))
						.ForegroundColor(FSlateColor::UseForeground())
#endif
						.ContentPadding(FMargin {12.0f, 2.0f})
						.ToolTipText(FText::FromString(TEXT("Saves the report as a text file, to attach or send another way")))
						.OnClicked_Lambda([this]() {
							SaveAsFile();
							return FReply::Handled();
						})
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("Save as file\u2026")))
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin {8.0f, 0.0f, 0.0f, 0.0f})
					[
						SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
						.ButtonStyle(&PjcShim::GetButtonStyle427(false))
						.ForegroundColor(FSlateColor::UseForeground())
#endif
						.ContentPadding(FMargin {12.0f, 2.0f})
						.OnClicked_Lambda([this]() {
							Close();
							return FReply::Handled();
						})
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("Cancel")))
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin {8.0f, 0.0f, 0.0f, 0.0f})
					[
						SNew(SButton)
#if ENGINE_MAJOR_VERSION < 5
						.ForegroundColor(FSlateColor::UseForeground())
#endif
						.ButtonStyle(&PjcShim::GetPrimaryButtonStyle())
						.ContentPadding(FMargin {12.0f, 2.0f})
						.ToolTipText(FText::FromString(TEXT("Opens a new GitHub issue with this title and text. A long report is copied to the clipboard instead.")))
						.OnClicked_Lambda([this]() {
							OpenIssue();
							Close();
							return FReply::Handled();
						})
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("Open GitHub issue")))
						]
					]
				]
			]
		]
	];
	// clang-format on
}

void SPjcReportWindow::Open() {
	// clang-format off
	const TSharedRef<SWindow> ReportWindow = SNew(SWindow)
		.Title(FText::FromString(TEXT("Report a bug")))
		.ClientSize(FVector2D {780.0f, 560.0f})
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.SizingRule(ESizingRule::UserSized);
	// clang-format on
	ReportWindow->SetContent(SNew(SPjcReportWindow).Window(ReportWindow));
	FSlateApplication::Get().AddModalWindow(ReportWindow, FSlateApplication::Get().GetActiveTopLevelWindow());
}

FString SPjcReportWindow::GetScanSettingsText() {
	const FPjcScanDraft Settings = FPjcScanDraft::FromSettings();
	const FString Plugins = PjcHelpJoined(Settings.IgnoredPlugins);
	const TCHAR* Primary = PjcHelpOnOff(Settings.bUnregisteredPrimaryUsed);
	const TCHAR* Cinematics = PjcHelpOnOff(Settings.bCinematicAssetsUsed);
	const FString Folders = PjcHelpJoined(Settings.ExcludedFolders);
	const FString Classes = PjcHelpJoined(Settings.ExcludedClasses);
	const FString Assets = PjcHelpJoined(Settings.ExcludedAssets);

	TArray<FString> Lines;
	Lines.Add(TEXT("Scan settings (Config/DefaultEditor.ini):"));
	Lines.Add(FString::Printf(TEXT("  project plugins: %s \u00B7 ignored plugins: %s"), PjcHelpOnOff(Settings.bScanProjectPlugins), *Plugins));
	Lines.Add(FString::Printf(TEXT("  unregistered primary classes: %s \u00B7 cinematics: %s"), Primary, Cinematics));
	Lines.Add(TEXT("  always used classes: ") + PjcHelpJoined(Settings.UsedClasses));
	Lines.Add(FString::Printf(TEXT("  excluded folders: %s"), *Folders));
	Lines.Add(FString::Printf(TEXT("  excluded classes: %s"), *Classes));
	Lines.Add(FString::Printf(TEXT("  excluded assets: %s"), *Assets));
	return FString::Join(Lines, TEXT("\n"));
}

FString SPjcReportWindow::GetLastScanText() {
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	if (!Result.IsValid()) return TEXT("Last scan: none in this session");

	int32 NumReview = 0;
	for (const auto& Pair : Result.Status) {
		NumReview += Pair.Value == EPjcAssetStatus::Review ? 1 : 0;
	}
	const FString Assets = PjcFormat::Count(Result.Assets.Num());
	const FString Unused = PjcFormat::Count(Result.Unused.Num());
	const FString Excluded = PjcFormat::Count(Result.Excluded.Num());
	const FString Counts = FString::Printf(TEXT("%s assets \u00B7 %s unused \u00B7 %d to review"), *Assets, *Unused, NumReview);
	const FString Scan = FString::Printf(TEXT("Last scan: %s \u00B7 %s excluded \u00B7 %.1f s"), *Counts, *Excluded, Result.Seconds);
	const int32 NumKept = UPjcSubsystem::GetLastCleanupKept().Num();
	return Scan + (NumKept > 0 ? FString::Printf(TEXT("\nLast cleanup: %d failed to delete"), NumKept) : FString {});
}

FString SPjcReportWindow::GetLogText(const int32 MaxLines) {
	// the file this editor writes: Saved/Logs/<Project>.log, or <Project>_2.log for a second editor
	const FString LogFile = FPlatformOutputDevices::GetAbsoluteLogFilename();
	GLog->Flush();
	FString Content;
	// the editor still writes the log
	if (!FFileHelper::LoadFileToString(Content, *LogFile, FFileHelper::EHashOptions::None, FILEREAD_AllowWrite)) return FString {};

	TArray<FString> Lines;
	Content.ParseIntoArrayLines(Lines);
	TArray<FString> Own;
	for (const auto& Line : Lines) {
		if (Line.Contains(TEXT("LogProjectCleaner"))) {
			Own.Add(Line);
		}
	}
	if (Own.Num() == 0) return FString {};

	const int32 First = FMath::Max(0, Own.Num() - MaxLines);
	TArray<FString> Shown {Own.GetData() + First, Own.Num() - First};
	const FString LogName = FPaths::GetCleanFilename(LogFile);
	const FString Range = FString::Printf(TEXT("newest %d of %d"), Shown.Num(), Own.Num());
	const FString Head = FString::Printf(TEXT("Project Cleaner lines of the editor log (Saved/Logs/%s, %s):"), *LogName, *Range);
	return Head + TEXT("\n") + FString::Join(Shown, TEXT("\n"));
}

FString SPjcReportWindow::GetLastCrashText() {
	// the newest crash folder; offered only when Project Cleaner is in its call stack
	TArray<FString> Folders;
	const FString CrashDir = FPaths::ProjectSavedDir() / TEXT("Crashes");
	IFileManager::Get().FindFiles(Folders, *(CrashDir / TEXT("*")), false, true);
	FString Newest;
	FDateTime NewestTime {0};
	for (const auto& Folder : Folders) {
		const FDateTime Time = IFileManager::Get().GetTimeStamp(*(CrashDir / Folder));
		if (Time > NewestTime) {
			Newest = Folder;
			NewestTime = Time;
		}
	}
	if (Newest.IsEmpty()) return FString {};

	FString Context;
	if (!FFileHelper::LoadFileToString(Context, *(CrashDir / Newest / TEXT("CrashContext.runtime-xml")))) return FString {};

	const int32 Start = Context.Find(TEXT("<CallStack>"));
	const int32 End = Context.Find(TEXT("</CallStack>"));
	if (Start < 0 || End <= Start) return FString {};

	const FString CallStack = Context.Mid(Start + 11, End - Start - 11).TrimStartAndEnd();
	if (!CallStack.Contains(TEXT("Pjc"))) return FString {};

	const FString When = NewestTime.ToString(TEXT("%Y-%m-%d %H:%M"));
	return FString::Printf(TEXT("Last crash (%s, Saved/Crashes/%s, Project Cleaner in the call stack):\n"), *When, *Newest) + CallStack;
}

FString SPjcReportWindow::HidePrivate(const FString& Text, const bool bUser, const bool bAssetPaths) {
	FString Hidden = Text;
	if (bUser) {
		const FString User = FPlatformProcess::UserName();
		const FString Machine = FPlatformProcess::ComputerName();
		if (!User.IsEmpty()) {
			Hidden.ReplaceInline(*User, TEXT("<user>"));
		}
		if (!Machine.IsEmpty()) {
			Hidden.ReplaceInline(*Machine, TEXT("<machine>"));
		}
	}
	if (bAssetPaths) {
		const FRegexPattern Pattern {TEXT("/Game/[A-Za-z0-9_\\-/.]+")};
		FRegexMatcher Matcher {Pattern, Hidden};
		FString Out;
		int32 Last = 0;
		while (Matcher.FindNext()) {
			Out += Hidden.Mid(Last, Matcher.GetMatchBeginning() - Last) + TEXT("<asset>");
			Last = Matcher.GetMatchEnding();
		}
		Hidden = Out + Hidden.Mid(Last);
	}
	return Hidden;
}

FString SPjcReportWindow::MakeReport() const {
	TArray<FString> Parts;
	Parts.Add(TEXT("### What happened\n") + (WhatHappened.IsEmpty() ? FString {TEXT("(describe what you did and what happened)")} : WhatHappened));
	const auto AddPart = [&Parts](const bool bOn, const TCHAR* Heading, const FString& Text) {
		if (bOn && !Text.IsEmpty()) {
			Parts.Add(FString::Printf(TEXT("### %s\n```\n%s\n```"), Heading, *Text));
		}
	};
	AddPart(bSystem, TEXT("System info"), SystemText);
	AddPart(bSettings, TEXT("Scan settings"), SettingsText);
	AddPart(bScan, TEXT("Last scan and cleanup"), ScanText);
	AddPart(bLog, TEXT("Project Cleaner log lines"), LogText);
	AddPart(bCrash, TEXT("Last crash"), CrashText);
	return HidePrivate(FString::Join(Parts, TEXT("\n\n")), bHideUser, bHidePaths);
}

void SPjcReportWindow::OpenIssue() const {
	const FString IssueTitle = Title.IsEmpty() ? FString {TEXT("Bug: ")} : Title;
	const FString Report = MakeReport();
	const FString Base = PjcConstants::UrlIssueTracker + TEXT("/new?title=") + PjcHelpUrlEncode(IssueTitle);
	FString Url = Base + TEXT("&body=") + PjcHelpUrlEncode(Report);
	if (Url.Len() > PjcHelpMaxUrl) {
		FPlatformApplicationMisc::ClipboardCopy(*Report);
		Url = Base + TEXT("&body=") + PjcHelpUrlEncode(TEXT("(paste the report from the clipboard here)"));
		UPjcSubsystem::ShowNotification(TEXT("The report is long: copied it, paste it into the issue"), SNotificationItem::CS_Success, 5.0f);
	}
	FPlatformProcess::LaunchURL(*Url, nullptr, nullptr);
}

void SPjcReportWindow::SaveAsFile() const {
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform) return;

	TArray<FString> Files;
	const void* ParentWindow = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
	const FString Folder = FPaths::ProjectSavedDir();
	const FString DialogTitle = TEXT("Save report");
	const FString FileName = TEXT("ProjectCleaner-report.md");
	const FString FileTypes = TEXT("Markdown|*.md");
	const bool bChosen = DesktopPlatform->SaveFileDialog(ParentWindow, DialogTitle, Folder, FileName, FileTypes, 0, Files);
	if (!bChosen || Files.Num() == 0) return;

	if (FFileHelper::SaveStringToFile(MakeReport(), *Files[0])) {
		UPjcSubsystem::ShowNotification(TEXT("Saved ") + FPaths::GetCleanFilename(Files[0]), SNotificationItem::CS_Success, 3.0f);
	}
}

void SPjcReportWindow::Close() const {
	if (const TSharedPtr<SWindow> Pinned = Window.Pin()) {
		Pinned->RequestDestroyWindow();
	}
}
