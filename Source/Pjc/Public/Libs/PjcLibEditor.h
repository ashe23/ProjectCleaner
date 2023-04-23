// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PjcTypes.h"
#include "Widgets/Notifications/SNotificationList.h"

struct FPjcLibEditor
{
	static void ShowNotification(const FString& Msg, const SNotificationItem::ECompletionState State, const float Duration);
	static void ShowNotificationWithOutputLog(const FString& Msg, const SNotificationItem::ECompletionState State, const float Duration);
	static void ShaderCompilationEnable();
	static void ShaderCompilationDisable();
	static void NavigateToPathInFileExplorer(const FString& InPath);
	static void OpenFileInFileExplorer(const FString& InFilePath);
	static bool EditorInPlayMode();
	static FPjcAssetExcludeSettings GetEditorAssetExcludeSettings();
};
