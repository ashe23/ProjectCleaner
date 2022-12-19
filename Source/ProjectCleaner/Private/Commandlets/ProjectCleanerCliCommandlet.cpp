// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Commandlets/ProjectCleanerCliCommandlet.h"
#include "ProjectCleanerSubsystem.h"

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

	const auto SubsystemPtr = GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>();
	SubsystemPtr->ProjectScan();

	UE_LOG(LogProjectCleanerCli, Display, TEXT("Scan Result - %s"), SubsystemPtr->GetScanData().ScanResult != EProjectCleanerScanResult::Success ? *SubsystemPtr->GetScanData().ScanResultMsg : TEXT("Ok"));
	UE_LOG(LogProjectCleanerCli, Display, TEXT("Assets All - %d"), SubsystemPtr->GetScanData().AssetsAll.Num());
	UE_LOG(LogProjectCleanerCli, Display, TEXT("Assets Used - %d"), SubsystemPtr->GetScanData().AssetsUnused.Num());
	UE_LOG(LogProjectCleanerCli, Display, TEXT("Assets Primary - %d"), SubsystemPtr->GetScanData().AssetsPrimary.Num());
	UE_LOG(LogProjectCleanerCli, Display, TEXT("Assets Indirect - %d"), SubsystemPtr->GetScanData().AssetsIndirect.Num());
	UE_LOG(LogProjectCleanerCli, Display, TEXT("Assets Excluded - %d"), SubsystemPtr->GetScanData().AssetsExcluded.Num());
	UE_LOG(LogProjectCleanerCli, Display, TEXT("Assets Unused - %d"), SubsystemPtr->GetScanData().AssetsUnused.Num());
	UE_LOG(LogProjectCleanerCli, Display, TEXT("Folders All - %d"), SubsystemPtr->GetScanData().FoldersAll.Num());
	UE_LOG(LogProjectCleanerCli, Display, TEXT("Folders Empty - %d"), SubsystemPtr->GetScanData().FoldersEmpty.Num());
	UE_LOG(LogProjectCleanerCli, Display, TEXT("Files Corrupted - %d"), SubsystemPtr->GetScanData().FilesCorrupted.Num());
	UE_LOG(LogProjectCleanerCli, Display, TEXT("Files NonEngine - %d"), SubsystemPtr->GetScanData().FilesNonEngine.Num());

	return Super::Main(Params);
}
