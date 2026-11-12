// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
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

enum class EPjcInspectorNodeKind : uint8
{
	Asset,
	// roots of one class, 4 or more, as one node
	Group,
	// why a root is used: the box above it
	Source,
	// unused assets of one class with the same links, as one box
	Stack,
};

struct FPjcInspectorNode
{
	EPjcInspectorNodeKind Kind = EPjcInspectorNodeKind::Asset;
	// Asset: its package. Source: the root (or group class) it is above
	FName Package;
	// Group: the class and the roots it stands for. Stack: the class and its assets
	FName GroupClass;
	TArray<FName> Roots;
	// Unused group: an asset many of the group use. It has no lines; its users carry a dot (Hubs: indices of such nodes)
	bool bHub = false;
	int32 NumUsers = 0;
	TArray<int32> Hubs;
	// Source: why the root is used
	FString Label;
	// Row, top 0, and place in the row (a row of the unused group is centered, so half steps)
	int32 Layer = 0;
	float Column = 0.0f;
};

struct FPjcInspectorEdge
{
	// Indices into the nodes; From references To
	int32 From = INDEX_NONE;
	int32 To = INDEX_NONE;
	EPjcLinkKind Kind = EPjcLinkKind::Hard;
	// Group edges stand for the references of every root in the group
	int32 NumLinks = 1;
	// The reference closes a circle: what it points to leads back to it
	bool bCircle = false;
};

// One reference inside a group of assets: indices into the group's names
struct FPjcGroupLink
{
	int32 From = 0;
	int32 To = 0;
	EPjcLinkKind Kind = EPjcLinkKind::Hard;
};

// Line of text above a row of the unused group
struct FPjcInspectorCaption
{
	int32 Layer = 0;
	FString Text;
};

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

struct FPjcInspectorRoot
{
	FName Package;
	// References from the root down to the asset, 0 when the asset is the root
	int32 Steps = 0;
};

/**
 * What keeps an asset used, as a graph for the Inspector: every root at once, top to bottom, each with the shortest path
 * from it to the asset (paths merge into the asset at the bottom) and a source box above it. For an unused asset: the
 * unused assets that reference each other around it (cleanup deletes them together), as one picture: assets with the
 * same links stand as one stack, an asset many of them use has no lines but a dot on its users, editor-only links are
 * left out unless asked for.
 */
struct FPjcInspectorGraph
{
	FName Focus;
	TArray<FPjcInspectorNode> Nodes;
	TArray<FPjcInspectorEdge> Edges;
	// Every root that keeps the asset used, nearest first
	TArray<FPjcInspectorRoot> Roots;
	int32 NumRoots = 0;
	// Root paths not drawn, past MaxPaths
	int32 NumHidden = 0;
	// The graph is the unused group around an unused asset
	bool bUnusedGroup = false;
	// Unused group: how many assets it has (they stand as fewer boxes), and the text above some of its rows
	int32 NumGroup = 0;
	TArray<FPjcInspectorCaption> Captions;

	static constexpr int32 MaxPaths = 6;
	static constexpr int32 GroupAt = 4;
	// Unused group: assets followed at most, and from how many users on an asset stands apart as shared
	static constexpr int32 MaxGroup = 400;
	static constexpr int32 HubAt = 8;

	// OpenGroups: classes whose roots are drawn one by one instead of as a group.
	// bEditorLinks: draw the editor-only links of an unused group too (on top, they move nothing)
	static FPjcInspectorGraph Make(
		const FPjcScanResult& Result,
		const FName Focus,
		const TSet<FName>& OpenGroups = {},
		const bool bEditorLinks = false,
		FPjcInspectorCache* Cache = nullptr
	);

	// The unused group as one picture, from its assets alone: Group[0] is the asset in focus, Classes their classes,
	// Links every reference inside the group. It asks no asset registry, so a test can build a group by hand
	static FPjcInspectorGraph
	MakeGroup(const TArray<FName>& Group, const TArray<FName>& Classes, const TArray<FPjcGroupLink>& Links, const bool bEditorLinks);
	// Used graph: assets of one class that all lead to the same next asset become one name in the paths, from GroupAt
	// on (a thousand sequences of one map are one box, not a thousand). Paths run from the root to the asset in focus;
	// step 0 (the root) stays as it is, and so does every asset on a path with bStack false. OutStacks: the new name
	// and the assets it stands for
	static void
	StackPaths(TArray<TArray<FName>>& Paths, const TArray<bool>& bStack, TFunctionRef<FName(FName)> ClassOf, TMap<FName, TArray<FName>>& OutStacks);
	// Kind of the reference From has to To, Hard when there is none
	static EPjcLinkKind GetLinkKind(const FName From, const FName To, FPjcInspectorCache* Cache = nullptr);
	// What uses the package (bUsedBy) or what it uses. Actors and objects a World Partition map saves in their own
	// files are part of their map, not assets: they are left out and the list goes on through them, so a map lists
	// the meshes its actors use and a mesh lists the maps. OutThrough: how many of those files the list went through
	static void
	GetLinks(const FName Package, const bool bUsedBy, TArray<FPjcInspectorLink>& Out, int32& OutThrough, FPjcInspectorCache* Cache = nullptr);
	// Package of an actor or object saved in its own file ('__ExternalActors__', '__ExternalObjects__')
	static bool IsExternal(const FName Package);

  private:

	static void ReadLinks(const FName Package, const bool bUsedBy, TArray<FPjcInspectorLink>& Out, int32& OutThrough, FPjcInspectorCache* Cache);

  public:

	int32 FindNode(const FName Package) const;
};
