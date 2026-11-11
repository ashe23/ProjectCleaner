// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcInspector.h"
#include "PjcShim.h"
#include "PjcSubsystem.h"
// Engine Headers
#include "Misc/PackageName.h"

namespace
{
	// unity builds join files: names unique to this file

	EPjcLinkKind PjcInspectorKindOf(const FAssetDependency& Dependency) {
		if (!EnumHasAnyFlags(Dependency.Properties, UE::AssetRegistry::EDependencyProperty::Game)) return EPjcLinkKind::EditorOnly;
		return EnumHasAnyFlags(Dependency.Properties, UE::AssetRegistry::EDependencyProperty::Hard) ? EPjcLinkKind::Hard : EPjcLinkKind::Soft;
	}

	// The map an actor or object file belongs to: '/Game/__ExternalActors__/Maps/Main/A/BC/Hash' is of '/Game/Maps/Main'
	FName PjcInspectorOwnerOf(const FName External) {
		FString Path = External.ToString();
		for (const auto& Folder : {PjcShim::GetExternalActorsFolderName(), PjcShim::GetExternalObjectsFolderName()}) {
			const FString Segment = TEXT("/") + Folder + TEXT("/");
			const int32 At = Folder.IsEmpty() ? INDEX_NONE : Path.Find(Segment, ESearchCase::CaseSensitive);
			if (At == INDEX_NONE) continue;

			Path = Path.Left(At) + TEXT("/") + Path.Mid(At + Segment.Len());
			break;
		}

		// the file sits a few hashed folders below the path of its map: the longest part that is a package
		IAssetRegistry& Registry = UPjcSubsystem::GetModuleAssetRegistry().Get();
		TArray<FAssetData> Assets;
		for (int32 Step = 0; Step < 4; ++Step) {
			int32 Slash = INDEX_NONE;
			if (!Path.FindLastChar(TEXT('/'), Slash) || Slash <= 0) break;

			Path = Path.Left(Slash);
			Assets.Reset();
			Registry.GetAssetsByPackageName(FName {*Path}, Assets);
			if (Assets.Num() > 0) return FName {*Path};
		}
		return NAME_None;
	}

	void PjcInspectorNames(const FName Package, const bool bUsedBy, TArray<FName>& Out) {
		TArray<FPjcInspectorLink> Links;
		int32 Through = 0;
		FPjcInspectorGraph::GetLinks(Package, bUsedBy, Links, Through);
		for (const FPjcInspectorLink& Link : Links) {
			Out.Add(Link.Package);
		}
	}

	// Root path: root first, the focus last
	struct FPjcRootPath
	{
		FName Root;
		TArray<FName> Path;
	};

	bool PjcInspectorIsRoot(const FPjcScanResult& Result, const FName Package) {
		const FPjcAssetUsage* Usage = Result.Usage.Find(Package);
		return Usage && Usage->bUsed && Usage->Parent.IsNone();
	}

	// Every root that keeps the package used, with the shortest path from it, breadth first over used referencers
	void PjcInspectorRootPaths(const FPjcScanResult& Result, const FName Focus, TArray<FPjcRootPath>& OutPaths) {
		TMap<FName, FName> Next;
		Next.Add(Focus, NAME_None);
		TArray<FName> Queue {Focus};
		TArray<FName> Roots;
		TArray<FName> Referencers;
		for (int32 Head = 0; Head < Queue.Num(); ++Head) {
			const FName Current = Queue[Head];
			if (PjcInspectorIsRoot(Result, Current)) {
				Roots.Add(Current);
			}

			Referencers.Reset();
			PjcInspectorNames(Current, true, Referencers);
			Referencers.Sort(FNameLexicalLess {});
			for (const FName Referencer : Referencers) {
				const FPjcAssetUsage* Usage = Result.Usage.Find(Referencer);
				if (!Usage || !Usage->bUsed || Next.Contains(Referencer)) continue;

				Next.Add(Referencer, Current);
				Queue.Add(Referencer);
			}
		}

		for (const FName Root : Roots) {
			FPjcRootPath& RootPath = OutPaths.AddDefaulted_GetRef();
			RootPath.Root = Root;
			for (FName Step = Root; !Step.IsNone(); Step = Next.FindRef(Step)) {
				RootPath.Path.Add(Step);
			}
		}

		// used without a referencer chain (localized copies, labels): the chain the scan recorded
		if (OutPaths.Num() == 0 && Result.IsUsed(Focus)) {
			TArray<FName> Chain {Focus};
			const FPjcAssetUsage* Usage = Result.Usage.Find(Focus);
			for (int32 Steps = 0; Usage && !Usage->Parent.IsNone() && Steps < 256; ++Steps) {
				Chain.Insert(Usage->Parent, 0);
				Usage = Result.Usage.Find(Usage->Parent);
			}
			FPjcRootPath& RootPath = OutPaths.AddDefaulted_GetRef();
			RootPath.Root = Chain[0];
			RootPath.Path = MoveTemp(Chain);
		}
	}

