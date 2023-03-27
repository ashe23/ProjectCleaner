// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcSubsystem.h"
#include "PjcConstants.h"
#include "PjcSettings.h"
#include "Pjc.h"
// Engine Headers
#include "AssetToolsModule.h"
#include "IAssetRegistry.h"
#include "IAssetTools.h"
#include "FileHelpers.h"
#include "Engine/AssetManager.h"
#include "Engine/AssetManagerTypes.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"
#include "EditorTutorial.h"
#include "EditorUtilityBlueprint.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "ObjectTools.h"
#include "ShaderCompiler.h"
#include "Engine/MapBuildDataRegistry.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Libs/PjcLibAsset.h"
#include "Libs/PjcLibEditor.h"

void UPjcSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ModuleAssetRegistry = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>(PjcConstants::ModuleAssetRegistryName).Get();
	ModuleAssetTools = &FModuleManager::LoadModuleChecked<FAssetToolsModule>(PjcConstants::ModuleAssetToolsName);
}

void UPjcSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

#if WITH_EDITOR
void UPjcSubsystem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();
}
#endif

void UPjcSubsystem::ProjectScan() { }

void UPjcSubsystem::ProjectClean() const { }

void UPjcSubsystem::Test(const FName& Path)
{
	FPjcLibEditor::ShowNotification(TEXT("AAA"), SNotificationItem::CS_None, 10.0f);
}

FPjcDelegateOnProjectScan& UPjcSubsystem::OnProjectScan()
{
	return DelegateOnProjectScan;
}

bool UPjcSubsystem::CanScanProject(FString& ErrMsg) const
{
	ErrMsg.Empty();

	if (ScannerState == EPjcScannerState::Scanning)
	{
		ErrMsg = TEXT("Scanning is in progress. Please wait until it has finished and then try again.");
		return false;
	}

	if (ScannerState == EPjcScannerState::Cleaning)
	{
		ErrMsg = TEXT("Cleaning is in progress. Please wait until it has finished and then try again.");
		return false;
	}

	if (ModuleAssetRegistry->IsLoadingAssets())
	{
		ErrMsg = TEXT("Scanning of the project has failed. AssetRegistry is still discovering assets. Please try again after it has finished.");
		return false;
	}

	if (GEditor && GEditor->PlayWorld || GIsPlayInEditorWorld)
	{
		ErrMsg = TEXT("Scanning of the project has failed. AssetRegistry is still discovering assets. Please try again after it has finished.");
		return false;
	}

	if (!IsRunningCommandlet())
	{
		if (!GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllAssetEditors())
		{
			ErrMsg = TEXT("Scanning of the project has failed because not all asset editors are closed.");
			return false;
		}

		FPjcLibAsset::FixupRedirectorsInProject(true);
	}

	if (FPjcLibAsset::ProjectContainsRedirectors())
	{
		ErrMsg = TEXT("Failed to scan project, because not all redirectors are fixed.");
		return false;
	}

	if (!FEditorFileUtils::SaveDirtyPackages(true, true, true, false, false, false))
	{
		ErrMsg = TEXT("Scanning of the project has failed because not all assets have been saved.");
		return false;
	}

	return true;
}


EPjcScannerState UPjcSubsystem::GetScannerState() const
{
	return ScannerState;
}
