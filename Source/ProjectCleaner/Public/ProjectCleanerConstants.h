// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace ProjectCleanerConstants
{
	// module
	static const FName ModuleName{TEXT("ProjectCleaner")};
	static const FName ModuleFullName{TEXT("FProjectCleanerModule")};
	static const FName ModuleStylesName{TEXT("ProjectCleanerStyles")};

	// tabs
	static const FName TabProjectCleaner{TEXT("TabProjectCleaner")};
	static const FName TabScanSettings{TEXT("TabScanSettings")};
	static const FName TabUnusedAssets{TEXT("TabUnusedAssets")};
	static const FName TabCorruptedAssets{TEXT("TabCorruptedFiles")};
	// static const FName TabExcludedAssets{TEXT("TabExcludedAssets")};
	static const FName TabIndirectAssets{TEXT("TabIndirectAssets")};
	static const FName TabNonEngineFiles{TEXT("TabNonEngineFiles")};

	// messages
	static const FString MsgFixingRedirectors{TEXT("Fixing up redirectors... ")};
	static const FString MsgLoadingRedirectors{TEXT("Loading redirectors... ")};
	static const FString MsgLoadingAssets{TEXT("Loading Assets... ")};
	static const FString MsgLoadingAssetsBlacklist{TEXT("Loading blacklisted folders and assets...")};
	static const FString MsgLoadingAssetsUnused{TEXT("Scanning for unused assets...")};
	static const FString MsgScanning{TEXT("Scanning... ")};
	static const FString MsgScanningContentFolder{TEXT("Scanning Content Folder... ")};
	static const FString MsgAssetRegistryStillWorking{TEXT("The AssetRegistry is still working. Please wait for the scan to finish")};
	static const FString MsgConfirmCleanProjectTitle{TEXT("Confirm project cleaning")};
	static const FString MsgConfirmCleanProject{TEXT("Are you sure you want to permanently delete unused assets?")};
	static const FString MsgConfirmDeleteEmptyFoldersTitle{TEXT("Confirm empty folders cleaning")};
	static const FString MsgConfirmDeleteEmptyFolders{TEXT("Are you sure you want to delete all empty folders in project?")};
	static const FString MsgTabNonEngineTitle{TEXT("List of NonEngine files inside Content folder")};
	static const FString MsgTabNonEngineDesc{TEXT("Make sure you delete all unnecessary files, in order to remove empty folders. This files wont be visible in ContentBrowser")};
	static const FString MsgNavigateToExplorer{TEXT("Double click on row to navigate file in FileExplorer")};
	static const FString MsgFileDeleteSuccess{TEXT("File deleted successfully")};
	static const FString MsgFileDeleteError{TEXT("Failed to delete file")};

	// path
	static const FName PathRelRoot{TEXT("/Game")};
	static const FName PathRelDevelopers{TEXT("/Game/Developers")};
	static const FName PathRelMegascans{TEXT("/Game/Megascans")};
	static const FName PathRelMegascansPresets{TEXT("/Game/MSPresets")};

	// folders
	static const FName FolderContent{TEXT("Content")};
	static const FName FolderDevelopers{TEXT("Developers")};
	static const FName FolderCollections{TEXT("Collections")};
	static const FName FolderMsPresets{TEXT("MSPresets")};

	// plugins
	static const FName PluginNameMegascans{TEXT("MegascansPlugin")};
	// static const FName PluginMegascansMsPresetsFolder{TEXT("MSPresets")};

	// url
	static const FString UrlWiki{TEXT("https://github.com/ashe23/ProjectCleaner/wiki")};

	// misc
	static constexpr int32 HeaderRowFontSize = 10;
}
