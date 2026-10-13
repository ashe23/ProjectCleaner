// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcTabAssetsUsage.h"
#include "Slate/SPjcAssetConnections.h"
#include "Slate/SPjcAssetGraph.h"
#include "Slate/SPjcItemUsage.h"
#include "PjcAssetGraph.h"
#include "PjcCmds.h"
#include "PjcSettings.h"
#include "PjcShim.h"
#include "PjcStyles.h"
#include "PjcSubsystem.h"
#include "PjcTypes.h"
// Engine Headers
#include "IContentBrowserSingleton.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Misc/PackageName.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	/** "in memory by AnimSequencerController /Engine/Transient.X" -> "held in memory by AnimSequencerController" */
	FString GetShortHolder(const FString& Holder) {
		const FString Prefix = TEXT("in memory by ");
		if (!Holder.StartsWith(Prefix)) return Holder;

		FString ClassName = Holder.RightChop(Prefix.Len());
		int32 SpaceIndex = INDEX_NONE;
		if (ClassName.FindChar(TEXT(' '), SpaceIndex)) {
			ClassName.LeftInline(SpaceIndex);
		}

		return TEXT("held in memory by ") + ClassName;
	}

	/** Engine helpers created for animations loaded in the editor, they keep animation assets from being deleted */
	bool IsAnimationHelper(const FString& Holder) {
		return Holder.Contains(TEXT("AnimDataController")) || Holder.Contains(TEXT("AnimSequencerController"));
	}
}	// namespace

