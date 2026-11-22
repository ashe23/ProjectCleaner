// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/PjcWidgets.h"
#include "PjcUnused.h"
#include "PjcFormat.h"
#include "PjcShim.h"
#include "PjcStyles.h"
// Engine Headers
#include "AssetRegistry/AssetData.h"
#include "AssetToolsModule.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IAssetTypeActions.h"
#include "Misc/Paths.h"
#include "Styling/SlateIconFinder.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SWindow.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Text/STextBlock.h"

namespace PjcWidgets
{
	const FAssetType& GetAssetType(const FAssetData& Asset) {
		static TMap<FName, FAssetType> Types;
		const FName ClassName = PjcShim::GetAssetClassName(Asset);
		if (const FAssetType* Found = Types.Find(ClassName)) return *Found;

		// as the Content Browser shows the type: its name, color and icon
		FAssetType Type;
		Type.Label = FPaths::GetExtension(ClassName.ToString()).IsEmpty() ? ClassName.ToString() : FPaths::GetExtension(ClassName.ToString());
		Type.Color = GetColor("ProjectCleaner.Color.Muted");
		const UClass* AssetClass = Asset.GetClass();
		if (AssetClass) {
			IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
			const TSharedPtr<IAssetTypeActions> Actions = AssetTools.GetAssetTypeActionsForClass(AssetClass).Pin();
			if (Actions.IsValid()) {
				Type.Label = Actions->GetName().ToString();
				Type.Color = FLinearColor {Actions->GetTypeColor()};
			}
			Type.Icon = FSlateIconFinder::FindIconBrushForClass(AssetClass);
		}
		return Types.Add(ClassName, Type);
	}

	TSharedRef<SToolTip> MakeToolTip(const FString& Title, const FString& Text, const FString& More) {
		const TSharedRef<SVerticalBox> Lines = SNew(SVerticalBox);
		// clang-format off
		Lines->AddSlot().AutoHeight()
		[
			SNew(STextBlock).Text(FText::FromString(Title)).Font(FPjcStyles::GetFont("Bold", 9))
		];
		// clang-format on
		if (!Text.IsEmpty()) {
			// clang-format off
			Lines->AddSlot().AutoHeight().Padding(FMargin {0.0f, 3.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock).Text(FText::FromString(Text)).WrapTextAt(280.0f)
			];
			// clang-format on
		}
		if (!More.IsEmpty()) {
			// clang-format off
			Lines->AddSlot().AutoHeight().Padding(FMargin {0.0f, 5.0f, 0.0f, 0.0f})
			[
				SNew(STextBlock).Text(FText::FromString(More)).ColorAndOpacity(FSlateColor::UseSubduedForeground())
			];
			// clang-format on
		}
		// clang-format off
		return SNew(SToolTip)
			[
				Lines
			];
		// clang-format on
	}

	// the engine draws a text-only tooltip in another look, so the text goes in as content
	class SPjcTextToolTip : public SToolTip
	{
	  public:

		SLATE_BEGIN_ARGS(SPjcTextToolTip) {}
		SLATE_END_ARGS()

		void Construct(const FArguments&, const TAttribute<FText>& InText) {
			Text = InText;
			// clang-format off
			SToolTip::Construct(SToolTip::FArguments()
				[
					SNew(STextBlock).Text(InText).WrapTextAt(360.0f)
				]);
			// clang-format on
		}

		virtual bool IsEmpty() const override {
			return Text.Get().IsEmpty();
		}

	  private:

		TAttribute<FText> Text;
	};

	TSharedRef<SToolTip> MakeToolTip(const TAttribute<FText>& Text) {
		return SNew(SPjcTextToolTip, Text);
	}

	SHeaderRow::FColumn::FArguments MakeColumn(SHeaderRow::FColumn::FArguments Column, const TAttribute<FText>& ToolTip) {
#if ENGINE_MAJOR_VERSION < 5
		// 4.27 headers take only text
		Column.DefaultTooltip(ToolTip);
#else
		Column.ToolTip(TSharedPtr<IToolTip> {MakeToolTip(ToolTip)});
#endif
		return Column;
	}

	FLinearColor GetColor(const FName Name) {
		return FPjcStyles::Get().GetColor(Name);
	}

	FVector2D GetContentSize(const TSharedRef<SWidget>& Content) {
		Content->SlatePrepass(1.0f);
		return FVector2D {Content->GetDesiredSize()};
	}

