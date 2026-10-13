// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/SPjcAssetConnections.h"
#include "Slate/SPjcItemUsage.h"
#include "PjcShim.h"
#include "PjcStyles.h"
#include "PjcTypes.h"
// Engine Headers
#include "Rendering/DrawElements.h"
#include "Styling/SlateIconFinder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	const FLinearColor CardColor {0.022f, 0.024f, 0.029f, 1.0f};
	const FLinearColor CardChainColor {0.04f, 0.044f, 0.052f, 1.0f};
	const FLinearColor CardHoverColor {0.07f, 0.076f, 0.09f, 1.0f};
	const FLinearColor TextGray {0.55f, 0.57f, 0.6f, 1.0f};
	const FLinearColor ChainLinkColor {1.0f, 1.0f, 1.0f, 0.85f};
}	// namespace

/** Curves between cards, painted over columns, arrow points to used asset */
class SPjcConnectionLinks final : public SLeafWidget
{
  public:

	SLATE_BEGIN_ARGS(SPjcConnectionLinks) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<SPjcAssetConnections>& InOwner) {
		Owner = InOwner;
		SetVisibility(EVisibility::HitTestInvisible);
	}

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override {
		return FVector2D::ZeroVector;
	}

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled
	) const override {
		const TSharedPtr<SPjcAssetConnections> PinnedOwner = Owner.Pin();
		if (!PinnedOwner.IsValid()) return LayerId;

		TArray<SPjcAssetConnections::FLink> Links;
		PinnedOwner->GetLinks(Links);

		const auto GetRect = [&AllottedGeometry](const TSharedPtr<SWidget>& Widget) {
			const FGeometry& Geometry = Widget->GetPaintSpaceGeometry();
			const FVector2D TopLeft = AllottedGeometry.AbsoluteToLocal(Geometry.LocalToAbsolute(FVector2D::ZeroVector));
			const FVector2D BottomRight = AllottedGeometry.AbsoluteToLocal(Geometry.LocalToAbsolute(Geometry.GetLocalSize()));
			return FSlateRect {TopLeft, BottomRight};
		};
		const auto IsVisibleIn = [&GetRect](const FSlateRect& Rect, const TSharedPtr<SWidget>& Clip) {
			if (!Clip.IsValid()) return true;

			const FSlateRect ClipRect = GetRect(Clip);
			return Rect.Bottom > ClipRect.Top + 2.0f && Rect.Top < ClipRect.Bottom - 2.0f;
		};

		for (const auto& Link : Links) {
			if (!Link.From.IsValid() || !Link.To.IsValid()) continue;

			const FSlateRect From = GetRect(Link.From);
			const FSlateRect To = GetRect(Link.To);
			if (!IsVisibleIn(From, Link.FromClip) || !IsVisibleIn(To, Link.ToClip)) continue;

			const FVector2D FromCenter = From.GetCenter();
			const FVector2D ToCenter = To.GetCenter();
			FVector2D Start;
			FVector2D End;
			FVector2D Bend;
			if (Link.bVertical) {
				Start = FVector2D {FromCenter.X, From.Bottom};
				End = FVector2D {ToCenter.X, To.Top};
				Bend = FVector2D {0.0f, FMath::Max(static_cast<float>(End.Y - Start.Y) * 0.5f, 8.0f)};
			}
			else {
				Start = FVector2D {From.Right, FromCenter.Y};
				End = FVector2D {To.Left, ToCenter.Y};
				Bend = FVector2D {FMath::Max(static_cast<float>(End.X - Start.X) * 0.5f, 20.0f), 0.0f};
			}

			FSlateDrawElement::MakeCubicBezierSpline(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(),
				Start,
				Start + Bend,
				End - Bend,
				End,
				Link.Thickness,
				ESlateDrawEffect::None,
				Link.Color
			);

			const FVector2D Dir = Bend.GetSafeNormal();
			const FVector2D Normal {-Dir.Y, Dir.X};
			const TArray<FVector2D> Arrow {End - Dir * 7.0f + Normal * 4.0f, End, End - Dir * 7.0f - Normal * 4.0f};
			FSlateDrawElement::MakeLines(
				OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Arrow, ESlateDrawEffect::None, Link.Color, true, Link.Thickness
			);
		}

		return LayerId + 1;
	}

  private:

	TWeakPtr<SPjcAssetConnections> Owner;
};

void SPjcAssetConnections::Construct(const FArguments& InArgs) {
	OnInspect = InArgs._OnInspect;
	OnOpen = InArgs._OnOpen;
	OnBrowse = InArgs._OnBrowse;
	OnShowInMap = InArgs._OnShowInMap;

	SAssignNew(Content, SBox);

	// clang-format off
	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			Content.ToSharedRef()
		]
		+ SOverlay::Slot()
		[
			SNew(SPjcConnectionLinks, SharedThis(this))
		]
	];
	// clang-format on

	Rebuild();
}

