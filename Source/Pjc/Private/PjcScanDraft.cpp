// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcScanDraft.h"
#include "PjcSettings.h"
#include "PjcShim.h"
#include "PjcTypes.h"

namespace
{
	// unity builds join files: names unique to this file
	TOptional<FPjcScanDraft> PjcSharedDraft;
	FSimpleMulticastDelegate PjcDraftEdited;

	// until edited in a panel, the draft follows the settings when they change elsewhere (e.g. Project Settings)
	bool bPjcDraftEdited = false;
	FPjcScanDraft PjcDraftBase;
}	// namespace

FPjcScanDraft FPjcScanDraft::FromSettings() {
	const UPjcScanSettings* Settings = GetDefault<UPjcScanSettings>();
	const UPjcAssetExcludeSettings* Exclusions = GetDefault<UPjcAssetExcludeSettings>();

	FPjcScanDraft Result;
	Result.bScanProjectPlugins = Settings->bScanProjectPlugins;
	Result.IgnoredPlugins = Settings->IgnoredPlugins;
	Result.bScanDevelopers = Settings->bScanDevelopers;
	Result.bUnregisteredPrimaryUsed = Settings->bUnregisteredPrimaryUsed;
	Result.bCinematicAssetsUsed = Settings->bCinematicAssetsUsed;
	for (const auto& UsedClass : Settings->UsedClasses) {
		if (!UsedClass.IsNull()) {
			Result.UsedClasses.Add(UsedClass.ToSoftObjectPath().ToString());
		}
	}
	for (const auto& Folder : Exclusions->ExcludedFolders) {
		if (!Folder.Path.IsEmpty()) {
			Result.ExcludedFolders.Add(Folder.Path);
		}
	}
	for (const auto& Class : Exclusions->ExcludedClasses) {
		if (!Class.IsNull()) {
			Result.ExcludedClasses.Add(Class.ToSoftObjectPath().ToString());
		}
	}
	for (const auto& Asset : Exclusions->ExcludedAssets) {
		if (!Asset.IsNull()) {
			Result.ExcludedAssets.Add(Asset.ToSoftObjectPath().ToString());
		}
	}
	return Result;
}

void FPjcScanDraft::ApplyToSettings() const {
	UPjcScanSettings* Settings = GetMutableDefault<UPjcScanSettings>();
	Settings->bScanProjectPlugins = bScanProjectPlugins;
	Settings->IgnoredPlugins = IgnoredPlugins;
	Settings->bScanDevelopers = bScanDevelopers;
	Settings->bUnregisteredPrimaryUsed = bUnregisteredPrimaryUsed;
	Settings->bCinematicAssetsUsed = bCinematicAssetsUsed;
	Settings->UsedClasses.Reset();
	for (const auto& ClassPath : UsedClasses) {
		Settings->UsedClasses.Add(TSoftClassPtr<UObject> {FSoftObjectPath {ClassPath}});
	}
	PjcShim::UpdateDefaultConfigFile(Settings);

	UPjcAssetExcludeSettings* Exclusions = GetMutableDefault<UPjcAssetExcludeSettings>();
	Exclusions->ExcludedFolders.Reset();
	for (const auto& Folder : ExcludedFolders) {
		Exclusions->ExcludedFolders.Add(FDirectoryPath {Folder});
	}
	Exclusions->ExcludedClasses.Reset();
	for (const auto& ClassPath : ExcludedClasses) {
		Exclusions->ExcludedClasses.Add(TSoftClassPtr<UObject> {FSoftObjectPath {ClassPath}});
	}
	Exclusions->ExcludedAssets.Reset();
	for (const auto& AssetPath : ExcludedAssets) {
		Exclusions->ExcludedAssets.Add(TSoftObjectPtr<UObject> {FSoftObjectPath {AssetPath}});
	}
	PjcShim::UpdateDefaultConfigFile(Exclusions);

	bPjcDraftEdited = false;
	PjcDraftBase = FromSettings();
}

bool FPjcScanDraft::operator==(const FPjcScanDraft& Other) const {
	const bool bScope
		= bScanProjectPlugins == Other.bScanProjectPlugins && IgnoredPlugins == Other.IgnoredPlugins && bScanDevelopers == Other.bScanDevelopers;
	const bool bRules = bUnregisteredPrimaryUsed == Other.bUnregisteredPrimaryUsed && bCinematicAssetsUsed == Other.bCinematicAssetsUsed;
	const bool bUsed = UsedClasses == Other.UsedClasses;
	const bool bExcluded = ExcludedFolders == Other.ExcludedFolders && ExcludedClasses == Other.ExcludedClasses;
	return bScope && bRules && bUsed && bExcluded && ExcludedAssets == Other.ExcludedAssets;
}

FPjcScanDraft& FPjcScanDraft::Get() {
	if (!PjcSharedDraft.IsSet()) {
		PjcSharedDraft = FromSettings();
		PjcDraftBase = PjcSharedDraft.GetValue();
	}
	else if (!bPjcDraftEdited) {
		// settings changed elsewhere: take them over, edits made since the last look stay otherwise
		FPjcScanDraft Settings = FromSettings();
		if (Settings != PjcDraftBase) {
			PjcDraftBase = Settings;
			PjcSharedDraft = MoveTemp(Settings);
		}
	}
	return PjcSharedDraft.GetValue();
}

bool FPjcScanDraft::IsChanged() {
	return Get() != FromSettings();
}

void FPjcScanDraft::NotifyEdited() {
	bPjcDraftEdited = true;
	PjcDraftEdited.Broadcast();
}

void FPjcScanDraft::Revert() {
	bPjcDraftEdited = false;
	PjcSharedDraft = FromSettings();
	PjcDraftBase = PjcSharedDraft.GetValue();
	PjcDraftEdited.Broadcast();
}

FSimpleMulticastDelegate& FPjcScanDraft::OnEdited() {
	return PjcDraftEdited;
}
