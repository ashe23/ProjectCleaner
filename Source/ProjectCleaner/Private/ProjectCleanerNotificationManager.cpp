// Fill out your copyright notice in the Description page of Project Settings.


#include "Public/ProjectCleanerNotificationManager.h"
#include "SNotificationList.h"
#include "NotificationManager.h"


void ProjectCleanerNotificationManager::Show(const FCleaningStats& Stats)
{
	// FText Text = FText::Format(LOCTEXT("LightBuildProgressMessage", "Building lighting{0}:  {1}%"), FText::FromString(ScenarioString), FText::AsNumber(LightmassProcessor->GetAsyncPercentDone()));
	// FStaticLightingManager::Get()->SetNotificationText( Text );
	// FNotificationInfo Info(FText::FromString(TEXT("Cleaning project. This can take some time, Please wait")));
	const int32 DeletePercentage = (static_cast<float>(Stats.DeletedAssetCount) / Stats.TotalAssetNum) * 100.0f;
	FNotificationInfo Info(FText::FromString(FString::Printf(
				TEXT("Deleted %d of %d assets. %d %%"),
				Stats.DeletedAssetCount,
				Stats.TotalAssetNum,
				DeletePercentage
			)
		)
	);
	Info.bFireAndForget = false;


	NotificationManager = FSlateNotificationManager::Get().AddNotification(Info);
	if (NotificationManager.IsValid())
	{
		NotificationManager.Pin()->SetCompletionState(SNotificationItem::CS_Pending);
	}
}

void ProjectCleanerNotificationManager::Update(const FCleaningStats& Stats)
{
	if (NotificationManager.IsValid())
	{
		const int32 DeletePercentage = (static_cast<float>(Stats.DeletedAssetCount) / Stats.TotalAssetNum) * 100.0f;
		NotificationManager.Pin()->SetText(FText::FromString(FString::Printf(
				TEXT("Deleted %d of %d assets. %d %%"),
				Stats.DeletedAssetCount,
				Stats.TotalAssetNum,
				DeletePercentage
			)
		));
	}
}

void ProjectCleanerNotificationManager::Hide()
{
	if (!NotificationManager.IsValid()) return;

	// Resetting old notification
	NotificationManager.Pin()->SetCompletionState(SNotificationItem::CS_Success);
	NotificationManager.Pin()->SetFadeOutDuration(5.0f);
	NotificationManager.Pin()->ExpireAndFadeout();
	NotificationManager.Reset();

	// New notification about success
	FNotificationInfo Info(FText::FromString(TEXT("All assets deleted sucessfully.")));
	Info.ExpireDuration = 10.0f;

	NotificationManager = FSlateNotificationManager::Get().AddNotification(Info);
}
