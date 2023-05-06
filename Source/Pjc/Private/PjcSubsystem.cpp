// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcSubsystem.h"
#include "Pjc.h"
#include "PjcConstants.h"
#include "Libs/PjcLibAsset.h"
// #include "Misc/ScopedSlowTask.h"

void UpdateAssetCountByPathRecursive(TMap<FString, int32>& Map, const FString& AssetPath)
{
	FString CurrentPath = AssetPath;

	// Iterate through all parent folders and update the asset count
	while (!CurrentPath.IsEmpty())
	{
		if (Map.Contains(CurrentPath))
		{
			Map[CurrentPath]++;
		}
		else
		{
			Map.Add(CurrentPath, 1);
		}

		// Remove the last folder in the path
		int32 LastSlashIndex;
		if (CurrentPath.FindLastChar('/', LastSlashIndex))
		{
			CurrentPath.LeftInline(LastSlashIndex, false);
		}
		else
		{
			CurrentPath.Empty();
		}
	}
}

void UPjcSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UPjcSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UPjcSubsystem::ToggleShowFoldersEmpty()
{
	bShowFoldersEmpty = !bShowFoldersEmpty;
	PostEditChange();
}

void UPjcSubsystem::ToggleShowFoldersExcluded()
{
	bShowFoldersExcluded = !bShowFoldersExcluded;
	PostEditChange();
}

bool UPjcSubsystem::CanShowFoldersEmpty() const
{
	return bShowFoldersEmpty;
}

bool UPjcSubsystem::CanShowFoldersExcluded() const
{
	return bShowFoldersExcluded;
}

void UPjcSubsystem::ScanProjectAssets()
{
	const double TimeStart = FPlatformTime::Seconds();

	// resetting cached data
	AssetsAll.Reset();
	AssetsUsed.Reset();
	AssetsUnused.Reset();
	AssetsPrimary.Reset();
	AssetsIndirect.Reset();
	AssetsEditor.Reset();
	AssetsExcluded.Reset();
	AssetsExtReferenced.Reset();
	AssetsIndirectInfoMap.Reset();
	MapNumAssetsAllByPath.Reset();
	MapNumAssetsUsedByPath.Reset();
	MapNumAssetsUnusedByPath.Reset();
	MapSizeAssetsAllByPath.Reset();
	MapSizeAssetsUsedByPath.Reset();
	MapSizeAssetsUnusedByPath.Reset();

	// loading initial data
	TArray<FAssetData> ContainerAssetsAll;
	TArray<FAssetData> ContainerAssetsIndirect;

	TSet<FName> ClassNamesPrimary;
	TSet<FName> ClassNamesEditor;
	TSet<FName> ClassNamesExcluded;

	FPjcLibAsset::GetClassNamesPrimary(ClassNamesPrimary);
	FPjcLibAsset::GetClassNamesEditor(ClassNamesEditor);
	FPjcLibAsset::GetClassNamesExcluded(ClassNamesExcluded);
	FPjcLibAsset::GetAssetsExcludedByPaths(AssetsExcluded);
	FPjcLibAsset::GetAssetsInPath(PjcConstants::PathRoot.ToString(), true, ContainerAssetsAll);
	FPjcLibAsset::GetAssetsIndirect(AssetsIndirectInfoMap);
	AssetsIndirectInfoMap.GetKeys(ContainerAssetsIndirect);
	AssetsIndirect.Append(ContainerAssetsIndirect);

	// traversing assets and filling them by category
	for (const auto& Asset : ContainerAssetsAll)
	{
		const FName AssetExactClassName = FPjcLibAsset::GetAssetExactClassName(Asset);
		const bool bIsPrimary = ClassNamesPrimary.Contains(Asset.AssetClass) || ClassNamesPrimary.Contains(AssetExactClassName);
		const bool bIsEditor = ClassNamesEditor.Contains(Asset.AssetClass) || ClassNamesEditor.Contains(AssetExactClassName);
		const bool bIsExcludedByClass = ClassNamesExcluded.Contains(Asset.AssetClass) || ClassNamesExcluded.Contains(AssetExactClassName);
		const bool bIsExtReferenced = FPjcLibAsset::AssetIsExtReferenced(Asset);
		const bool bIsUsed = bIsPrimary || bIsEditor || bIsExcludedByClass || bIsExtReferenced;

		if (bIsPrimary)
		{
			AssetsPrimary.Emplace(Asset);
		}

		if (bIsEditor)
		{
			AssetsEditor.Emplace(Asset);
		}

		if (bIsExcludedByClass)
		{
			AssetsExcluded.Emplace(Asset);
		}

		if (bIsExtReferenced)
		{
			AssetsExtReferenced.Emplace(Asset);
		}

		if (bIsUsed)
		{
			AssetsUsed.Emplace(Asset);
		}
	}

	// now filling used assets container and getting their dependencies recursive
	AssetsUsed.Append(AssetsPrimary);
	AssetsUsed.Append(AssetsEditor);
	AssetsUsed.Append(AssetsIndirect);
	AssetsUsed.Append(AssetsExcluded);
	AssetsUsed.Append(AssetsExtReferenced);
	FPjcLibAsset::GetAssetsDeps(AssetsUsed);

	// now filling unused assets
	AssetsAll.Append(ContainerAssetsAll);
	AssetsUnused = AssetsAll.Difference(AssetsUsed);

	CategorizeAssetsByPath();

	const double TimeElapsed = FPlatformTime::Seconds() - TimeStart;
	UE_LOG(LogProjectCleaner, Warning, TEXT("Project Assets Scanned In %.2f seconds"), TimeElapsed);

	if (DelegateOnScanAssets.IsBound())
	{
		DelegateOnScanAssets.Broadcast();
	}
}

