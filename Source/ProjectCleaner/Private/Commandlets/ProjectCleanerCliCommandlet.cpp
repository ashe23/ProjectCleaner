// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Commandlets/ProjectCleanerCliCommandlet.h"

#include "ProjectCleanerSubsystem.h"
#include "ProjectCleanerTypes.h"
#include "Libs/ProjectCleanerLibAsset.h"

DEFINE_LOG_CATEGORY_STATIC(LogProjectCleanerCli, Display, All);

UProjectCleanerCliCommandlet::UProjectCleanerCliCommandlet()
{
	IsClient = false;
	IsServer = false;
}

int32 UProjectCleanerCliCommandlet::Main(const FString& Params)
{
	UE_LOG(LogProjectCleanerCli, Display, TEXT("===================================="));
	UE_LOG(LogProjectCleanerCli, Display, TEXT("=======  ProjectCleaner CLI  ======="));
	UE_LOG(LogProjectCleanerCli, Display, TEXT("===================================="));

	const FString ProjectPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	const FString ProjectName = FApp::GetProjectName();
	

	UE_LOG(LogProjectCleanerCli, Display, TEXT("ProjectPath: %s"), *ProjectPath);
	UE_LOG(LogProjectCleanerCli, Display, TEXT("ProjectName: %s"), *ProjectName);

	if (!GEditor) return -1;
	
	UProjectCleanerSubsystem* SubsystemPtr = GEditor->GetEditorSubsystem<UProjectCleanerSubsystem>();
	if (!SubsystemPtr) return -1;
	
	// todo:ashe23 add settings version
	SubsystemPtr->ProjectScan();
	
	const FProjectCleanerScanData ScanData = SubsystemPtr->GetScanData();
	
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

	return 0;
}