	FName PjcInspectorClassOf(const FName Package) {
		TArray<FAssetData> Assets;
		UPjcSubsystem::GetModuleAssetRegistry().Get().GetAssetsByPackageName(Package, Assets);
		return Assets.Num() > 0 ? PjcShim::GetAssetClassName(Assets[0]) : NAME_None;
	}
}	// namespace

EPjcLinkKind FPjcInspectorGraph::GetLinkKind(const FName From, const FName To) {
	TArray<FAssetDependency> Dependencies;
	IAssetRegistry& Registry = UPjcSubsystem::GetModuleAssetRegistry().Get();
	Registry.GetDependencies(FAssetIdentifier {From}, Dependencies, UE::AssetRegistry::EDependencyCategory::Package);
	for (const auto& Dependency : Dependencies) {
		if (Dependency.AssetId.PackageName == To) return PjcInspectorKindOf(Dependency);
	}

	// no reference of its own: through an actor the map saves in its own file
	TArray<FPjcInspectorLink> Links;
	int32 Through = 0;
	GetLinks(From, false, Links, Through);
	for (const FPjcInspectorLink& Link : Links) {
		if (Link.Package == To) return Link.Kind;
	}
	return EPjcLinkKind::Hard;
}

bool FPjcInspectorGraph::IsExternal(const FName Package) {
	return UPjcSubsystem::FolderIsExternal(FPackageName::GetLongPackagePath(Package.ToString()));
}

void FPjcInspectorGraph::GetLinks(const FName Package, const bool bUsedBy, TArray<FPjcInspectorLink>& Out, int32& OutThrough) {
	Out.Reset();
	OutThrough = 0;
	IAssetRegistry& Registry = UPjcSubsystem::GetModuleAssetRegistry().Get();
	TArray<FAssetDependency> Uses;
	TArray<FAssetDependency> UsedBy;
	Registry.GetDependencies(FAssetIdentifier {Package}, Uses, UE::AssetRegistry::EDependencyCategory::Package);
	Registry.GetReferencers(FAssetIdentifier {Package}, UsedBy, UE::AssetRegistry::EDependencyCategory::Package);

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
			const FName Actor = Link.AssetId.PackageName;
			if (!IsExternal(Actor)) continue;

			const FName Owner = PjcInspectorOwnerOf(Actor);
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
		const FName Actor = Link.AssetId.PackageName;
		if (!IsExternal(Actor)) continue;

		const FName Owner = PjcInspectorOwnerOf(Actor);
		Through.Add(Actor);
		if (!Owner.IsNone() && Owner != Package) {
			Add(Owner, PjcInspectorKindOf(Link), true);
			Through.Remove(Actor);
			++OutThrough;
		}
	}
	for (const FAssetDependency& Link : UsedBy) {
		const FName Actor = Link.AssetId.PackageName;
		if (IsExternal(Actor) && PjcInspectorOwnerOf(Actor) == Package) Through.Add(Actor);
	}
	OutThrough += Through.Num();
	for (const FName Actor : Through) {
		Far.Reset();
		Registry.GetDependencies(FAssetIdentifier {Actor}, Far, UE::AssetRegistry::EDependencyCategory::Package);
		for (const FAssetDependency& Used : Far) {
			Add(Used.AssetId.PackageName, PjcInspectorKindOf(Used), true);
		}
	}
}