const TSet<FAssetData>& UPjcSubsystem::GetAssetsAll() const
{
	return AssetsAll;
}

const TSet<FAssetData>& UPjcSubsystem::GetAssetsUsed() const
{
	return AssetsUsed;
}

const TSet<FAssetData>& UPjcSubsystem::GetAssetsUnused() const
{
	return AssetsUnused;
}

const TSet<FAssetData>& UPjcSubsystem::GetAssetsPrimary() const
{
	return AssetsPrimary;
}

const TSet<FAssetData>& UPjcSubsystem::GetAssetsIndirect() const
{
	return AssetsIndirect;
}

const TSet<FAssetData>& UPjcSubsystem::GetAssetsEditor() const
{
	return AssetsEditor;
}

const TSet<FAssetData>& UPjcSubsystem::GetAssetsExcluded() const
{
	return AssetsExcluded;
}

const TSet<FAssetData>& UPjcSubsystem::GetAssetsExtReferenced() const
{
	return AssetsExcluded;
}

int32 UPjcSubsystem::GetNumAssetsTotalInPath(const FString& InPath) const
{
	return MapNumAssetsAllByPath.Contains(InPath) ? MapNumAssetsAllByPath[InPath] : 0;
}

int32 UPjcSubsystem::GetNumAssetsUsedInPath(const FString& InPath) const
{
	return MapNumAssetsUsedByPath.Contains(InPath) ? MapNumAssetsUsedByPath[InPath] : 0;
}

int32 UPjcSubsystem::GetNumAssetsUnusedInPath(const FString& InPath) const
{
	return MapNumAssetsUnusedByPath.Contains(InPath) ? MapNumAssetsUnusedByPath[InPath] : 0;
}

int64 UPjcSubsystem::GetSizeAssetsTotalInPath(const FString& InPath) const
{
	return MapSizeAssetsAllByPath.Contains(InPath) ? MapSizeAssetsAllByPath[InPath] : 0;
}

int64 UPjcSubsystem::GetSizeAssetsUsedInPath(const FString& InPath) const
{
	return MapSizeAssetsUsedByPath.Contains(InPath) ? MapSizeAssetsUsedByPath[InPath] : 0;
}

int64 UPjcSubsystem::GetSizeAssetsUnusedInPath(const FString& InPath) const
{
	return MapSizeAssetsUnusedByPath.Contains(InPath) ? MapSizeAssetsUnusedByPath[InPath] : 0;
}

FPjcDelegateOnScanAssets& UPjcSubsystem::OnScanAssets()
{
	return DelegateOnScanAssets;
}

#if WITH_EDITOR
void UPjcSubsystem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();
}
#endif

void UPjcSubsystem::CategorizeAssetsByPath()
{
	MapNumAssetsAllByPath.Reset();
	MapNumAssetsUsedByPath.Reset();
	MapNumAssetsUnusedByPath.Reset();
	MapSizeAssetsAllByPath.Reset();
	MapSizeAssetsUsedByPath.Reset();
	MapSizeAssetsUnusedByPath.Reset();

	for (const FAssetData& Asset : AssetsAll)
	{
		const FString AssetPath = Asset.PackagePath.ToString();
		const int64 AssetSize = FPjcLibAsset::GetAssetSize(Asset);
		UpdateMapInfo(MapNumAssetsAllByPath, MapSizeAssetsAllByPath, AssetPath, AssetSize);
	}

	for (const FAssetData& Asset : AssetsUsed)
	{
		const FString AssetPath = Asset.PackagePath.ToString();
		const int64 AssetSize = FPjcLibAsset::GetAssetSize(Asset);
		UpdateMapInfo(MapNumAssetsUsedByPath, MapSizeAssetsUsedByPath, AssetPath, AssetSize);
	}

	for (const FAssetData& Asset : AssetsUnused)
	{
		const FString AssetPath = Asset.PackagePath.ToString();
		const int64 AssetSize = FPjcLibAsset::GetAssetSize(Asset);
		UpdateMapInfo(MapNumAssetsUnusedByPath, MapSizeAssetsUnusedByPath, AssetPath, AssetSize);
	}
}

void UPjcSubsystem::UpdateMapInfo(TMap<FString, int32>& MapNum, TMap<FString, int64>& MapSize, const FString& AssetPath, int64 AssetSize)
{
	FString CurrentPath = AssetPath;

	// Iterate through all parent folders and update the asset count and size
	while (!CurrentPath.IsEmpty())
	{
		if (MapNum.Contains(CurrentPath))
		{
			MapNum[CurrentPath]++;
			MapSize[CurrentPath] += AssetSize;
		}
		else
		{
			MapNum.Add(CurrentPath, 1);
			MapSize.Add(CurrentPath, AssetSize);
		}

		// Remove the last folder in the path
		int32 LastSlashIndex;
		if (CurrentPath.FindLastChar('/', LastSlashIndex))
		{
			CurrentPath.LeftInline(LastSlashIndex, false);
		}
		else
		{
			CurrentPath.Empty();
		}
	}
}
