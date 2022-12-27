// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Commandlets/ProjectCleanerCliCommandlet.h"

#include "ProjectCleanerTypes.h"
#include "Libs/ProjectCleanerLibAsset.h"

DEFINE_LOG_CATEGORY_STATIC(LogProjectCleanerCli, Display, All);

UProjectCleanerCliCommandlet::UProjectCleanerCliCommandlet()
{
	IsServer = false;
}

int32 UProjectCleanerCliCommandlet::Main(const FString& Params)
{
	UE_LOG(LogProjectCleanerCli, Display, TEXT("===================================="));
	UE_LOG(LogProjectCleanerCli, Display, TEXT("=======  ProjectCleaner CLI  ======="));
	UE_LOG(LogProjectCleanerCli, Display, TEXT("===================================="));

	FProjectCleanerScanSettings ScanSettings;
	FProjectCleanerScanData ScanData;
	UProjectCleanerLibAsset::ProjectScan(ScanSettings, ScanData);
	//
	// const auto SubsystemPtr = GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>();
	// SubsystemPtr->ProjectScan();

	UE_LOG(LogProjectCleanerCli, Display, TEXT("Scan Result - %s"), ScanData.ScanResult != EProjectCleanerScanResult::Success ? *ScanData.ScanResultMsg : TEXT("Ok"));
	UE_LOG(LogProjectCleanerCli, Display, TEXT("	Assets All - %d"), ScanData.AssetsAll.Num());
	UE_LOG(LogProjectCleanerCli, Display, TEXT("	Assets Used - %d"), ScanData.AssetsUnused.Num());
	UE_LOG(LogProjectCleanerCli, Display, TEXT("	Assets Primary - %d"), ScanData.AssetsPrimary.Num());
	UE_LOG(LogProjectCleanerCli, Display, TEXT("	Assets Indirect - %d"), ScanData.AssetsIndirect.Num());
	UE_LOG(LogProjectCleanerCli, Display, TEXT("	Assets Excluded - %d"), ScanData.AssetsExcluded.Num());
	UE_LOG(LogProjectCleanerCli, Display, TEXT("	Assets Unused - %d"), ScanData.AssetsUnused.Num());
	UE_LOG(LogProjectCleanerCli, Display, TEXT("	Folders All - %d"), ScanData.FoldersAll.Num());
	UE_LOG(LogProjectCleanerCli, Display, TEXT("	Folders Empty - %d"), ScanData.FoldersEmpty.Num());
	UE_LOG(LogProjectCleanerCli, Display, TEXT("	Files Corrupted - %d"), ScanData.FilesCorrupted.Num());
	UE_LOG(LogProjectCleanerCli, Display, TEXT("	Files NonEngine - %d"), ScanData.FilesNonEngine.Num());

	return Super::Main(Params);
}
