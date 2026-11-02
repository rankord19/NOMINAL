#include "Spacecraft/SSpacecraftDesigner.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

namespace
{
FString Specifications(const FSpacecraftModule& M)
{
	FString Result = FString::Printf(TEXT("%s\n%.0f kg"), *M.Id.ToString(), M.MassKg);
	switch (M.Category)
	{
	case ESpacecraftCategory::Power: return Result + FString::Printf(TEXT("  |  %.0f W rated"), M.RatedPowerW);
	case ESpacecraftCategory::Communications: return Result + FString::Printf(TEXT("  |  %.1f m dish diameter"), M.AntennaDiameterM);
	case ESpacecraftCategory::Propulsion: return Result + FString::Printf(TEXT("  |  %.0f N  |  %.0f s Isp"), M.ThrustN, M.SpecificImpulseS);
	default: return Result;
	}
}
}

void SSpacecraftDesigner::Construct(const FArguments& Args)
{
	Configuration = Args._Configuration;
	Install = Args._Install;
	SelectedSlot = FSpacecraftCatalog::Slots()[0].Id;
	TSharedRef<SVerticalBox> Categories = SNew(SVerticalBox);
	for (const auto& Slot : FSpacecraftCatalog::Slots())
	{
		Categories->AddSlot().AutoHeight().Padding(0, 2)
		[ SNew(SButton).Text_Lambda([this, Slot]() { return FText::FromString((SelectedSlot == Slot.Id ? TEXT(">  ") : TEXT("    ")) + Slot.Name); })
			.OnClicked_Lambda([this, Id = Slot.Id]() { SelectedSlot = Id; RefreshOptions(); return FReply::Handled(); }) ];
	}
	ChildSlot
	[
		SNew(SBorder).Padding(20).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.015f, 0.025f, 0.04f, 1))
		[
			SNew(SScrollBox) + SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)
				[ SNew(STextBlock).Text(FText::FromString(TEXT("SPACECRAFT DESIGNER"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 20)) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,16)
				[ SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(TEXT("ILLUSTRATIVE PROTOTYPE DATA\nTemporary geometry, not validated hardware. Static design preview; no in-flight refit simulation."))).ColorAndOpacity(FLinearColor(.95f,.7f,.3f)) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,16)
				[ SNew(STextBlock).Text(this, &SSpacecraftDesigner::Summary).AutoWrapText(true) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12) [ Categories ]
				+ SVerticalBox::Slot().AutoHeight() [ SAssignNew(Options, SVerticalBox) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0,16,0,0)
				[ SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(Status); }).AutoWrapText(true).ColorAndOpacity(FLinearColor(.2f,.8f,.9f)) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0,12,0,0)
				[ SNew(STextBlock).Text(FText::FromString(TEXT("Changes persist during this Play session. Restarting Play restores defaults."))).AutoWrapText(true) ]
			]
		]
	];
	RefreshOptions();
}

FText SSpacecraftDesigner::Summary() const
{
	FString Result = FString::Printf(TEXT("BASE BUS  |  %.0f kg  |  %.0f W\nTOTAL DRY MASS  %.0f kg\nRATED GENERATION  %.0f W\n"),
		FSpacecraftCatalog::BusMassKg, FSpacecraftCatalog::BusRatedPowerW, Configuration->TotalDryMassKg(), Configuration->TotalRatedPowerW());
	for (const auto& Slot : FSpacecraftCatalog::Slots())
		if (const auto* M = Configuration->Installed(Slot.Id)) Result += TEXT("\n") + Slot.Name + TEXT(": ") + M->Name;
	return FText::FromString(Result);
}

void SSpacecraftDesigner::RefreshOptions()
{
	Options->ClearChildren();
	const auto* Slot = FSpacecraftCatalog::FindSlot(SelectedSlot);
	for (const auto& M : FSpacecraftCatalog::Modules())
	{
		if (!Slot || M.Category != Slot->Category) continue;
		Options->AddSlot().AutoHeight().Padding(0,6)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight() [ SNew(STextBlock).Text(FText::FromString(M.Name)).Font(FCoreStyle::GetDefaultFontStyle("Bold", 12)) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0,4) [ SNew(STextBlock).Text(FText::FromString(Specifications(M))).AutoWrapText(true) ]
			+ SVerticalBox::Slot().AutoHeight()
			[ SNew(SButton)
				.Text_Lambda([this, Id=M.Id]() { return FText::FromString(Configuration->Installed(SelectedSlot)->Id == Id ? TEXT("Installed") : TEXT("Install / replace")); })
				.IsEnabled_Lambda([this, Id=M.Id]() { return Configuration->Installed(SelectedSlot)->Id != Id; })
				.OnClicked_Lambda([this, Id=M.Id]() { Status = Install(SelectedSlot, Id); return FReply::Handled(); }) ]
		];
	}
}
