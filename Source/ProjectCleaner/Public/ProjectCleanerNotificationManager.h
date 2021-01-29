// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class SNotificationItem;
class FSlateNotificationManager;


/**
 * 
 */
class PROJECTCLEANER_API ProjectCleanerNotificationManager
{
public:
	void Show();
	// void UpdateProgress();
	void Hide();


	TWeakPtr<SNotificationItem> NotificationManager;
};
