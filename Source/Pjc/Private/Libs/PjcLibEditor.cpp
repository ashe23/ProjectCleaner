// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Libs/PjcLibEditor.h"
#include "Libs/PjcLibPath.h"
// Engine Headers
#include "ShaderCompiler.h"
#include "EditorSettings/PjcEditorAssetExcludeSettings.h"
#include "Framework/Notifications/NotificationManager.h"

void FPjcLibEditor::ShowNotification(const FString& Msg, const SNotificationItem::ECompletionState State, const float Duration)
{
	FNotificationInfo Info{FText::FromString(Msg)};
	Info.Text = FText::FromString(Msg);
	Info.ExpireDuration = Duration;

	const auto NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
	if (!NotificationPtr.IsValid()) return;

	NotificationPtr.Get()->SetCompletionState(State);
}

void FPjcLibEditor::ShowNotificationWithOutputLog(const FString& Msg, const SNotificationItem::ECompletionState State, const float Duration)
{
	FNotificationInfo Info{FText::FromString(Msg)};
	Info.Text = FText::FromString(Msg);
	Info.ExpireDuration = Duration;
	Info.Hyperlink = FSimpleDelegate::CreateLambda([]()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(FName{TEXT("OutputLog")});
	});
	Info.HyperlinkText = FText::FromString(TEXT("Show OutputLog..."));

	const auto NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
	if (!NotificationPtr.IsValid()) return;

	NotificationPtr.Get()->SetCompletionState(State);
}

void FPjcLibEditor::ShaderCompilationEnable()
{
	if (!GShaderCompilingManager) return;

	GShaderCompilingManager->SkipShaderCompilation(false);
}

void FPjcLibEditor::ShaderCompilationDisable()
{
	if (!GShaderCompilingManager) return;

	GShaderCompilingManager->SkipShaderCompilation(true);
}

void FPjcLibEditor::NavigateToPathInFileExplorer(const FString& InPath)
{
	const FString PathAbsolute = FPjcLibPath::ToAbsolute(InPath);
	if (PathAbsolute.IsEmpty()) return;
	if (!FPjcLibPath::IsValid(PathAbsolute)) return;

	FPlatformProcess::ExploreFolder(*PathAbsolute);
}

void FPjcLibEditor::OpenFileInFileExplorer(const FString& InFilePath)
{
	const FString PathAbsolute = FPjcLibPath::ToAbsolute(InFilePath);
	if (PathAbsolute.IsEmpty()) return;
	if (!FPjcLibPath::IsValid(PathAbsolute)) return;
	if (!FPjcLibPath::IsFile(PathAbsolute)) return;

	FPlatformProcess::LaunchFileInDefaultExternalApplication(*PathAbsolute);
}

void FPjcLibEditor::OpenAssetEditor(const FAssetData& InAssetData)
{
	if (!InAssetData.IsValid()) return;
	if (!GEditor) return;

	TArray<FName> AssetNames;
	AssetNames.Add(InAssetData.ToSoftObjectPath().GetAssetPathName());

	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorsForAssets(AssetNames);
}

bool FPjcLibEditor::EditorInPlayMode()
{
	return GEditor && GEditor->PlayWorld || GIsPlayInEditorWorld;
}

FPjcAssetExcludeSettings FPjcLibEditor::GetEditorAssetExcludeSettings()
{
	FPjcAssetExcludeSettings AssetExcludeSettings;

	const UPjcEditorAssetExcludeSettings* EditorAssetExcludeSettings = GetDefault<UPjcEditorAssetExcludeSettings>();
	if (!EditorAssetExcludeSettings) return AssetExcludeSettings;

	AssetExcludeSettings.ExcludedPackagePaths.Reserve(EditorAssetExcludeSettings->ExcludedPaths.Num());
	AssetExcludeSettings.ExcludedObjectPaths.Reserve(EditorAssetExcludeSettings->ExcludedAssets.Num());
	AssetExcludeSettings.ExcludedClassNames.Reserve(EditorAssetExcludeSettings->ExcludedClasses.Num());

	for (const auto& ExcludedPath : EditorAssetExcludeSettings->ExcludedPaths)
	{
		AssetExcludeSettings.ExcludedPackagePaths.Emplace(FName{*ExcludedPath.Path});
	}

	for (const auto& ExcludedAsset : EditorAssetExcludeSettings->ExcludedAssets)
	{
		if (!ExcludedAsset.LoadSynchronous()) continue;

		AssetExcludeSettings.ExcludedObjectPaths.Emplace(ExcludedAsset.ToSoftObjectPath().GetAssetPathName());
	}

	for (const auto& ExcludedClass : EditorAssetExcludeSettings->ExcludedClasses)
	{
		if (!ExcludedClass.LoadSynchronous()) continue;

		AssetExcludeSettings.ExcludedClassNames.Emplace(ExcludedClass.Get()->GetFName());
	}

	return AssetExcludeSettings;
}
