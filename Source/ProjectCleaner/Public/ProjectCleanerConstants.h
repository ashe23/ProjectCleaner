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
	static const FString MsgAssetRegistryStillWorking{TEXT("The AssetRegistry is still working. Please wait for the scan to finish")};

	// path
	static const FString PathRelRoot{TEXT("/Game")};
	static const FString PathRelDevelopers{TEXT("/Game/Developers")};
	static const FString PathRelMegascans{TEXT("/Game/Megascans")};
	static const FString PathRelMegascansPresets{TEXT("/Game/MSPresets")};

	// plugins
	static const FName PluginMegascans{TEXT("MegascansPlugin")};
	static const FString PluginMegascansMsPresetsFolder{TEXT("MSPresets")};
	
	// url
	static const FString UrlWiki{TEXT("https://github.com/ashe23/ProjectCleaner/wiki")};

	// misc
	static constexpr int32 HeaderRowFontSize = 12;
}
