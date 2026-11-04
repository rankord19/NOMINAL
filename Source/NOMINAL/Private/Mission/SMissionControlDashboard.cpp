#include "Mission/SMissionControlDashboard.h"
#include "Mission/MissionControlGameMode.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
FLinearColor Ink(const TCHAR* Hex) { return FLinearColor(FColor::FromHex(Hex)); }
const FLinearColor Background = Ink(TEXT("0B1017"));
const FLinearColor Panel = Ink(TEXT("131D28"));
const FLinearColor White = Ink(TEXT("E7EDF3"));
const FLinearColor Muted = Ink(TEXT("95A6B8"));
const FLinearColor Accent = Ink(TEXT("80D8CC"));
const FLinearColor Warning = Ink(TEXT("E9B982"));
const FLinearColor Danger = Ink(TEXT("EF9C98"));

const FButtonStyle& ButtonStyle(bool Primary = false)
{
	static const FButtonStyle Quiet = FButtonStyle()
		.SetNormal(FSlateRoundedBoxBrush(Ink(TEXT("1B2A39")), 4.f))
		.SetHovered(FSlateRoundedBoxBrush(Ink(TEXT("2C4457")), 4.f))
		.SetPressed(FSlateRoundedBoxBrush(Ink(TEXT("35576A")), 4.f))
		.SetNormalPadding(FMargin(18, 13)).SetPressedPadding(FMargin(18, 13));
	static const FButtonStyle Main = FButtonStyle(Quiet)
		.SetNormal(FSlateRoundedBoxBrush(Ink(TEXT("254D50")), 4.f))
		.SetHovered(FSlateRoundedBoxBrush(Ink(TEXT("34696B")), 4.f))
		.SetPressed(FSlateRoundedBoxBrush(Ink(TEXT("1D4043")), 4.f));
	return Primary ? Main : Quiet;
}

TSharedRef<STextBlock> Label(const TCHAR* Text, int32 Size = 12, FLinearColor Color = Muted, bool Bold = false)
{
	return SNew(STextBlock).Text(FText::FromString(Text))
		.Font(FCoreStyle::GetDefaultFontStyle(Bold ? "Bold" : "Regular", Size))
		.ColorAndOpacity(Color).AutoWrapText(true);
}

TSharedRef<SBorder> Surface(TSharedRef<SWidget> Content, FMargin Padding = FMargin(28))
{
	return SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(Panel).Padding(Padding)[Content];
}

TSharedRef<SButton> Action(const TCHAR* Text, TFunction<void()> Execute, bool Primary = false)
{
	auto Caption = Label(Text, 13, Primary ? Accent : White, true);
	Caption->SetAutoWrapText(false);
	return SNew(SButton).ButtonStyle(&ButtonStyle(Primary)).HAlign(HAlign_Center)
		.OnClicked_Lambda([Execute]() { Execute(); return FReply::Handled(); })
		[Caption];
}
}

