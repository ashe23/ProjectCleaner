// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetRegistryModule.h"

struct FPjcLibAsset
{
	static FAssetRegistryModule& GetAssetRegistry();

private:
	static void GetClassNamesPrimary(TSet<FName>& OutClassNames);
	static void GetClassNamesEditor(TSet<FName>& OutClassNames);
};