void SPjcTabAssetsUsage::Construct(const FArguments& InArgs) {
	Cmds = MakeShareable(new FUICommandList);
	Cmds->MapAction(FPjcCmds::Get().ScanProject, FExecuteAction::CreateRaw(this, &SPjcTabAssetsUsage::Scan));

	SAssignNew(ListView, SListView<TSharedPtr<FPjcUsageItem>>)
		.ListItemsSource(&ItemsFiltered)
		.SelectionMode(ESelectionMode::Single)
		.OnGenerateRow(this, &SPjcTabAssetsUsage::OnGenerateRow)
		.OnSelectionChanged(this, &SPjcTabAssetsUsage::OnListSelectionChanged)
		.HeaderRow(GetHeaderRow(true));

	SAssignNew(ListUsedBy, SListView<TSharedPtr<FPjcUsageItem>>)
		.ListItemsSource(&ItemsUsedBy)
		.SelectionMode(ESelectionMode::Single)
		.OnGenerateRow(this, &SPjcTabAssetsUsage::OnGenerateRow)
		.OnMouseButtonDoubleClick(this, &SPjcTabAssetsUsage::OnLinkDoubleClicked)
		.HeaderRow(GetHeaderRow(false));

	SAssignNew(ListUses, SListView<TSharedPtr<FPjcUsageItem>>)
		.ListItemsSource(&ItemsUses)
		.SelectionMode(ESelectionMode::Single)
		.OnGenerateRow(this, &SPjcTabAssetsUsage::OnGenerateRow)
		.OnMouseButtonDoubleClick(this, &SPjcTabAssetsUsage::OnLinkDoubleClicked)
		.HeaderRow(GetHeaderRow(false));

	SAssignNew(ChainBox, SVerticalBox);

	SAssignNew(GraphView, SPjcAssetGraph)
		.OnSelectionChanged_Raw(this, &SPjcTabAssetsUsage::OnGraphSelectionChanged)
		.OnAssetDoubleClicked_Raw(this, &SPjcTabAssetsUsage::OnGraphAssetDoubleClicked);

	SAssignNew(ConnectionsView, SPjcAssetConnections)
		.OnInspect_Lambda([this](const FName& PackageName) {
			Inspect(PackageName);
		})
		.OnOpen_Raw(this, &SPjcTabAssetsUsage::OnGraphAssetDoubleClicked)
		.OnBrowse_Lambda([this](const FName&) {
			OnShowInContentBrowser();
		})
		.OnShowInMap_Lambda([this](const FName& PackageName) {
			View = EView::Map;
			GraphView->SetSelection(PackageName, true);
		});

	// clang-format off
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				CreateToolbar()
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin{10.0f, 0.0f})
			[
				CreateViewToggle()
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			.Padding(FMargin{10.0f, 0.0f})
			[
				SNew(STextBlock)
				.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Gray"))
				.Text_Raw(this, &SPjcTabAssetsUsage::GetSummaryText)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(300.0f)
				[
					SNew(SSearchBox)
					.HintText(FText::FromString(TEXT("Search name, path, class or reason...")))
					.OnTextChanged_Raw(this, &SPjcTabAssetsUsage::OnSearchTextChanged)
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin{5.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(SComboButton)
				.OnGetMenuContent_Raw(this, &SPjcTabAssetsUsage::CreateFilterMenu)
				.ButtonContent()
				[
					SNew(STextBlock).Text_Raw(this, &SPjcTabAssetsUsage::GetFilterText)
				]
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SSplitter)
			.Orientation(Orient_Horizontal)
			+ SSplitter::Slot()
			.Value(0.62f)
			[
				SNew(SWidgetSwitcher)
				.WidgetIndex_Lambda([this]() { return static_cast<int32>(View); })
				+ SWidgetSwitcher::Slot()
				[
					GraphView.ToSharedRef()
				]
				+ SWidgetSwitcher::Slot()
				[
					ConnectionsView.ToSharedRef()
				]
				+ SWidgetSwitcher::Slot()
				.Padding(5.0f)
				[
					ListView.ToSharedRef()
				]
			]
			+ SSplitter::Slot()
			.Value(0.38f)
			[
				SNew(SBox)
				.Visibility_Lambda([this]() { return Current.IsValid() && View != EView::Connections ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					CreateInspector()
				]
			]
		]
	];
	// clang-format on

	if (GetDefault<UPjcEditorSettings>()->bScanOnOpen) {
		Scan();
	}
}

void SPjcTabAssetsUsage::Inspect(const FName& PackageName, const bool bRecordHistory) {
	if (!bScanned) {
		Scan();
	}

	const TSharedPtr<FPjcUsageItem> Item = FindOrMakeItem(PackageName);
	if (!Item.IsValid()) return;

	if (bRecordHistory && Current.IsValid() && Current->PackageName != PackageName) {
		HistoryBack.Add(Current->PackageName);
		HistoryForward.Reset();
	}
	Current = Item;

	// what it is used by and what it uses, native class packages left out
	TArray<FName> Referencers;
	TArray<FName> Dependencies;
	UPjcSubsystem::GetModuleAssetRegistry().Get().GetReferencers(PackageName, Referencers);
	UPjcSubsystem::GetModuleAssetRegistry().Get().GetDependencies(PackageName, Dependencies);

	const auto FillLinks = [this](const TArray<FName>& Packages, TArray<TSharedPtr<FPjcUsageItem>>& OutItems) {
		OutItems.Reset(Packages.Num());
		for (const auto& Package : Packages) {
			if (Package.ToString().StartsWith(TEXT("/Script/"))) continue;

			const TSharedPtr<FPjcUsageItem> LinkItem = FindOrMakeItem(Package);
			if (LinkItem.IsValid()) {
				OutItems.AddUnique(LinkItem);
			}
		}
		OutItems.Sort([](const TSharedPtr<FPjcUsageItem>& A, const TSharedPtr<FPjcUsageItem>& B) {
			const int32 OrderA = SPjcItemUsage::GetStatusOrder(*A);
			const int32 OrderB = SPjcItemUsage::GetStatusOrder(*B);
			return OrderA != OrderB ? OrderA < OrderB : SPjcItemUsage::GetItemName(*A) < SPjcItemUsage::GetItemName(*B);
		});
	};
	FillLinks(Referencers, ItemsUsedBy);
	FillLinks(Dependencies, ItemsUses);

	// loaded unused asset: what holds it now decides if cleanup can delete it
	CurrentHolder = Item->bScanned && !Item->Usage.bUsed && Item->Asset.IsValid() ? UPjcSubsystem::GetAssetHolder(Item->Asset) : FString {};
	CurrentWhy = BuildWhyText(*Item);
	RebuildChain();
	UpdateConnections();

	ListUsedBy->RequestListRefresh();
	ListUses->RequestListRefresh();

	// asset list and graph follow inspected asset, without inspecting it again. Graph moves to it unless it was clicked there
	TGuardValue<bool> SyncGuard {bSyncingSelection, true};
	if (ItemsFiltered.Contains(Item)) {
		ListView->SetSelection(Item, ESelectInfo::Direct);
		ListView->RequestScrollIntoView(Item);
	}
	else {
		ListView->ClearSelection();
	}
	GraphView->SetSelection(PackageName, !bSelectingInGraph);
}

void SPjcTabAssetsUsage::Scan() {
	if (UPjcSubsystem::GetModuleAssetRegistry().Get().IsLoadingAssets()) {
		UPjcSubsystem::ShowNotificationWithOutputLog(
			TEXT("Failed to scan project. AssetRegistry is still discovering assets. Please try again after it has finished."),
			SNotificationItem::CS_Fail,
			5.0f
		);
		return;
	}

	const FName Inspected = Current.IsValid() ? Current->PackageName : NAME_None;

	TArray<FAssetData> AssetsAll;
	TMap<FName, FPjcAssetUsage> Usage;
	{
		FPjcScanCacheScope ScanCacheScope;
		UPjcSubsystem::GetAssetsAll(AssetsAll);
		UPjcSubsystem::GetAssetsUsage(Usage);
	}

	const TMap<FName, FString>& Kept = UPjcSubsystem::GetLastCleanupKept();

	ItemsAll.Reset(AssetsAll.Num());
	ItemsByPackage.Reset();
	NumUsed = 0;
	NumUnused = 0;
	NumNotDeleted = 0;
	SizeUsed = 0;
	SizeUnused = 0;

	for (const auto& Asset : AssetsAll) {
		const TSharedPtr<FPjcUsageItem> Item = MakeShared<FPjcUsageItem>();
		Item->PackageName = Asset.PackageName;
		Item->Asset = Asset;
		Item->Usage = Usage.FindRef(Asset.PackageName);
		Item->ClassName = FPackageName::ObjectPathToObjectName(PjcShim::GetAssetClassName(Asset).ToString());
		Item->Size = UPjcSubsystem::GetAssetSize(Asset);
		Item->bScanned = true;

		if (Item->Usage.bUsed) {
			++NumUsed;
			SizeUsed += Item->Size;
		}
		else {
			++NumUnused;
			SizeUnused += Item->Size;
			if (const FString* KeptBy = Kept.Find(Asset.PackageName)) {
				Item->Kept = *KeptBy;
				++NumNotDeleted;
			}
		}

		ItemsAll.Emplace(Item);
		ItemsByPackage.Emplace(Asset.PackageName, Item);
	}

	// reasons name parent assets, so all items must exist first
	for (const auto& Item : ItemsAll) {
		Item->ReasonText = GetShortReason(*Item);
	}

	Graph = MakeShared<FPjcAssetGraph>();
	Graph->Build(ItemsAll);
	GraphView->SetGraph(Graph);

	bScanned = true;
	Current.Reset();
	ListUpdateView();

	if (!Inspected.IsNone()) {
		Inspect(Inspected, false);
	}
}

void SPjcTabAssetsUsage::ListUpdateView() {
	ItemsFiltered.Reset(ItemsAll.Num());

	const FString Search = SearchText.ToString();
	for (const auto& Item : ItemsAll) {
		const bool bNotDeleted = !Item->Usage.bUsed && !Item->Kept.IsEmpty();
		if (Filter == EFilter::Used && !Item->Usage.bUsed) continue;
		if (Filter == EFilter::Unused && Item->Usage.bUsed) continue;
		if (Filter == EFilter::NotDeleted && !bNotDeleted) continue;

		// clang-format off
		if (
			!Search.IsEmpty() &&
			!Item->PackageName.ToString().Contains(Search) &&
			!Item->ClassName.Contains(Search) &&
			!Item->ReasonText.Contains(Search)
		)
		{
			continue;
		}
		// clang-format on

		ItemsFiltered.Emplace(Item);
	}

	ListSort();
	ListView->RequestListRefresh();

	// graph shows all assets, ones that pass search and filter stand out
	TSet<FName> Emphasized;
	Emphasized.Reserve(ItemsFiltered.Num());
	for (const auto& Item : ItemsFiltered) {
		Emphasized.Add(Item->PackageName);
	}
	GraphView->SetEmphasis(Emphasized, Filter != EFilter::All || !Search.IsEmpty());
}

void SPjcTabAssetsUsage::ListSort() {
	const bool bAscending = SortMode != EColumnSortMode::Descending;
	const FName Column = SortColumn;

	ItemsFiltered.Sort([bAscending, Column](const TSharedPtr<FPjcUsageItem>& A, const TSharedPtr<FPjcUsageItem>& B) {
		const FString NameA = SPjcItemUsage::GetItemName(*A);
		const FString NameB = SPjcItemUsage::GetItemName(*B);

		int32 Result = 0;
		if (Column.IsEqual(TEXT("Class"))) {
			Result = A->ClassName.Compare(B->ClassName);
		}
		else if (Column.IsEqual(TEXT("Status"))) {
			Result = SPjcItemUsage::GetStatusOrder(*A) - SPjcItemUsage::GetStatusOrder(*B);
		}
		else if (Column.IsEqual(TEXT("Why"))) {
			Result = A->ReasonText.Compare(B->ReasonText);
		}

		if (Result == 0) {
			Result = NameA.Compare(NameB);
		}

		return bAscending ? Result < 0 : Result > 0;
	});
}

void SPjcTabAssetsUsage::RebuildChain() {
	ChainBox->ClearChildren();
	ItemsChain.Reset();
	if (!Current.IsValid() || !Current->bScanned || Current->Usage.Parent.IsNone()) return;

	// from asset used for its own reason down to inspected one
	TArray<TSharedPtr<FPjcUsageItem>>& Chain = ItemsChain;
	TSet<FName> Visited;
	TSharedPtr<FPjcUsageItem> Step = Current;
	while (Step.IsValid() && !Visited.Contains(Step->PackageName) && Chain.Num() < 64) {
		Visited.Add(Step->PackageName);
		Chain.Insert(Step, 0);

		if (Step->Usage.Parent.IsNone()) break;
		Step = FindOrMakeItem(Step->Usage.Parent);
	}

	for (int32 Index = 0; Index < Chain.Num(); ++Index) {
		const TSharedPtr<FPjcUsageItem> ChainItem = Chain[Index];
		const FName ChainPackage = ChainItem->PackageName;
		const bool bInspected = ChainItem == Current;

		// clang-format off
		ChainBox->AddSlot()
		.AutoHeight()
		.Padding(FMargin{Index * 14.0f, 1.0f, 0.0f, 1.0f})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin{0.0f, 0.0f, 5.0f, 0.0f})
			[
				SNew(STextBlock)
				.Text(FText::FromString(Index == 0 ? TEXT("●") : TEXT("└")))
				.ColorAndOpacity(SPjcItemUsage::GetStatusColor(*ChainItem))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SWidgetSwitcher)
				.WidgetIndex(bInspected ? 1 : 0)
				+ SWidgetSwitcher::Slot()
				[
					SNew(SHyperlink)
					.Text(FText::FromString(SPjcItemUsage::GetItemName(*ChainItem)))
					.ToolTipText(FText::FromName(ChainPackage))
					.OnNavigate_Lambda([this, ChainPackage]() { Inspect(ChainPackage); })
				]
				+ SWidgetSwitcher::Slot()
				[
					SNew(STextBlock)
					.Text(FText::FromString(SPjcItemUsage::GetItemName(*ChainItem)))
					.Font(FPjcStyles::GetFont("Bold", 10))
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			.Padding(FMargin{8.0f, 0.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock)
				.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Gray"))
				.Text(FText::FromString(Index == 0 ? ChainItem->ClassName + TEXT(", ") + ChainItem->ReasonText : ChainItem->ClassName))
			]
		];
		// clang-format on
	}
}

