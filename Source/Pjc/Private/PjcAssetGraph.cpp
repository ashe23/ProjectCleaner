// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcAssetGraph.h"
#include "PjcShim.h"
#include "PjcSubsystem.h"
#include "PjcTypes.h"
// Engine Headers
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"

namespace
{
	// graph units, circle area is size on disk: 1 MB asset has radius 20
	constexpr double RadiusPerSqrtByte = 0.02;
	constexpr float RadiusMin = 4.0f;

	// folder with more top level assets than this share of its area is split by subfolders
	constexpr int32 RegionSplitMin = 150;
	constexpr float RegionSplitShare = 0.4f;
	constexpr int32 RegionDepthMax = 6;

	// vector component type, float before UE5
	using FReal = decltype(FVector2D::X);

	/** Gap kept around circle inside its owner */
	float GetPadding(const float Radius) {
		return 2.0f + Radius * 0.05f;
	}

	/** Gap kept between top level groups */
	float GetGap(const float Radius) {
		return 4.0f + Radius * 0.04f;
	}

	/** Gap kept between folder regions */
	float GetRegionGap(const float Radius) {
		return 24.0f + Radius * 0.05f;
	}

	double SquareRoot(const double Value) {
#if ENGINE_MAJOR_VERSION >= 5
		return FMath::Sqrt(Value);
#else
		return sqrt(Value);
#endif
	}

	FVector2D ToVector(const double X, const double Y) {
		return FVector2D(static_cast<FReal>(X), static_cast<FReal>(Y));
	}

	struct FPackCircle
	{
		double X = 0.0;
		double Y = 0.0;
		double R = 0.0;
	};

	/** Puts C next to A and B, touching both */
	void PlaceCircle(const FPackCircle& B, const FPackCircle& A, FPackCircle& C) {
		const double Dx = B.X - A.X;
		const double Dy = B.Y - A.Y;
		const double D2 = Dx * Dx + Dy * Dy;
		if (D2 <= 0.0) {
			C.X = A.X + C.R;
			C.Y = A.Y;
			return;
		}

		const double A2 = FMath::Square(A.R + C.R);
		const double B2 = FMath::Square(B.R + C.R);
		if (A2 > B2) {
			const double X = (D2 + B2 - A2) / (2.0 * D2);
			const double Y = SquareRoot(FMath::Max(0.0, B2 / D2 - X * X));
			C.X = B.X - X * Dx - Y * Dy;
			C.Y = B.Y - X * Dy + Y * Dx;
		}
		else {
			const double X = (D2 + A2 - B2) / (2.0 * D2);
			const double Y = SquareRoot(FMath::Max(0.0, A2 / D2 - X * X));
			C.X = A.X + X * Dx - Y * Dy;
			C.Y = A.Y + X * Dy + Y * Dx;
		}
	}

	bool CirclesIntersect(const FPackCircle& A, const FPackCircle& B) {
		const double Dr = A.R + B.R - 1e-6;
		return Dr > 0.0 && Dr * Dr > FMath::Square(B.X - A.X) + FMath::Square(B.Y - A.Y);
	}

