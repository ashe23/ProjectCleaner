// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * Help window, from Help in the plugin menu: the links, the system info a bug report needs with Copy, and Report a bug.
 */
class SPjcHelpWindow final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcHelpWindow) {}
	SLATE_ARGUMENT(TWeakPtr<SWindow>, Window)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Opens the window until it is closed; Report a bug opens its own window after it
	static void Show();

	// Plugin version from its descriptor, e.g. 3.1.0
	static FString GetPluginVersion();

	// Plugin, engine and OS versions, the project and its last scan, the scan settings in one line
	static FString GetSystemInfo();

  private:

	void Close(const bool bInReport);

	TWeakPtr<SWindow> Window;
	bool bReport = false;
};

/**
 * Report a bug window: the user writes what happened, picks what goes in and sees the exact text. Project Cleaner sends
 * nothing: Open GitHub issue opens a new issue in the browser with the text (or copies a long one), Save as file writes it.
 */
class SPjcReportWindow final : public SCompoundWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcReportWindow) {}
	SLATE_ARGUMENT(TSharedPtr<SWindow>, Window)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	static void Open();

	// What a report can include, as text
	static FString GetScanSettingsText();
	static FString GetLastScanText();
	// Project Cleaner lines of this editor's log, at most MaxLines of the newest
	static FString GetLogText(const int32 MaxLines = 40);
	// Call stack of the newest crash in Saved/Crashes when Project Cleaner is in it, empty otherwise
	static FString GetLastCrashText();
	// The user and machine name become <user> and <machine>; asset paths become <asset> when asked
	static FString HidePrivate(const FString& Text, const bool bUser, const bool bAssetPaths);

	// The issue text as it is posted: what happened, the chosen parts, private parts hidden
	FString MakeReport() const;

  private:

	void OpenIssue() const;
	void SaveAsFile() const;
	void Close() const;

	TWeakPtr<SWindow> Window;
	FString Title;
	FString WhatHappened;
	bool bSystem = true;
	bool bSettings = true;
	bool bScan = true;
	bool bLog = true;
	bool bCrash = false;
	bool bHideUser = true;
	bool bHidePaths = false;
	FString SystemText;
	FString SettingsText;
	FString ScanText;
	FString LogText;
	FString CrashText;
};
