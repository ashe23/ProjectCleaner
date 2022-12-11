// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "ProjectCleanerSubsystem.h"
#include "Settings/ProjectCleanerExcludeSettings.h"
// Engine Headers
#include "ProjectCleaner.h"
#include "AssetRegistry/AssetRegistryModule.h"

UProjectCleanerSubsystem::UProjectCleanerSubsystem()
	: ModuleAssetRegistry(&FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName))
{
}

void UProjectCleanerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	GetMutableDefault<UProjectCleanerExcludeSettings>()->OnChange().AddLambda([&](const FName PropertyName)
	{
		UE_LOG(LogProjectCleaner, Warning, TEXT("Subsystem: Exclude settings updated, %s"), *PropertyName.ToString());

		ScanDataState = EProjectCleanerScanDataState::ObsoleteBySettings;
	});
}

void UProjectCleanerSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

#if WITH_EDITOR
void UProjectCleanerSubsystem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();
}
#endif

void UProjectCleanerSubsystem::ProjectScan()
{
	CheckEditorState();

	if (EditorState != EProjectCleanerEditorState::Idle || ScanState != EProjectCleanerScanState::Idle)
	{
		return;
	}

	ScanState = EProjectCleanerScanState::Scanning;
	ScanDataState = EProjectCleanerScanDataState::None;
	AssetRegistryDelegatesUnregister();


	// todo:ashe23

	ScanState = EProjectCleanerScanState::Idle;
	ScanDataState = EProjectCleanerScanDataState::Actual;
	AssetRegistryDelegatesRegister();
}

void UProjectCleanerSubsystem::CheckEditorState()
{
	if (!GEditor) return;

	if (GEditor->PlayWorld || GIsPlayInEditorWorld)
	{
		EditorState = EProjectCleanerEditorState::PlayMode;
		return;
	}

	if (ModuleAssetRegistry->Get().IsLoadingAssets())
	{
		EditorState = EProjectCleanerEditorState::AssetRegistryWorking;
		return;
	}

	EditorState = EProjectCleanerEditorState::Idle;
}

void UProjectCleanerSubsystem::NotifyMainTabActivated()
{
	AssetRegistryDelegatesRegister();
}

void UProjectCleanerSubsystem::NotifyMainTabClosed()
{
	AssetRegistryDelegatesUnregister();
}

EProjectCleanerEditorState UProjectCleanerSubsystem::GetEditorState() const
{
	return EditorState;
}

EProjectCleanerScanState UProjectCleanerSubsystem::GetScanState() const
{
	return ScanState;
}

EProjectCleanerScanDataState UProjectCleanerSubsystem::GetScanDataState() const
{
	return ScanDataState;
}

void UProjectCleanerSubsystem::AssetRegistryDelegatesRegister()
{
	if (!ModuleAssetRegistry) return;

	if (!DelegateHandleAssetAdded.IsValid())
	{
		DelegateHandleAssetAdded = ModuleAssetRegistry->Get().OnAssetAdded().AddLambda([&](const FAssetData&)
		{
			ScanDataState = EProjectCleanerScanDataState::ObsoleteByAssetRegistry;
		});
	}

	if (!DelegateHandleAssetRemoved.IsValid())
	{
		DelegateHandleAssetRemoved = ModuleAssetRegistry->Get().OnAssetRemoved().AddLambda([&](const FAssetData&)
		{
			ScanDataState = EProjectCleanerScanDataState::ObsoleteByAssetRegistry;
		});
	}

	if (!DelegateHandleAssetRenamed.IsValid())
	{
		DelegateHandleAssetRenamed = ModuleAssetRegistry->Get().OnAssetRenamed().AddLambda([&](const FAssetData&, const FString&)
		{
			ScanDataState = EProjectCleanerScanDataState::ObsoleteByAssetRegistry;
		});
	}

	if (!DelegateHandleAssetUpdated.IsValid())
	{
		DelegateHandleAssetUpdated = ModuleAssetRegistry->Get().OnAssetUpdated().AddLambda([&](const FAssetData&)
		{
			ScanDataState = EProjectCleanerScanDataState::ObsoleteByAssetRegistry;
		});
	}

	if (!DelegateHandlePathAdded.IsValid())
	{
		DelegateHandlePathAdded = ModuleAssetRegistry->Get().OnPathAdded().AddLambda([&](const FString&)
		{
			ScanDataState = EProjectCleanerScanDataState::ObsoleteByAssetRegistry;
		});
	}

	if (!DelegateHandlePathRemoved.IsValid())
	{
		DelegateHandlePathRemoved = ModuleAssetRegistry->Get().OnPathRemoved().AddLambda([&](const FString&)
		{
			ScanDataState = EProjectCleanerScanDataState::ObsoleteByAssetRegistry;
		});
	}
}

void UProjectCleanerSubsystem::AssetRegistryDelegatesUnregister()
{
	if (!ModuleAssetRegistry) return;

	if (DelegateHandleAssetAdded.IsValid())
	{
		ModuleAssetRegistry->Get().OnAssetAdded().Remove(DelegateHandleAssetAdded);
		DelegateHandleAssetAdded.Reset();
	}

	if (DelegateHandleAssetRemoved.IsValid())
	{
		ModuleAssetRegistry->Get().OnAssetRemoved().Remove(DelegateHandleAssetRemoved);
		DelegateHandleAssetRemoved.Reset();
	}

	if (DelegateHandleAssetRenamed.IsValid())
	{
		ModuleAssetRegistry->Get().OnAssetRenamed().Remove(DelegateHandleAssetRenamed);
		DelegateHandleAssetRenamed.Reset();
	}

	if (DelegateHandleAssetUpdated.IsValid())
	{
		ModuleAssetRegistry->Get().OnAssetUpdated().Remove(DelegateHandleAssetUpdated);
		DelegateHandleAssetUpdated.Reset();
	}

	if (DelegateHandlePathAdded.IsValid())
	{
		ModuleAssetRegistry->Get().OnPathAdded().Remove(DelegateHandlePathAdded);
		DelegateHandlePathAdded.Reset();
	}

	if (DelegateHandlePathRemoved.IsValid())
	{
		ModuleAssetRegistry->Get().OnPathRemoved().Remove(DelegateHandlePathRemoved);
		DelegateHandlePathRemoved.Reset();
	}
}
