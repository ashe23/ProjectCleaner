// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#include "UI/ProjectCleanerNotificationManager.h"
// Engine Headers
#include "Framework/Notifications/NotificationManager.h"

TWeakPtr<SNotificationItem> ProjectCleanerNotificationManager::Add(
	const FString& Text,
	const SNotificationItem::ECompletionState CompletionState
)
{
	FNotificationInfo Info(FText::FromString(Text));
	Info.bFireAndForget = false;

	const TWeakPtr<SNotificationItem> NotificationManager = FSlateNotificationManager::Get().AddNotification(Info);
	if (NotificationManager.IsValid())
	{
		NotificationManager.Pin()->SetCompletionState(CompletionState);
	}

	return NotificationManager;
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

void ProjectCleanerNotificationManager::Hide(TWeakPtr<SNotificationItem> NotificationManager, const FText& FinalText)
{
	if (!NotificationManager.IsValid()) return;

	NotificationManager.Pin()->SetText(FinalText);
	NotificationManager.Pin()->SetCompletionState(SNotificationItem::CS_Success);
	NotificationManager.Pin()->SetFadeOutDuration(5.0f);
	NotificationManager.Pin()->Fadeout();
}

void ProjectCleanerNotificationManager::Reset(TWeakPtr<SNotificationItem> NotificationManager)
{
	if (!NotificationManager.IsValid()) return;

	NotificationManager.Pin()->ExpireAndFadeout();
	NotificationManager.Reset();
}
