#include "Spacecraft/SElectricalPowerPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

void SElectricalPowerPanel::Construct(const FArguments& Args)
{
	Model = Args._Model;
	Configuration = Args._Configuration;
	auto LoadRows = SNew(SVerticalBox);
	for (const auto& Load : Model->GetLoads())
	{
		LoadRows->AddSlot().AutoHeight().Padding(0, 5)
		[ SNew(STextBlock).Text_Lambda([this, Id=Load.Id]() { return LoadText(Id); }).AutoWrapText(true) ];
		if (!Load.bCritical)
			LoadRows->AddSlot().AutoHeight()
			[ SNew(SButton).Text_Lambda([this, Id=Load.Id]()
				{
					const auto* L = Model->GetLoads().FindByPredicate([Id](const auto& Item) { return Item.Id == Id; });
					return FText::FromString(L->bRequested ? TEXT("Disable demand") : TEXT("Enable demand"));
				})
				.OnClicked_Lambda([this, Id=Load.Id]()
				{
					const auto* L = Model->GetLoads().FindByPredicate([Id](const auto& Item) { return Item.Id == Id; });
					Model->SetLoadEnabled(Id, !L->bRequested);
					return FReply::Handled();
				}) ];
	}
	auto TimeButtons = SNew(SHorizontalBox);
	for (double Seconds : {60.0, 600.0, 3600.0})
		TimeButtons->AddSlot().AutoWidth().Padding(0,0,8,0)
		[ SNew(SButton).Text(FText::FromString(FString::Printf(TEXT("Advance %.0f s"), Seconds)))
			.OnClicked_Lambda([this, Seconds]()
			{
				const double Before = Model->StoredEnergyWh();
				if (Model->Advance(Seconds)) LastStep = FString::Printf(TEXT("Last step: %.0f s | %.2f -> %.2f Wh"), Seconds, Before, Model->StoredEnergyWh());
				return FReply::Handled();
			}) ];
	ChildSlot
	[
		SNew(SBorder).Padding(20).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(.015f,.025f,.04f,1))
		[
			SNew(SScrollBox) + SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,10)
				[ SNew(STextBlock).Text(FText::FromString(TEXT("ELECTRICAL POWER SYSTEM"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 20)) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)
				[ SNew(STextBlock).AutoWrapText(true).ColorAndOpacity(FLinearColor(.95f,.7f,.3f))
					.Text(FText::FromString(TEXT("ILLUSTRATIVE PROTOTYPE DATA\nSunlight/eclipse are manual test conditions, independent of the static lunar scene."))) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)
				[ SNew(STextBlock).Text(this, &SElectricalPowerPanel::Summary).AutoWrapText(true) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,10)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)
					[ SNew(SButton).Text(FText::FromString(TEXT("Full sunlight"))).OnClicked_Lambda([this]() { Model->SetIllumination(EEPSIllumination::Sunlight); return FReply::Handled(); }) ]
					+ SHorizontalBox::Slot().AutoWidth()
					[ SNew(SButton).Text(FText::FromString(TEXT("Eclipse / no sunlight"))).OnClicked_Lambda([this]() { Model->SetIllumination(EEPSIllumination::Eclipse); return FReply::Handled(); }) ]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12) [ LoadRows ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8) [ TimeButtons ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,10)
				[ SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(LastStep); }).AutoWrapText(true) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,10)
				[ SNew(STextBlock).Text(this, &SElectricalPowerPanel::Alerts).AutoWrapText(true).ColorAndOpacity(FLinearColor(.95f,.7f,.3f)) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,10)
				[ SNew(SButton).Text(FText::FromString(TEXT("Restore requested loads"))).IsEnabled_Lambda([this]() { return Model->HasShedLoads(); })
					.OnClicked_Lambda([this]() { Model->RestoreRequestedLoads(); return FReply::Handled(); }) ]
				+ SVerticalBox::Slot().AutoHeight()
				[ SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(TEXT("Priority: avionics > transmitter > instrument. Shed loads stay disconnected until restored. Power loss does not declare mission failure. No time passes while switching views."))) ]
			]
		]
	];
}

FText SElectricalPowerPanel::Summary() const
{
	return FText::FromString(FString::Printf(TEXT("SOLAR  %s\nRATED CAPACITY  %.0f W\nTEST CONDITION  %s\nAVAILABLE GENERATION  %.0f W\nREQUESTED DEMAND  %.0f W\nACTIVE CONSUMPTION  %.0f W\nNET POWER BALANCE  %+.0f W\nBATTERY  %.2f / %.0f Wh  (%.1f%%)\nDRY MASS  %.0f kg  |  EPS TIME  %.0f s"),
		*Configuration->Installed(TEXT("power"))->Name, Model->RatedGenerationW(),
		Model->GetIllumination() == EEPSIllumination::Sunlight ? TEXT("FULL SUNLIGHT") : TEXT("ECLIPSE"),
		Model->AvailableGenerationW(), Model->RequestedConsumptionW(), Model->ActiveConsumptionW(), Model->NetPowerW(),
		Model->StoredEnergyWh(), Model->CapacityWh(), Model->ChargeFraction()*100, Configuration->TotalDryMassKg(), Model->ElapsedSeconds()));
}

FText SElectricalPowerPanel::LoadText(FName Id) const
{
	const auto* L = Model->GetLoads().FindByPredicate([Id](const auto& Item) { return Item.Id == Id; });
	const TCHAR* Status = !L->bRequested ? TEXT("OFF (commanded)") : L->bShed ? TEXT("UNAVAILABLE (shed)") : TEXT("POWERED");
	return FText::FromString(FString::Printf(TEXT("%s | %.0f W%s\n%s"), *L->Name, L->Watts, L->bCritical ? TEXT(" | CRITICAL / always requested") : TEXT(""), Status));
}

FText SElectricalPowerPanel::Alerts() const
{
	FString Text;
	if (Model->StoredEnergyWh() <= 0) Text += TEXT("BATTERY DEPLETED.\n");
	else if (Model->IsLowEnergy()) Text += TEXT("LOW ENERGY: at or below 20% usable capacity.\n");
	if (Model->HasShedLoads()) Text += TEXT("INSUFFICIENT POWER / LOADS SHED. Restore after correcting supply or demand.\n");
	if (Model->NetPowerW() < 0) Text += TEXT("DEFICIT: battery supplies the active loads.");
	else if (Model->NetPowerW() > 0) Text += Model->ChargeFraction() >= 1 ? TEXT("BATTERY FULL: surplus generation is curtailed.") : TEXT("SURPLUS: battery will charge on Advance.");
	else Text += TEXT("BALANCED: stored energy will not change.");
	return FText::FromString(Text);
}
