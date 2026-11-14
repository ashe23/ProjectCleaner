// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcInspector.h"
#include "PjcShim.h"
#include "PjcSubsystem.h"
// Engine Headers
#include "Misc/PackageName.h"
#include "Misc/ScopedSlowTask.h"

namespace
{
	// unity builds join files: names unique to this file

	EPjcLinkKind PjcInspectorKindOf(const FAssetDependency& Dependency) {
		if (!EnumHasAnyFlags(Dependency.Properties, UE::AssetRegistry::EDependencyProperty::Game)) return EPjcLinkKind::EditorOnly;
		return EnumHasAnyFlags(Dependency.Properties, UE::AssetRegistry::EDependencyProperty::Hard) ? EPjcLinkKind::Hard : EPjcLinkKind::Soft;
	}

	// The map an actor or object file belongs to: '/Game/__ExternalActors__/Maps/Main/A/BC/Hash' is of '/Game/Maps/Main'
	FName PjcInspectorOwnerOf(const FName External, FPjcInspectorCache* Cache) {
		if (!Cache) return UPjcSubsystem::GetExternalOwner(External);
		if (const FName* Known = Cache->Owners.Find(External)) return *Known;

		const FName Owner = UPjcSubsystem::GetExternalOwner(External);
		Cache->Owners.Add(External, Owner);
		return Owner;
	}

	// from this many files on a list shows that it works: a map can have thousands
	constexpr int32 PjcInspectorSlowAt = 64;

	// Order of the kinds of root when a route is picked: what the game needs first, what only the editor or the last
	// cook keeps last
	int32 PjcInspectorRootRank(const EPjcUsageReason Reason) {
		switch (Reason) {
			case EPjcUsageReason::Primary :
			case EPjcUsageReason::PrimaryClass :		return 0;
			case EPjcUsageReason::GameplayCue :			return 1;
			case EPjcUsageReason::Cinematic :			return 2;
			case EPjcUsageReason::EngineReferenced :	return 4;
			case EPjcUsageReason::Editor :
			case EPjcUsageReason::Splash :				return 5;
			case EPjcUsageReason::CookPackaged :
			case EPjcUsageReason::GameplayCueNotFired : return 6;
			default :									return 3;
		}
	}

	int32 PjcInspectorKindRank(const FPjcModelLink& Link) {
		if (Link.Via != EPjcUsageReason::None) return 0;
		return Link.Kind == EPjcLinkKind::Hard ? 0 : Link.Kind == EPjcLinkKind::Soft ? 1 : 2;
	}
}	// namespace

