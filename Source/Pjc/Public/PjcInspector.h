// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "PjcScanResult.h"

// What a reference is, from the asset registry dependency properties
enum class EPjcLinkKind : uint8
{
	// loads together with the asset
	Hard,
	// loaded on demand by path
	Soft,
	// kept in the editor, does not ship with the game
	EditorOnly,
};

// Reference kinds an answer counts, one bit per EPjcLinkKind
namespace PjcLinkKinds
{
	constexpr uint8 Hard = 1;
	constexpr uint8 Soft = 2;
	constexpr uint8 EditorOnly = 4;
	constexpr uint8 All = Hard | Soft | EditorOnly;

	inline uint8 Of(const EPjcLinkKind Kind) {
		return Kind == EPjcLinkKind::Hard ? Hard : Kind == EPjcLinkKind::Soft ? Soft : EditorOnly;
	}
}	// namespace PjcLinkKinds

// One row of an asset's "Used by" or "Uses" list
struct FPjcInspectorLink
{
	FName Package;
	EPjcLinkKind Kind = EPjcLinkKind::Hard;
	// Not a reference of its own: it goes through an actor or object the map saves in its own file
	bool bThrough = false;
};

// What the Inspector read from the asset registry, kept so the next asset opens fast: a map with thousands of actor
// files takes seconds to read. Its owner empties it when the project is scanned again
struct FPjcInspectorCache
{
	struct FLinks
	{
		TArray<FPjcInspectorLink> Links;
		int32 Through = 0;
	};

	TMap<FName, FLinks> UsedBy;
	TMap<FName, FLinks> Uses;
	// actor or object file -> its map, None when it has none
	TMap<FName, FName> Owners;

	void Reset() {
		UsedBy.Reset();
		Uses.Reset();
		Owners.Reset();
	}
};

// A reference between two assets of the model
struct FPjcModelLink
{
	int32 Other = INDEX_NONE;
	EPjcLinkKind Kind = EPjcLinkKind::Hard;
	// Not a reference: the scan keeps the asset for this one (a localized copy, an asset in the folder of a label)
	EPjcUsageReason Via = EPjcUsageReason::None;
	bool bThrough = false;
};

// A chain of references from a root down to an asset, root first
struct FPjcRoute
{
	TArray<int32> Path;
	int32 Root = INDEX_NONE;
	int32 Steps = 0;
	// the root is an editor asset or a reference on the way is editor only: only the editor needs it this way
	bool bEditor = false;
};

/**
 * Every reference between the scanned assets, read once per scan for the Inspector. It answers one question at a time
 * about an asset: which root keeps it and through what, what every route passes, what goes when it goes, what uses it
 * and what it uses over all levels. Assets outside the scanned content are in it as the ends of references.
 */
struct FPjcInspectorModel
{
	// The scan it was read for
	FDateTime Time {0};
	TArray<FName> Packages;
	TArray<FAssetData> Assets;
	TArray<TArray<FPjcModelLink>> Uses;
	TArray<TArray<FPjcModelLink>> UsedBy;
	TArray<int64> Sizes;
	// Scanned: in the scan result. Used: kept by the scan (Review too). Root: kept for its own reason, not through another asset
	TArray<bool> bScanned;
	TArray<bool> bUsed;
	TArray<bool> bRoot;
	TArray<EPjcUsageReason> Reasons;

	// Reads every reference of the scanned assets (a progress window when it takes long)
	void Build(const FPjcScanResult& Result, FPjcInspectorCache* Cache = nullptr);
	bool IsBuiltFor(const FPjcScanResult& Result) const {
		return Result.IsValid() && Time == Result.Time;
	}
	int32 Find(const FName Package) const;
	int32 Num() const {
		return Packages.Num();
	}
	static bool Counts(const uint8 Kinds, const FPjcModelLink& Link) {
		return (PjcLinkKinds::Of(Link.Kind) & Kinds) != 0;
	}