void SPjcAssetConnections::SetAsset(
	const TSharedPtr<FPjcUsageItem>& InItem,
	const TArray<TSharedPtr<FPjcUsageItem>>& InChain,
	const TArray<TSharedPtr<FPjcUsageItem>>& InUsedBy,
	const TArray<TSharedPtr<FPjcUsageItem>>& InUses,
	const FText& InWhy,
	const FString& InOwned
) {
	Item = InItem;
	Chain = InChain;
	UsedBy = InUsedBy;
	Uses = InUses;
	Why = InWhy;
	Owned = InOwned;

	// parent in chain leads used by list
	if (Chain.Num() >= 2) {
		const TSharedPtr<FPjcUsageItem> Parent = Chain[Chain.Num() - 2];
		if (UsedBy.Remove(Parent) > 0) {
			UsedBy.Insert(Parent, 0);
		}
	}

	Rebuild();
}

void SPjcAssetConnections::Rebuild() {
	ChainCards.Reset();
	UsedByCards.Reset();
	UsesCards.Reset();
	CenterCard.Reset();
	ChainBox.Reset();
	UsedByList.Reset();
	UsesList.Reset();

	if (!Item.IsValid()) {
		// clang-format off
		Content->SetContent(
			SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(TextGray)
				.Font(FPjcStyles::GetFont(TEXT("Bold"), 13))
				.Text(FText::FromString(TEXT("Select an asset on the Map or in the List to see what uses it and what it uses.")))
			]
		);
		// clang-format on
		return;
	}

	// chain from asset used for its own reason down to grandparent, parent is first in used by list
	const TSharedRef<SVerticalBox> ChainColumn = SNew(SVerticalBox);
	for (int32 Index = 0; Index + 2 < Chain.Num(); ++Index) {
		const TSharedRef<SWidget> Card = MakeCard(Chain[Index], true);
		ChainCards.Add(Card);
		ChainColumn->AddSlot().AutoHeight().Padding(FMargin {0.0f, 0.0f, 0.0f, 18.0f})[Card];
	}

	SAssignNew(UsedByList, SListView<TSharedPtr<FPjcUsageItem>>)
		.ListItemsSource(&UsedBy)
		.SelectionMode(ESelectionMode::None)
		.OnGenerateRow(this, &SPjcAssetConnections::OnGenerateRow, true);
	SAssignNew(UsesList, SListView<TSharedPtr<FPjcUsageItem>>)
		.ListItemsSource(&Uses)
		.SelectionMode(ESelectionMode::None)
		.OnGenerateRow(this, &SPjcAssetConnections::OnGenerateRow, false);

	CenterCard = MakeCenter();

	const TSharedRef<SHorizontalBox> Columns = SNew(SHorizontalBox);
	if (ChainCards.Num() > 0) {
		SAssignNew(ChainBox, SScrollBox) + SScrollBox::Slot()[ChainColumn];
		Columns->AddSlot().AutoWidth().Padding(
			FMargin {0.0f, 0.0f, 40.0f, 0.0f}
		)[MakeColumn(FText::FromString(TEXT("How it is used")), SNew(SBox).WidthOverride(230.0f).VAlign(VAlign_Center)[ChainBox.ToSharedRef()])];
	}

	const auto ListOrNote = [](const TSharedRef<SWidget>& List, const bool bEmpty, const TCHAR* Note) -> TSharedRef<SWidget> {
		if (!bEmpty) return List;

		return SNew(STextBlock).ColorAndOpacity(TextGray).AutoWrapText(true).Text(FText::FromString(Note));
	};
	const FString UsedByTitle = FString::Printf(TEXT("Used by (%d)"), UsedBy.Num());
	const FString UsesTitle = FString::Printf(TEXT("Uses (%d)"), Uses.Num());
	const TCHAR* UsedByNote
		= Item->bScanned && !Item->Usage.bUsed ? TEXT("Nothing uses it.") : TEXT("No asset uses it, it is used for its own reason.");
	Columns->AddSlot().FillWidth(
		1.0f
	)[MakeColumn(FText::FromString(UsedByTitle), ListOrNote(UsedByList.ToSharedRef(), UsedBy.Num() == 0, UsedByNote))];
	Columns->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {48.0f, 0.0f})[SNew(SBox).WidthOverride(330.0f)[CenterCard.ToSharedRef()]];
	Columns->AddSlot().FillWidth(
		1.0f
	)[MakeColumn(FText::FromString(UsesTitle), ListOrNote(UsesList.ToSharedRef(), Uses.Num() == 0, TEXT("Uses no other assets.")))];

	Content->SetContent(SNew(SBox).Padding(FMargin {12.0f, 8.0f})[Columns]);
}