void SPjcTabAssetsUsage::ClearInspected() {
	if (Current.IsValid()) {
		HistoryBack.Add(Current->PackageName);
		HistoryForward.Reset();
	}

	Current.Reset();
	CurrentHolder.Reset();
	CurrentWhy = FText::GetEmpty();
	ItemsUsedBy.Reset();
	ItemsUses.Reset();
	ItemsChain.Reset();
	ChainBox->ClearChildren();
	ListUsedBy->RequestListRefresh();
	ListUses->RequestListRefresh();
	UpdateConnections();

	TGuardValue<bool> SyncGuard {bSyncingSelection, true};
	ListView->ClearSelection();
	GraphView->SetSelection(NAME_None, false);
}

void SPjcTabAssetsUsage::UpdateConnections() {
	// what only it uses, deleting or keeping it decides about them too
	FString Owned;
	const int32 Node = Current.IsValid() && Graph.IsValid() ? Graph->Find(Current->PackageName) : INDEX_NONE;
	if (Node != INDEX_NONE && Graph->GetNodes()[Node].NumOwned > 0) {
		const FPjcGraphNode& GraphNode = Graph->GetNodes()[Node];
		Owned = FString::Printf(
			TEXT("Only it uses %d asset(s), %s with them."), GraphNode.NumOwned, *FText::AsMemory(GraphNode.SizeOwned, IEC).ToString()
		);
	}

	ConnectionsView->SetAsset(Current, ItemsChain, ItemsUsedBy, ItemsUses, CurrentWhy, Owned);
}