void FPjcInspectorModel::Build(const FPjcScanResult& Result, FPjcInspectorCache* Cache) {
	Time = Result.Time;
	Packages.Reset();
	Assets.Reset();
	Uses.Reset();
	UsedBy.Reset();
	Sizes.Reset();
	bScanned.Reset();
	bUsed.Reset();
	bRoot.Reset();
	Reasons.Reset();
	IndexOf.Reset();
	DominatorCache.Reset();
	FPjcInspectorCache OwnCache;
	if (!Cache) Cache = &OwnCache;

	TMap<FName, const FAssetData*> Known;
	for (const FAssetData& Asset : Result.Assets) {
		Known.Add(Asset.PackageName, &Asset);
	}
	for (const FAssetData& Asset : Result.Redirectors) {
		Known.Add(Asset.PackageName, &Asset);
	}
	IAssetRegistry& Registry = UPjcSubsystem::GetModuleAssetRegistry().Get();
	TArray<FAssetData> Found;
	const auto Add = [&](const FName Package) {
		if (const int32* Existing = IndexOf.Find(Package)) return *Existing;

		const int32 Index = Packages.Add(Package);
		IndexOf.Add(Package, Index);
		if (const FAssetData* const* Asset = Known.Find(Package)) {
			Assets.Add(**Asset);
		}
		else {
			Found.Reset();
			Registry.GetAssetsByPackageName(Package, Found);
			Assets.Add(Found.Num() > 0 ? Found[0] : FAssetData {});
		}
		Uses.AddDefaulted();
		UsedBy.AddDefaulted();
		Sizes.Add(Result.GetSize(Package));
		const FPjcAssetUsage* Usage = Result.Usage.Find(Package);
		bScanned.Add(Usage != nullptr);
		bUsed.Add(Usage && Usage->bUsed);
		bRoot.Add(Usage && Usage->bUsed && Usage->Parent.IsNone());
		Reasons.Add(Usage ? Usage->Reason : EPjcUsageReason::None);
		return Index;
	};

	TArray<FName> Scanned;
	Result.Usage.GenerateKeyArray(Scanned);
	Scanned.Sort(FNameLexicalLess {});
	for (const FName Package : Scanned) {
		Add(Package);
	}

	// a project with thousands of assets takes a moment: a progress window once it does
	FScopedSlowTask Reading {static_cast<float>(Scanned.Num()), FText::FromString(TEXT("Reading references..."))};
	Reading.MakeDialogDelayed(0.5f);
	TArray<FPjcInspectorLink> Links;
	int32 Through = 0;
	for (int32 From = 0; From < Scanned.Num(); ++From) {
		Reading.EnterProgressFrame(1.0f);
		GetLinks(Scanned[From], false, Links, Through, Cache);
		for (const FPjcInspectorLink& Link : Links) {
			const int32 To = Add(Link.Package);
			if (To == From) continue;

			Uses[From].Add(FPjcModelLink {To, Link.Kind, EPjcUsageReason::None, Link.bThrough});
			UsedBy[To].Add(FPjcModelLink {From, Link.Kind, EPjcUsageReason::None, Link.bThrough});
		}
	}

	// kept for another asset, not through a reference of the game: a localized copy for its source, an asset for the
	// label that lists it or whose folder it is in, a cue for what fires it. The route goes through that asset
	for (int32 Node = 0; Node < Scanned.Num(); ++Node) {
		const FPjcAssetUsage* Usage = Result.Usage.Find(Scanned[Node]);
		if (!Usage || !Usage->bUsed || Usage->Parent.IsNone() || Usage->Reason == EPjcUsageReason::Dependency) continue;

		const int32* Parent = IndexOf.Find(Usage->Parent);
		if (!Parent || *Parent == Node) continue;

		FPjcModelLink* Existing = Uses[*Parent].FindByPredicate([Node](const FPjcModelLink& Link) {
			return Link.Other == Node;
		});
		if (Existing) {
			Existing->Via = Usage->Reason;
			FPjcModelLink* Back = UsedBy[Node].FindByPredicate([Parent](const FPjcModelLink& Link) {
				return Link.Other == *Parent;
			});
			if (Back) Back->Via = Usage->Reason;
			continue;
		}
		Uses[*Parent].Add(FPjcModelLink {Node, EPjcLinkKind::Hard, Usage->Reason, false});
		UsedBy[Node].Add(FPjcModelLink {*Parent, EPjcLinkKind::Hard, Usage->Reason, false});
	}
}

int32 FPjcInspectorModel::Find(const FName Package) const {
	const int32* Index = IndexOf.Find(Package);
	return Index ? *Index : INDEX_NONE;
}