void SPjcAssetConnections::GetLinks(TArray<FLink>& OutLinks) const {
	if (!Item.IsValid() || !CenterCard.IsValid()) return;

	for (int32 Index = 0; Index + 1 < ChainCards.Num(); ++Index) {
		OutLinks.Add(FLink {ChainCards[Index], ChainCards[Index + 1], ChainBox, ChainBox, ChainLinkColor, 2.5f, true});
	}

	const TSharedPtr<FPjcUsageItem> Parent = Chain.Num() >= 2 ? Chain[Chain.Num() - 2] : nullptr;
	for (const auto& Pair : UsedByCards) {
		const TSharedPtr<SWidget> Card = Pair.Value.Pin();
		if (!Card.IsValid()) continue;

		const bool bParent = Parent.IsValid() && Pair.Key == Parent.Get();
		if (bParent && ChainCards.Num() > 0) {
			OutLinks.Add(FLink {ChainCards.Last(), Card, ChainBox, UsedByList, ChainLinkColor, 2.5f, false});
		}
		const FLinearColor Color = bParent ? ChainLinkColor : SPjcItemUsage::GetStatusColor(*Pair.Key).CopyWithNewOpacity(0.55f);
		OutLinks.Add(FLink {Card, CenterCard, UsedByList, nullptr, Color, bParent ? 2.5f : 1.5f, false});
	}

	for (const auto& Pair : UsesCards) {
		const TSharedPtr<SWidget> Card = Pair.Value.Pin();
		if (!Card.IsValid()) continue;

		OutLinks.Add(FLink {CenterCard, Card, nullptr, UsesList, SPjcItemUsage::GetStatusColor(*Pair.Key).CopyWithNewOpacity(0.55f), 1.5f, false});
	}
}

void SPjcAssetConnections::Defer(const FPjcOnConnectionAsset& Delegate, const FName& Package) {
	RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateLambda([Delegate, Package](double, float) {
							Delegate.ExecuteIfBound(Package);
							return EActiveTimerReturnType::Stop;
						}));
}

TSharedRef<SWidget> SPjcAssetConnections::MakeCard(const TSharedPtr<FPjcUsageItem>& InItem, const bool bHighlight) {
	const FName Package = InItem->PackageName;
	const UClass* AssetClass = InItem->Asset.IsValid() ? InItem->Asset.GetClass() : nullptr;
	FString Details = InItem->ClassName;
	if (InItem->bScanned) {
		Details += TEXT(", ") + FText::AsMemory(InItem->Size, IEC).ToString();
	}
	else {
		Details += Details.IsEmpty() ? TEXT("not scanned") : TEXT(", not scanned");
	}

	TSharedPtr<SBorder> Card;
	// clang-format off
	SAssignNew(Card, SBorder)
	.BorderImage(PjcShim::GetBrush(TEXT("WhiteBrush")))
	.Padding(0.0f)
	.Cursor(EMouseCursor::Hand)
	.ToolTipText(FText::FromString(Package.ToString() + TEXT("\n") + InItem->ReasonText))
	.OnMouseButtonDown_Lambda([this, Package](const FGeometry&, const FPointerEvent& MouseEvent) {
		if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();

		Defer(OnInspect, Package);
		return FReply::Handled();
	})
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(4.0f)
			[
				SNew(SImage)
				.Image(PjcShim::GetBrush(TEXT("WhiteBrush")))
				.ColorAndOpacity(SPjcItemUsage::GetStatusColor(*InItem))
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(FMargin{8.0f, 5.0f, 6.0f, 5.0f})
		[
			SNew(SBox)
			.WidthOverride(16.0f)
			.HeightOverride(16.0f)
			[
				SNew(SImage).Image(FSlateIconFinder::FindIconBrushForClass(AssetClass))
			]
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Center)
		.Padding(FMargin{0.0f, 5.0f, 8.0f, 5.0f})
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Font(FPjcStyles::GetFont(TEXT("Bold"), 9))
				.Text(FText::FromString(SPjcItemUsage::GetItemName(*InItem)))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Font(FPjcStyles::GetFont(TEXT("Regular"), 8))
				.ColorAndOpacity(TextGray)
				.Text(FText::FromString(Details))
			]
		]
	];
	// clang-format on

	const TWeakPtr<SBorder> WeakCard = Card;
	Card->SetBorderBackgroundColor(TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateLambda([WeakCard, bHighlight]() {
		const TSharedPtr<SBorder> PinnedCard = WeakCard.Pin();
		if (PinnedCard.IsValid() && PinnedCard->IsHovered()) return FSlateColor {CardHoverColor};

		return FSlateColor {bHighlight ? CardChainColor : CardColor};
	})));

	return Card.ToSharedRef();
}

