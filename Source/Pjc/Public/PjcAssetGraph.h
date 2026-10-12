// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FPjcUsageItem;

/** Scanned asset in usage graph */
struct FPjcGraphNode
{
	TSharedPtr<FPjcUsageItem> Item;
	/**
	 * Asset that owns this one: every way to it, from assets used for their own reason (unused ones: from unused assets
	 * nothing uses), goes through owner. INDEX_NONE for top level assets
	 */
	int32 Owner = INDEX_NONE;
	/** Assets owned directly, biggest first */
	TArray<int32> Owned;
	TArray<int32> Uses;
	TArray<int32> UsedBy;
	/** Own size plus size of everything owned, what deleting it with everything only it uses frees */
	int64 SizeOwned = 0;
	/** Number of assets owned, directly or not */
	int32 NumOwned = 0;
	int32 Depth = 0;
	/** Graph units. Offset from owner, Position is absolute */
	FVector2D Offset = FVector2D::ZeroVector;
	FVector2D Position = FVector2D::ZeroVector;
	/** Circle of asset itself, area is size on disk */
	float Radius = 0.0f;
	/** Circle around asset and everything it owns, its center is ClusterOffset away from Position */
	float ClusterRadius = 0.0f;
	FVector2D ClusterOffset = FVector2D::ZeroVector;

	FVector2D GetClusterCenter() const {
		return Position + ClusterOffset;
	}
};

/** Folder area of graph, holds top level assets of one folder. Used and unused assets have separate regions */
struct FPjcGraphRegion
{
	/** Folder path, e.g. /Game/Characters */
	FString Path;
	bool bUsed = true;
	/** Top level assets in it, biggest first */
	TArray<int32> Nodes;
	/** Assets in it, owned ones too */
	int32 NumAssets = 0;
	int64 Size = 0;
	FVector2D Center = FVector2D::ZeroVector;
	float Radius = 0.0f;
};

/**
 * Scanned assets as graph: links are dependencies (same as scan follows), assets are grouped by ownership, every owner
 * holds what only it uses. Used and unused assets are grouped separately, so groups are deleted or kept as whole.
 * Layout is packed: used and unused areas side by side, in them folder regions, in regions top level assets (linked
 * ones next to each other), around every owner what it owns.
 */
class FPjcAssetGraph final
{
  public:

	/** Items must be scanned assets with usage */
	void Build(const TArray<TSharedPtr<FPjcUsageItem>>& Items);

	int32 Find(const FName& PackageName) const;
	const TArray<FPjcGraphNode>& GetNodes() const;
	const TArray<FPjcGraphRegion>& GetRegions() const;
	/** Region of asset, the one of its top level owner */
	int32 GetRegion(const int32 Node) const;
	const TArray<int32>& GetTopLevel() const;
	/** Owners come before assets they own */
	const TArray<int32>& GetOrder() const;
	/** Area holding everything */
	FBox2D GetBounds() const;
	/** Area of used or unused assets, invalid if there are none */
	FBox2D GetGroupBounds(const bool bUsed) const;
	int32 GetNumLinks() const;

	/** Asset used for own reason, start of usage chain */
	static bool IsUsageRoot(const FPjcGraphNode& Node);

  private:

	void BuildLinks();
	void BuildOwners();
	void BuildClusters();
	void BuildRegions();
	void BuildLayout();
	void UpdatePositions();

	TArray<FPjcGraphNode> Nodes;
	TMap<FName, int32> NodeIndices;
	TArray<int32> TopLevel;
	TArray<int32> Order;
	TArray<FPjcGraphRegion> Regions;
	TArray<int32> NodeRegions;
	FBox2D GroupBounds[2];
	int32 NumLinks = 0;
};
