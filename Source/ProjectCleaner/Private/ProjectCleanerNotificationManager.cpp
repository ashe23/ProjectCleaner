// Fill out your copyright notice in the Description page of Project Settings.


#include "Public/ProjectCleanerNotificationManager.h"
#include "SNotificationList.h"
#include "NotificationManager.h"


void ProjectCleanerNotificationManager::Show()
{
	FNotificationInfo Info(FText::FromString(TEXT("Cleaning project. This can take some time, Please wait")));
	Info.bFireAndForget = false;

	NotificationManager = FSlateNotificationManager::Get().AddNotification(Info);
	if (NotificationManager.IsValid())
	{
		NotificationManager.Pin()->SetCompletionState(SNotificationItem::CS_Pending);
	}
}

void ProjectCleanerNotificationManager::Hide()
{
	if(!NotificationManager.IsValid()) return;

	// Resetting old notification
	NotificationManager.Pin()->SetCompletionState(SNotificationItem::CS_Success);
	NotificationManager.Pin()->ExpireAndFadeout();
	NotificationManager.Reset();

	// New notification about success
	FNotificationInfo Info(FText::FromString(TEXT("All assets deleted sucessfully.")));
	Info.ExpireDuration = 10.0f;

	NotificationManager = FSlateNotificationManager::Get().AddNotification(Info);

}