TSharedPtr<FPjcUsageItem> SPjcTabAssetsUsage::FindOrMakeItem(const FName& PackageName) {
	if (const TSharedPtr<FPjcUsageItem>* Found = ItemsByPackage.Find(PackageName)) {
		return *Found;
	}

	// package outside scan (engine, not scanned plugin, actor saved in its own file), shown in links and chains only
	const TSharedPtr<FPjcUsageItem> Item = MakeShared<FPjcUsageItem>();
	Item->PackageName = PackageName;

	TArray<FAssetData> Assets;
	UPjcSubsystem::GetModuleAssetRegistry().Get().GetAssetsByPackageName(PackageName, Assets);
	for (const auto& Asset : Assets) {
		if (!Item->Asset.IsValid() || Asset.IsUAsset()) {
			Item->Asset = Asset;
		}
	}
	if (Item->Asset.IsValid()) {
		Item->ClassName = FPackageName::ObjectPathToObjectName(PjcShim::GetAssetClassName(Item->Asset).ToString());
	}
	Item->ReasonText = GetShortReason(*Item);

	ItemsByPackage.Emplace(PackageName, Item);
	return Item;
}

FString SPjcTabAssetsUsage::GetShortReason(const FPjcUsageItem& Item) {
	if (!Item.bScanned) {
		const FString Package = Item.PackageName.ToString();
		if (UPjcSubsystem::FolderIsExternal(Package)) return TEXT("Part of a level (actor or object saved in its own file)");
		if (UPjcSubsystem::PathIsInsideProject(Package)) return TEXT("Not scanned");

		return TEXT("Outside scanned content (engine or plugin)");
	}

	if (Item.Usage.bUsed) return UPjcSubsystem::GetUsageReasonText(Item.Usage);
	if (!Item.Kept.IsEmpty()) return TEXT("Cleanup could not delete it, ") + GetShortHolder(Item.Kept);
	if (Item.Usage.bFolderLabelOnly) return TEXT("Only folder label ") + GetFolderLabelName(Item) + TEXT(" keeps it");

	return TEXT("Nothing used references it");
}

FString SPjcTabAssetsUsage::GetFolderLabelName(const FPjcUsageItem& Item) {
	// walk up to the label that keeps the chain
	FPjcAssetUsage Usage = Item.Usage;
	for (int32 Step = 0; Step < 64 && !Usage.Parent.IsNone(); ++Step) {
		if (Usage.Reason == EPjcUsageReason::FolderLabel) {
			return FPackageName::GetShortName(Usage.Parent);
		}

		const TSharedPtr<FPjcUsageItem>* Parent = ItemsByPackage.Find(Usage.Parent);
		if (!Parent) break;
		Usage = (*Parent)->Usage;
	}

	return TEXT("(folder label)");
}

