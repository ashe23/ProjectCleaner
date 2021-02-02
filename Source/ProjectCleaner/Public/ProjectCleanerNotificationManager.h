// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "StructsContainer.h"

class SNotificationItem;
class FSlateNotificationManager;


/**
 * 
 */
class PROJECTCLEANER_API ProjectCleanerNotificationManager
{
public:
	void Show(const FCleaningStats& Stats);
	void Update(const FCleaningStats& Stats);
	void Hide();


	TWeakPtr<SNotificationItem> NotificationManager;
};
