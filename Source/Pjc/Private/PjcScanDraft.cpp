// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcScanDraft.h"
#include "PjcSettings.h"
#include "PjcShim.h"

namespace
{
	// unity builds join files: names unique to this file
	TOptional<FPjcScanDraft> PjcSharedDraft;
	FSimpleMulticastDelegate PjcDraftEdited;
}	// namespace

FPjcScanDraft FPjcScanDraft::FromSettings() {
	const UPjcScanSettings* Settings = GetDefault<UPjcScanSettings>();

	FPjcScanDraft Result;
	Result.bScanProjectPlugins = Settings->bScanProjectPlugins;
	Result.IgnoredPlugins = Settings->IgnoredPlugins;
	Result.bUnregisteredPrimaryUsed = Settings->bUnregisteredPrimaryUsed;
	Result.bFolderLabelsUsed = Settings->bFolderLabelsUsed;
	Result.bCinematicAssetsUsed = Settings->bCinematicAssetsUsed;
	for (const auto& UsedClass : Settings->UsedClasses) {
		if (!UsedClass.IsNull()) {
			Result.UsedClasses.Add(UsedClass.ToSoftObjectPath().ToString());
		}
	}
	return Result;
}

void FPjcScanDraft::ApplyToSettings() const {
	UPjcScanSettings* Settings = GetMutableDefault<UPjcScanSettings>();
	Settings->bScanProjectPlugins = bScanProjectPlugins;
	Settings->IgnoredPlugins = IgnoredPlugins;
	Settings->bUnregisteredPrimaryUsed = bUnregisteredPrimaryUsed;
	Settings->bFolderLabelsUsed = bFolderLabelsUsed;
	Settings->bCinematicAssetsUsed = bCinematicAssetsUsed;
	Settings->UsedClasses.Reset();
	for (const auto& ClassPath : UsedClasses) {
		Settings->UsedClasses.Add(TSoftClassPtr<UObject> {FSoftObjectPath {ClassPath}});
	}
	PjcShim::UpdateDefaultConfigFile(Settings);
}

bool FPjcScanDraft::operator==(const FPjcScanDraft& Other) const {
	return bScanProjectPlugins == Other.bScanProjectPlugins && IgnoredPlugins == Other.IgnoredPlugins
		&& bUnregisteredPrimaryUsed == Other.bUnregisteredPrimaryUsed && bFolderLabelsUsed == Other.bFolderLabelsUsed
		&& bCinematicAssetsUsed == Other.bCinematicAssetsUsed && UsedClasses == Other.UsedClasses;
}

FPjcScanDraft& FPjcScanDraft::Get() {
	if (!PjcSharedDraft.IsSet()) {
		PjcSharedDraft = FromSettings();
	}
	return PjcSharedDraft.GetValue();
}

bool FPjcScanDraft::IsChanged() {
	return Get() != FromSettings();
}

void FPjcScanDraft::Revert() {
	PjcSharedDraft = FromSettings();
	PjcDraftEdited.Broadcast();
}

FSimpleMulticastDelegate& FPjcScanDraft::OnEdited() {
	return PjcDraftEdited;
}