const FPjcInspectorModel::FDominators& FPjcInspectorModel::GetDominators(const uint8 Kinds) const {
	for (const FDominators& Existing : DominatorCache) {
		if (Existing.Kinds == Kinds) return Existing;
	}

	// Cooper, Harvey and Kennedy over a virtual root above every root: an asset's dominator subtree is what goes when
	// it goes, its immediate dominator the one asset every route passes
	const int32 N = Num();
	const int32 V = N;
	TArray<int32> Roots;
	for (int32 Node = 0; Node < N; ++Node) {
		if (bRoot[Node]) Roots.Add(Node);
	}
	const auto Counted = [this, Kinds](const FPjcModelLink& Link) {
		return Counts(Kinds, Link) || Link.Via != EPjcUsageReason::None;
	};

	// depth first from the virtual root: post order
	TArray<int32> Post;
	Post.Reserve(N + 1);
	TArray<int32> PostNum;
	PostNum.Init(INDEX_NONE, N + 1);
	TArray<bool> Seen;
	Seen.Init(false, N + 1);
	struct FFrame
	{
		int32 Node;
		int32 Next;
	};
	TArray<FFrame> Stack;
	Stack.Add(FFrame {V, 0});
	Seen[V] = true;
	while (Stack.Num() > 0) {
		const int32 Top = Stack.Num() - 1;
		const int32 Node = Stack[Top].Node;
		int32 Pushed = INDEX_NONE;
		if (Node == V) {
			while (Pushed == INDEX_NONE && Stack[Top].Next < Roots.Num()) {
				const int32 Root = Roots[Stack[Top].Next++];
				if (!Seen[Root]) Pushed = Root;
			}
		}
		else {
			const TArray<FPjcModelLink>& Links = Uses[Node];
			while (Pushed == INDEX_NONE && Stack[Top].Next < Links.Num()) {
				const FPjcModelLink& Link = Links[Stack[Top].Next++];
				if (Counted(Link) && !Seen[Link.Other]) Pushed = Link.Other;
			}
		}
		if (Pushed != INDEX_NONE) {
			Seen[Pushed] = true;
			Stack.Add(FFrame {Pushed, 0});
			continue;
		}
		PostNum[Node] = Post.Num();
		Post.Add(Node);
		PjcShim::PopArray(Stack);
	}

	TArray<int32> Idom;
	Idom.Init(INDEX_NONE, N + 1);
	Idom[V] = V;
	const auto Intersect = [&Idom, &PostNum](int32 A, int32 B) {
		while (A != B) {
			while (PostNum[A] < PostNum[B]) A = Idom[A];
			while (PostNum[B] < PostNum[A]) B = Idom[B];
		}
		return A;
	};
	for (bool bChanged = true; bChanged;) {
		bChanged = false;
		for (int32 Order = Post.Num() - 1; Order >= 0; --Order) {
			const int32 Node = Post[Order];
			if (Node == V) continue;

			int32 New = bRoot[Node] ? V : INDEX_NONE;
			for (const FPjcModelLink& Link : UsedBy[Node]) {
				if (!Counted(Link) || !Seen[Link.Other] || Idom[Link.Other] == INDEX_NONE) continue;
				New = New == INDEX_NONE ? Link.Other : Intersect(Link.Other, New);
			}
			if (New != INDEX_NONE && Idom[Node] != New) {
				Idom[Node] = New;
				bChanged = true;
			}
		}
	}

	FDominators& Out = DominatorCache.AddDefaulted_GetRef();
	Out.Kinds = Kinds;
	Out.Idom.SetNum(N);
	Out.Kids.SetNum(N);
	for (int32 Node = 0; Node < N; ++Node) {
		Out.Idom[Node] = Idom[Node] == V ? Virtual : Idom[Node];
		if (Idom[Node] >= 0 && Idom[Node] < N) Out.Kids[Idom[Node]].Add(Node);
	}
	// steps from the nearest root
	Out.Depth.Init(INDEX_NONE, N);
	TArray<int32> Queue = Roots;
	for (const int32 Root : Roots) {
		Out.Depth[Root] = 0;
	}
	for (int32 Head = 0; Head < Queue.Num(); ++Head) {
		for (const FPjcModelLink& Link : Uses[Queue[Head]]) {
			if (!Counted(Link) || Out.Depth[Link.Other] != INDEX_NONE) continue;

			Out.Depth[Link.Other] = Out.Depth[Queue[Head]] + 1;
			Queue.Add(Link.Other);
		}
	}
	return Out;
}

