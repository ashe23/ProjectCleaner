// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcSubsystem.h"

#include "PjcConstants.h"
#include "PjcTypes.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Libs/PjcLibAsset.h"

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

	// traversing assets and filling them by category
	for (const auto& Asset : ContainerAssetsAll)
	{
		const FName AssetExactClassName = FPjcLibAsset::GetAssetExactClassName(Asset);
		const bool bIsPrimary =  ClassNamesPrimary.Contains(Asset.AssetClass) || ClassNamesPrimary.Contains(AssetExactClassName);
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
	AssetsUsed.Append(ContainerAssetsIndirect);
	AssetsUsed.Append(AssetsExcluded);
	AssetsUsed.Append(AssetsExtReferenced);
	FPjcLibAsset::LoadAssetsDependencies(AssetsUsed);

	// now filling unused assets
	AssetsAll.Append(ContainerAssetsAll);
	AssetsUnused = AssetsAll.Difference(AssetsUsed);
}

const TSet<FAssetData>& UPjcSubsystem::GetAssetsAll() const
{
	return AssetsAll;
}

#if WITH_EDITOR
void UPjcSubsystem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();
}
#endif
