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
	static const FName TabUnusedAssets{TEXT("TabUnusedAssets")};
	static const FName TabExcludedAssets{TEXT("TabExcludedAssets")};
	static const FName TabIndirectAssets{TEXT("TabIndirectAssets")};
	static const FName TabNonEngineFiles{TEXT("TabNonEngineFiles")};
	static const FName TabCorruptedFiles{TEXT("TabCorruptedFiles")};

	// messages
	static const FString MsgFixingRedirectors{TEXT("Fixing up redirectors... ")};
	static const FString MsgLoadingRedirectors{TEXT("Loading redirectors... ")};
	static const FString MsgLoadingAssets{TEXT("Loading Assets... ")};
	static const FString MsgAssetRegistryStillWorking{TEXT("The AssetRegistry is still working. Please wait for the scan to finish")};

	// path
	static const FString PathRelativeRoot{TEXT("/Game")};
	static const FString PathMegascans{TEXT("/Game/Megascans")};
	static const FString PathMegascansPresets{TEXT("/Game/MSPresets")};

	// url
	static const FString UrlWiki{TEXT("https://github.com/ashe23/ProjectCleaner/wiki")};
}
