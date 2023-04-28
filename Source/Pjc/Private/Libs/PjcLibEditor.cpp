// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Libs/PjcLibEditor.h"
#include "PjcConstants.h"
// Engine Headers
#include "AssetRegistry/AssetRegistryModule.h"

bool FPjcLibEditor::EditorInPlayMode()
{
	return GEditor && GEditor->PlayWorld || GIsPlayInEditorWorld;
}

bool FPjcLibEditor::AssetRegistryIsWorking()
{
	return FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistry).Get().IsLoadingAssets();
}