FText SPjcTabAssetsUsage::BuildWhyText(const FPjcUsageItem& Item) {
	TArray<FString> Paragraphs;
	const FPjcAssetUsage& Usage = Item.Usage;
	const FString ParentName = Usage.Parent.IsNone() ? FString {} : FPackageName::GetShortName(Usage.Parent);

	if (!Item.bScanned) {
		Paragraphs.Add(Item.ReasonText + TEXT(". Project Cleaner does not scan it and never deletes it."));
		Paragraphs.Add(TEXT("Its references still count: assets it uses are used."));
		return FText::FromString(FString::Join(Paragraphs, TEXT("\n\n")));
	}

	if (Usage.bUsed) {
		switch (Usage.Reason) {
			case EPjcUsageReason::Primary :
				Paragraphs.Add(
					FString::Printf(
						TEXT("Used: it is a primary asset (%s). ")
							TEXT("Asset Manager cooks primary assets like maps into the game, so everything they use is used too."),
						*Usage.Detail
					)
				);
				break;
			case EPjcUsageReason::EngineReferenced :
				Paragraphs.Add(
					FString::Printf(
						TEXT("Used: %s references it. That package is outside scanned content ")
							TEXT("(engine, a plugin that is not scanned or an actor saved in its own file), so its references always count."),
						*Usage.Detail
					)
				);
				break;
			case EPjcUsageReason::Indirect :
				Paragraphs.Add(
					FString::Printf(
						TEXT("Used: it is named in %s. Code or config can load assets by name, so it is kept."),
						Usage.Detail.IsEmpty() ? TEXT("project source or config files") : *Usage.Detail
					)
				);
				break;
			case EPjcUsageReason::Excluded :
				Paragraphs.Add(
					FString::Printf(TEXT("Used: %s. Exclusions are managed in the Assets Unused tab and in Project Cleaner settings."), *Usage.Detail)
				);
				break;
			case EPjcUsageReason::Editor : Paragraphs.Add(TEXT("Used: it is an editor utility asset (setting Editor Assets Used).")); break;
			case EPjcUsageReason::GameplayCue :
				Paragraphs.Add(
					FString::Printf(
						TEXT("Used: it is GameplayCue notify %s. ") TEXT("GameplayCue manager loads notifies by their tag, nothing references them."),
						*Usage.Detail
					)
				);
				break;
			case EPjcUsageReason::Megascans :		 Paragraphs.Add(TEXT("Used: it is a Megascans preset.")); break;
			case EPjcUsageReason::FolderLabelAsset : Paragraphs.Add(FString::Printf(TEXT("Used: folder label %s lists it."), *ParentName)); break;
			case EPjcUsageReason::FolderLabel :
				Paragraphs.Add(
					FString::Printf(
						TEXT("Used: folder label %s has \"Label Assets In My Directory\" on, ")
							TEXT("which marks every asset in its folder for cooking. ")
								TEXT("Turn off Folder Labels Used (Project Settings > Plugins > Project Cleaner) to see such assets as unused."),
						*ParentName
					)
				);
				break;
			case EPjcUsageReason::Dependency :
			default :
				Paragraphs.Add(
					FString::Printf(
						TEXT("Used: %s uses it%s, and %s is used. ")
							TEXT("The chain below shows how it connects to an asset that is used for its own reason."),
						*ParentName,
						Usage.Detail.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(" (%s)"), *Usage.Detail),
						*ParentName
					)
				);
				break;
		}

		if (Usage.bFolderLabelOnly && Usage.Reason != EPjcUsageReason::FolderLabel) {
			Paragraphs.Add(FString::Printf(TEXT("Only folder label %s keeps this chain used."), *GetFolderLabelName(Item)));
		}

		return FText::FromString(FString::Join(Paragraphs, TEXT("\n\n")));
	}

	if (Usage.bFolderLabelOnly) {
		Paragraphs.Add(
			FString::Printf(
				TEXT("Unused: only folder label %s keeps it, and setting Folder Labels Used is off. Clean Project deletes it."),
				*GetFolderLabelName(Item)
			)
		);
	}
	else {
		Paragraphs.Add(TEXT("Unused: no used asset references it, so Clean Project deletes it."));
	}

	int32 NumUnusedReferencers = 0;
	for (const auto& Referencer : ItemsUsedBy) {
		NumUnusedReferencers += Referencer->bScanned && !Referencer->Usage.bUsed ? 1 : 0;
	}
	if (NumUnusedReferencers > 0) {
		Paragraphs.Add(
			FString::Printf(TEXT("%d unused asset(s) use it (see Used by). They are unused too, so they are deleted together."), NumUnusedReferencers)
		);
	}

	if (!Item.Kept.IsEmpty()) {
		Paragraphs.Add(FString::Printf(TEXT("Last cleanup could not delete it: %s. Project Cleaner never force deletes."), *Item.Kept));
	}

	if (!CurrentHolder.IsEmpty()) {
		Paragraphs.Add(FString::Printf(TEXT("Right now it is %s, deleting it would be refused."), *GetShortHolder(CurrentHolder)));
		Paragraphs.Add(
			IsAnimationHelper(CurrentHolder)
				? TEXT("The engine creates this helper when it loads an animation. ")
					  TEXT("Clean Project releases it before deleting, so it blocks only while an animation editor uses it.")
				: TEXT("Close editors or levels that use it, or restart the editor, then clean again.")
		);
	}
	else if (!Item.Kept.IsEmpty()) {
		Paragraphs.Add(TEXT("Nothing holds it in memory now, Clean Project can try again."));
	}

	return FText::FromString(FString::Join(Paragraphs, TEXT("\n\n")));
}

TSharedRef<SWidget> SPjcTabAssetsUsage::CreateToolbar() const {
	FToolBarBuilder ToolBarBuilder {Cmds, FMultiBoxCustomization::None};
	ToolBarBuilder.BeginSection(NAME_None);
	{
		ToolBarBuilder.AddToolBarButton(
			FPjcCmds::Get().ScanProject,
			NAME_None,
			FText::FromString(TEXT("Scan")),
			FText::FromString(TEXT("Scan project and find out why each asset is used or unused"))
		);
	}
	ToolBarBuilder.EndSection();

	return ToolBarBuilder.MakeWidget();
}

