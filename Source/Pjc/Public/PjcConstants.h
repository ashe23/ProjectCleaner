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
	static const FName ModulePropertyEditor{TEXT("PropertyEditor")};
	static const FName ModuleMegascans{TEXT("MegascansPlugin")};

	// paths
	static FName PathRoot{TEXT("/Game")};
	static FName PathDevelopers{TEXT("/Game/Developers")};
	static FName PathMSPresets{TEXT("/Game/MSPresets")};

	// tabs
	static const FName TabProjectCleaner{TEXT("TabProjectCleaner")};
	static const FName TabAssetsUnused{TEXT("TabAssetsUnused")};
	static const FName TabAssetsIndirect{TEXT("TabAssetsIndirect")};
	static const FName TabAssetsCorrupted{TEXT("TabAssetsCorrupted")};
	static const FName TabFilesExternal{TEXT("TabFilesExternal")};

	// widget index
	static int32 WidgetIndexIdle = 0;
	static int32 WidgetIndexWorking = 1;

	// misc
	static constexpr int32 BucketSize = 500;
	static const FName EmptyTagName{TEXT("PjcEmptyTag")};
	static const TSet<FString> EngineFileExtensions{TEXT("umap"), TEXT("uasset"), TEXT("collection")};
	static const TSet<FString> SourceFileExtensions{TEXT("cpp"), TEXT("h"), TEXT("cs")};
	static const TSet<FString> ConfigFileExtensions{TEXT("ini")};
	static const FString UrlGithub{TEXT("https://github.com/ashe23/ProjectCleaner")};
	static const FString UrlDocs{TEXT("https://github.com/ashe23/ProjectCleaner/wiki")};
	static const FString UrlIssueTracker{TEXT("https://github.com/ashe23/ProjectCleaner/issues")};
}
