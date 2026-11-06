// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcInspector.h"
#include "PjcShim.h"
#include "PjcSubsystem.h"
// Engine Headers
#include "Misc/PackageName.h"

namespace
{
	// unity builds join files: names unique to this file

	/** Root path: root first, the focus last */
	struct FPjcRootPath
	{
		FName Root;
		TArray<FName> Path;
	};

	bool PjcInspectorIsRoot(const FPjcScanResult& Result, const FName Package) {
		const FPjcAssetUsage* Usage = Result.Usage.Find(Package);
		return Usage && Usage->bUsed && Usage->Parent.IsNone();
	}

	/** Every root that keeps the package used, with the shortest path from it, breadth first over used referencers */
	void PjcInspectorRootPaths(const FPjcScanResult& Result, const FName Focus, TArray<FPjcRootPath>& OutPaths) {
		IAssetRegistry& AssetRegistry = UPjcSubsystem::GetModuleAssetRegistry().Get();
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
			AssetRegistry.GetReferencers(Current, Referencers);
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
	UPjcSubsystem::GetModuleAssetRegistry().Get().GetDependencies(
		FAssetIdentifier {From}, Dependencies, UE::AssetRegistry::EDependencyCategory::Package
	);
	for (const auto& Dependency : Dependencies) {
		if (Dependency.AssetId.PackageName != To) continue;

		if (!EnumHasAnyFlags(Dependency.Properties, UE::AssetRegistry::EDependencyProperty::Game)) return EPjcLinkKind::EditorOnly;
		return EnumHasAnyFlags(Dependency.Properties, UE::AssetRegistry::EDependencyProperty::Hard) ? EPjcLinkKind::Hard : EPjcLinkKind::Soft;
	}
	return EPjcLinkKind::Hard;
}

int32 FPjcInspectorGraph::FindNode(const FName Package) const {
	return Nodes.IndexOfByPredicate([Package](const FPjcInspectorNode& Node) {
		return Node.Kind == EPjcInspectorNodeKind::Asset && Node.Package == Package;
	});
}

FPjcInspectorGraph FPjcInspectorGraph::Make(const FPjcScanResult& Result, const FName Focus, const TSet<FName>& OpenGroups) {
	FPjcInspectorGraph Graph;
	Graph.Focus = Focus;
	IAssetRegistry& AssetRegistry = UPjcSubsystem::GetModuleAssetRegistry().Get();

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

	// unused: the unused assets around it, top row what nothing of them references, then down along the references
	if (!Result.IsUsed(Focus) && Result.Usage.Contains(Focus)) {
		Graph.bUnusedGroup = true;
		TArray<FName> Group {Focus};
		TSet<FName> Seen {Focus};
		TArray<FName> Names;
		for (int32 Head = 0; Head < Group.Num() && Group.Num() < 200; ++Head) {
			Names.Reset();
			AssetRegistry.GetReferencers(Group[Head], Names);
			AssetRegistry.GetDependencies(Group[Head], Names);
			Names.Sort(FNameLexicalLess {});
			for (const FName Name : Names) {
				const FPjcAssetUsage* Usage = Result.Usage.Find(Name);
				if (!Usage || Usage->bUsed || Seen.Contains(Name)) continue;

				Seen.Add(Name);
				Group.Add(Name);
			}
		}
		if (Group.Num() > MaxUnused) {
			Group.SetNum(MaxUnused);
		}
		const TSet<FName> InGroup {Group};

		// layer: longest reference chain down from a top asset
		TMap<FName, TArray<FName>> Uses;
		for (const FName Name : Group) {
			Names.Reset();
			AssetRegistry.GetDependencies(Name, Names);
			TArray<FName>& Inside = Uses.Add(Name);
			for (const FName Dependency : Names) {
				if (InGroup.Contains(Dependency) && Dependency != Name) Inside.Add(Dependency);
			}
		}
		TMap<FName, int32> Layers;
		for (const FName Name : Group) {
			const bool bReferenced = Group.ContainsByPredicate([&Uses, Name](const FName Other) {
				return Uses[Other].Contains(Name);
			});
			if (!bReferenced) Layers.Add(Name, 0);
		}
		if (Layers.Num() == 0) Layers.Add(Focus, 0);
		for (int32 Pass = 0; Pass < Group.Num(); ++Pass) {
			for (const FName Name : Group) {
				const int32* Layer = Layers.Find(Name);
				if (!Layer) continue;

				const int32 Below = *Layer + 1;
				for (const FName Dependency : Uses[Name]) {
					const int32* Current = Layers.Find(Dependency);
					if ((!Current || *Current < Below) && Below < Group.Num()) Layers.Add(Dependency, Below);
				}
			}
		}

		Group.Sort([](const FName A, const FName B) {
			return FPackageName::GetShortName(A) < FPackageName::GetShortName(B);
		});
		TMap<int32, int32> Columns;
		for (const FName Name : Group) {
			FPjcInspectorNode& Node = Graph.Nodes.AddDefaulted_GetRef();
			Node.Package = Name;
			Node.Layer = Layers.Contains(Name) ? Layers[Name] : 0;
			Node.Column = Columns.FindOrAdd(Node.Layer)++;
		}
		for (const FName Name : Group) {
			for (const FName Dependency : Uses[Name]) {
				AddEdge(Graph.FindNode(Name), Graph.FindNode(Dependency), GetLinkKind(Name, Dependency));
			}
		}
		return Graph;
	}

	// used: root paths, roots of one class from GroupAt on as one group node
	TArray<FPjcRootPath> Paths;
	PjcInspectorRootPaths(Result, Focus, Paths);
	Graph.NumRoots = Paths.Num();

	struct FItem
	{
		/** Group class, None for one root */
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
		Node.Column = Column[Pair.Key];
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
			Node.Column = ItemIndex;
			Top = Graph.Nodes.Num() - 1;
		}
		else {
			Top = Graph.FindNode(Paths[Item.Paths[0]].Root);
		}

		// the source above: why the root is used
		FPjcInspectorNode Source;
		Source.Kind = EPjcInspectorNodeKind::Source;
		Source.Layer = Graph.Nodes[Top].Layer - 1;
		Source.Column = ItemIndex;
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
