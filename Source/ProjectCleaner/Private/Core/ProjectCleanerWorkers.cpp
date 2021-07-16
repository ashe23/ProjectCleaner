// // Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.
//
// #include "Core/ProjectCleanerWorkers.h"
// #include "StructsContainer.h"
// #include "Core/ProjectCleanerUtility.h"
//
// FProjectCleanerFileWorker::FProjectCleanerFileWorker(FProjectCleanerData* NewCleanerData, UCleanerConfigs* NewCleanerConfigs) :
// 	Thread(nullptr),
// 	bWorking(true),
// 	CleanerData(NewCleanerData),
// 	CleanerConfigs(NewCleanerConfigs)
// {
// 	ensure(CleanerData);
// 	ensure(CleanerConfigs);
// 	Thread = FRunnableThread::Create(this, TEXT("ProjectCleanerFileWorker"));
// }
//
// FProjectCleanerFileWorker::~FProjectCleanerFileWorker()
// {
// 	if (Thread)
// 	{
// 		Thread->Kill();
// 		delete Thread;
// 	}
// }
//
// bool FProjectCleanerFileWorker::Init()
// {
// 	return true;
// }
//
// uint32 FProjectCleanerFileWorker::Run()
// {
// 	// ProjectCleanerUtility::GetSourceAndConfigFiles(CleanerData->ProjectSourceAndConfigsFiles);
// 	// ProjectCleanerUtility::GetProjectFilesFromDisk(CleanerData->ProjectAllAssetsFiles);
// 	// ProjectCleanerUtility::GetEmptyFolders(CleanerData->EmptyFolders, CleanerConfigs->bScanDeveloperContents);
// 	// ProjectCleanerUtility::GetNonEngineFiles(CleanerData->NonEngineFiles, CleanerData->ProjectAllAssetsFiles);
//
// 	bWorking = false;
// 	return 0;
// }
//
// void FProjectCleanerFileWorker::Stop()
// {
// 	bWorking = false;
// }
//
// bool FProjectCleanerFileWorker::Finished() const
// {
// 	return !bWorking;
// }
