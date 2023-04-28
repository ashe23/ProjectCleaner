// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace PjcConstants
{
	// modules
	static const FName ModulePjcName{TEXT("ProjectCleaner")};
	static const FName ModulePjcStylesName{TEXT("ProjectCleanerStyles")};
	static const FName ModulePjcTitle{TEXT("Project Cleaner")};
	static const FName ModuleAssetRegistry{TEXT("AssetRegistry")};
	static const FName ModuleAssetTools{TEXT("AssetTools")};
	static const FName ModuleContentBrowser{TEXT("ContentBrowser")};

	// paths
	static FName PathRoot{TEXT("/Game")};
	static FName PathMSPresets{TEXT("/Game/MSPresets")};

	// tabs
	static const FName TabProjectCleaner{TEXT("TabProjectCleaner")};
	// static const FName TabAssetsBrowser{TEXT("TabAssetsBrowser")};
	// static const FName TabFilesBrowser{TEXT("TabFilesBrowser")};
	// static const FName TabScanSettings{TEXT("TabScanSettings")};
	// static const FName TabScanInfo{TEXT("TabScanInfo")};
	// static const FName TabFilesCorrupted{TEXT("TabFilesCorrupted")};

	// widget index
	static int32 WidgetIndexIdle = 0;
	static int32 WidgetIndexWorking = 1;

	// misc
	static const FName EmptyTagName{TEXT("PjcEmptyTag")};
	static const TSet<FString> EngineFileExtensions{TEXT("umap"), TEXT("uasset"), TEXT("collection")};
	static const TSet<FString> SourceFileExtensions{TEXT("cpp"), TEXT("h"), TEXT("cs")};
	static const TSet<FString> ConfigFileExtensions{TEXT("ini")};
	static const FString UrlWiki{TEXT("")}; // todo:ashe23 change later
}