	void ShowModal(const TSharedRef<SWindow>& Window) {
		const TSharedPtr<SWindow> Parent = FSlateApplication::Get().GetActiveTopLevelWindow();
		if (!Parent.IsValid()) {
			FSlateApplication::Get().AddModalWindow(Window, Parent);
			return;
		}
		// the center of the editor window, fully on its screen
		const TWeakPtr<SWindow> WeakParent = Parent;
		const auto GetCentered = [WeakParent](const FVector2D& Size) {
			const TSharedPtr<SWindow> Owner = WeakParent.Pin();
			if (!Owner.IsValid()) return FVector2D::ZeroVector;

			const FSlateRect OwnerRect = Owner->GetRectInScreen();
			const FSlateRect Area = FSlateApplication::Get().GetWorkArea(OwnerRect);
			FVector2D Position {(OwnerRect.Left + OwnerRect.Right - Size.X) * 0.5f, (OwnerRect.Top + OwnerRect.Bottom - Size.Y) * 0.5f};
			Position.X = FMath::Max(Area.Left, FMath::Min(Position.X, Area.Right - Size.X));
			Position.Y = FMath::Max(Area.Top, FMath::Min(Position.Y, Area.Bottom - Size.Y));
			return Position;
		};
		struct FCentering
		{
			FVector2D Size = FVector2D::ZeroVector;
			FVector2D Position = FVector2D::ZeroVector;
			double Start = 0.0;
			bool bOpened = false;
		};
		const TSharedRef<FCentering> State = MakeShared<FCentering>();

		// before it opens: the window opens there, at the size it starts with
		State->Size = FVector2D {Window->GetSizeInScreen()};
		State->Position = GetCentered(State->Size);
		Window->MoveWindowTo(State->Position);

		// an auto sized window takes its size in the first frames: centered again on each change, until the user moves it
		const TWeakPtr<SWindow> WeakWindow = Window;
		const FDelegateHandle Handle = FSlateApplication::Get().OnPostTick().AddLambda([State, WeakWindow, GetCentered](float) {
			const TSharedPtr<SWindow> Shown = WeakWindow.Pin();
			if (!Shown.IsValid() || !Shown->GetNativeWindow().IsValid()) return;

			const double Now = FPlatformTime::Seconds();
			if (State->Start == 0.0) State->Start = Now;
			if (Now - State->Start > 2.0) return;

			const FVector2D Size {Shown->GetSizeInScreen()};
			const FVector2D Position {Shown->GetPositionInScreen()};
			// the first frame always: the engine can open it elsewhere
			if (State->bOpened && (Size.Equals(State->Size, 1.0f) || !Position.Equals(State->Position, 2.0f))) return;

			State->bOpened = true;
			State->Size = Size;
			State->Position = GetCentered(Size);
			Shown->MoveWindowTo(State->Position);
		});
		FSlateApplication::Get().AddModalWindow(Window, Parent);
		FSlateApplication::Get().OnPostTick().Remove(Handle);
	}

	FLinearColor GetIconColor(const FLinearColor& ClassColor) {
		FLinearColor Color = ClassColor + (FLinearColor::White - ClassColor) * 0.25f;
		Color.A = 1.0f;
		return Color;
	}

