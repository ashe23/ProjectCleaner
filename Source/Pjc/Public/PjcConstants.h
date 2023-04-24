// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace PjcConstants
{
	// modules
	static const FName ModuleName{TEXT("ProjectCleaner")};
	static const FName ModuleStylesName{TEXT("ProjectCleanerStyles")};
	static const FName ModuleTitle{TEXT("Project Cleaner")};
	static const FName ModuleAssetRegistryName{TEXT("AssetRegistry")};
	static const FName ModuleAssetToolsName{TEXT("AssetTools")};
	static const FName ModuleMegascans{TEXT("MegascansPlugin")};
	static const FName ModuleContentBrowser{TEXT("ContentBrowser")};

	// paths
	static FName PathRelRoot{TEXT("/Game")};
	static FName PathRelMSPresets{TEXT("/Game/MSPresets")};

	// tabs
	static const FName TabProjectCleaner{TEXT("TabProjectCleaner")};
	static const FName TabAssetsBrowser{TEXT("TabAssetsBrowser")};
	static const FName TabFilesBrowser{TEXT("TabFilesBrowser")};
	// static const FName TabScanSettings{TEXT("TabScanSettings")};
	// static const FName TabScanInfo{TEXT("TabScanInfo")};
	// static const FName TabFilesCorrupted{TEXT("TabFilesCorrupted")};

	// widget index
	static int32 WidgetIndexIdle = 0;
	static int32 WidgetIndexWorking = 1;

	// misc
	// static int32 NoSize{-1};
	// static int32 HeaderRowFontSize{10};
	// static FMargin HeaderRowMargin{5.0f};
	static const FName EmptyTagName{TEXT("PjcEmptyTag")};
	static const TSet<FString> EngineFileExtensions{TEXT("umap"), TEXT("uasset"), TEXT("collection")};
	static const TSet<FString> SourceFileExtensions{TEXT("cpp"), TEXT("h"), TEXT("cs")};
	static const TSet<FString> ConfigFileExtensions{TEXT("ini")};
	static const FString UrlWiki{TEXT("")};
}