int32 FPjcInspectorModel::GetDepth(const int32 Node, const uint8 Kinds) const {
	return Packages.IsValidIndex(Node) ? GetDominators(Kinds).Depth[Node] : INDEX_NONE;
}

TArray<int32> FPjcInspectorModel::GetRetained(const int32 Node, const uint8 Kinds) const {
	TArray<int32> Out;
	if (!Packages.IsValidIndex(Node)) return Out;

	const FDominators& Dominators = GetDominators(Kinds);
	if (Dominators.Idom[Node] == INDEX_NONE) return Out;

	Out.Add(Node);
	for (int32 Head = 0; Head < Out.Num(); ++Head) {
		Out.Append(Dominators.Kids[Out[Head]]);
	}
	return Out;
}

TArray<int32> FPjcInspectorModel::GetChoke(const int32 Node, const uint8 Kinds) const {
	TArray<int32> Out;
	if (!Packages.IsValidIndex(Node)) return Out;

	const FDominators& Dominators = GetDominators(Kinds);
	for (int32 Step = Dominators.Idom[Node]; Step >= 0 && Out.Num() < Num(); Step = Dominators.Idom[Step]) {
		Out.Add(Step);
	}
	return Out;
}

int32 FPjcInspectorModel::GetKeeper(const int32 Node, const uint8 Kinds) const {
	if (!Packages.IsValidIndex(Node) || bRoot[Node]) return INDEX_NONE;

	const int32 Idom = GetDominators(Kinds).Idom[Node];
	return Idom == Virtual ? SeveralRoots : Idom;
}

TArray<FPjcRoute> FPjcInspectorModel::GetRoutes(const int32 Node, const uint8 Kinds, const TMap<int32, int32>& Prefer) const {
	TArray<FPjcRoute> Routes;
	if (!Packages.IsValidIndex(Node)) return Routes;

	// up from the asset, breadth first over the used assets that use it: the shortest route to every root
	TMap<int32, int32> Next;
	Next.Add(Node, INDEX_NONE);
	TArray<int32> Queue {Node};
	TArray<int32> Roots;
	for (int32 Head = 0; Head < Queue.Num(); ++Head) {
		const int32 Current = Queue[Head];
		if (bRoot[Current]) Roots.Add(Current);
		for (const FPjcModelLink& Link : UsedBy[Current]) {
			if ((!Counts(Kinds, Link) && Link.Via == EPjcUsageReason::None) || !bUsed[Link.Other] || Next.Contains(Link.Other)) continue;

			Next.Add(Link.Other, Current);
			Queue.Add(Link.Other);
		}
	}

	struct FScored
	{
		FPjcRoute Route;
		TArray<int32, TInlineAllocator<6>> Score;
	};
	TArray<FScored> Scored;
	for (const int32 Root : Roots) {
		FScored& Item = Scored.AddDefaulted_GetRef();
		for (int32 Step = Root; Step != INDEX_NONE; Step = Next.FindRef(Step)) {
			Item.Route.Path.Add(Step);
			if (Step == Node) break;
		}
		Item.Route.Root = Root;
		Item.Route.Steps = Item.Route.Path.Num() - 1;
		int32 Worst = 0;
		for (int32 Index = 1; Index < Item.Route.Path.Num(); ++Index) {
			const int32 To = Item.Route.Path[Index];
			const FPjcModelLink* Link = Uses[Item.Route.Path[Index - 1]].FindByPredicate([To](const FPjcModelLink& Existing) {
				return Existing.Other == To;
			});
			Worst = FMath::Max(Worst, Link ? PjcInspectorKindRank(*Link) : 0);
		}
		const EPjcUsageReason RootReason = Reasons[Root];
		Item.Route.bEditor = Worst == 2 || RootReason == EPjcUsageReason::Editor || RootReason == EPjcUsageReason::Splash;
		const bool bProject = Packages[Root].ToString().StartsWith(TEXT("/Game/"));
		Item.Score = {-Prefer.FindRef(Root), Item.Route.bEditor ? 1 : 0, PjcInspectorRootRank(RootReason), Worst, bProject ? 0 : 1, Item.Route.Steps};
	}
	Scored.Sort([this](const FScored& A, const FScored& B) {
		for (int32 Index = 0; Index < A.Score.Num(); ++Index) {
			if (A.Score[Index] != B.Score[Index]) return A.Score[Index] < B.Score[Index];
		}
		return Packages[A.Route.Root].LexicalLess(Packages[B.Route.Root]);
	});
	for (FScored& Item : Scored) {
		Routes.Add(MoveTemp(Item.Route));
	}
	return Routes;
}

