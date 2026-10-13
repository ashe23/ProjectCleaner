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

	// vector component type, float before UE5
	using FReal = decltype(FVector2D::X);

	/** Gap kept around circle inside its folder */
	float GetGap(const float Radius) {
		return 2.0f + Radius * 0.04f;
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
	Folders.Reset();
	RootFolders.Reset();
	Bounds = FBox2D {ForceInit};
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
	BuildFolders();
	BuildLayout();
}

int32 FPjcAssetGraph::Find(const FName& PackageName) const {
	const int32* Index = NodeIndices.Find(PackageName);
	return Index ? *Index : INDEX_NONE;
}

const TArray<FPjcGraphNode>& FPjcAssetGraph::GetNodes() const {
	return Nodes;
}

const TArray<FPjcGraphFolder>& FPjcAssetGraph::GetFolders() const {
	return Folders;
}

const TArray<int32>& FPjcAssetGraph::GetRootFolders() const {
	return RootFolders;
}

FBox2D FPjcAssetGraph::GetBounds() const {
	return Bounds;
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

	// owners come before what they own
	TArray<int32> Order;
	Order.Reserve(Num);
	for (int32 Step = 1; Step < ReversePostOrder.Num(); ++Step) {
		const int32 Index = ReversePostOrder[Step];
		FPjcGraphNode& Node = Nodes[Index];
		Node.Owner = Dominator[Index] == Root ? INDEX_NONE : Dominator[Index];
		if (Node.Owner != INDEX_NONE) {
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
}

void FPjcAssetGraph::BuildFolders() {
	// folder tree like content browser shows it
	struct FTreeFolder
	{
		FString Path;
		TArray<int32> Folders;
		TArray<int32> Assets;
	};
	TArray<FTreeFolder> Tree;
	TMap<FString, int32> TreeIndices;
	TArray<int32> TreeRoots;

	TFunction<int32(const FString&)> GetTreeFolder;
	GetTreeFolder = [&](const FString& Path) {
		if (const int32* Found = TreeIndices.Find(Path)) return *Found;

		const int32 Slash = Path.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		const int32 Parent = Slash > 0 ? GetTreeFolder(Path.Left(Slash)) : INDEX_NONE;
		const int32 Index = Tree.Add(FTreeFolder {Path, {}, {}});
		TreeIndices.Add(Path, Index);
		if (Parent == INDEX_NONE) {
			TreeRoots.Add(Index);
		}
		else {
			Tree[Parent].Folders.Add(Index);
		}
		return Index;
	};
	for (int32 Index = 0; Index < Nodes.Num(); ++Index) {
		Tree[GetTreeFolder(FPackageName::GetLongPackagePath(Nodes[Index].Item->PackageName.ToString()))].Assets.Add(Index);
	}

	// folders holding nothing but one folder are merged with it, parents get lower indices than subfolders
	const auto GetLastName = [](const FString& Path) {
		return Path.Mid(Path.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromEnd) + 1);
	};
	TFunction<void(int32, int32)> AddFolder;
	AddFolder = [&](int32 TreeIndex, const int32 Parent) {
		FString Name = GetLastName(Tree[TreeIndex].Path);
		while (Tree[TreeIndex].Assets.Num() == 0 && Tree[TreeIndex].Folders.Num() == 1) {
			TreeIndex = Tree[TreeIndex].Folders[0];
			Name += TEXT("/") + GetLastName(Tree[TreeIndex].Path);
		}

		const int32 Index = Folders.AddDefaulted();
		Folders[Index].Path = Tree[TreeIndex].Path;
		Folders[Index].Name = Name;
		Folders[Index].Parent = Parent;
		Folders[Index].Assets = Tree[TreeIndex].Assets;
		for (const int32 Asset : Tree[TreeIndex].Assets) {
			Nodes[Asset].Folder = Index;
		}
		if (Parent == INDEX_NONE) {
			RootFolders.Add(Index);
		}
		else {
			Folders[Parent].Folders.Add(Index);
		}

		for (const int32 Child : Tree[TreeIndex].Folders) {
			AddFolder(Child, Index);
		}
	};
	for (const int32 TreeRoot : TreeRoots) {
		AddFolder(TreeRoot, INDEX_NONE);
	}

	// totals from deepest folders up
	for (int32 Index = Folders.Num() - 1; Index >= 0; --Index) {
		FPjcGraphFolder& Folder = Folders[Index];
		for (const int32 Asset : Folder.Assets) {
			const FPjcUsageItem& Item = *Nodes[Asset].Item;
			++Folder.NumAssets;
			Folder.Size += Item.Size;
			if (!Item.Usage.bUsed) {
				++Folder.NumUnused;
				Folder.SizeUnused += Item.Size;
				Folder.NumNotDeleted += Item.Kept.IsEmpty() ? 0 : 1;
			}
		}
		for (const int32 Subfolder : Folder.Folders) {
			Folder.NumAssets += Folders[Subfolder].NumAssets;
			Folder.NumUnused += Folders[Subfolder].NumUnused;
			Folder.NumNotDeleted += Folders[Subfolder].NumNotDeleted;
			Folder.Size += Folders[Subfolder].Size;
			Folder.SizeUnused += Folders[Subfolder].SizeUnused;
		}
	}

	for (auto& Folder : Folders) {
		Folder.Folders.Sort([this](const int32 A, const int32 B) {
			return Folders[A].Size != Folders[B].Size ? Folders[A].Size > Folders[B].Size : A < B;
		});
		Folder.Assets.Sort([this](const int32 A, const int32 B) {
			return Nodes[A].Item->Size != Nodes[B].Item->Size ? Nodes[A].Item->Size > Nodes[B].Item->Size : A < B;
		});
	}
}

void FPjcAssetGraph::BuildLayout() {
	// every folder packs its subfolders and assets, deepest first so subfolders know their size
	struct FPackItem
	{
		int32 Index;
		bool bFolder;
		float Radius;
	};
	TArray<FVector2D> AssetOffsets;
	TArray<FVector2D> FolderOffsets;
	AssetOffsets.SetNum(Nodes.Num());
	FolderOffsets.SetNum(Folders.Num());

	TArray<FPackItem> Items;
	TArray<FPackCircle> Circles;
	const auto Pack = [&]() {
		Items.Sort([](const FPackItem& A, const FPackItem& B) {
			return A.Radius > B.Radius;
		});
		Circles.Reset(Items.Num());
		for (const auto& Item : Items) {
			Circles.Add(FPackCircle {0.0, 0.0, Item.Radius + GetGap(Item.Radius) * 0.5});
		}

		const double Radius = PackCentered(Circles);
		for (int32 Step = 0; Step < Items.Num(); ++Step) {
			const FVector2D Offset = ToVector(Circles[Step].X, Circles[Step].Y);
			(Items[Step].bFolder ? FolderOffsets[Items[Step].Index] : AssetOffsets[Items[Step].Index]) = Offset;
		}
		return Radius;
	};

	for (int32 Index = Folders.Num() - 1; Index >= 0; --Index) {
		FPjcGraphFolder& Folder = Folders[Index];
		Items.Reset();
		for (const int32 Subfolder : Folder.Folders) {
			Items.Add(FPackItem {Subfolder, true, Folders[Subfolder].Radius});
		}
		for (const int32 Asset : Folder.Assets) {
			Items.Add(FPackItem {Asset, false, Nodes[Asset].Radius});
		}

		// room for folder outline
		const double Radius = Pack();
		Folder.Radius = static_cast<float>(Radius * 1.02 + 4.0);
	}

	// mount points side by side
	Items.Reset();
	for (const int32 Index : RootFolders) {
		Items.Add(FPackItem {Index, true, Folders[Index].Radius * 1.05f});
	}
	Pack();

	for (int32 Index = 0; Index < Folders.Num(); ++Index) {
		FPjcGraphFolder& Folder = Folders[Index];
		Folder.Center = Folder.Parent == INDEX_NONE ? FolderOffsets[Index] : Folders[Folder.Parent].Center + FolderOffsets[Index];
		for (const int32 Asset : Folder.Assets) {
			Nodes[Asset].Position = Folder.Center + AssetOffsets[Asset];
		}
		if (Folder.Parent == INDEX_NONE) {
			Bounds += Folder.Center - FVector2D {Folder.Radius, Folder.Radius};
			Bounds += Folder.Center + FVector2D {Folder.Radius, Folder.Radius};
		}
	}
}