TSharedRef<SWidget> SPjcTabAssetsUsage::CreateViewToggle() {
	const auto MakeButton = [this](const TCHAR* InLabel, const TCHAR* InToolTip, const EView InView) {
		return SNew(SCheckBox)
			.Style(&PjcShim::GetStyle().GetWidgetStyle<FCheckBoxStyle>("ToggleButtonCheckbox"))
			.ToolTipText(FText::FromString(InToolTip))
			.IsChecked_Lambda([this, InView]() {
				return View == InView ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
			.OnCheckStateChanged_Lambda([this, InView](ECheckBoxState) {
				View = InView;
			})[SNew(SBox).Padding(FMargin {10.0f, 2.0f})[SNew(STextBlock).Text(FText::FromString(InLabel))]];
	};

	// clang-format off
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			MakeButton(TEXT("Map"), TEXT("Project folders as map: circle area is size on disk, red part of folder ring is unused. Scroll to open folders"), EView::Map)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			MakeButton(TEXT("Connections"), TEXT("Selected asset with its usage chain, assets it is used by and assets it uses"), EView::Connections)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			MakeButton(TEXT("List"), TEXT("Assets as sortable list"), EView::List)
		];
	// clang-format on
}

TSharedRef<SWidget> SPjcTabAssetsUsage::CreateFilterMenu() {
	FMenuBuilder MenuBuilder {true, nullptr};

	const auto AddFilter = [&](const EFilter InFilter, const TCHAR* InLabel, const TCHAR* InToolTip) {
		MenuBuilder.AddMenuEntry(
			FText::FromString(InLabel),
			FText::FromString(InToolTip),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, InFilter]() {
					Filter = InFilter;
					ListUpdateView();
				}),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, InFilter]() {
					return Filter == InFilter;
				})
			),
			NAME_None,
			EUserInterfaceActionType::RadioButton
		);
	};

	AddFilter(EFilter::All, TEXT("All"), TEXT("Show all scanned assets"));
	AddFilter(EFilter::Used, TEXT("Used"), TEXT("Show used assets"));
	AddFilter(EFilter::Unused, TEXT("Unused"), TEXT("Show unused assets, Clean Project deletes them"));
	AddFilter(EFilter::NotDeleted, TEXT("Not deleted"), TEXT("Show unused assets last cleanup could not delete"));

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> SPjcTabAssetsUsage::CreateInspector() {
	const auto MakeButton = [](const TCHAR* InLabel, const TCHAR* InToolTip, const FOnClicked& InOnClicked, const TAttribute<bool>& InEnabled) {
		return SNew(SButton).Text(FText::FromString(InLabel)).ToolTipText(FText::FromString(InToolTip)).OnClicked(InOnClicked).IsEnabled(InEnabled);
	};

	const TAttribute<bool> HasAsset = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateRaw(this, &SPjcTabAssetsUsage::CurrentHasAsset));

	// clang-format off
	return SNew(SWidgetSwitcher)
		.WidgetIndex_Raw(this, &SPjcTabAssetsUsage::GetInspectorIndex)
		+ SWidgetSwitcher::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Justification(ETextJustify::Center)
			.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Gray"))
			.ShadowOffset(FVector2D{0.5f, 0.5f})
			.ShadowColorAndOpacity(FLinearColor::Black)
			.Font(FPjcStyles::GetFont("Bold", 15))
			.Text(FText::FromString(TEXT("Select an asset to see why it is used or unused.")))
		]
		+ SWidgetSwitcher::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(5.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(FMargin{0.0f, 0.0f, 2.0f, 0.0f})
				[
					MakeButton(
						TEXT("<"),
						TEXT("Back"),
						FOnClicked::CreateLambda([this]() { OnBack(); return FReply::Handled(); }),
						TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([this]() { return HistoryBack.Num() > 0; }))
					)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(FMargin{0.0f, 0.0f, 10.0f, 0.0f})
				[
					MakeButton(
						TEXT(">"),
						TEXT("Forward"),
						FOnClicked::CreateLambda([this]() { OnForward(); return FReply::Handled(); }),
						TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([this]() { return HistoryForward.Num() > 0; }))
					)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Font(FPjcStyles::GetFont("Bold", 16))
					.Text_Raw(this, &SPjcTabAssetsUsage::GetTitleText)
					.ToolTipText_Raw(this, &SPjcTabAssetsUsage::GetTitleText)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(FMargin{5.0f, 0.0f, 0.0f, 0.0f})
				[
					MakeButton(
						TEXT("X"),
						TEXT("Close (Esc in graph)"),
						FOnClicked::CreateLambda([this]() { ClearInspected(); return FReply::Handled(); }),
						true
					)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin{10.0f, 0.0f, 5.0f, 5.0f})
			[
				SNew(STextBlock)
				.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Gray"))
				.Text_Raw(this, &SPjcTabAssetsUsage::GetSubtitleText)
				.ToolTipText_Raw(this, &SPjcTabAssetsUsage::GetSubtitleText)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin{10.0f, 5.0f, 5.0f, 5.0f})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SBorder)
					.BorderImage(FPjcStyles::Get().GetBrush("ProjectCleaner.BgWhite"))
					.BorderBackgroundColor_Raw(this, &SPjcTabAssetsUsage::GetStatusColor)
					.Padding(FMargin{10.0f, 3.0f})
					[
						SNew(STextBlock)
						.Font(FPjcStyles::GetFont("Bold", 11))
						.ColorAndOpacity(FPjcStyles::Get().GetColor("ProjectCleaner.Color.Black"))
						.Text_Raw(this, &SPjcTabAssetsUsage::GetStatusText)
					]
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SBox)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(FMargin{5.0f, 0.0f, 0.0f, 0.0f})
				[
					MakeButton(
						TEXT("Connections"),
						TEXT("Show what uses it and what it uses"),
						FOnClicked::CreateLambda([this]() { View = EView::Connections; return FReply::Handled(); }),
						true
					)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(FMargin{5.0f, 0.0f, 0.0f, 0.0f})
				[
					MakeButton(
						TEXT("Browse"),
						TEXT("Select asset in Content Browser"),
						FOnClicked::CreateLambda([this]() { OnShowInContentBrowser(); return FReply::Handled(); }),
						HasAsset
					)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(FMargin{5.0f, 0.0f, 0.0f, 0.0f})
				[
					MakeButton(
						TEXT("Open"),
						TEXT("Open asset editor"),
						FOnClicked::CreateLambda([this]() { OnOpenAsset(); return FReply::Handled(); }),
						HasAsset
					)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(FMargin{5.0f, 0.0f, 0.0f, 0.0f})
				[
					MakeButton(
						TEXT("References"),
						TEXT("Open engine Reference Viewer for this asset"),
						FOnClicked::CreateLambda([this]() { OnOpenReferenceViewer(); return FReply::Handled(); }),
						HasAsset
					)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin{10.0f, 5.0f})
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Font(FPjcStyles::GetFont("Light", 11))
				.Text_Lambda([this]() { return CurrentWhy; })
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin{10.0f, 5.0f})
			[
				SNew(SVerticalBox)
				.Visibility_Lambda([this]() { return ChainBox->NumSlots() > 0 ? EVisibility::Visible : EVisibility::Collapsed; })
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin{0.0f, 0.0f, 0.0f, 5.0f})
				[
					SNew(STextBlock)
					.Font(FPjcStyles::GetFont("Bold", 10))
					.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
					.Text(FText::FromString(TEXT("How it is used")))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBox)
					.MaxDesiredHeight(220.0f)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							ChainBox.ToSharedRef()
						]
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(5.0f)
			[
				SNew(SSeparator).Thickness(3.0f)
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(5.0f)
			[
				SNew(SSplitter)
				.Orientation(Orient_Horizontal)
				+ SSplitter::Slot()
				.Value(0.5f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(5.0f)
					[
						SNew(STextBlock)
						.Font(FPjcStyles::GetFont("Bold", 10))
						.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
						.ToolTipText(FText::FromString(TEXT("Assets that reference this one. Double click to inspect.")))
						.Text_Raw(this, &SPjcTabAssetsUsage::GetUsedByHeaderText)
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						ListUsedBy.ToSharedRef()
					]
				]
				+ SSplitter::Slot()
				.Value(0.5f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(5.0f)
					[
						SNew(STextBlock)
						.Font(FPjcStyles::GetFont("Bold", 10))
						.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
						.ToolTipText(FText::FromString(TEXT("Assets this one references. Double click to inspect.")))
						.Text_Raw(this, &SPjcTabAssetsUsage::GetUsesHeaderText)
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						ListUses.ToSharedRef()
					]
				]
			]
		];
	// clang-format on
}

