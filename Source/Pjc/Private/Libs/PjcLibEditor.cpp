// // Copyright Ashot Barkhudaryan. All Rights Reserved.
//
// #include "Libs/PjcLibEditor.h"
// #include "PjcConstants.h"
// // Engine Headers
// #include "AssetToolsModule.h"
// #include "AssetRegistry/AssetRegistryModule.h"
// #include "Misc/ScopedSlowTask.h"
//
// bool FPjcLibEditor::EditorInPlayMode()
// {
// 	return GEditor && GEditor->PlayWorld || GIsPlayInEditorWorld;
// }
//
// bool FPjcLibEditor::AssetRegistryIsWorking()
// {
// 	return FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistry).Get().IsLoadingAssets();
// }
//
// bool FPjcLibEditor::ProjectContainsRedirectors()
// {
// 	FARFilter Filter;
// 	Filter.bRecursivePaths = true;
// 	Filter.PackagePaths.Emplace(PjcConstants::PathRoot);
// 	Filter.ClassNames.Emplace(UObjectRedirector::StaticClass()->GetFName());
//
// 	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistry);
// 	TArray<FAssetData> Redirectors;
// 	ModuleAssetRegistry.Get().GetAssets(Filter, Redirectors);
//
// 	return Redirectors.Num() > 0;
// }
//
// void FPjcLibEditor::FixupRedirectorsInProject()
// {
// 	FScopedSlowTask SlowTask{
// 		1.0f,
// 		FText::FromString(TEXT("Fixing redirectors...")),
// 		GIsEditor && !IsRunningCommandlet()
// 	};
// 	SlowTask.MakeDialog(false, false);
// 	SlowTask.EnterProgressFrame(1.0f);
//
// 	FARFilter Filter;
// 	Filter.bRecursivePaths = true;
// 	Filter.PackagePaths.Emplace(PjcConstants::PathRoot);
// 	Filter.ClassNames.Emplace(UObjectRedirector::StaticClass()->GetFName());
//
// 	TArray<FAssetData> AssetList;
// 	const FAssetRegistryModule& ModuleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistry);
// 	ModuleAssetRegistry.Get().GetAssets(Filter, AssetList);
//
// 	if (AssetList.Num() == 0) return;
//
// 	FScopedSlowTask LoadingTask(
// 		AssetList.Num(),
// 		FText::FromString(TEXT("Loading redirectors...")),
// 		GIsEditor && !IsRunningCommandlet()
// 	);
// 	LoadingTask.MakeDialog(false, false);
//
// 	TArray<UObjectRedirector*> Redirectors;
// 	Redirectors.Reserve(AssetList.Num());
//
// 	for (const auto& Asset : AssetList)
// 	{
// 		LoadingTask.EnterProgressFrame(1.0f);
//
// 		UObject* AssetObj = Asset.GetAsset();
// 		if (!AssetObj) continue;
//
// 		UObjectRedirector* Redirector = CastChecked<UObjectRedirector>(AssetObj);
// 		if (!Redirector) continue;
//
// 		Redirectors.Emplace(Redirector);
// 	}
//
// 	Redirectors.Shrink();
//
// 	const FAssetToolsModule& ModuleAssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(PjcConstants::ModuleAssetTools);
// 	ModuleAssetTools.Get().FixupReferencers(Redirectors, false);
// }