	/**
	 * Front chain circle packing (Wang et al. 2006, same as d3 packSiblings). Circles are placed in given order, each next to
	 * previous ones, so neighbors in order end up close. First circle stays at origin
	 */
	void PackCircles(TArray<FPackCircle>& Circles) {
		const int32 Num = Circles.Num();
		if (Num == 0) return;

		Circles[0].X = 0.0;
		Circles[0].Y = 0.0;
		if (Num == 1) return;

		Circles[1].X = Circles[0].R + Circles[1].R;
		Circles[1].Y = 0.0;
		if (Num == 2) return;

		PlaceCircle(Circles[1], Circles[0], Circles[2]);

		TArray<int32> Next;
		TArray<int32> Prev;
		Next.Init(INDEX_NONE, Num);
		Prev.Init(INDEX_NONE, Num);

		int32 A = 0;
		int32 B = 1;
		Next[0] = 1;
		Prev[2] = 1;
		Next[1] = 2;
		Prev[0] = 2;
		Next[2] = 0;
		Prev[1] = 0;

		const auto Score = [&](const int32 Index) {
			const FPackCircle& CircleA = Circles[Index];
			const FPackCircle& CircleB = Circles[Next[Index]];
			const double Sum = CircleA.R + CircleB.R;
			const double Dx = (CircleA.X * CircleB.R + CircleB.X * CircleA.R) / Sum;
			const double Dy = (CircleA.Y * CircleB.R + CircleB.Y * CircleA.R) / Sum;
			return Dx * Dx + Dy * Dy;
		};

		// each retry shrinks front chain, guard is only against broken input
		int64 Guard = static_cast<int64>(Num) * Num * 4 + 64;
		for (int32 Index = 3; Index < Num; ++Index) {
			PlaceCircle(Circles[A], Circles[B], Circles[Index]);

			// closest intersecting circle on front chain, distance measured along the chain
			int32 J = Next[B];
			int32 K = Prev[A];
			double Sj = Circles[B].R;
			double Sk = Circles[A].R;
			bool bRetry = false;
			do {
				if (Sj <= Sk) {
					if (CirclesIntersect(Circles[J], Circles[Index])) {
						B = J;
						Next[A] = B;
						Prev[B] = A;
						bRetry = true;
						break;
					}
					Sj += Circles[J].R;
					J = Next[J];
				}
				else {
					if (CirclesIntersect(Circles[K], Circles[Index])) {
						A = K;
						Next[A] = B;
						Prev[B] = A;
						bRetry = true;
						break;
					}
					Sk += Circles[K].R;
					K = Prev[K];
				}
			} while (J != Next[K]);

			if (bRetry && --Guard > 0) {
				--Index;
				continue;
			}

			// insert between A and B, then continue from pair closest to origin
			Prev[Index] = A;
			Next[Index] = B;
			Next[A] = Index;
			Prev[B] = Index;
			B = Index;

			double BestScore = Score(A);
			int32 Step = Index;
			while ((Step = Next[Step]) != B) {
				const double StepScore = Score(Step);
				if (StepScore < BestScore) {
					A = Step;
					BestScore = StepScore;
				}
			}
			B = Next[A];
		}
	}

	/** Breadth first over given neighbors, starts from items in given order, so linked items follow each other */
	TArray<int32> GetLinkedOrder(const TArray<int32>& Items, const TMap<int32, TArray<int32>>& Neighbors) {
		TArray<int32> Result;
		TSet<int32> Added;
		Result.Reserve(Items.Num());
		Added.Reserve(Items.Num());
		for (const int32 Start : Items) {
			if (Added.Contains(Start)) continue;

			Added.Add(Start);
			for (int32 Head = Result.Add(Start); Head < Result.Num(); ++Head) {
				const TArray<int32>* ItemNeighbors = Neighbors.Find(Result[Head]);
				for (int32 Step = 0; ItemNeighbors && Step < ItemNeighbors->Num(); ++Step) {
					const int32 Neighbor = (*ItemNeighbors)[Step];
					if (Added.Contains(Neighbor)) continue;

					Added.Add(Neighbor);
					Result.Add(Neighbor);
				}
			}
		}
		return Result;
	}

	/** Packs circles, then moves them so circle around them is centered at origin. Returns its radius */
	double PackCentered(TArray<FPackCircle>& Circles) {
		PackCircles(Circles);
		if (Circles.Num() == 0) return 0.0;

		double MinX = Circles[0].X - Circles[0].R;
		double MinY = Circles[0].Y - Circles[0].R;
		double MaxX = Circles[0].X + Circles[0].R;
		double MaxY = Circles[0].Y + Circles[0].R;
		for (const auto& Circle : Circles) {
			MinX = FMath::Min(MinX, Circle.X - Circle.R);
			MinY = FMath::Min(MinY, Circle.Y - Circle.R);
			MaxX = FMath::Max(MaxX, Circle.X + Circle.R);
			MaxY = FMath::Max(MaxY, Circle.Y + Circle.R);
		}

		const double CenterX = (MinX + MaxX) * 0.5;
		const double CenterY = (MinY + MaxY) * 0.5;
		double Radius = 0.0;
		for (auto& Circle : Circles) {
			Circle.X -= CenterX;
			Circle.Y -= CenterY;
			Radius = FMath::Max(Radius, SquareRoot(Circle.X * Circle.X + Circle.Y * Circle.Y) + Circle.R);
		}
		return Radius;
	}
}	// namespace