TSharedRef<SWidget> SPjcAssetConnections::MakeCenter() {
	const FName Package = Item->PackageName;
	const bool bHasAsset = Item->Asset.IsValid();
	const FLinearColor StatusColor = SPjcItemUsage::GetStatusColor(*Item);
	FString Details = Item->ClassName;
	if (Item->bScanned) {
		Details += TEXT(", ") + FText::AsMemory(Item->Size, IEC).ToString();
	}

	const auto MakeButton = [this, Package, bHasAsset](const TCHAR* InLabel, const TCHAR* InToolTip, const FPjcOnConnectionAsset& InDelegate) {
		return SNew(SButton)
			.Text(FText::FromString(InLabel))
			.ToolTipText(FText::FromString(InToolTip))
			.IsEnabled(bHasAsset)
			.OnClicked_Lambda([this, Package, InDelegate]() {
				Defer(InDelegate, Package);
				return FReply::Handled();
			});
	};

	// clang-format off
	return SNew(SBorder)
		.BorderImage(PjcShim::GetBrush(TEXT("WhiteBrush")))
		.BorderBackgroundColor(CardChainColor)
		.Padding(FMargin{14.0f, 12.0f})
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Font(FPjcStyles::GetFont(TEXT("Bold"), 14))
				.AutoWrapText(true)
				.Text(FText::FromString(SPjcItemUsage::GetItemName(*Item)))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin{0.0f, 2.0f, 0.0f, 8.0f})
			[
				SNew(STextBlock)
				.ColorAndOpacity(TextGray)
				.AutoWrapText(true)
				.Text(FText::FromString(Details + TEXT("\n") + Package.ToString()))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Left)
			[
				SNew(SBorder)
				.BorderImage(PjcShim::GetBrush(TEXT("WhiteBrush")))
				.BorderBackgroundColor(StatusColor)
				.Padding(FMargin{8.0f, 2.0f})
				[
					SNew(STextBlock)
					.Font(FPjcStyles::GetFont(TEXT("Bold"), 10))
					.ColorAndOpacity(FLinearColor::Black)
					.Text(FText::FromString(SPjcItemUsage::GetStatusText(*Item).ToString().ToUpper()))
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin{0.0f, 8.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Font(FPjcStyles::GetFont(TEXT("Light"), 10))
				.Text(Why)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin{0.0f, 8.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.ColorAndOpacity(TextGray)
				.Visibility(Owned.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
				.Text(FText::FromString(Owned))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin{0.0f, 12.0f, 0.0f, 0.0f})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(FMargin{0.0f, 0.0f, 5.0f, 0.0f})
				[
					MakeButton(TEXT("Open"), TEXT("Open asset editor"), OnOpen)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(FMargin{0.0f, 0.0f, 5.0f, 0.0f})
				[
					MakeButton(TEXT("Browse"), TEXT("Select asset in Content Browser"), OnBrowse)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					MakeButton(TEXT("Show on Map"), TEXT("Show asset with its links on the Map"), OnShowInMap)
				]
			]
		];
	// clang-format on
}

TSharedRef<SWidget> SPjcAssetConnections::MakeColumn(const FText& Title, const TSharedRef<SWidget>& InContent) {
	// clang-format off
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin{0.0f, 0.0f, 0.0f, 6.0f})
		[
			SNew(STextBlock)
			.Font(FPjcStyles::GetFont(TEXT("Bold"), 10))
			.ColorAndOpacity(FPjcStyles::Get().GetSlateColor("ProjectCleaner.Color.Green"))
			.Text(Title)
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			InContent
		];
	// clang-format on
}

TSharedRef<ITableRow>
SPjcAssetConnections::OnGenerateRow(TSharedPtr<FPjcUsageItem> InItem, const TSharedRef<STableViewBase>& OwnerTable, const bool bUsedBy) {
	const bool bParent = bUsedBy && Chain.Num() >= 2 && InItem == Chain[Chain.Num() - 2];
	const TSharedRef<SWidget> Card = MakeCard(InItem, bParent);
	(bUsedBy ? UsedByCards : UsesCards).Add(InItem.Get(), Card);

	return SNew(STableRow<TSharedPtr<FPjcUsageItem>>, OwnerTable).ShowSelection(false).Padding(FMargin {0.0f, 3.0f})[Card];
}
