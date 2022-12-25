// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Libs/ProjectCleanerLibNotification.h"
// Engine Headers
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/INotificationWidget.h"

void UProjectCleanerLibNotification::ShowModal(const FString& Msg, const EProjectCleanerModalState State, const float Duration)
{
	FNotificationInfo Info{FText::FromString(Msg)};
	Info.ExpireDuration = Duration;

	const auto NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
	if (!NotificationPtr.IsValid()) return;

	NotificationPtr.Get()->SetCompletionState(GetCompletionStateFromModalState(State));
}

void UProjectCleanerLibNotification::ShowModalOutputLog(const FString& Msg, const EProjectCleanerModalState State, const float Duration)
{
	FNotificationInfo Info{FText::FromString(Msg)};
	Info.ExpireDuration = Duration;
	Info.Hyperlink = FSimpleDelegate::CreateLambda([]()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(FName{TEXT("OutputLog")});
	});
	Info.HyperlinkText = FText::FromString(TEXT("Show OutputLog..."));

	const auto NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
	if (!NotificationPtr.IsValid()) return;

	NotificationPtr.Get()->SetCompletionState(GetCompletionStateFromModalState(State));
}

SNotificationItem::ECompletionState UProjectCleanerLibNotification::GetCompletionStateFromModalState(const EProjectCleanerModalState ModalState)
{
	switch (ModalState)
	{
		case EProjectCleanerModalState::None: return SNotificationItem::CS_None;
		case EProjectCleanerModalState::OK: return SNotificationItem::CS_Success;
		case EProjectCleanerModalState::Pending: return SNotificationItem::CS_Pending;
		case EProjectCleanerModalState::Error: return SNotificationItem::CS_Fail;
		default: return SNotificationItem::CS_None;
	}
}