void FPjcAssetGraph::Build(const TArray<TSharedPtr<FPjcUsageItem>>& Items) {
	Nodes.Reset();
	NodeIndices.Reset();
	TopLevel.Reset();
	Order.Reset();
	Regions.Reset();
	NodeRegions.Reset();
	GroupBounds[0] = FBox2D {ForceInit};
	GroupBounds[1] = FBox2D {ForceInit};
	NumLinks = 0;

	// sorted by name, so same project gives same picture
	TArray<TSharedPtr<FPjcUsageItem>> Sorted;
	Sorted.Reserve(Items.Num());
	for (const auto& Item : Items) {
		if (Item.IsValid() && Item->bScanned) {
			Sorted.Add(Item);
		}
	}
	Sorted.Sort([](const TSharedPtr<FPjcUsageItem>& A, const TSharedPtr<FPjcUsageItem>& B) {
		return A->PackageName.Compare(B->PackageName) < 0;
	});

	Nodes.SetNum(Sorted.Num());
	NodeIndices.Reserve(Sorted.Num());
	for (int32 Index = 0; Index < Sorted.Num(); ++Index) {
		FPjcGraphNode& Node = Nodes[Index];
		Node.Item = Sorted[Index];
		Node.Radius
			= FMath::Max(RadiusMin, static_cast<float>(SquareRoot(static_cast<double>(FMath::Max<int64>(Node.Item->Size, 0))) * RadiusPerSqrtByte));
		NodeIndices.Add(Node.Item->PackageName, Index);
	}

	BuildLinks();
	BuildOwners();
	BuildClusters();
	BuildRegions();
	BuildLayout();
	UpdatePositions();
}

int32 FPjcAssetGraph::Find(const FName& PackageName) const {
	const int32* Index = NodeIndices.Find(PackageName);
	return Index ? *Index : INDEX_NONE;
}

const TArray<FPjcGraphNode>& FPjcAssetGraph::GetNodes() const {
	return Nodes;
}

const TArray<FPjcGraphRegion>& FPjcAssetGraph::GetRegions() const {
	return Regions;
}

int32 FPjcAssetGraph::GetRegion(const int32 Node) const {
	return NodeRegions.IsValidIndex(Node) ? NodeRegions[Node] : INDEX_NONE;
}

const TArray<int32>& FPjcAssetGraph::GetTopLevel() const {
	return TopLevel;
}

const TArray<int32>& FPjcAssetGraph::GetOrder() const {
	return Order;
}

FBox2D FPjcAssetGraph::GetBounds() const {
	FBox2D Bounds {ForceInit};
	for (const auto& Group : GroupBounds) {
		if (Group.bIsValid) {
			Bounds += Group;
		}
	}
	return Bounds;
}

FBox2D FPjcAssetGraph::GetGroupBounds(const bool bUsed) const {
	return GroupBounds[bUsed ? 0 : 1];
}

int32 FPjcAssetGraph::GetNumLinks() const {
	return NumLinks;
}

bool FPjcAssetGraph::IsUsageRoot(const FPjcGraphNode& Node) {
	return Node.Item.IsValid() && Node.Item->Usage.bUsed && Node.Item->Usage.Parent.IsNone();
}

void FPjcAssetGraph::BuildLinks() {
	IAssetRegistry& AssetRegistry = UPjcSubsystem::GetModuleAssetRegistry().Get();

	const TArray<FString> RootPaths = UPjcSubsystem::GetProjectRootPaths();
	const auto IsInsideProject = [&RootPaths](const FName& PackageName) {
		const FString Path = PackageName.ToString();
		for (const auto& RootPath : RootPaths) {
			if (Path.StartsWith(RootPath) && (Path.Len() == RootPath.Len() || Path[RootPath.Len()] == TEXT('/'))) return true;
		}
		return false;
	};

	// packages inside project that are not scanned assets (actors saved in their own files) pass links on, like scan does
	TMap<FName, TArray<int32>> Through;
	const auto GetThrough = [&](const FName& Start) {
		if (const TArray<int32>* Found = Through.Find(Start)) return *Found;

		TArray<int32> Result;
		TSet<FName> Visited {Start};
		TArray<FName> Queue {Start};
		TArray<FName> Deps;
		for (int32 Head = 0; Head < Queue.Num(); ++Head) {
			Deps.Reset();
			AssetRegistry.GetDependencies(Queue[Head], Deps);
			for (const auto& Dep : Deps) {
				if (const int32* DepIndex = NodeIndices.Find(Dep)) {
					Result.AddUnique(*DepIndex);
					continue;
				}
				if (Visited.Contains(Dep) || !IsInsideProject(Dep)) continue;

				Visited.Add(Dep);
				Queue.Add(Dep);
			}
		}

		Through.Add(Start, Result);
		return Result;
	};

	TArray<FName> Deps;
	for (int32 Index = 0; Index < Nodes.Num(); ++Index) {
		TArray<int32>& Uses = Nodes[Index].Uses;

		Deps.Reset();
		AssetRegistry.GetDependencies(Nodes[Index].Item->PackageName, Deps);
		for (const auto& Dep : Deps) {
			if (const int32* DepIndex = NodeIndices.Find(Dep)) {
				if (*DepIndex != Index) {
					Uses.AddUnique(*DepIndex);
				}
				continue;
			}
			if (!IsInsideProject(Dep)) continue;

			for (const int32 Linked : GetThrough(Dep)) {
				if (Linked != Index) {
					Uses.AddUnique(Linked);
				}
			}
		}
		Uses.Sort();
	}

	for (int32 Index = 0; Index < Nodes.Num(); ++Index) {
		for (const int32 Use : Nodes[Index].Uses) {
			Nodes[Use].UsedBy.Add(Index);
		}
		NumLinks += Nodes[Index].Uses.Num();
	}
}

