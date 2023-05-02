// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Libs/PjcLibAsset.h"
#include "PjcConstants.h"
// Engine Headers
#include "EditorTutorial.h"
#include "EditorUtilityBlueprint.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "Engine/AssetManager.h"

FAssetRegistryModule& FPjcLibAsset::GetAssetRegistry()
{
	return FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistry);
}

void FPjcLibAsset::GetClassNamesPrimary(TSet<FName>& OutClassNames)
{
	// getting list of primary asset classes that are defined in AssetManager
	const auto& AssetManager = UAssetManager::Get();
	if (!AssetManager.IsValid()) return;

	TSet<FName> ClassNamesPrimaryBase;
	TArray<FPrimaryAssetTypeInfo> AssetTypeInfos;
	AssetManager.Get().GetPrimaryAssetTypeInfoList(AssetTypeInfos);
	ClassNamesPrimaryBase.Reserve(AssetTypeInfos.Num());

	for (const auto& AssetTypeInfo : AssetTypeInfos)
	{
		if (!AssetTypeInfo.AssetBaseClassLoaded) continue;

		ClassNamesPrimaryBase.Emplace(AssetTypeInfo.AssetBaseClassLoaded->GetFName());
	}

	// getting list of primary assets classes that are derived from main primary assets
	const FAssetRegistryModule& AssetRegistry = GetAssetRegistry();

	OutClassNames.Empty();
	AssetRegistry.Get().GetDerivedClassNames(ClassNamesPrimaryBase.Array(), TSet<FName>{}, OutClassNames);
}

void FPjcLibAsset::GetClassNamesEditor(TSet<FName>& OutClassNames)
{
	const TArray<FName> ClassNamesEditorBase{
		UEditorUtilityWidget::StaticClass()->GetFName(),
		UEditorUtilityBlueprint::StaticClass()->GetFName(),
		UEditorUtilityWidgetBlueprint::StaticClass()->GetFName(),
		UEditorTutorial::StaticClass()->GetFName()
	};

	const FAssetRegistryModule& ModuleAssetRegistry = GetAssetRegistry();

	OutClassNames.Empty();
	ModuleAssetRegistry.Get().GetDerivedClassNames(ClassNamesEditorBase, TSet<FName>{}, OutClassNames);
}
