// Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.

#pragma once

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
	 * @param NotificationPtr
	 * @return 
	 */
	static void Add(
		const FText& Text,
		const SNotificationItem::ECompletionState CompletionState,
		TWeakPtr<SNotificationItem>& NotificationPtr
	);

	/**
	 * @brief Transient Notification , used when no later updates needed for it
	 * @param Text 
	 * @param CompletionState 
	 * @param ExpireDuration 
	 */
	static void AddTransient(
		const FText& Text,
	    const SNotificationItem::ECompletionState CompletionState,
		const float ExpireDuration = 3.0f
	);

	/**
	 * @brief Updates content of given Notification
	 * @param NotificationManager
	 * @param Text
	 */
	static void Update(TWeakPtr<SNotificationItem> NotificationManager, const FText& Text);

	/**
	 * @brief Hides already existing notification
	 * @param NotificationManager
	 * @param CompletionState
	 * @param FinalText 
	 */
	static void Hide(TWeakPtr<SNotificationItem> NotificationManager, const SNotificationItem::ECompletionState CompletionState, const FText& FinalText);

	/**
	 * @brief Reset given notification
	 * @param NotificationManager 
	 */
	static void Reset(TWeakPtr<SNotificationItem> NotificationManager);

	/**
	 * @brief Show Confirmation Window with given Title and ContextText
	 * @param Title 
	 * @param ContentText 
	 * @return EAppReturnType
	 */
	static EAppReturnType::Type ShowConfirmationWindow(const FText& Title, const FText& ContentText);

	/**
	 * @brief Check if Confirmation Window cancelled by user
	 * @param Status 
	 * @return bool
	 */
	static bool IsConfirmationWindowCanceled(EAppReturnType::Type Status);
};