void FPjcAssetGraph::BuildOwners() {
	// owner is immediate dominator (Cooper, Harvey, Kennedy: A Simple, Fast Dominance Algorithm). Virtual root links to assets
	// used for own reason and to unused assets no unused asset uses. Used and unused assets are linked only among themselves
	const int32 Num = Nodes.Num();
	const int32 Root = Num;

	TArray<TArray<int32>> Succ;
	TArray<TArray<int32>> Pred;
	Succ.SetNum(Num + 1);
	Pred.SetNum(Num + 1);
	const auto AddEdge = [&](const int32 From, const int32 To) {
		if (From == To) return;

		Succ[From].Add(To);
		Pred[To].Add(From);
	};

	for (int32 Index = 0; Index < Num; ++Index) {
		const FPjcAssetUsage& Usage = Nodes[Index].Item->Usage;
		for (const int32 Use : Nodes[Index].Uses) {
			if (Nodes[Use].Item->Usage.bUsed == Usage.bUsed) {
				AddEdge(Index, Use);
			}
		}
		if (!Usage.bUsed) continue;

		// usage chain may go through packages that are not scanned, parent link keeps it
		const int32* Parent = Usage.Parent.IsNone() ? nullptr : NodeIndices.Find(Usage.Parent);
		if (Parent && Nodes[*Parent].Item->Usage.bUsed) {
			AddEdge(*Parent, Index);
		}
		else {
			AddEdge(Root, Index);
		}
	}
	for (int32 Index = 0; Index < Num; ++Index) {
		if (!Nodes[Index].Item->Usage.bUsed && Pred[Index].Num() == 0) {
			AddEdge(Root, Index);
		}
	}

	TArray<bool> Visited;
	TArray<int32> PostOrder;
	TArray<TPair<int32, int32>> Stack;
	const auto Visit = [&](const int32 Start) {
		Visited[Start] = true;
		Stack.Add(TPair<int32, int32> {Start, 0});
		while (Stack.Num() > 0) {
			const int32 Current = Stack.Last().Key;
			const int32 SuccIndex = Stack.Last().Value++;
			if (SuccIndex < Succ[Current].Num()) {
				const int32 Next = Succ[Current][SuccIndex];
				if (!Visited[Next]) {
					Visited[Next] = true;
					Stack.Add(TPair<int32, int32> {Next, 0});
				}
				continue;
			}

			PostOrder.Add(Current);
			PjcShim::PopArray(Stack);
		}
	};

	// unused cycles nothing else uses: first asset of each (by name) gets owned by root
	Visited.Init(false, Num + 1);
	Visit(Root);
	for (int32 Index = 0; Index < Num; ++Index) {
		if (Visited[Index]) continue;

		AddEdge(Root, Index);
		Visit(Index);
	}

	Visited.Init(false, Num + 1);
	PostOrder.Reset(Num + 1);
	Visit(Root);

	TArray<int32> ReversePostOrder;
	TArray<int32> OrderIndex;
	ReversePostOrder.Reserve(PostOrder.Num());
	OrderIndex.Init(INDEX_NONE, Num + 1);
	for (int32 Index = PostOrder.Num() - 1; Index >= 0; --Index) {
		OrderIndex[PostOrder[Index]] = ReversePostOrder.Num();
		ReversePostOrder.Add(PostOrder[Index]);
	}

	TArray<int32> Dominator;
	Dominator.Init(INDEX_NONE, Num + 1);
	Dominator[Root] = Root;
	const auto Intersect = [&](int32 A, int32 B) {
		while (A != B) {
			while (OrderIndex[A] > OrderIndex[B]) {
				A = Dominator[A];
			}
			while (OrderIndex[B] > OrderIndex[A]) {
				B = Dominator[B];
			}
		}
		return A;
	};

	bool bChanged = true;
	while (bChanged) {
		bChanged = false;
		for (int32 Step = 1; Step < ReversePostOrder.Num(); ++Step) {
			const int32 Index = ReversePostOrder[Step];
			int32 NewDominator = INDEX_NONE;
			for (const int32 P : Pred[Index]) {
				if (Dominator[P] == INDEX_NONE) continue;

				NewDominator = NewDominator == INDEX_NONE ? P : Intersect(P, NewDominator);
			}
			if (NewDominator != INDEX_NONE && Dominator[Index] != NewDominator) {
				Dominator[Index] = NewDominator;
				bChanged = true;
			}
		}
	}

	Order.Reserve(Num);
	for (int32 Step = 1; Step < ReversePostOrder.Num(); ++Step) {
		const int32 Index = ReversePostOrder[Step];
		FPjcGraphNode& Node = Nodes[Index];
		Node.Owner = Dominator[Index] == Root ? INDEX_NONE : Dominator[Index];
		Node.Depth = Node.Owner == INDEX_NONE ? 0 : Nodes[Node.Owner].Depth + 1;
		if (Node.Owner == INDEX_NONE) {
			TopLevel.Add(Index);
		}
		else {
			Nodes[Node.Owner].Owned.Add(Index);
		}
		Order.Add(Index);
	}

	for (int32 Step = Order.Num() - 1; Step >= 0; --Step) {
		FPjcGraphNode& Node = Nodes[Order[Step]];
		Node.SizeOwned += Node.Item->Size;
		if (Node.Owner != INDEX_NONE) {
			Nodes[Node.Owner].SizeOwned += Node.SizeOwned;
			Nodes[Node.Owner].NumOwned += Node.NumOwned + 1;
		}
	}

	const auto BiggerFirst = [this](const int32 A, const int32 B) {
		return Nodes[A].SizeOwned != Nodes[B].SizeOwned ? Nodes[A].SizeOwned > Nodes[B].SizeOwned : A < B;
	};
	for (auto& Node : Nodes) {
		Node.Owned.Sort(BiggerFirst);
	}
	TopLevel.Sort(BiggerFirst);
}

