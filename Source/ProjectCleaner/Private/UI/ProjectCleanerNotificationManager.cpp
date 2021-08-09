// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "UI/ProjectCleanerNotificationManager.h"
// Engine Headers
#include "Framework/Notifications/NotificationManager.h"

void ProjectCleanerNotificationManager::Add(
	const FText& Text,
	const SNotificationItem::ECompletionState CompletionState,
	TWeakPtr<SNotificationItem>& NotificationPtr)
{
	FNotificationInfo Info(Text);
	Info.bFireAndForget = false;

	NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
	if (NotificationPtr.IsValid())
	{
		NotificationPtr.Pin()->SetCompletionState(CompletionState);
	}
}

void ProjectCleanerNotificationManager::AddTransient(
	const FText& Text,
    const SNotificationItem::ECompletionState CompletionState,
	const float ExpireDuration)
{
	FNotificationInfo Info(Text);
	Info.bFireAndForget = true;
	Info.ExpireDuration = ExpireDuration;

	const TWeakPtr<SNotificationItem> NotificationManager = FSlateNotificationManager::Get().AddNotification(Info);
	if (NotificationManager.IsValid())
	{
		NotificationManager.Pin()->SetCompletionState(CompletionState);
	}
}


void ProjectCleanerNotificationManager::Update(TWeakPtr<SNotificationItem> NotificationManager, const FText& Text)
{
	if (NotificationManager.IsValid())
	{
		NotificationManager.Pin()->SetText(Text);
	}
}

void ProjectCleanerNotificationManager::Hide(
	TWeakPtr<SNotificationItem> NotificationManager,
	const SNotificationItem::ECompletionState CompletionState,
	const FText& FinalText)
{
	if (!NotificationManager.IsValid()) return;

	NotificationManager.Pin()->SetText(FinalText);
	NotificationManager.Pin()->SetCompletionState(CompletionState);
	NotificationManager.Pin()->SetFadeOutDuration(5.0f);
	NotificationManager.Pin()->Fadeout();
}

void ProjectCleanerNotificationManager::Reset(TWeakPtr<SNotificationItem> NotificationManager)
{
	if (!NotificationManager.IsValid()) return;

	NotificationManager.Pin()->ExpireAndFadeout();
	NotificationManager.Reset();
}

EAppReturnType::Type ProjectCleanerNotificationManager::ShowConfirmationWindow(const FText& Title, const FText& ContentText)
{
	return FMessageDialog::Open(
		EAppMsgType::YesNo,
		ContentText,
		&Title
	);
}

bool ProjectCleanerNotificationManager::IsConfirmationWindowCanceled(EAppReturnType::Type Status)
{
	return Status == EAppReturnType::Type::No || Status == EAppReturnType::Cancel;
}