TSharedRef<SHeaderRow> SPjcTabAssetsUsage::GetHeaderRow(const bool bAssetList) {
	const auto MakeColumn = [&](const FName InColumn, const TCHAR* InLabel, const float InWidth, const TCHAR* InToolTip) {
		SHeaderRow::FColumn::FArguments Args = SHeaderRow::Column(InColumn);
		Args.FillWidth(InWidth)
			.HAlignHeader(HAlign_Left)
			.VAlignHeader(VAlign_Center)
			.HeaderContentPadding(FMargin {5.0f})[SNew(STextBlock)
													  .Text(FText::FromString(InLabel))
													  .ToolTipText(FText::FromString(InToolTip))
													  .ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
													  .Font(FPjcStyles::GetFont("Light", 10.0f))];

		if (bAssetList) {
			Args.SortMode_Raw(this, &SPjcTabAssetsUsage::GetColumnSortMode, InColumn).OnSort_Raw(this, &SPjcTabAssetsUsage::OnListSort);
		}

		return Args;
	};

	TSharedRef<SHeaderRow> HeaderRow = SNew(SHeaderRow);
	HeaderRow->AddColumn(MakeColumn(TEXT("Name"), TEXT("Name"), bAssetList ? 0.3f : 0.5f, TEXT("Asset name")));
	HeaderRow->AddColumn(MakeColumn(TEXT("Class"), TEXT("Class"), bAssetList ? 0.17f : 0.3f, TEXT("Asset class")));
	HeaderRow->AddColumn(MakeColumn(TEXT("Status"), TEXT("Status"), bAssetList ? 0.13f : 0.2f, TEXT("Used, unused or not deleted by last cleanup")));
	if (bAssetList) {
		HeaderRow->AddColumn(MakeColumn(TEXT("Why"), TEXT("Why"), 0.4f, TEXT("Why asset is used or unused")));
	}

	return HeaderRow;
}