void FPjcAssetGraph::BuildClusters() {
	// owned assets are packed around their owner, smallest groups first so owners know their size
	TArray<FPackCircle> Circles;
	for (int32 Step = Order.Num() - 1; Step >= 0; --Step) {
		FPjcGraphNode& Node = Nodes[Order[Step]];
		if (Node.Owned.Num() == 0) {
			Node.ClusterRadius = Node.Radius;
			continue;
		}

		Circles.Reset(Node.Owned.Num() + 1);
		Circles.Add(FPackCircle {0.0, 0.0, Node.Radius + GetPadding(Node.Radius)});
		for (const int32 OwnedIndex : Node.Owned) {
			const float Radius = Nodes[OwnedIndex].ClusterRadius;
			Circles.Add(FPackCircle {0.0, 0.0, Radius + GetPadding(Radius)});
		}
		PackCircles(Circles);

		// group circle centered on bounds of packed circles, owner is not always in the middle but circle stays tight
		double MinX = -Node.Radius;
		double MinY = -Node.Radius;
		double MaxX = Node.Radius;
		double MaxY = Node.Radius;
		for (int32 OwnedStep = 0; OwnedStep < Node.Owned.Num(); ++OwnedStep) {
			const FPackCircle& Circle = Circles[OwnedStep + 1];
			const double Radius = Nodes[Node.Owned[OwnedStep]].ClusterRadius;
			MinX = FMath::Min(MinX, Circle.X - Radius);
			MinY = FMath::Min(MinY, Circle.Y - Radius);
			MaxX = FMath::Max(MaxX, Circle.X + Radius);
			MaxY = FMath::Max(MaxY, Circle.Y + Radius);
		}
		const double CenterX = (MinX + MaxX) * 0.5;
		const double CenterY = (MinY + MaxY) * 0.5;

		double ClusterRadius = SquareRoot(CenterX * CenterX + CenterY * CenterY) + Node.Radius;
		for (int32 OwnedStep = 0; OwnedStep < Node.Owned.Num(); ++OwnedStep) {
			FPjcGraphNode& OwnedNode = Nodes[Node.Owned[OwnedStep]];
			const FPackCircle& Circle = Circles[OwnedStep + 1];
			const double Dist = SquareRoot(FMath::Square(Circle.X - CenterX) + FMath::Square(Circle.Y - CenterY));
			ClusterRadius = FMath::Max(ClusterRadius, Dist + OwnedNode.ClusterRadius);

			// packed circle is owned group, asset itself is off its center
			OwnedNode.Offset = ToVector(Circle.X, Circle.Y) - OwnedNode.ClusterOffset;
		}

		// room for group outline
		Node.ClusterOffset = ToVector(CenterX, CenterY);
		Node.ClusterRadius = static_cast<float>(ClusterRadius * 1.03 + 2.0);
	}
}

