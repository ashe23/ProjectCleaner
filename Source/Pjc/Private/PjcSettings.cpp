// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcSettings.h"

UPjcScanSettings::UPjcScanSettings() {}

FName UPjcScanSettings::GetCategoryName() const {
	return TEXT("Plugins");
}

#if WITH_EDITOR
FText UPjcScanSettings::GetSectionText() const {
	return FText::FromString(TEXT("Project Cleaner"));
}

FText UPjcScanSettings::GetSectionDescription() const {
	return FText::FromString(TEXT("Rules that decide which assets Project Cleaner considers used. Shared with the team via DefaultEditor.ini."));
}
#endif

TArray<FString> UPjcScanSettings::GetCinematicClassPaths() {
	// classes from disabled plugins or other engine versions simply do not resolve and are skipped
	return {
	  TEXT("/Script/LevelSequence.LevelSequence"),
	  TEXT("/Script/TemplateSequence.TemplateSequence"),
	  TEXT("/Script/MovieRenderPipelineCore.MoviePipelineQueue"),
	  TEXT("/Script/MovieRenderPipelineCore.MoviePipelinePrimaryConfig"),
	  TEXT("/Script/MovieRenderPipelineCore.MoviePipelineMasterConfig"),
	  TEXT("/Script/MovieRenderPipelineCore.MovieGraphConfig"),
	};
}

FName UPjcEditorSettings::GetContainerName() const {
	return TEXT("Editor");
}

FName UPjcEditorSettings::GetCategoryName() const {
	return TEXT("Plugins");
}

#if WITH_EDITOR
FText UPjcEditorSettings::GetSectionText() const {
	return FText::FromString(TEXT("Project Cleaner"));
}

FText UPjcEditorSettings::GetSectionDescription() const {
	return FText::FromString(TEXT("Project Cleaner interface preferences."));
}

void UPjcEditorSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) {
	Super::PostEditChangeProperty(PropertyChangedEvent);

	OnChanged.Broadcast();
}
#endif
