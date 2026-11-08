// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcCookInfo.h"
#include "PjcShim.h"
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
	const FPjcCookInfo PjcCookInfoNone;
	// by platform folder
	TMap<FString, FPjcCookInfo> PjcCookInfoCache;
}	// namespace

bool FPjcCookInfo::IsSavedSince(const FName Package) const {
	// what the package was saved with tells it exactly; file time when that is not known (restoring from source
	// control touches files without changes)
	const uint32* CookDigest = SavedDigests.Find(Package);
	const uint32 EditorDigest = CookDigest && *CookDigest != 0 ? PjcShim::GetPackageSavedDigest(Package) : 0;
	if (EditorDigest != 0) return EditorDigest != *CookDigest;

	FString PackageFile;
	return PjcShim::DoesPackageExist(Package.ToString(), &PackageFile) && IFileManager::Get().GetTimeStamp(*PackageFile) > Time;
}

const FPjcCookInfo& FPjcCookInfo::GetLast() {
	const TArray<const FPjcCookInfo*> Cooks = GetAll();
	return Cooks.Num() > 0 ? *Cooks[0] : PjcCookInfoNone;
}

TArray<const FPjcCookInfo*> FPjcCookInfo::GetAll() {
	TArray<FString> Platforms;
	IFileManager::Get().FindFiles(Platforms, *(GetCookedDir() / TEXT("*")), false, true);
	Platforms.Sort();

	TArray<const FPjcCookInfo*> Cooks;
	for (const auto& Platform : Platforms) {
		const FString File = GetCookedDir() / Platform / FApp::GetProjectName() / TEXT("Metadata") / TEXT("DevelopmentAssetRegistry.bin");
		const FDateTime Time = IFileManager::Get().GetTimeStamp(*File);
		if (Time == FDateTime::MinValue()) {
			PjcCookInfoCache.Remove(Platform);
			continue;
		}

		FPjcCookInfo& Cook = PjcCookInfoCache.FindOrAdd(Platform);
		if (Cook.File != File || Cook.Time != Time) {
			Cook = FPjcCookInfo {};
			Cook.Platform = Platform;
			Cook.File = File;
			Cook.Time = Time;
			Cook.bReadFailed = !Read(File, Cook.Cooked, Cook.SavedDigests);
		}
		Cooks.Add(&Cook);
	}

	// platform folders that are gone
	for (auto It = PjcCookInfoCache.CreateIterator(); It; ++It) {
		if (!Platforms.Contains(It.Key())) {
			It.RemoveCurrent();
		}
	}

	Cooks.Sort([](const FPjcCookInfo& A, const FPjcCookInfo& B) {
		return A.Time != B.Time ? A.Time > B.Time : A.Platform < B.Platform;
	});
	return Cooks;
}

FString FPjcCookInfo::GetCookedDir() {
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Cooked"));
}

bool FPjcCookInfo::Read(const FString& InFile, TMap<FName, int64>& OutCooked, TMap<FName, uint32>& OutSavedDigests) {
	OutCooked.Reset();
	OutSavedDigests.Reset();

	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *InFile)) return false;

	FMemoryReader Reader {Bytes};
	FAssetRegistryLoadOptions Options;
	Options.bLoadDependencies = false;
	Options.bLoadPackageData = true;
	FAssetRegistryState State;
	if (!State.Load(Reader, Options) || Reader.IsError()) return false;

	// the cooker marks packages it did not cook with a negative size (editor only, failed, skipped).
	// Script packages are native code: listed with size 0, they are not content
	for (const auto& Pair : State.GetAssetPackageDataMap()) {
		if (!Pair.Value || Pair.Key.ToString().StartsWith(TEXT("/Script/"))) continue;

		OutSavedDigests.Add(Pair.Key, PjcShim::GetPackageSavedDigest(*Pair.Value));
		if (Pair.Value->DiskSize >= 0) {
			OutCooked.Add(Pair.Key, Pair.Value->DiskSize);
		}
	}
	return true;
}