void FPjcAssetGraph::BuildRegions() {
	// folder of asset at given depth below mount point, e.g. depth 1: /Game/Characters
	const auto GetFolder = [](const FString& PackageName, const int32 Depth) {
		int32 NumSlashes = 0;
		for (int32 Index = 1; Index < PackageName.Len(); ++Index) {
			if (PackageName[Index] == TEXT('/') && ++NumSlashes > Depth) {
				return PackageName.Left(Index);
			}
		}
		return FPackageName::GetLongPackagePath(PackageName);
	};

	// one region per folder, folder holding most of its area is split by subfolders
	for (const bool bUsed : {true, false}) {
		TArray<int32> Items;
		for (const int32 Index : TopLevel) {
			if (Nodes[Index].Item->Usage.bUsed == bUsed) {
				Items.Add(Index);
			}
		}
		const int32 SplitAbove = FMath::Max(RegionSplitMin, static_cast<int32>(Items.Num() * RegionSplitShare));

		TFunction<void(const TArray<int32>&, int32)> Split;
		Split = [&](const TArray<int32>& InItems, const int32 Depth) {
			TMap<FString, TArray<int32>> ByFolder;
			for (const int32 Index : InItems) {
				ByFolder.FindOrAdd(GetFolder(Nodes[Index].Item->PackageName.ToString(), Depth)).Add(Index);
			}

			for (const auto& Folder : ByFolder) {
				bool bHasSubfolders = false;
				for (const int32 Index : Folder.Value) {
					bHasSubfolders = bHasSubfolders || GetFolder(Nodes[Index].Item->PackageName.ToString(), Depth + 1) != Folder.Key;
				}
				if (Folder.Value.Num() > SplitAbove && bHasSubfolders && Depth < RegionDepthMax) {
					Split(Folder.Value, Depth + 1);
					continue;
				}

				FPjcGraphRegion& Region = Regions.AddDefaulted_GetRef();
				Region.Path = Folder.Key;
				Region.bUsed = bUsed;
				Region.Nodes = Folder.Value;
				for (const int32 Index : Folder.Value) {
					Region.NumAssets += Nodes[Index].NumOwned + 1;
					Region.Size += Nodes[Index].SizeOwned;
				}
			}
		};
		Split(Items, 1);
	}

	NodeRegions.Init(INDEX_NONE, Nodes.Num());
	for (int32 RegionIndex = 0; RegionIndex < Regions.Num(); ++RegionIndex) {
		for (const int32 Index : Regions[RegionIndex].Nodes) {
			NodeRegions[Index] = RegionIndex;
		}
	}
	for (const int32 Index : Order) {
		if (Nodes[Index].Owner != INDEX_NONE) {
			NodeRegions[Index] = NodeRegions[Nodes[Index].Owner];
		}
	}
}

