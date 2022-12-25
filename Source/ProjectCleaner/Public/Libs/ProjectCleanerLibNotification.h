// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "ProjectCleanerLibNotification.generated.h"

UENUM(BlueprintType)
enum class EProjectCleanerModalState : uint8
{
	None UMETA(DisplayName = "None"),
	OK UMETA(DisplayName = "OK"),
	Pending UMETA(DisplayName = "Pending"),
	Error UMETA(DisplayName = "Error"),
};

UCLASS()
class UProjectCleanerLibNotification final : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|Lib|Notification")
	static void ShowModal(const FString& Msg, const EProjectCleanerModalState State = EProjectCleanerModalState::None, const float Duration = 2.0f);

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|Lib|Notification")
	static void ShowModalOutputLog(const FString& Msg, const EProjectCleanerModalState State = EProjectCleanerModalState::None, const float Duration = 2.0f);

	UFUNCTION(BlueprintCallable, Category="ProjectCleaner|Lib|Notification")
	static EAppReturnType::Type ShowDialogWindow(const FString& Title, const FString& Msg, const EAppMsgType::Type MsgType);

private:
	static SNotificationItem::ECompletionState GetCompletionStateFromModalState(const EProjectCleanerModalState ModalState);
};
