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
};

struct FPjcInspectorNode
{
	EPjcInspectorNodeKind Kind = EPjcInspectorNodeKind::Asset;
	// Asset: its package. Source: the root (or group class) it is above
	FName Package;
	// Group: the class and the roots it stands for
	FName GroupClass;
	TArray<FName> Roots;
	// Source: why the root is used
	FString Label;
	// Row, top 0, and order in the row
	int32 Layer = 0;
	int32 Column = 0;
};

struct FPjcInspectorEdge
{
	// Indices into the nodes; From references To
	int32 From = INDEX_NONE;
	int32 To = INDEX_NONE;
	EPjcLinkKind Kind = EPjcLinkKind::Hard;
	// Group edges stand for the references of every root in the group
	int32 NumLinks = 1;
};

// One row of an asset's "Used by" or "Uses" list
struct FPjcInspectorLink
{
	FName Package;
	EPjcLinkKind Kind = EPjcLinkKind::Hard;
	// Not a reference of its own: it goes through an actor or object the map saves in its own file
	bool bThrough = false;
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
 * unused assets that reference each other around it (cleanup deletes them together).
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

	static constexpr int32 MaxPaths = 6;
	static constexpr int32 GroupAt = 4;
	static constexpr int32 MaxUnused = 24;

	// OpenGroups: classes whose roots are drawn one by one instead of as a group
	static FPjcInspectorGraph Make(const FPjcScanResult& Result, const FName Focus, const TSet<FName>& OpenGroups = {});

	// Kind of the reference From has to To, Hard when there is none
	static EPjcLinkKind GetLinkKind(const FName From, const FName To);
	// What uses the package (bUsedBy) or what it uses. Actors and objects a World Partition map saves in their own
	// files are part of their map, not assets: they are left out and the list goes on through them, so a map lists
	// the meshes its actors use and a mesh lists the maps. OutThrough: how many of those files the list went through
	static void GetLinks(const FName Package, const bool bUsedBy, TArray<FPjcInspectorLink>& Out, int32& OutThrough);
	// Package of an actor or object saved in its own file ('__ExternalActors__', '__ExternalObjects__')
	static bool IsExternal(const FName Package);

	int32 FindNode(const FName Package) const;
};