TSharedRef<ITableRow> SPjcTabAssetsUsage::OnGenerateRow(TSharedPtr<FPjcUsageItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const {
	return SNew(SPjcItemUsage, OwnerTable).Item(Item).HighlightText(SearchText);
}

void SPjcTabAssetsUsage::OnListSelectionChanged(TSharedPtr<FPjcUsageItem> Item, ESelectInfo::Type SelectInfo) {
	if (bSyncingSelection || !Item.IsValid()) return;

	Inspect(Item->PackageName);
}

void SPjcTabAssetsUsage::OnLinkDoubleClicked(TSharedPtr<FPjcUsageItem> Item) {
	if (!Item.IsValid()) return;

	Inspect(Item->PackageName);
}

void SPjcTabAssetsUsage::OnGraphSelectionChanged(const FName& PackageName) {
	if (PackageName.IsNone()) {
		ClearInspected();
		return;
	}

	TGuardValue<bool> GraphGuard {bSelectingInGraph, true};
	Inspect(PackageName);
}

void SPjcTabAssetsUsage::OnGraphAssetDoubleClicked(const FName& PackageName) {
	const TSharedPtr<FPjcUsageItem>* Item = ItemsByPackage.Find(PackageName);
	if (Item && (*Item)->Asset.IsValid()) {
		UPjcSubsystem::OpenAssetEditor((*Item)->Asset);
	}
}

void SPjcTabAssetsUsage::OnListSort(EColumnSortPriority::Type SortPriority, const FName& ColumnName, EColumnSortMode::Type InSortMode) {
	SortMode = SortColumn == ColumnName && SortMode == EColumnSortMode::Ascending ? EColumnSortMode::Descending : EColumnSortMode::Ascending;
	SortColumn = ColumnName;

	ListSort();
	ListView->RequestListRefresh();
}

EColumnSortMode::Type SPjcTabAssetsUsage::GetColumnSortMode(const FName ColumnName) const {
	return SortColumn == ColumnName ? SortMode : EColumnSortMode::None;
}

void SPjcTabAssetsUsage::OnSearchTextChanged(const FText& InText) {
	SearchText = InText;
	ListUpdateView();
}

void SPjcTabAssetsUsage::OnBack() {
	if (HistoryBack.Num() == 0) return;

	if (Current.IsValid()) {
		HistoryForward.Add(Current->PackageName);
	}
	Inspect(PjcShim::PopArray(HistoryBack), false);
}

void SPjcTabAssetsUsage::OnForward() {
	if (HistoryForward.Num() == 0) return;

	if (Current.IsValid()) {
		HistoryBack.Add(Current->PackageName);
	}
	Inspect(PjcShim::PopArray(HistoryForward), false);
}

void SPjcTabAssetsUsage::OnShowInContentBrowser() const {
	if (!CurrentHasAsset()) return;

	UPjcSubsystem::GetModuleContentBrowser().Get().SyncBrowserToAssets(TArray<FAssetData> {Current->Asset});
}

void SPjcTabAssetsUsage::OnOpenAsset() const {
	if (!CurrentHasAsset()) return;

	UPjcSubsystem::OpenAssetEditor(Current->Asset);
}

void SPjcTabAssetsUsage::OnOpenReferenceViewer() const {
	if (!CurrentHasAsset()) return;

	UPjcSubsystem::OpenReferenceViewer(TArray<FAssetData> {Current->Asset});
}

bool SPjcTabAssetsUsage::CurrentHasAsset() const {
	return Current.IsValid() && Current->Asset.IsValid();
}

FText SPjcTabAssetsUsage::GetSummaryText() const {
	if (!bScanned) {
		return FText::FromString(TEXT("Press Scan to find out why each asset is used or unused."));
	}

	FString Summary = FString::Printf(
		TEXT("%d assets: %d used (%s), %d unused (%s)"),
		ItemsAll.Num(),
		NumUsed,
		*FText::AsMemory(SizeUsed, IEC).ToString(),
		NumUnused,
		*FText::AsMemory(SizeUnused, IEC).ToString()
	);
	if (NumNotDeleted > 0) {
		Summary += FString::Printf(TEXT(", %d not deleted by last cleanup"), NumNotDeleted);
	}

	return FText::FromString(Summary);
}

FText SPjcTabAssetsUsage::GetFilterText() const {
	switch (Filter) {
		case EFilter::Used :	   return FText::FromString(TEXT("Used"));
		case EFilter::Unused :	   return FText::FromString(TEXT("Unused"));
		case EFilter::NotDeleted : return FText::FromString(TEXT("Not deleted"));
		case EFilter::All :
		default :				   return FText::FromString(TEXT("All"));
	}
}

FText SPjcTabAssetsUsage::GetTitleText() const {
	return Current.IsValid() ? FText::FromString(SPjcItemUsage::GetItemName(*Current)) : FText::GetEmpty();
}

FText SPjcTabAssetsUsage::GetSubtitleText() const {
	if (!Current.IsValid()) return FText::GetEmpty();

	FString Subtitle = Current->ClassName.IsEmpty() ? FString {} : Current->ClassName + TEXT("    ");
	Subtitle += Current->PackageName.ToString();
	if (Current->bScanned) {
		Subtitle += TEXT("    ") + FText::AsMemory(Current->Size, IEC).ToString();
	}

	return FText::FromString(Subtitle);
}

FText SPjcTabAssetsUsage::GetStatusText() const {
	return Current.IsValid() ? FText::FromString(SPjcItemUsage::GetStatusText(*Current).ToString().ToUpper()) : FText::GetEmpty();
}

FSlateColor SPjcTabAssetsUsage::GetStatusColor() const {
	return Current.IsValid() ? FSlateColor {SPjcItemUsage::GetStatusColor(*Current)} : FSlateColor {FLinearColor::Transparent};
}

FText SPjcTabAssetsUsage::GetUsedByHeaderText() const {
	return FText::FromString(FString::Printf(TEXT("Used by (%d)"), ItemsUsedBy.Num()));
}

FText SPjcTabAssetsUsage::GetUsesHeaderText() const {
	return FText::FromString(FString::Printf(TEXT("Uses (%d)"), ItemsUses.Num()));
}

int32 SPjcTabAssetsUsage::GetInspectorIndex() const {
	return Current.IsValid() ? 1 : 0;
}