TArray<int32> FPjcInspectorModel::GetKeyRoutes(const TArray<FPjcRoute>& Routes) const {
	TArray<int32> Out;
	if (Routes.Num() == 0) return Out;

	Out.Add(0);
	const auto ClassOf = [this](const FPjcRoute& Route) {
		return PjcShim::GetAssetClassName(Assets[Route.Root]);
	};
	const auto UserOf = [](const FPjcRoute& Route) {
		return Route.Path.Num() > 1 ? Route.Path[Route.Path.Num() - 2] : Route.Root;
	};
	// first the routes that differ in both, then in one of them
	for (const bool bBoth : {true, false}) {
		for (int32 Index = 1; Index < Routes.Num() && Out.Num() < 3; ++Index) {
			if (Out.Contains(Index)) continue;

			const bool bClash = Out.ContainsByPredicate([&](const int32 Taken) {
				const bool bClass = ClassOf(Routes[Taken]) != ClassOf(Routes[Index]);
				const bool bUser = UserOf(Routes[Taken]) != UserOf(Routes[Index]);
				return bBoth ? !(bClass && bUser) : !(bClass || bUser);
			});
			if (!bClash) Out.Add(Index);
		}
	}
	return Out;
}

FPjcInspectorModel::FReach FPjcInspectorModel::GetReach(const int32 Node, const bool bUsedBy, const uint8 Kinds) const {
	FReach Reach;
	if (!Packages.IsValidIndex(Node)) return Reach;

	TSet<int32> Seen {Node};
	TArray<int32> Queue {Node};
	for (int32 Head = 0; Head < Queue.Num(); ++Head) {
		for (const FPjcModelLink& Link : (bUsedBy ? UsedBy : Uses)[Queue[Head]]) {
			if (!Counts(Kinds, Link) || Seen.Contains(Link.Other)) continue;

			Seen.Add(Link.Other);
			Queue.Add(Link.Other);
			++Reach.Num;
			Reach.Size += Sizes[Link.Other];
		}
	}
	return Reach;
}

