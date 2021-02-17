// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "StructsContainer.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "CoreMinimal.h"

class FSlateNotificationManager;


/**
 * 
 */
class PROJECTCLEANER_API ProjectCleanerNotificationManager
{
public:
	// Create new persistent notification, will be in memory until Reset function call
	TWeakPtr<SNotificationItem> Add(const FString& Text,
	                                const SNotificationItem::ECompletionState CompletionState) const;

	// Transient Notification , used when no later updates needed for it
	void AddTransient(const FString& Text,
	                  const SNotificationItem::ECompletionState
	                  CompletionState,
	                  const float ExpireDuration = 3.0f) const;

	// Updates content of given Notification
	void Update(TWeakPtr<SNotificationItem> NotificationManager, const FCleaningStats& Stats) const;
	void Hide(TWeakPtr<SNotificationItem> NotificationManager) const;

	// Reset given notification
	static void Reset(TWeakPtr<SNotificationItem> NotificationManager);


	FCleaningStats CachedStats;
};
