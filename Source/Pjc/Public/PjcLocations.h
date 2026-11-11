// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// One place inside an asset that holds a reference to another asset
struct FPjcLocation
{
	// The asset that is set there
	FName Target;
	// Where: the parts of the asset from the outside in ("Components", "Mesh"), then the property that holds it
	TArray<FString> Parts;
	FString Property;
};

// The places of one asset
struct FPjcLocations
{
	TArray<FPjcLocation> Places;
	// Assets the asset registry says it uses, but no property of the loaded asset holds
	TArray<FName> NotFound;
	// The package was not in memory before: Unload lets it go again
	bool bLoadedHere = false;
};

/**
 * Where inside an asset each asset it uses is set. The asset registry knows which assets a package uses, not where:
 * for that the package is loaded and every object in it is walked, property by property. Nothing is loaded unless
 * asked for (the Inspector asks on a button), maps never.
 */
class FPjcLocationReader
{
  public:

	// Whether the package may be read: an asset of the scanned kind, not a map
	static bool CanRead(const FName Package);
	// Size on disk of the package and of what it loads with it that is not in memory yet
	static int64 GetLoadSize(const FName Package);
	// Loads the package and reads it. False when it cannot be loaded
	static bool Read(const FName Package, FPjcLocations& Out);
	// Lets the packages go that Read loaded. What the editor still holds stays in memory; returns how many went
	static int32 Unload(const TArray<FName>& Packages);
	static bool IsLoaded(const FName Package);
};
