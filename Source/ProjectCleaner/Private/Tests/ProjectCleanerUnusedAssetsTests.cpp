#include "CoreTypes.h"
#include "Misc/AutomationTest.h"
#include "FileHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/MaterialFactoryNew.h"
#include "ObjectTools.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProjectCleanerUnusedAssetsTests, "ProjectCleaner.UnusedAssets", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)


// Handle creation and deletion of assets
struct FCleanerAssetHelper
{
	template<typename TAssetClass, typename TFactoryClass>
	static void CreateAsset(const FString& AssetName, const FString& PackagePath, UClass* AssetClass, UClass* FactoryClass)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
		
		const FString PackageName = PackagePath + TEXT("/") + AssetName;
		const EObjectFlags Flags = RF_Public | RF_Standalone;
		
		UPackage* Package = CreatePackage(*PackageName);
		if (!Package) return;
		
		UFactory* FactoryInst = NewObject<TFactoryClass>();
		if (!FactoryInst) return;
		
		UObject* CreatedAsset = FactoryInst->FactoryCreateNew(AssetClass, Package, *AssetName, Flags, nullptr, GWarn);
		if (CreatedAsset)
		{
			AssetRegistryModule.AssetCreated(CreatedAsset);
			Package->MarkPackageDirty();
			UE_LOG(LogTemp, Display, TEXT("Created Assets"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Cant create asset"));
		}
	}
};

bool FProjectCleanerUnusedAssetsTests::RunTest(const FString& Parameters)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	const FString ProjectContentDir{"/Game"};

	// TEST CASE 1 all assets are under / Game folder, all filters disabled, no excluded assets
	TArray<FAssetData> UnusedAssets;
	{
		FCleanerAssetHelper::CreateAsset<UMaterial, UMaterialFactoryNew>(TEXT("M_TestMaterial"), ProjectContentDir, UMaterial::StaticClass(), UMaterialFactoryNew::StaticClass());
		AssetRegistryModule.Get().GetAssetsByPath(*ProjectContentDir, UnusedAssets);

		TestEqual(TEXT("UnusedAssets num must"), UnusedAssets.Num(), 1);
	}

	// cleanup
	ObjectTools::DeleteAssets(UnusedAssets, false);
	
	//FString MaterialBaseName = "M_Material";
	//FString PackageName = "/Game/";
	//PackageName += MaterialBaseName;
	//UPackage* Package = CreatePackage(nullptr, *PackageName);

	//auto MaterialFactory = NewObject<UMaterialFactoryNew>();
	//UMaterial* UnrealMaterial = (UMaterial*)MaterialFactory->FactoryCreateNew(UMaterial::StaticClass(), Package, *MaterialBaseName, RF_Standalone | RF_Public, nullptr, GWarn);
	//AssetRegistryModule.Get().AssetCreated(UnrealMaterial);
	//Package->FullyLoad();
	//Package->SetDirtyFlag(true);

	//FEditorFileUtils::SaveDirtyPackages(
	//	true,
	//	true,
	//	true,
	//	false,
	//	false,
	//	false
	//);

	//TArray<FAssetData> Assets;
	//AssetRegistryModule.Get().GetAssetsByPath(TEXT("/Game"), Assets);

	//for (const auto& Asset : Assets)
	//{
	//	UE_LOG(LogTemp, Display, TEXT("%s"), *Asset.PackageName.ToString());
	//}

	//TestEqual(TEXT("Assets Num must"), Assets.Num(), 0);
	return true;
}

#endif