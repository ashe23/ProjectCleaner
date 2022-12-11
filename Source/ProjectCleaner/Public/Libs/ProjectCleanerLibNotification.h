// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "ProjectCleanerLibNotification.generated.h"

UCLASS()
class UProjectCleanerLibNotification final : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static void Show(const FString& Msg, const EProjectCleanerModalStatus ModalStatus, const float Duration = 2.0f);
	static void ShowOutputLog(const FString& Msg, const EProjectCleanerModalStatus ModalStatus, const float Duration = 2.0f);
private:
	static SNotificationItem::ECompletionState GetCompletionStateFromModalStatus(const EProjectCleanerModalStatus ModalStatus);
};