int32 FPjcInspectorGraph::FindNode(const FName Package) const {
	return Nodes.IndexOfByPredicate([Package](const FPjcInspectorNode& Node) {
		return Node.Kind == EPjcInspectorNodeKind::Asset && Node.Package == Package;
	});
}

FPjcInspectorGraph
FPjcInspectorGraph::MakeGroup(const TArray<FName>& Group, const TArray<FName>& Classes, const TArray<FPjcGroupLink>& Links, const bool bEditorLinks) {
	FPjcInspectorGraph Graph;
	const int32 Num = Group.Num();
	if (Num == 0 || Classes.Num() != Num) return Graph;

	Graph.Focus = Group[0];
	Graph.bUnusedGroup = true;
	Graph.NumGroup = Num;

	// the picture is built from the references the game follows; editor-only ones are drawn on top and move nothing
	TArray<FPjcGroupLink> Game;
	TArray<FPjcGroupLink> Editor;
	for (const FPjcGroupLink& Link : Links) {
		if (!Group.IsValidIndex(Link.From) || !Group.IsValidIndex(Link.To) || Link.From == Link.To) continue;

		(Link.Kind == EPjcLinkKind::EditorOnly ? Editor : Game).Add(Link);
	}

	// shared: what many of the group use. A line from each user would cover the picture
	TArray<int32> NumUsers;
	NumUsers.SetNumZeroed(Num);
	for (const FPjcGroupLink& Link : Game) {
		++NumUsers[Link.To];
	}
	const auto IsHub = [&NumUsers](const int32 Index) {
		return Index != 0 && NumUsers[Index] >= HubAt;
	};

	// same class, same users, same uses: one stack
	TArray<TArray<int32>> Ins;
	TArray<TArray<int32>> Outs;
	Ins.SetNum(Num);
	Outs.SetNum(Num);
	for (const FPjcGroupLink& Link : Game) {
		if (IsHub(Link.From) || IsHub(Link.To)) continue;

		Outs[Link.From].AddUnique(Link.To);
		Ins[Link.To].AddUnique(Link.From);
	}
	const auto Joined = [](TArray<int32>& Indices) {
		Indices.Sort();
		FString Text;
		for (const int32 Index : Indices) {
			Text += FString::FromInt(Index) + TEXT(",");
		}
		return Text;
	};
	TArray<FString> Signatures;
	TMap<FString, TArray<int32>> BySignature;
	for (int32 Index = 0; Index < Num; ++Index) {
		if (IsHub(Index)) continue;

		const FString Signature
			= Index == 0 ? FString {TEXT("focus")} : Classes[Index].ToString() + TEXT("|") + Joined(Ins[Index]) + TEXT("|") + Joined(Outs[Index]);
		if (!BySignature.Contains(Signature)) Signatures.Add(Signature);
		BySignature.FindOrAdd(Signature).Add(Index);
	}
	TArray<int32> BoxOf;
	BoxOf.Init(INDEX_NONE, Num);
	for (const FString& Signature : Signatures) {
		TArray<int32>& Members = BySignature[Signature];
		Members.Sort([&Group](const int32 A, const int32 B) {
			return FPackageName::GetShortName(Group[A]) < FPackageName::GetShortName(Group[B]);
		});
		FPjcInspectorNode& Node = Graph.Nodes.AddDefaulted_GetRef();
		if (Members.Num() > 1) {
			Node.Kind = EPjcInspectorNodeKind::Stack;
			Node.GroupClass = Classes[Members[0]];
			for (const int32 Member : Members) {
				Node.Roots.Add(Group[Member]);
			}
		}
		else {
			Node.Package = Group[Members[0]];
		}
		for (const int32 Member : Members) {
			BoxOf[Member] = Graph.Nodes.Num() - 1;
		}
	}
	const int32 NumBoxes = Graph.Nodes.Num();
	for (int32 Index = 0; Index < Num; ++Index) {
		if (!IsHub(Index)) continue;

		FPjcInspectorNode& Node = Graph.Nodes.AddDefaulted_GetRef();
		Node.Package = Group[Index];
		Node.bHub = true;
		Node.NumUsers = NumUsers[Index];
		BoxOf[Index] = Graph.Nodes.Num() - 1;
	}
	for (const FPjcGroupLink& Link : Game) {
		if (IsHub(Link.To) && !IsHub(Link.From)) Graph.Nodes[BoxOf[Link.From]].Hubs.AddUnique(BoxOf[Link.To]);
	}

	// lines between the boxes
	TMap<uint64, int32> EdgeOf;
	const auto EdgeKey = [](const int32 From, const int32 To) {
		return (static_cast<uint64>(From) << 32) | static_cast<uint32>(To);
	};
	TArray<TArray<int32>> Preds;
	TArray<TArray<int32>> Succs;
	Preds.SetNum(NumBoxes);
	Succs.SetNum(NumBoxes);
	for (const FPjcGroupLink& Link : Game) {
		if (IsHub(Link.From) || IsHub(Link.To)) continue;

		const int32 From = BoxOf[Link.From];
		const int32 To = BoxOf[Link.To];
		if (From == To) continue;

		if (const int32* Existing = EdgeOf.Find(EdgeKey(From, To))) {
			FPjcInspectorEdge& Edge = Graph.Edges[*Existing];
			++Edge.NumLinks;
			if (Link.Kind == EPjcLinkKind::Hard) Edge.Kind = EPjcLinkKind::Hard;
			continue;
		}
		EdgeOf.Add(EdgeKey(From, To), Graph.Edges.Add(FPjcInspectorEdge {From, To, Link.Kind, 1}));
		Preds[To].Add(From);
		Succs[From].Add(To);
	}

	// row: the longest chain from the top. A reference that closes a circle does not count and goes back up
	TArray<int32> Layers;
	TArray<uint8> State;
	Layers.SetNumZeroed(NumBoxes);
	State.SetNumZeroed(NumBoxes);
	TFunction<int32(int32)> Depth = [&](const int32 Box) -> int32 {
		if (State[Box] == 2) return Layers[Box];

		State[Box] = 1;
		int32 Deepest = 0;
		for (const int32 Pred : Preds[Box]) {
			if (State[Pred] == 1) {
				Graph.Edges[EdgeOf[EdgeKey(Pred, Box)]].bCircle = true;
				continue;
			}
			Deepest = FMath::Max(Deepest, Depth(Pred) + 1);
		}
		State[Box] = 2;
		Layers[Box] = Deepest;
		return Deepest;
	};
	const int32 FocusBox = BoxOf[0];
	TArray<int32> Lone;
	TArray<TArray<int32>> Rows;
	for (int32 Box = 0; Box < NumBoxes; ++Box) {
		Depth(Box);
	}
	for (int32 Box = 0; Box < NumBoxes; ++Box) {
		if (Preds[Box].Num() == 0 && Succs[Box].Num() == 0 && Box != FocusBox) {
			Lone.Add(Box);
			continue;
		}
		if (!Rows.IsValidIndex(Layers[Box])) Rows.SetNum(Layers[Box] + 1);
		Rows[Layers[Box]].Add(Box);
	}
	const auto TitleOf = [&Graph](const int32 Box) {
		const FPjcInspectorNode& Node = Graph.Nodes[Box];
		return Node.Kind == EPjcInspectorNodeKind::Stack ? FString {TEXT("~")} : FPackageName::GetShortName(Node.Package);
	};
	for (TArray<int32>& Row : Rows) {
		Row.Sort([&TitleOf](const int32 A, const int32 B) {
			return TitleOf(A) < TitleOf(B);
		});
	}
	// order in the rows: next to what a box is linked with, a few sweeps down and up
	TArray<float> At;
	At.SetNumZeroed(NumBoxes);
	const auto Place = [&Rows, &At]() {
		for (const TArray<int32>& Row : Rows) {
			for (int32 Index = 0; Index < Row.Num(); ++Index) {
				At[Row[Index]] = Row.Num() > 1 ? static_cast<float>(Index) / (Row.Num() - 1) : 0.5f;
			}
		}
	};
	Place();
	for (int32 Sweep = 0; Sweep < 6; ++Sweep) {
		const bool bDown = Sweep % 2 == 0;
		for (int32 Step = 0; Step < Rows.Num(); ++Step) {
			TArray<int32>& Row = Rows[bDown ? Step : Rows.Num() - 1 - Step];
			TMap<int32, float> Middle;
			for (const int32 Box : Row) {
				const TArray<int32>& Linked = bDown ? Preds[Box] : Succs[Box];
				float Sum = 0.0f;
				for (const int32 Other : Linked) {
					Sum += At[Other];
				}
				Middle.Add(Box, Linked.Num() > 0 ? Sum / Linked.Num() : At[Box]);
			}
			Row.StableSort([&Middle](const int32 A, const int32 B) {
				return Middle[A] < Middle[B];
			});
			Place();
		}
	}
	int32 Widest = 4;
	for (const TArray<int32>& Row : Rows) {
		Widest = FMath::Max(Widest, Row.Num());
	}
	for (int32 Layer = 0; Layer < Rows.Num(); ++Layer) {
		for (int32 Index = 0; Index < Rows[Layer].Num(); ++Index) {
			FPjcInspectorNode& Node = Graph.Nodes[Rows[Layer][Index]];
			Node.Layer = Layer;
			Node.Column = Index + (Widest - Rows[Layer].Num()) * 0.5f;
		}
	}
	int32 Next = Rows.Num();
	if (Lone.Num() > 0) {
		Graph.Captions.Add(FPjcInspectorCaption {Next, TEXT("Linked only through the shared assets")});
		for (int32 Index = 0; Index < Lone.Num(); ++Index) {
			FPjcInspectorNode& Node = Graph.Nodes[Lone[Index]];
			Node.Layer = Next + Index / Widest;
			Node.Column = static_cast<float>(Index % Widest);
		}
		Next += (Lone.Num() + Widest - 1) / Widest;
	}
	if (Graph.Nodes.Num() > NumBoxes) {
		Graph.Captions.Add(FPjcInspectorCaption {Next, TEXT("Shared by many: a dot on every asset that uses it, no lines")});
		for (int32 Index = NumBoxes; Index < Graph.Nodes.Num(); ++Index) {
			Graph.Nodes[Index].Layer = Next;
			Graph.Nodes[Index].Column = (Index - NumBoxes) * 2.0f;
		}
	}

	// editor-only references on top. One that leads back to an asset that reaches it closes a circle
	if (bEditorLinks) {
		const auto Reaches = [&Succs](const int32 From, const int32 To) {
			TArray<int32> Queue {From};
			TSet<int32> Seen {From};
			while (Queue.Num() > 0) {
				const int32 Box = Queue.Pop();
				if (Box == To) return true;

				for (const int32 Other : Succs[Box]) {
					if (!Seen.Contains(Other)) {
						Seen.Add(Other);
						Queue.Add(Other);
					}
				}
			}
			return false;
		};
		for (const FPjcGroupLink& Link : Editor) {
			if (IsHub(Link.From) || IsHub(Link.To)) continue;

			const int32 From = BoxOf[Link.From];
			const int32 To = BoxOf[Link.To];
			if (From == To || EdgeOf.Contains(EdgeKey(From, To))) continue;

			FPjcInspectorEdge Edge {From, To, EPjcLinkKind::EditorOnly, 1};
			Edge.bCircle = Reaches(To, From);
			EdgeOf.Add(EdgeKey(From, To), Graph.Edges.Add(Edge));
		}
	}
	return Graph;
}

