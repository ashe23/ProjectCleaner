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
	 * nothing uses), goes through owner. INDEX_NONE for assets nothing owns
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
	/** Folder holding it */
	int32 Folder = INDEX_NONE;
	/** Graph units, circle area is size on disk */
	FVector2D Position = FVector2D::ZeroVector;
	float Radius = 0.0f;
};

/** Folder in graph, as in content browser. Folder holding nothing but one folder is merged with it, e.g. "Game/MyProject" */
struct FPjcGraphFolder
{
	/** Folder path, e.g. /Game/Characters */
	FString Path;
	/** Shown name, merged folders joined with slash */
	FString Name;
	int32 Parent = INDEX_NONE;
	/** Subfolders and assets directly in it, biggest first */
	TArray<int32> Folders;
	TArray<int32> Assets;
	/** Everything inside, subfolders too */
	int32 NumAssets = 0;
	int32 NumUnused = 0;
	int32 NumNotDeleted = 0;
	int64 Size = 0;
	int64 SizeUnused = 0;
	/** Circle holding subfolders and assets */
	FVector2D Center = FVector2D::ZeroVector;
	float Radius = 0.0f;
};

/**
 * Scanned assets as map: folders are circles holding their subfolders and assets, asset circle area is its size.
 * Links are dependencies (same as scan follows). Ownership tells what every asset holds alone: everything only it uses,
 * used and unused assets apart.
 */
class FPjcAssetGraph final
{
  public:

	/** Items must be scanned assets with usage */
	void Build(const TArray<TSharedPtr<FPjcUsageItem>>& Items);

	int32 Find(const FName& PackageName) const;
	const TArray<FPjcGraphNode>& GetNodes() const;
	/** Parents come before their subfolders */
	const TArray<FPjcGraphFolder>& GetFolders() const;
	/** Mount points, e.g. /Game and project plugins */
	const TArray<int32>& GetRootFolders() const;
	/** Area holding everything */
	FBox2D GetBounds() const;
	int32 GetNumLinks() const;

	/** Asset used for own reason, start of usage chain */
	static bool IsUsageRoot(const FPjcGraphNode& Node);

  private:

	void BuildLinks();
	void BuildOwners();
	void BuildFolders();
	void BuildLayout();

	TArray<FPjcGraphNode> Nodes;
	TMap<FName, int32> NodeIndices;
	TArray<FPjcGraphFolder> Folders;
	TArray<int32> RootFolders;
	FBox2D Bounds {ForceInit};
	int32 NumLinks = 0;
};
