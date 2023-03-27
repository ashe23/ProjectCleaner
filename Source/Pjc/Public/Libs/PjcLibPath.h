// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FPjcLibPath
{
	static FString ProjectDir();
	static FString ContentDir();
	static FString SourceDir();
	static FString ConfigDir();
	static FString PluginsDir();
	static FString SavedDir();
	static FString DevelopersDir();
	static FString CollectionsDir();
	static FString CurrentUserDevelopersDir();
	static FString CurrentUserCollectionsDir();
	static FString Normalize(const FString& InPath);
	static FString ToAbsolute(const FString& InPath);
	static FString ToAssetPath(const FString& InPath);
	static FName ToObjectPath(const FString& InPath);
};