void SMissionControlDashboard::Construct(const FArguments& Args)
{
	Session = Args._Session;
	Phase = Args._Phase;
	const auto Weak = Session;
	auto Navigation = SNew(SVerticalBox);
	Navigation->AddSlot().AutoHeight().Padding(0, 0, 0, 8)[Label(TEXT("NOMINAL"), 26, White, true)];
	Navigation->AddSlot().AutoHeight().Padding(0, 0, 0, 48)[Label(TEXT("MISSION OPERATIONS"), 10, Accent, true)];
	Navigation->AddSlot().AutoHeight().Padding(0, 0, 0, 12)[Label(TEXT("WORKSPACES"), 10)];
	Navigation->AddSlot().AutoHeight().Padding(0, 0, 0, 10)
		[Surface(Label(TEXT("01   Mission Control"), 13, Accent, true), FMargin(16, 16))];
	Navigation->AddSlot().AutoHeight().Padding(0, 0, 0, 10)
		[Action(TEXT("Spacecraft Designer"), [Weak]() { if (Weak.IsValid()) Weak->ShowDesigner(); })];
	Navigation->AddSlot().AutoHeight().Padding(0, 0, 0, 10)
		[Action(TEXT("Electrical Power"), [Weak]() { if (Weak.IsValid()) Weak->ShowEPS(); })];
	Navigation->AddSlot().AutoHeight()
		[Action(TEXT("Orbital View"), [Weak]() { if (Weak.IsValid()) Weak->ShowOrbitalView(true); })];
	Navigation->AddSlot().FillHeight(1)[SNew(SSpacer)];
	Navigation->AddSlot().AutoHeight().Padding(0, 20, 0, 8)[Label(TEXT("LOCAL SESSION"), 10, Accent, true)];
	Navigation->AddSlot().AutoHeight()[Label(TEXT("Changes persist until Play ends.\nRestart Play for a new mission."), 11)];

	auto Lifecycle = SNew(SHorizontalBox);
	const TCHAR* Stages[] = {TEXT("Planning"), TEXT("Spacecraft\ndesign"), TEXT("Launch\npreparation"),
		TEXT("Launch"), TEXT("Flight\noperations"), TEXT("Science\noperations"), TEXT("Completed")};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Stages); ++Index)
	{
		Lifecycle->AddSlot().FillWidth(1).Padding(0, 0, Index == 6 ? 0 : 8, 0)
		[
			SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor_Lambda([this, Index]() { return static_cast<int32>(Phase.Get()) == Index ? Ink(TEXT("254D50")) : Panel; })
			.Padding(14, 14)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)
				[Label(*FString::Printf(TEXT("%02d"), Index + 1), 11, Accent, true)]
				+ SVerticalBox::Slot().AutoHeight()[Label(Stages[Index], 12, White)]
			]
		];
	}

	auto Hero = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)[Label(TEXT("CURRENT MISSION PHASE"), 11, Accent, true)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)
		[
			SNew(STextBlock).Text_Lambda([Weak]() { return Weak.IsValid() ? Weak->PhaseText() : FText::GetEmpty(); })
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 34))
			.ColorAndOpacity_Lambda([this]() { return Phase.Get() == EMissionPhase::Failed ? Danger : White; })
			.AutoWrapText(true)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,24)
		[
			SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 13)).ColorAndOpacity(Muted).AutoWrapText(true)
			.Text_Lambda([this]()
			{
				if (Phase.Get() == EMissionPhase::Completed) return FText::FromString(TEXT("Operations ended. Completion does not grade scientific success."));
				if (Phase.Get() == EMissionPhase::Failed) return FText::FromString(TEXT("Mission closed as failed. Further phase requests will be rejected and logged."));
				return FText::FromString(TEXT("Review the spacecraft and power state, then advance the mission when ready."));
			})
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0,0,12,0)
			[Action(TEXT("Advance phase  →"), [Weak]() { if (Weak.IsValid()) Weak->AdvancePhase(); }, true)]
			+ SHorizontalBox::Slot().AutoWidth()
			[Action(TEXT("Request failure"), [Weak]() { if (Weak.IsValid()) Weak->RequestPhase(EMissionPhase::Failed); })]
		];

	auto Snapshot = SNew(SVerticalBox);
	Snapshot->AddSlot().AutoHeight().Padding(0,0,0,24)[Label(TEXT("SPACECRAFT / POWER"), 11, Accent, true)];
	const TCHAR* MetricNames[] = {TEXT("DRY MASS"), TEXT("AVAILABLE SOLAR"), TEXT("NET POWER"), TEXT("STORED ENERGY")};
	for (int32 Index = 0; Index < 4; ++Index)
	{
		Snapshot->AddSlot().AutoHeight().Padding(0,0,0,6)[Label(MetricNames[Index], 10)];
		Snapshot->AddSlot().AutoHeight().Padding(0,0,0,12)
		[
			SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 22)).ColorAndOpacity(White)
			.Text_Lambda([Weak, Index]()
			{
				if (!Weak.IsValid()) return FText::GetEmpty();
				const auto& EPS = Weak->GetEPS();
				switch (Index)
				{
				case 0: return FText::FromString(FString::Printf(TEXT("%.0f kg"), Weak->GetSpacecraftConfiguration().TotalDryMassKg()));
				case 1: return FText::FromString(FString::Printf(TEXT("%.0f W"), EPS.AvailableGenerationW()));
				case 2: return FText::FromString(FString::Printf(TEXT("%+.0f W"), EPS.NetPowerW()));
				default: return FText::FromString(FString::Printf(TEXT("%.1f / %.0f Wh"), EPS.StoredEnergyWh(), EPS.CapacityWh()));
				}
			})
		];
	}
	Snapshot->AddSlot().AutoHeight().Padding(0,0,0,10)
		[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 11)).ColorAndOpacity(Warning).AutoWrapText(true)
		.Text_Lambda([Weak]()
		{
			if (!Weak.IsValid()) return FText::GetEmpty();
			const auto& EPS = Weak->GetEPS();
			return FText::FromString(FString::Printf(TEXT("%s  /  EPS time %.0f s\nIllustrative hardware · manual illumination"),
				EPS.GetIllumination() == EEPSIllumination::Sunlight ? TEXT("SUNLIGHT") : TEXT("ECLIPSE"), EPS.ElapsedSeconds()));
		})];
	Snapshot->AddSlot().AutoHeight()[Action(TEXT("Open electrical power  →"), [Weak]() { if (Weak.IsValid()) Weak->ShowEPS(); })];

	ChildSlot
	[
		SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Background).Padding(0)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[SNew(SBox).WidthOverride(270)[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(Ink(TEXT("0F1721"))).Padding(24, 36)[Navigation]]]
			+ SHorizontalBox::Slot().FillWidth(1).Padding(40, 32)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)[Label(TEXT("OPERATIONS / OVERVIEW"), 11, Accent, true)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,28)[Label(TEXT("Mission Control"), 30, White, true)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,20)[Surface(Hero)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)[Label(TEXT("MISSION LIFECYCLE"), 11, Muted, true)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,24)[Lifecycle]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1).Padding(0,0,20,0)
						[Surface(SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)[Label(TEXT("MISSION EVENT LOG"), 11, Accent, true)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,24)[Label(TEXT("Accepted and rejected requests / latest 100"), 11)]
							+ SVerticalBox::Slot().AutoHeight()
							[SNew(SBox).HeightOverride(260)[SNew(SScrollBox) + SScrollBox::Slot()
								[SNew(STextBlock).Text_Lambda([Weak]() { return Weak.IsValid() ? Weak->LogText() : FText::GetEmpty(); })
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12)).ColorAndOpacity(White).LineHeightPercentage(1.7f).AutoWrapText(true)]]]
							)
						]
						+ SHorizontalBox::Slot().FillWidth(0.55f)[Surface(Snapshot)]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0,20,0,0)
					[Label(TEXT("Manual lifecycle controls · Readiness is not evaluated automatically · EPS advances only on command"), 11)]
				]
			]
		]
	];
}