	// Steps from the nearest root over the kinds in scope, INDEX_NONE when no root reaches it
	int32 GetDepth(const int32 Node, const uint8 Kinds) const;
	// What goes when it goes: the asset and every asset only it keeps used, the asset first. Empty when no root reaches it
	TArray<int32> GetRetained(const int32 Node, const uint8 Kinds) const;
	// The assets every route to it passes, closest first. Empty when several roots keep it with nothing they all pass
	TArray<int32> GetChoke(const int32 Node, const uint8 Kinds) const;
	// The asset that keeps it used on every route (its first choke), INDEX_NONE for a root, for several roots with no
	// single keeper (-2), or for an asset no root reaches
	int32 GetKeeper(const int32 Node, const uint8 Kinds) const;
	static constexpr int32 SeveralRoots = -2;

	// Every root that keeps it with its shortest route, best first: preferred roots, routes the game needs, the kind of
	// root (primary asset, cue, cinematic, config, engine, editor, review), hard before soft before editor only, project
	// before plugin content, then fewer steps. Prefer: root node -> 1 preferred, -1 avoided
	TArray<FPjcRoute> GetRoutes(const int32 Node, const uint8 Kinds, const TMap<int32, int32>& Prefer = {}) const;
	// The top route and up to two that differ most from it: another class of root, another asset using it
	TArray<int32> GetKeyRoutes(const TArray<FPjcRoute>& Routes) const;

	struct FReach
	{
		int32 Num = 0;
		int64 Size = 0;
	};
	// Every asset that uses it over all levels (bUsedBy), or that it uses
	FReach GetReach(const int32 Node, const bool bUsedBy, const uint8 Kinds) const;

	struct FWhatIf
	{
		// Still used: the route that keeps it then
		bool bStill = false;
		TArray<int32> Path;
		bool bEditor = false;
		// Not used then: how many assets become unused with it and their size
		int32 Num = 0;
		int64 Size = 0;
	};
	// If User did not use Focus: what keeps it used, or what becomes unused
	FWhatIf GetWhatIf(const int32 Focus, const int32 User, const uint8 Kinds) const;

	struct FSearch
	{
		// Assets on the branches from the asset to a match, the matches, how many assets were looked at
		TSet<int32> Keep;
		TSet<int32> Hits;
		int32 Searched = 0;
	};
	static constexpr int32 MaxHits = 100;
	// Search over all levels of what uses it (bUsedBy) or what it uses
	FSearch
	Search(const int32 Node, const bool bUsedBy, const uint8 Kinds, TFunctionRef<bool(const int32 From, const FPjcModelLink& Link)> Match) const;

	// What uses the package (bUsedBy) or what it uses. Actors and objects a World Partition map saves in their own
	// files are part of their map, not assets: they are left out and the list goes on through them, so a map lists
	// the meshes its actors use and a mesh lists the maps. OutThrough: how many of those files the list went through
	static void
	GetLinks(const FName Package, const bool bUsedBy, TArray<FPjcInspectorLink>& Out, int32& OutThrough, FPjcInspectorCache* Cache = nullptr);
	// Package of an actor or object saved in its own file ('__ExternalActors__', '__ExternalObjects__')
	static bool IsExternal(const FName Package);

  private:

	struct FDominators
	{
		uint8 Kinds = 0;
		// Immediate dominator of each node; Virtual for a root's, INDEX_NONE when no root reaches it
		TArray<int32> Idom;
		TArray<TArray<int32>> Kids;
		TArray<int32> Depth;
	};
	static constexpr int32 Virtual = -3;
	const FDominators& GetDominators(const uint8 Kinds) const;
	mutable TArray<FDominators> DominatorCache;

	TMap<FName, int32> IndexOf;

	static void ReadLinks(const FName Package, const bool bUsedBy, TArray<FPjcInspectorLink>& Out, int32& OutThrough, FPjcInspectorCache* Cache);
};
