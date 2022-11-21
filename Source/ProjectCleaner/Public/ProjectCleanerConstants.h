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

	// messages
	static const FString MsgFixingRedirectors{TEXT("Fixing up redirectors... ")};
	static const FString MsgLoadingAssets{TEXT("Loading Assets... ")};

	// path
	static const FString PathRelativeRoot{TEXT("/Game")};
	static const FString PathMegascansPlugin{TEXT("/Game/MSPresets")};
	
}
