#include "ProjectCleanerNotificationManager.h"
// Engine Headers
#include "Framework/Notifications/NotificationManager.h"


TWeakPtr<SNotificationItem> ProjectCleanerNotificationManager::Add(const FString& Text,
                                                                   const SNotificationItem::ECompletionState
                                                                   CompletionState) const
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

void ProjectCleanerNotificationManager::AddTransient(const FString& Text,
                                                     const SNotificationItem::ECompletionState
                                                     CompletionState,
                                                     const float ExpireDuration) const
{
	FNotificationInfo Info(FText::FromString(Text));
	Info.bFireAndForget = true;
	Info.ExpireDuration = ExpireDuration;


	const TWeakPtr<SNotificationItem> NotificationManager = FSlateNotificationManager::Get().AddNotification(Info);
	if (NotificationManager.IsValid())
	{
		NotificationManager.Pin()->SetCompletionState(CompletionState);
	}
}


void ProjectCleanerNotificationManager::Update(TWeakPtr<SNotificationItem> NotificationManager,
                                               const FCleaningStats& Stats) const
{
	if (NotificationManager.IsValid())
	{
		NotificationManager.Pin()->SetText(FText::FromString(FString::Printf(
				TEXT("Deleted %d of %d assets. %d %%"),
				Stats.DeletedAssetCount,
				Stats.TotalAssetNum,
				Stats.GetPercentage()
			)
		));
	}
}

void ProjectCleanerNotificationManager::Hide(TWeakPtr<SNotificationItem> NotificationManager, const FText& FinalText) const
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