void FPjcAssetGraph::BuildLayout() {
	// links between top level assets (through anything they own) and between regions
	TArray<int32> TopOf;
	TopOf.SetNumUninitialized(Nodes.Num());
	for (const int32 Index : Order) {
		const int32 Owner = Nodes[Index].Owner;
		TopOf[Index] = Owner == INDEX_NONE ? Index : TopOf[Owner];
	}

	TSet<uint64> LinkKeys;
	TMap<int32, TArray<int32>> Neighbors;
	TMap<int32, TArray<int32>> RegionNeighbors;
	const auto AddLink = [&LinkKeys](TMap<int32, TArray<int32>>& InNeighbors, const int32 A, const int32 B, const uint64 Kind) {
		bool bAlreadyAdded = false;
		LinkKeys.Add(Kind << 62 | static_cast<uint64>(FMath::Min(A, B)) << 31 | static_cast<uint64>(FMath::Max(A, B)), &bAlreadyAdded);
		if (bAlreadyAdded) return;

		InNeighbors.FindOrAdd(A).Add(B);
		InNeighbors.FindOrAdd(B).Add(A);
	};
	for (int32 Index = 0; Index < Nodes.Num(); ++Index) {
		for (const int32 Use : Nodes[Index].Uses) {
			const int32 A = TopOf[Index];
			const int32 B = TopOf[Use];
			if (A == B) continue;

			const int32 RegionA = NodeRegions[A];
			const int32 RegionB = NodeRegions[B];
			if (RegionA == RegionB) {
				AddLink(Neighbors, A, B, 0);
			}
			else if (Regions[RegionA].bUsed == Regions[RegionB].bUsed) {
				AddLink(RegionNeighbors, RegionA, RegionB, 1);
			}
		}
	}

	// top level assets packed in their region, region circle centered at origin for now
	TArray<FVector2D> LocalCenters;
	LocalCenters.SetNum(Nodes.Num());
	TArray<FPackCircle> Circles;
	for (auto& Region : Regions) {
		const TArray<int32> Linked = GetLinkedOrder(Region.Nodes, Neighbors);
		Circles.Reset(Linked.Num());
		for (const int32 Index : Linked) {
			const float Radius = Nodes[Index].ClusterRadius;
			Circles.Add(FPackCircle {0.0, 0.0, Radius + GetGap(Radius) * 0.5});
		}

		const double Radius = PackCentered(Circles);
		for (int32 Step = 0; Step < Linked.Num(); ++Step) {
			LocalCenters[Linked[Step]] = ToVector(Circles[Step].X, Circles[Step].Y);
		}
		Region.Radius = static_cast<float>(Radius * 1.03 + 10.0);
	}

	// regions packed in used and unused areas, unused area right of used one
	double AreaLeft = 0.0;
	for (const bool bUsed : {true, false}) {
		TArray<int32> GroupRegions;
		for (int32 RegionIndex = 0; RegionIndex < Regions.Num(); ++RegionIndex) {
			if (Regions[RegionIndex].bUsed == bUsed) {
				GroupRegions.Add(RegionIndex);
			}
		}
		if (GroupRegions.Num() == 0) continue;

		GroupRegions.Sort([this](const int32 A, const int32 B) {
			return Regions[A].Radius != Regions[B].Radius ? Regions[A].Radius > Regions[B].Radius : A < B;
		});
		const TArray<int32> Linked = GetLinkedOrder(GroupRegions, RegionNeighbors);

		Circles.Reset(Linked.Num());
		for (const int32 RegionIndex : Linked) {
			const float Radius = Regions[RegionIndex].Radius;
			Circles.Add(FPackCircle {0.0, 0.0, Radius + GetRegionGap(Radius) * 0.5});
		}
		const double Radius = PackCentered(Circles);

		const FVector2D AreaCenter = ToVector(AreaLeft + Radius, 0.0);
		FBox2D& Bounds = GroupBounds[bUsed ? 0 : 1];
		for (int32 Step = 0; Step < Linked.Num(); ++Step) {
			FPjcGraphRegion& Region = Regions[Linked[Step]];
			Region.Center = AreaCenter + ToVector(Circles[Step].X, Circles[Step].Y);
			Bounds += Region.Center - FVector2D {Region.Radius, Region.Radius};
			Bounds += Region.Center + FVector2D {Region.Radius, Region.Radius};

			for (const int32 Index : Region.Nodes) {
				Nodes[Index].Position = Region.Center + LocalCenters[Index] - Nodes[Index].ClusterOffset;
			}
		}
		AreaLeft += Radius * 2.0 + FMath::Max(300.0, Radius * 0.15);
	}
}

void FPjcAssetGraph::UpdatePositions() {
	for (const int32 Index : Order) {
		FPjcGraphNode& Node = Nodes[Index];
		if (Node.Owner != INDEX_NONE) {
			Node.Position = Nodes[Node.Owner].Position + Node.Offset;
		}
	}
}
