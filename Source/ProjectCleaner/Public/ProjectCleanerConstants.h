// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace ProjectCleanerConstants
{
	// module
	static const FName ModuleName{TEXT("ProjectCleaner")};
	static const FName ModuleFullName{TEXT("FProjectCleanerModule")};
	static const FName ModuleStylesName{TEXT("ProjectCleanerStyles")};
	static const FName ModuleTitle{TEXT("Project Cleaner")};

	// tabs
	static const FName TabProjectCleaner{TEXT("TabProjectCleaner")};
	static const FName TabScanSettings{TEXT("TabScanSettings")};
	static const FName TabUnusedAssets{TEXT("TabUnusedAssets")};
	static const FName TabIndirectAssets{TEXT("TabIndirectAssets")};
	static const FName TabCorruptedAssets{TEXT("TabCorruptedAssets")};
	static const FName TabNonEngineFiles{TEXT("TabNonEngineFiles")};

	// widget index
	static int32 WidgetIndexIdle = 0;
	static int32 WidgetIndexWorking = 1;

	// paths
	static FName PathRelRoot{TEXT("/Game")};

	// ui settings
	static constexpr int32 HeaderRowFontSize = 10;
}