	TSharedRef<SWidget> MakeStats(const TArray<FStat>& Stats, const float LinesFill) {
		if (Stats.Num() < 4) return SNullWidget::NullWidget;

		const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
		const FLinearColor Line = GetColor("ProjectCleaner.Color.Line");
		const FSlateColor Muted = FSlateColor::UseSubduedForeground();
		const auto IsZero = [](const FString& Value) {
			return Value == TEXT("0") || Value == TEXT("\u2013") || Value == PjcFormat::Size(0);
		};
		// large: the label above, the value with its unit smaller beside it
		const auto Large = [&](const FStat& Stat) -> TSharedRef<SWidget> {
			const TAttribute<FString> Value = Stat.Value;
			const FSlateColor Color {GetColor(Stat.Color)};
			const auto Number = [Value]() {
				FString Left = Value.Get();
				Value.Get().Split(TEXT(" "), &Left, nullptr);
				return FText::FromString(Left);
			};
			const auto Unit = [Value]() {
				FString Right;
				Value.Get().Split(TEXT(" "), nullptr, &Right);
				return FText::FromString(Right);
			};
			// clang-format off
			return SNew(SBox)
				.Padding(FMargin {14.0f, 9.0f, 14.0f, 10.0f})
				.ToolTip(MakeToolTip(Stat.Label, Stat.ToolTip))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						MakeCardTitle(Stat.Label)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 2.0f, 0.0f, 0.0f})
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)
						[
							SNew(STextBlock)
							.Text_Lambda(Number)
							.Font(GetFontMono(16))
							.ColorAndOpacity_Lambda([Value, IsZero, Color, Muted]() { return IsZero(Value.Get()) ? Muted : Color; })
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(FMargin {5.0f, 0.0f, 0.0f, 3.0f})
						[
							SNew(STextBlock).Text_Lambda(Unit).Font(GetFontMono(10)).ColorAndOpacity(Muted)
						]
					]
				];
			// clang-format on
		};
		// small: the label, the value on the right
		const auto Small = [&](const FStat& Stat) -> TSharedRef<SWidget> {
			const TAttribute<FString> Value = Stat.Value;
			// clang-format off
			return SNew(SHorizontalBox).ToolTip(MakeToolTip(Stat.Label, Stat.ToolTip))
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(FText::FromString(Stat.Label)).ColorAndOpacity(Muted)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {10.0f, 0.0f, 0.0f, 0.0f})
				[
					SNew(STextBlock)
					.Text_Lambda([Value]() { return FText::FromString(Value.Get()); })
					.Font(GetFontMono(10))
					.ColorAndOpacity_Lambda([Value, IsZero, Muted]() { return IsZero(Value.Get()) ? Muted : FSlateColor::UseForeground(); })
				];
			// clang-format on
		};
		const auto Separator = [White, Line]() -> TSharedRef<SWidget> {
			// clang-format off
			return SNew(SBox).WidthOverride(1.0f)
				[
					SNew(SImage).Image(White).ColorAndOpacity(Line)
				];
			// clang-format on
		};

		// clang-format off
		return SNew(SBorder).BorderImage(White).BorderBackgroundColor(Line).Padding(1.0f)
			[
				SNew(SBorder).BorderImage(White).BorderBackgroundColor(GetColor("ProjectCleaner.Color.Recess")).Padding(0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[
						Large(Stats[0])
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						Separator()
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[
						Large(Stats[1])
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						Separator()
					]
					+ SHorizontalBox::Slot().FillWidth(LinesFill).VAlign(VAlign_Center).Padding(FMargin {14.0f, 8.0f})
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							Small(Stats[2])
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(FMargin {0.0f, 5.0f, 0.0f, 0.0f})
						[
							Small(Stats[3])
						]
					]
				]
			];
		// clang-format on
	}

	TSharedRef<SWidget> MakeStepMark(TFunction<bool()> IsDone, TFunction<bool()> IsRunning) {
		const FSlateColor Ok {GetColor("ProjectCleaner.Color.Ok")};
		// clang-format off
		return SNew(SBox).WidthOverride(14.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(SOverlay)
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([IsDone]() { return FText::FromString(IsDone() ? TEXT("\u2713") : TEXT("\u00B7")); })
					.ColorAndOpacity_Lambda([IsDone, Ok]() { return IsDone() ? Ok : FSlateColor::UseSubduedForeground(); })
					.Visibility_Lambda([IsRunning]() { return IsRunning() ? EVisibility::Hidden : EVisibility::Visible; })
				]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(SCircularThrobber)
					.PieceImage(FPjcStyles::Get().GetBrush("ProjectCleaner.Spinner"))
					.NumPieces(10)
					.Period(0.8f)
					.Radius(6.0f)
					.ColorAndOpacity(GetColor("ProjectCleaner.Color.Accent"))
					.Visibility_Lambda([IsRunning]() { return IsRunning() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
				]
			];
		// clang-format on
	}

	FSetColors GetSetColors(const EPjcAssetSet Set) {
		// the set's palette color for bars and folder tints; size map boxes a shade darker, so white names read on them
		const auto Make = [](const FName Fill) {
			FSetColors Colors;
			Colors.Bar = GetColor(Fill);
			Colors.Box = Colors.Bar * FLinearColor {0.5f, 0.5f, 0.5f, 1.0f};
			Colors.BoxText = GetColor("ProjectCleaner.Color.Strong");
			Colors.Tint = Colors.Bar;
			return Colors;
		};
		switch (Set) {
			case EPjcAssetSet::Excluded : return Make("ProjectCleaner.Color.ExcludedFill");
			case EPjcAssetSet::Used :	  return Make("ProjectCleaner.Color.UsedFill");
			case EPjcAssetSet::Review :	  return Make("ProjectCleaner.Color.ReviewFill");
			case EPjcAssetSet::Unused :
			default :					  return Make("ProjectCleaner.Color.UnusedFill");
		}
	}

	FString GetSetLabel(const EPjcAssetSet Set) {
		switch (Set) {
			case EPjcAssetSet::Excluded : return TEXT("Excluded");
			case EPjcAssetSet::Used :	  return TEXT("Used");
			case EPjcAssetSet::Review :	  return TEXT("Review");
			case EPjcAssetSet::Unused :
			default :					  return TEXT("Unused");
		}
	}

	FString GetSetWord(const EPjcAssetSet Set) {
		return Set == EPjcAssetSet::Review ? TEXT("to review") : GetSetLabel(Set).ToLower();
	}

	FSlateFontInfo GetFontMono(const int32 Size) {
		return FCoreStyle::GetDefaultFontStyle("Mono", Size);
	}

	TSharedRef<SWidget> MakeCard(const TSharedRef<SWidget>& Content, const FMargin& Padding) {
		// clang-format off
		return SNew(SBorder).BorderImage(PjcShim::GetBrushCard()).Padding(Padding)
			[
				Content
			];
		// clang-format on
	}

	TSharedRef<SWidget> MakeCardTitle(const FString& Title) {
		// clang-format off
		return SNew(STextBlock)
			.Text(FText::FromString(Title.ToUpper()))
			.Font(FPjcStyles::GetFont("Bold", 8))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground());
		// clang-format on
	}

	TSharedRef<SWidget> MakeSwatch(const FLinearColor& Color, const float Size) {
		// clang-format off
		return SNew(SBox).WidthOverride(Size).HeightOverride(Size)
			[
				SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(Color)
			];
		// clang-format on
	}

	TSharedRef<SWidget> MakeBar(const TArray<FBarSegment>& Segments, const float Height, const double Total) {
		double Sum = 0.0;
		for (const auto& Segment : Segments) {
			Sum += FMath::Max(Segment.Value, 0.0);
		}
		const double Scale = FMath::Max(Sum, Total);

		const TSharedRef<SHorizontalBox> Fill = SNew(SHorizontalBox);
		if (Scale > 0.0) {
			for (const auto& Segment : Segments) {
				if (Segment.Value <= 0.0) continue;

				// a tiny share stays visible
				const float Share = FMath::Max(static_cast<float>(Segment.Value / Scale), 0.003f);
				// clang-format off
				Fill->AddSlot().FillWidth(Share)
				[
					SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(Segment.Color).ToolTip(Segment.ToolTip)
				];
				// clang-format on
			}
			if (Scale > Sum) {
				// clang-format off
				Fill->AddSlot().FillWidth(static_cast<float>((Scale - Sum) / Scale))
				[
					SNew(SSpacer)
				];
				// clang-format on
			}
		}

		// clang-format off
		return SNew(SBox).HeightOverride(Height)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity(GetColor("ProjectCleaner.Color.Track"))
				]
				+ SOverlay::Slot()
				[
					Fill
				]
			];
		// clang-format on
	}

	TSharedRef<SWidget> MakeFilterPill(
		const FString& Label,
		const FString& ToolTip,
		const FString& More,
		const FLinearColor& Color,
		const int32 Num,
		const bool bDot,
		TFunction<bool()> IsOn,
		TFunction<bool()> IsGrey,
		TFunction<void()> OnClick
	) {
		const FLinearColor Recess = GetColor("ProjectCleaner.Color.Recess");
		const FLinearColor Line = GetColor("ProjectCleaner.Color.Line");
		const FLinearColor Grey = GetColor("ProjectCleaner.Color.Hover");
		const FLinearColor Fill = FMath::Lerp(Recess, Color, 0.22f);
		// clang-format off
		return SNew(SButton)
			.ButtonStyle(&PjcShim::GetStyle().GetWidgetStyle<FButtonStyle>(TEXT("NoBorder")))
#if ENGINE_MAJOR_VERSION < 5
			.ForegroundColor(FSlateColor::UseForeground())
#endif
			.ContentPadding(FMargin {0.0f})
			.ToolTip(MakeToolTip(Label, ToolTip, More))
			.OnClicked_Lambda([OnClick]() {
				OnClick();
				return FReply::Handled();
			})
			[
				SNew(SBorder)
				.BorderImage(FPjcStyles::Get().GetBrush("ProjectCleaner.Pill"))
				.BorderBackgroundColor_Lambda([IsOn, Color, Line]() { return FSlateColor {IsOn() ? Color : Line}; })
				.Padding(FMargin {1.0f})
				[
					SNew(SBorder)
					.BorderImage(FPjcStyles::Get().GetBrush("ProjectCleaner.Pill"))
					.BorderBackgroundColor_Lambda([IsOn, Fill, Recess]() { return FSlateColor {IsOn() ? Fill : Recess}; })
					.Padding(FMargin {bDot ? 8.0f : 10.0f, 2.0f, 10.0f, 2.0f})
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {0.0f, 0.0f, 6.0f, 0.0f})
						[
							SNew(SBox).WidthOverride(7.0f).HeightOverride(7.0f).Visibility(bDot ? EVisibility::Visible : EVisibility::Collapsed)
							[
								SNew(SImage)
								.Image(FPjcStyles::Get().GetBrush("ProjectCleaner.Dot"))
								.ColorAndOpacity_Lambda([IsGrey, Color, Grey]() { return FSlateColor {IsGrey() ? Grey : Color}; })
							]
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
						[
							SNew(SBox).Clipping(EWidgetClipping::ClipToBounds)
							[
								SNew(STextBlock)
								.Text(FText::FromString(Label))
								.ColorAndOpacity_Lambda([IsGrey]() { return IsGrey() ? FSlateColor::UseSubduedForeground() : FSlateColor::UseForeground(); })
							]
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin {7.0f, 0.0f, 0.0f, 0.0f})
						[
							SNew(STextBlock)
							.Text(FText::FromString(PjcFormat::Count(Num)))
							.Font(GetFontMono(8))
							.ColorAndOpacity_Lambda([IsOn]() { return IsOn() ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground(); })
						]
					]
				]
			];
		// clang-format on
	}

	TSharedRef<SWidget> MakeFilterHeading(const TAttribute<FText>& Text, const TAttribute<FText>& ToolTip) {
		return SNew(STextBlock)
			.Text(Text)
			.ToolTip(PjcWidgets::MakeToolTip(ToolTip))
			.Font(FPjcStyles::GetFont("Bold", 8))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground());
	}

	FSlateIcon GetMenuIcon(const EPjcMenuIcon Icon) {
		switch (Icon) {
			case EPjcMenuIcon::Inspect :  return FPjcStyles::GetIcon("ProjectCleaner.Menu.Inspect");
			case EPjcMenuIcon::Browse :	  return FPjcStyles::GetIcon("ProjectCleaner.Menu.Browse");
			case EPjcMenuIcon::Delete :	  return FPjcStyles::GetIcon("ProjectCleaner.Menu.Delete");
			case EPjcMenuIcon::Exclude :  return FPjcStyles::GetIcon("ProjectCleaner.Menu.Exclude");
			case EPjcMenuIcon::Include :  return FPjcStyles::GetIcon("ProjectCleaner.Menu.Include");
			case EPjcMenuIcon::Explorer : return FPjcStyles::GetIcon("ProjectCleaner.Menu.Explorer");
			case EPjcMenuIcon::SizeMap :  return FPjcStyles::GetIcon("ProjectCleaner.Menu.SizeMap");
			case EPjcMenuIcon::Expand :	  return FPjcStyles::GetIcon("ProjectCleaner.Menu.Expand");
			case EPjcMenuIcon::Collapse : return FPjcStyles::GetIcon("ProjectCleaner.Menu.Collapse");
			case EPjcMenuIcon::Reset :	  return FPjcStyles::GetIcon("ProjectCleaner.Menu.Reset");
			default :					  return FSlateIcon {};
		}
	}

	void AddMenuEntry(FMenuBuilder& Menu, const EPjcMenuIcon Icon, const FString& Label, const FString& ToolTip, TFunction<void()> OnClick) {
		AddMenuEntry(Menu, Icon, Label, ToolTip, true, MoveTemp(OnClick));
	}

	void AddMenuEntry(
		FMenuBuilder& Menu,
		const EPjcMenuIcon Icon,
		const FString& Label,
		const FString& ToolTip,
		const bool bEnabled,
		TFunction<void()> OnClick
	) {
		const FCanExecuteAction CanClick = FCanExecuteAction::CreateLambda([bEnabled]() {
			return bEnabled;
		});
		const FUIAction Action {FExecuteAction::CreateLambda(MoveTemp(OnClick)), CanClick};
		Menu.AddMenuEntry(FText::FromString(Label), FText::FromString(ToolTip), GetMenuIcon(Icon), Action);
	}

	void AddMenuToggle(FMenuBuilder& Menu, const FString& Label, const FString& ToolTip, const bool* bChecked, TFunction<void()> OnClick) {
		const FIsActionChecked IsChecked = FIsActionChecked::CreateLambda([bChecked]() {
			return bChecked && *bChecked;
		});
		const FUIAction Action {FExecuteAction::CreateLambda(MoveTemp(OnClick)), FCanExecuteAction {}, IsChecked};
		const FText Tip = FText::FromString(ToolTip);
		Menu.AddMenuEntry(FText::FromString(Label), Tip, FSlateIcon {}, Action, NAME_None, EUserInterfaceActionType::ToggleButton);
	}
}	// namespace PjcWidgets
