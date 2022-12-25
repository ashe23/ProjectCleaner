// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Libs/ProjectCleanerLibAsset.h"
#include "ProjectCleanerConstants.h"
// Engine Headers
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"

bool UProjectCleanerLibAsset::AssetRegistryWorking()
{
	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	return ModuleAssetRegistry.Get().IsLoadingAssets();
}

FString UProjectCleanerLibAsset::GetAssetClassName(const FAssetData& AssetData)
{
	if (!AssetData.IsValid()) return {};

	if (AssetData.AssetClass.IsEqual("Blueprint"))
	{
		const auto GeneratedClassName = AssetData.TagsAndValues.FindTag(TEXT("GeneratedClass")).GetValue();
		const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(*GeneratedClassName);
		return FPackageName::ObjectPathToObjectName(ClassObjectPath);
	}

	return AssetData.AssetClass.ToString();
}

void UProjectCleanerLibAsset::GetAssetsAll(TArray<FAssetData>& Assets)
{
	Assets.Empty();

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	ModuleAssetRegistry.Get().GetAssetsByPath(ProjectCleanerConstants::PathRelRoot, Assets, true);

	// todo:ashe23 for ue5 make sure we exclude __External*__ folders
}

void UProjectCleanerLibAsset::GetAssetsPrimary(TArray<FAssetData>& AssetsPrimary)
{
	AssetsPrimary.Empty();

	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	// getting list of primary asset classes that are defined in AssetManager
	const auto& AssetManager = UAssetManager::Get();
	if (!AssetManager.IsValid()) return;

	TArray<FName> PrimaryAssetClasses;
	TArray<FPrimaryAssetTypeInfo> AssetTypeInfos;

	AssetManager.Get().GetPrimaryAssetTypeInfoList(AssetTypeInfos);
	PrimaryAssetClasses.Reserve(AssetTypeInfos.Num());

	for (const auto& AssetTypeInfo : AssetTypeInfos)
	{
		if (!AssetTypeInfo.AssetBaseClassLoaded) continue;

		PrimaryAssetClasses.AddUnique(AssetTypeInfo.AssetBaseClassLoaded->GetFName());
	}

	// getting list of primary assets classes that are derived from main primary assets
	TSet<FName> DerivedFromPrimaryAssets;
	{
		const TSet<FName> ExcludedClassNames;
		ModuleAssetRegistry.Get().GetDerivedClassNames(PrimaryAssetClasses, ExcludedClassNames, DerivedFromPrimaryAssets);
	}

	for (const auto& DerivedClassName : DerivedFromPrimaryAssets)
	{
		PrimaryAssetClasses.AddUnique(DerivedClassName);
	}

	FARFilter Filter;
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);

	for (const auto& ClassName : PrimaryAssetClasses)
	{
		Filter.ClassNames.Add(ClassName);
	}
	ModuleAssetRegistry.Get().GetAssets(Filter, AssetsPrimary);

	// getting primary blueprint assets
	FARFilter FilterBlueprint;
	FilterBlueprint.bRecursivePaths = true;
	FilterBlueprint.bRecursiveClasses = true;
	FilterBlueprint.PackagePaths.Add(ProjectCleanerConstants::PathRelRoot);
	FilterBlueprint.ClassNames.Add(UBlueprint::StaticClass()->GetFName());

	TArray<FAssetData> BlueprintAssets;
	ModuleAssetRegistry.Get().GetAssets(FilterBlueprint, BlueprintAssets);

	for (const auto& BlueprintAsset : BlueprintAssets)
	{
		const FName BlueprintClass = FName{*GetAssetClassName(BlueprintAsset)};
		if (PrimaryAssetClasses.Contains(BlueprintClass))
		{
			AssetsPrimary.AddUnique(BlueprintAsset);
		}
	}
}
