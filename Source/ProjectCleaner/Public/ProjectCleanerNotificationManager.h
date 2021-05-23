// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "StructsContainer.h"
// Engine Headers
#include "Widgets/Notifications/SNotificationList.h"
#include "CoreMinimal.h"

class FSlateNotificationManager;


/**
 * 
 */
class PROJECTCLEANER_API ProjectCleanerNotificationManager
{
public:
	/**
	 * @brief Create new persistent notification, will be in memory until Reset function call
	 * @param Text 
	 * @param CompletionState 
	 * @return 
	 */
	TWeakPtr<SNotificationItem> Add(const FString& Text,
	                                const SNotificationItem::ECompletionState CompletionState) const;

	/**
	 * @brief Transient Notification , used when no later updates needed for it
	 * @param Text 
	 * @param CompletionState 
	 * @param ExpireDuration 
	 */
	void AddTransient(const FString& Text,
	                  const SNotificationItem::ECompletionState
	                  CompletionState,
	                  const float ExpireDuration = 3.0f) const;

	/**
	 * @brief Updates content of given Notification
	 * @param NotificationManager 
	 * @param Stats 
	 */
	void Update(TWeakPtr<SNotificationItem> NotificationManager, const FCleaningStats& Stats) const;

	
	/**
	 * @brief Hides already existing notification
	 * @param NotificationManager 
	 * @param FinalText 
	 */
	void Hide(TWeakPtr<SNotificationItem> NotificationManager, const FText& FinalText) const;

	/**
	 * @brief Reset given notification
	 * @param NotificationManager 
	 */
	static void Reset(TWeakPtr<SNotificationItem> NotificationManager);
};