FPjcInspectorModel::FWhatIf FPjcInspectorModel::GetWhatIf(const int32 Focus, const int32 User, const uint8 Kinds) const {
	FWhatIf Out;
	if (!Packages.IsValidIndex(Focus) || !Packages.IsValidIndex(User)) return Out;

	// every root but the user, never through the user
	TArray<int32> Prev;
	Prev.Init(INDEX_NONE, Num());
	TArray<bool> Seen;
	Seen.Init(false, Num());
	TArray<int32> Queue;
	for (int32 Node = 0; Node < Num(); ++Node) {
		if (!bRoot[Node] || Node == User) continue;

		Seen[Node] = true;
		Queue.Add(Node);
	}
	for (int32 Head = 0; Head < Queue.Num(); ++Head) {
		for (const FPjcModelLink& Link : Uses[Queue[Head]]) {
			if ((!Counts(Kinds, Link) && Link.Via == EPjcUsageReason::None) || Link.Other == User || Seen[Link.Other]) continue;

			Seen[Link.Other] = true;
			Prev[Link.Other] = Queue[Head];
			Queue.Add(Link.Other);
		}
	}
	if (Seen[Focus]) {
		Out.bStill = true;
		for (int32 Step = Focus; Step != INDEX_NONE; Step = Prev[Step]) {
			Out.Path.Insert(Step, 0);
		}
		Out.bEditor = Reasons[Out.Path[0]] == EPjcUsageReason::Editor;
		for (int32 Index = 1; Index < Out.Path.Num(); ++Index) {
			const int32 To = Out.Path[Index];
			const FPjcModelLink* Link = Uses[Out.Path[Index - 1]].FindByPredicate([To](const FPjcModelLink& Existing) {
				return Existing.Other == To;
			});
			if (Link && PjcInspectorKindRank(*Link) == 2) Out.bEditor = true;
		}
		return Out;
	}
	// what no other root reaches then becomes unused with it
	const FDominators& Dominators = GetDominators(Kinds);
	for (int32 Node = 0; Node < Num(); ++Node) {
		if (Node == User || Seen[Node] || Dominators.Idom[Node] == INDEX_NONE) continue;

		++Out.Num;
		Out.Size += Sizes[Node];
	}
	return Out;
}

FPjcInspectorModel::FSearch FPjcInspectorModel::Search(
	const int32 Node,
	const bool bUsedBy,
	const uint8 Kinds,
	TFunctionRef<bool(const int32 From, const FPjcModelLink& Link)> Match
) const {
	FSearch Out;
	if (!Packages.IsValidIndex(Node)) return Out;

	TMap<int32, int32> Prev;
	Prev.Add(Node, INDEX_NONE);
	TArray<int32> Queue {Node};
	TArray<int32> Hits;
	for (int32 Head = 0; Head < Queue.Num() && Hits.Num() < MaxHits; ++Head) {
		for (const FPjcModelLink& Link : (bUsedBy ? UsedBy : Uses)[Queue[Head]]) {
			if (!Counts(Kinds, Link) || Prev.Contains(Link.Other)) continue;

			Prev.Add(Link.Other, Queue[Head]);
			Queue.Add(Link.Other);
			if (Match(Queue[Head], Link)) Hits.Add(Link.Other);
		}
	}
	Out.Searched = Queue.Num() - 1;
	for (const int32 Hit : Hits) {
		Out.Hits.Add(Hit);
		for (int32 Step = Hit; Step != INDEX_NONE && Step != Node && !Out.Keep.Contains(Step); Step = Prev.FindRef(Step)) {
			Out.Keep.Add(Step);
		}
	}
	return Out;
}

bool FPjcInspectorModel::IsExternal(const FName Package) {
	return UPjcSubsystem::FolderIsExternal(FPackageName::GetLongPackagePath(Package.ToString()));
}

void FPjcInspectorModel::GetLinks(
	const FName Package,
	const bool bUsedBy,
	TArray<FPjcInspectorLink>& Out,
	int32& OutThrough,
	FPjcInspectorCache* Cache
) {
	if (Cache) {
		if (const FPjcInspectorCache::FLinks* Known = (bUsedBy ? Cache->UsedBy : Cache->Uses).Find(Package)) {
			Out = Known->Links;
			OutThrough = Known->Through;
			return;
		}
	}
	ReadLinks(Package, bUsedBy, Out, OutThrough, Cache);
	if (Cache) {
		(bUsedBy ? Cache->UsedBy : Cache->Uses).Add(Package, FPjcInspectorCache::FLinks {Out, OutThrough});
	}
}

