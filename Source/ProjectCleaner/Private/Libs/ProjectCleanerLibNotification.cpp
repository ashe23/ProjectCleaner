// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Libs/ProjectCleanerLibNotification.h"
// Engine Headers
#include "ProjectCleanerConstants.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"

void UProjectCleanerLibNotification::Show(const FString& Msg, const EProjectCleanerModalStatus ModalStatus, const float Duration)
{
	FNotificationInfo Info{FText::FromString(Msg)};
	Info.ExpireDuration = Duration;

	const auto NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
	if (!NotificationPtr.IsValid()) return;

	NotificationPtr.Get()->SetCompletionState(GetCompletionStateFromModalStatus(ModalStatus));
}

void UProjectCleanerLibNotification::ShowOutputLog(const FString& Msg, const EProjectCleanerModalStatus ModalStatus, const float Duration)
{
	FNotificationInfo Info{FText::FromString(Msg)};
	Info.ExpireDuration = Duration;
	Info.Hyperlink = FSimpleDelegate::CreateLambda([]()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(ProjectCleanerConstants::TabOutputLog);
	});
	Info.HyperlinkText = FText::FromString(ProjectCleanerConstants::MsgOutputLogTitle);

	const auto NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
	if (!NotificationPtr.IsValid()) return;

	NotificationPtr.Get()->SetCompletionState(GetCompletionStateFromModalStatus(ModalStatus));
}

SNotificationItem::ECompletionState UProjectCleanerLibNotification::GetCompletionStateFromModalStatus(const EProjectCleanerModalStatus ModalStatus)
{
	if (ModalStatus == EProjectCleanerModalStatus::Pending)
	{
		return SNotificationItem::CS_Pending;
	}
	if (ModalStatus == EProjectCleanerModalStatus::Error)
	{
		return SNotificationItem::CS_Fail;
	}
	if (ModalStatus == EProjectCleanerModalStatus::OK)
	{
		return SNotificationItem::CS_Success;
	}

	return SNotificationItem::CS_None;
}