FPjcInspectorGraph FPjcInspectorGraph::Make(const FPjcScanResult& Result, const FName Focus, const TSet<FName>& OpenGroups, const bool bEditorLinks) {
	FPjcInspectorGraph Graph;
	Graph.Focus = Focus;

	const auto AddEdge = [&Graph](const int32 From, const int32 To, const EPjcLinkKind Kind) {
		if (From == INDEX_NONE || To == INDEX_NONE || From == To) return;

		FPjcInspectorEdge* Edge = Graph.Edges.FindByPredicate([From, To](const FPjcInspectorEdge& Existing) {
			return Existing.From == From && Existing.To == To;
		});
		if (Edge) {
			++Edge->NumLinks;
			return;
		}
		Graph.Edges.Add(FPjcInspectorEdge {From, To, Kind, 1});
	};

	// unused: the unused assets around it as one picture
	if (!Result.IsUsed(Focus) && Result.Usage.Contains(Focus)) {
		TArray<FName> Group {Focus};
		TMap<FName, int32> IndexOf;
		IndexOf.Add(Focus, 0);
		TArray<FPjcInspectorLink> Links;
		int32 Through = 0;
		for (int32 Head = 0; Head < Group.Num() && Group.Num() < MaxGroup; ++Head) {
			for (const bool bUsedBy : {true, false}) {
				GetLinks(Group[Head], bUsedBy, Links, Through);
				Links.Sort([](const FPjcInspectorLink& A, const FPjcInspectorLink& B) {
					return A.Package.LexicalLess(B.Package);
				});
				for (const FPjcInspectorLink& Link : Links) {
					const FPjcAssetUsage* Usage = Result.Usage.Find(Link.Package);
					if (!Usage || Usage->bUsed || IndexOf.Contains(Link.Package) || Group.Num() >= MaxGroup) continue;

					IndexOf.Add(Link.Package, Group.Num());
					Group.Add(Link.Package);
				}
			}
		}
		TArray<FName> Classes;
		TArray<FPjcGroupLink> GroupLinks;
		for (int32 From = 0; From < Group.Num(); ++From) {
			Classes.Add(PjcInspectorClassOf(Group[From]));
			GetLinks(Group[From], false, Links, Through);
			for (const FPjcInspectorLink& Link : Links) {
				const int32* To = IndexOf.Find(Link.Package);
				if (To && *To != From) GroupLinks.Add(FPjcGroupLink {From, *To, Link.Kind});
			}
		}
		return MakeGroup(Group, Classes, GroupLinks, bEditorLinks);
	}

	// used: root paths, roots of one class from GroupAt on as one group node
	TArray<FPjcRootPath> Paths;
	PjcInspectorRootPaths(Result, Focus, Paths);
	Graph.NumRoots = Paths.Num();
	for (const FPjcRootPath& Path : Paths) {
		Graph.Roots.Add(FPjcInspectorRoot {Path.Root, Path.Path.Num() - 1});
	}
	Graph.Roots.Sort([](const FPjcInspectorRoot& A, const FPjcInspectorRoot& B) {
		if (A.Steps != B.Steps) return A.Steps < B.Steps;
		return A.Package.LexicalLess(B.Package);
	});

	struct FItem
	{
		// Group class, None for one root
		FName Group;
		TArray<int32> Paths;
		int32 Length = 0;
	};
	TMap<FName, TArray<int32>> ByClass;
	for (int32 Index = 0; Index < Paths.Num(); ++Index) {
		ByClass.FindOrAdd(PjcInspectorClassOf(Paths[Index].Root)).Add(Index);
	}
	TArray<FItem> Items;
	for (const auto& Pair : ByClass) {
		if (Pair.Value.Num() >= GroupAt && !Pair.Key.IsNone() && !OpenGroups.Contains(Pair.Key)) {
			FItem& Item = Items.AddDefaulted_GetRef();
			Item.Group = Pair.Key;
			Item.Paths = Pair.Value;
			for (const int32 Index : Pair.Value) {
				Item.Length = FMath::Max(Item.Length, Paths[Index].Path.Num());
			}
			continue;
		}
		for (const int32 Index : Pair.Value) {
			FItem& Item = Items.AddDefaulted_GetRef();
			Item.Paths = {Index};
			Item.Length = Paths[Index].Path.Num();
		}
	}
	Items.Sort([&Paths](const FItem& A, const FItem& B) {
		if (A.Length != B.Length) return A.Length > B.Length;
		return Paths[A.Paths[0]].Root.LexicalLess(Paths[B.Paths[0]].Root);
	});
	Graph.NumHidden = FMath::Max(0, Items.Num() - MaxPaths);
	if (Items.Num() > MaxPaths) {
		Items.SetNum(MaxPaths);
	}

	// distance from the bottom: the longest path through a node decides its row
	TMap<FName, int32> Distance;
	TMap<FName, int32> Column;
	for (int32 ItemIndex = 0; ItemIndex < Items.Num(); ++ItemIndex) {
		const FItem& Item = Items[ItemIndex];
		for (const int32 PathIndex : Item.Paths) {
			const TArray<FName>& Path = Paths[PathIndex].Path;
			for (int32 Step = Item.Group.IsNone() ? 0 : 1; Step < Path.Num(); ++Step) {
				int32& Dist = Distance.FindOrAdd(Path[Step], -1);
				Dist = FMath::Max(Dist, Path.Num() - 1 - Step);
				Column.FindOrAdd(Path[Step], ItemIndex);
			}
		}
	}
	int32 MaxDistance = 0;
	for (const auto& Pair : Distance) {
		MaxDistance = FMath::Max(MaxDistance, Pair.Value);
	}
	for (const auto& Pair : Distance) {
		FPjcInspectorNode& Node = Graph.Nodes.AddDefaulted_GetRef();
		Node.Package = Pair.Key;
		Node.Layer = 2 + MaxDistance - Pair.Value;
		Node.Column = static_cast<float>(Column[Pair.Key]);
	}

	for (int32 ItemIndex = 0; ItemIndex < Items.Num(); ++ItemIndex) {
		const FItem& Item = Items[ItemIndex];
		int32 Top = INDEX_NONE;
		if (!Item.Group.IsNone()) {
			// the group stands where its roots would: one row above the asset they all reach first
			FPjcInspectorNode& Node = Graph.Nodes.AddDefaulted_GetRef();
			Node.Kind = EPjcInspectorNodeKind::Group;
			Node.GroupClass = Item.Group;
			int32 Layer = TNumericLimits<int32>::Max();
			for (const int32 PathIndex : Item.Paths) {
				Node.Roots.Add(Paths[PathIndex].Root);
				const TArray<FName>& Path = Paths[PathIndex].Path;
				if (Path.Num() > 1) {
					const int32 Next = Graph.FindNode(Path[1]);
					Layer = FMath::Min(Layer, Next != INDEX_NONE ? Graph.Nodes[Next].Layer - 1 : 1);
				}
			}
			Node.Layer = Layer == TNumericLimits<int32>::Max() ? 1 : Layer;
			Node.Column = static_cast<float>(ItemIndex);
			Top = Graph.Nodes.Num() - 1;
		}
		else {
			Top = Graph.FindNode(Paths[Item.Paths[0]].Root);
		}

		// the source above: why the root is used
		FPjcInspectorNode Source;
		Source.Kind = EPjcInspectorNodeKind::Source;
		Source.Layer = Graph.Nodes[Top].Layer - 1;
		Source.Column = static_cast<float>(ItemIndex);
		if (!Item.Group.IsNone()) {
			Source.Package = Item.Group;
			const FPjcAssetUsage* Usage = Result.Usage.Find(Paths[Item.Paths[0]].Root);
			Source.Label = Usage ? UPjcSubsystem::GetUsageReasonText(*Usage) : FString {};
		}
		else {
			Source.Package = Paths[Item.Paths[0]].Root;
			const FPjcAssetUsage* Usage = Result.Usage.Find(Source.Package);
			Source.Label = Usage ? UPjcSubsystem::GetUsageReasonText(*Usage) : FString {};
		}
		const int32 SourceIndex = Graph.Nodes.Add(Source);
		Graph.Edges.Add(FPjcInspectorEdge {SourceIndex, Top, EPjcLinkKind::Hard, 1});

		for (const int32 PathIndex : Item.Paths) {
			const TArray<FName>& Path = Paths[PathIndex].Path;
			for (int32 Step = 1; Step < Path.Num(); ++Step) {
				const int32 From = Step == 1 && !Item.Group.IsNone() ? Top : Graph.FindNode(Path[Step - 1]);
				AddEdge(From, Graph.FindNode(Path[Step]), GetLinkKind(Path[Step - 1], Path[Step]));
			}
		}
	}

	// a used asset without any root (should not happen) still shows itself
	if (Graph.FindNode(Focus) == INDEX_NONE) {
		FPjcInspectorNode& Node = Graph.Nodes.AddDefaulted_GetRef();
		Node.Package = Focus;
	}
	return Graph;
}
