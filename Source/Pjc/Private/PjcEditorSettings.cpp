// // Copyright Ashot Barkhudaryan. All Rights Reserved.
//
// #include "PjcEditorSettings.h"
//
// FName UPjcEditorSettings::GetContainerName() const
// {
// 	return FName{TEXT("Project")};
// }
//
// FName UPjcEditorSettings::GetCategoryName() const
// {
// 	return FName{TEXT("Plugins")};
// }
//
// FName UPjcEditorSettings::GetSectionName() const
// {
// 	return FName{TEXT("PjcSettings")};
// }
//
// FText UPjcEditorSettings::GetSectionText() const
// {
// 	return FText::FromString(TEXT("Project Cleaner"));
// }
//
// FText UPjcEditorSettings::GetSectionDescription() const
// {
// 	return FText::FromString(TEXT("Project Cleaner Settings"));
// }
//
// #if WITH_EDITOR
// void UPjcEditorSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
// {
// 	Super::PostEditChangeProperty(PropertyChangedEvent);
//
// 	SaveConfig();
// }
// #endif
