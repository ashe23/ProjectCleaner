// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcCookInfo.h"
// Engine Headers
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryState.h"
#include "HAL/FileManager.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/MemoryReader.h"

namespace
{
	// unity builds join files: names unique to this file
	FPjcCookInfo PjcCookInfoLast;
}	// namespace

const FPjcCookInfo& FPjcCookInfo::GetLast() {
	// newest registry over every platform folder
	FString NewestPlatform;
	FString NewestFile;
	FDateTime NewestTime {0};
	TArray<FString> Platforms;
	IFileManager::Get().FindFiles(Platforms, *(GetCookedDir() / TEXT("*")), false, true);
	for (const auto& Platform : Platforms) {
		const FString File = GetCookedDir() / Platform / FApp::GetProjectName() / TEXT("Metadata") / TEXT("DevelopmentAssetRegistry.bin");
		const FDateTime Time = IFileManager::Get().GetTimeStamp(*File);
		if (Time != FDateTime::MinValue() && Time > NewestTime) {
			NewestPlatform = Platform;
			NewestFile = File;
			NewestTime = Time;
		}
	}

	if (NewestFile.IsEmpty()) {
		PjcCookInfoLast = FPjcCookInfo {};
		return PjcCookInfoLast;
	}
	if (PjcCookInfoLast.File == NewestFile && PjcCookInfoLast.Time == NewestTime) {
		return PjcCookInfoLast;
	}

	PjcCookInfoLast = FPjcCookInfo {};
	PjcCookInfoLast.Platform = NewestPlatform;
	PjcCookInfoLast.File = NewestFile;
	PjcCookInfoLast.Time = NewestTime;
	PjcCookInfoLast.bReadFailed = !Read(NewestFile, PjcCookInfoLast.Cooked);
	return PjcCookInfoLast;
}

FString FPjcCookInfo::GetCookedDir() {
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Cooked"));
}

bool FPjcCookInfo::Read(const FString& InFile, TMap<FName, int64>& OutCooked) {
	OutCooked.Reset();

	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *InFile)) return false;

	FMemoryReader Reader {Bytes};
	FAssetRegistryLoadOptions Options;
	Options.bLoadDependencies = false;
	Options.bLoadPackageData = true;
	FAssetRegistryState State;
	if (!State.Load(Reader, Options) || Reader.IsError()) return false;

	// the cooker marks packages it did not cook with a negative size (editor only, failed, skipped)
	for (const auto& Pair : State.GetAssetPackageDataMap()) {
		if (Pair.Value && Pair.Value->DiskSize >= 0) {
			OutCooked.Add(Pair.Key, Pair.Value->DiskSize);
		}
	}
	return true;
}