void FPjcInspectorModel::ReadLinks(
	const FName Package,
	const bool bUsedBy,
	TArray<FPjcInspectorLink>& Out,
	int32& OutThrough,
	FPjcInspectorCache* Cache
) {
	Out.Reset();
	OutThrough = 0;
	IAssetRegistry& Registry = UPjcSubsystem::GetModuleAssetRegistry().Get();
	TArray<FAssetDependency> Uses;
	TArray<FAssetDependency> UsedBy;
	Registry.GetDependencies(FAssetIdentifier {Package}, Uses, UE::AssetRegistry::EDependencyCategory::Package);
	Registry.GetReferencers(FAssetIdentifier {Package}, UsedBy, UE::AssetRegistry::EDependencyCategory::Package);
	// a window that waits for the answer shows its progress once this takes long (the Inspector tab opens one)
	const bool bSlow = (bUsedBy ? UsedBy : Uses).Num() + (bUsedBy ? 0 : UsedBy.Num()) >= PjcInspectorSlowAt;
	FScopedSlowTask Reading {static_cast<float>((UsedBy.Num() + Uses.Num()) * 2 + 1), FText::GetEmpty(), bSlow};
	const auto Step = [&Reading, bSlow]() {
		if (bSlow) Reading.EnterProgressFrame(1.0f);
	};

	TSet<FName> Seen {Package};
	const auto Add = [&Out, &Seen](const FName Name, const EPjcLinkKind Kind, const bool bThrough) {
		if (Name.IsNone() || Seen.Contains(Name) || IsExternal(Name) || Name.ToString().StartsWith(TEXT("/Script/"))) return;

		Seen.Add(Name);
		Out.Add(FPjcInspectorLink {Name, Kind, bThrough});
	};

	// its own references first
	for (const FAssetDependency& Link : bUsedBy ? UsedBy : Uses) {
		Add(Link.AssetId.PackageName, PjcInspectorKindOf(Link), false);
	}

	TSet<FName> Through;
	TArray<FAssetDependency> Far;
	if (bUsedBy) {
		// an actor file references it: the map of that actor uses it
		for (const FAssetDependency& Link : UsedBy) {
			Step();
			const FName Actor = Link.AssetId.PackageName;
			if (!IsExternal(Actor)) continue;

			const FName Owner = PjcInspectorOwnerOf(Actor, Cache);
			if (Owner == Package) continue;

			Through.Add(Actor);
			if (!Owner.IsNone()) {
				Add(Owner, PjcInspectorKindOf(Link), true);
				continue;
			}
			Far.Reset();
			Registry.GetReferencers(FAssetIdentifier {Actor}, Far, UE::AssetRegistry::EDependencyCategory::Package);
			for (const FAssetDependency& User : Far) {
				Add(User.AssetId.PackageName, PjcInspectorKindOf(Link), true);
			}
		}
		OutThrough = Through.Num();
		return;
	}

	// a map uses what its actor files use: the files it references and the ones that say they are its own.
	// An actor file of another map stands for that map
	for (const FAssetDependency& Link : Uses) {
		Step();
		const FName Actor = Link.AssetId.PackageName;
		if (!IsExternal(Actor)) continue;

		const FName Owner = PjcInspectorOwnerOf(Actor, Cache);
		Through.Add(Actor);
		if (!Owner.IsNone() && Owner != Package) {
			Add(Owner, PjcInspectorKindOf(Link), true);
			Through.Remove(Actor);
			++OutThrough;
		}
	}
	for (const FAssetDependency& Link : UsedBy) {
		Step();
		const FName Actor = Link.AssetId.PackageName;
		if (IsExternal(Actor) && PjcInspectorOwnerOf(Actor, Cache) == Package) Through.Add(Actor);
	}
	OutThrough += Through.Num();
	for (const FName Actor : Through) {
		Step();
		Far.Reset();
		Registry.GetDependencies(FAssetIdentifier {Actor}, Far, UE::AssetRegistry::EDependencyCategory::Package);
		for (const FAssetDependency& Used : Far) {
			Add(Used.AssetId.PackageName, PjcInspectorKindOf(Used), true);
		}
	}
}
