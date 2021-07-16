// // Copyright 2021. Ashot Barkhudaryan. All Rights Reserved.
//
// #pragma once
//
// #include "CoreMinimal.h"
// #include "HAL/Runnable.h"
//
// /**
// *
// */
// class FProjectCleanerFileWorker : public FRunnable
// {
// public:
// 	// Constructor, create the thread by calling this
// 	FProjectCleanerFileWorker(struct FProjectCleanerData* NewCleanerData, class UCleanerConfigs* NewCleanerConfigs);
//
// 	// Destructor
// 	virtual ~FProjectCleanerFileWorker() override;
//
// 	// Overriden from FRunnable
// 	// Do not call these functions youself, that will happen automatically
// 	virtual bool Init() override; // Do your setup here, allocate memory, ect.
// 	virtual uint32 Run() override; // Main data processing happens here
// 	virtual void Stop() override; // Clean up any memory you allocated here
// 	bool Finished() const;
// private:
// 	// Thread handle. Control the thread using this, with operators like Kill and Suspend
// 	FRunnableThread* Thread;
// 	// Used to know when the thread should exit, changed in Stop(), read in Run()
// 	bool bWorking;
// 	FProjectCleanerData* CleanerData;
// 	UCleanerConfigs* CleanerConfigs;
// };
//
// // class FProjectCleanerFileWorker : public FNonAbandonableTask
// // {
// // 	friend class FAutoDeleteAsyncTask<FProjectCleanerFileWorker>;
// // public:
// // 	FProjectCleanerFileWorker(FProjectCleanerData* Data, UCleanerConfigs* Configs, UExcludeOptions* Options) :
// // 		CleanerData(Data),
// // 		CleanerConfigs(Configs),
// // 		ExcludeOptions(Options)
// // 	{
// // 		ensure(Data && CleanerConfigs && ExcludeOptions);
// // 	}
// //
// // 	FORCEINLINE TStatId GetStatId() const
// // 	{
// // 		RETURN_QUICK_DECLARE_CYCLE_STAT(FProjectCleanerWorkers, STATGROUP_ThreadPoolAsyncTasks);
// // 	}
// //
// // 	void DoWork()
// // 	{
// // 		ProjectCleanerUtility::GetPrimaryAssetClasses(CleanerData->PrimaryAssetClasses);
// // 		ProjectCleanerUtility::GetEmptyFolders(CleanerData->EmptyFolders, CleanerConfigs->bScanDeveloperContents);
// // 		ProjectCleanerUtility::GetSourceAndConfigFiles(CleanerData->SourceAndConfigFiles);
// // 		ProjectCleanerUtility::GetProjectFilesFromDisk(CleanerData->ProjectFilesFromDisk);
// // 		ProjectCleanerUtility::GetNonEngineFiles(CleanerData->NonEngineFiles, CleanerData->ProjectFilesFromDisk);
// // 	}
// //
// // private:
// // 	FProjectCleanerData* CleanerData;
// // 	UCleanerConfigs* CleanerConfigs;
// // 	UExcludeOptions* ExcludeOptions;
// // };
//
