#include "Spacecraft/SpacecraftConfiguration.h"

bool FSpacecraftModule::IsValid() const
{
	for (double Value : {MassKg, RatedPowerW, AntennaDiameterM, ThrustN, SpecificImpulseS})
		if (!FMath::IsFinite(Value) || Value < 0) return false;
	if (Id.IsNone() || Name.IsEmpty() || MassKg <= 0) return false;
	switch (Category)
	{
	case ESpacecraftCategory::Power: return RatedPowerW > 0;
	case ESpacecraftCategory::Communications: return AntennaDiameterM > 0;
	case ESpacecraftCategory::Propulsion: return ThrustN > 0 && SpecificImpulseS > 0;
	default: return false;
	}
}

const TArray<FSpacecraftModule>& FSpacecraftCatalog::Modules()
{
	// Deliberately illustrative, not a mission specification or engineering certification.
	static const TArray<FSpacecraftModule> Data = {
		{TEXT("power.wing6"), TEXT("Six-panel wings"), ESpacecraftCategory::Power, 60, 1200},
		{TEXT("power.wing2"), TEXT("Compact two-panel wings"), ESpacecraftCategory::Power, 25, 400},
		{TEXT("comms.dish12"), TEXT("1.2 m dish assembly"), ESpacecraftCategory::Communications, 18, 0, 1.2},
		{TEXT("comms.dish06"), TEXT("0.6 m dish assembly"), ESpacecraftCategory::Communications, 8, 0, 0.6},
		{TEXT("propulsion.standard"), TEXT("Standard engine assembly"), ESpacecraftCategory::Propulsion, 30, 0, 0, 400, 300},
		{TEXT("propulsion.compact"), TEXT("Compact engine assembly"), ESpacecraftCategory::Propulsion, 14, 0, 0, 100, 280}
	};
	return Data;
}

const TArray<FSpacecraftSlot>& FSpacecraftCatalog::Slots()
{
	static const TArray<FSpacecraftSlot> Data = {
		{TEXT("power"), TEXT("Power generation"), ESpacecraftCategory::Power, TEXT("power.wing6")},
		{TEXT("communications"), TEXT("Communications"), ESpacecraftCategory::Communications, TEXT("comms.dish12")},
		{TEXT("propulsion"), TEXT("Propulsion"), ESpacecraftCategory::Propulsion, TEXT("propulsion.standard")}
	};
	return Data;
}

const FSpacecraftModule* FSpacecraftCatalog::FindModule(FName Id)
{
	return Modules().FindByPredicate([Id](const auto& Module) { return Module.Id == Id; });
}

const FSpacecraftSlot* FSpacecraftCatalog::FindSlot(FName Id)
{
	return Slots().FindByPredicate([Id](const auto& Slot) { return Slot.Id == Id; });
}

FSpacecraftConfiguration::FSpacecraftConfiguration()
{
	for (const auto& Slot : FSpacecraftCatalog::Slots())
	{
		FString Reason;
		const bool bInstalled = TryInstall(Slot.Id, Slot.DefaultModule, Reason);
		checkf(bInstalled, TEXT("Invalid default spacecraft catalog: %s"), *Reason);
	}
}

bool FSpacecraftConfiguration::TryInstall(FName SlotId, FName ModuleId, FString& OutReason)
{
	const auto* Slot = FSpacecraftCatalog::FindSlot(SlotId);
	const auto* Module = FSpacecraftCatalog::FindModule(ModuleId);
	if (!Slot) OutReason = TEXT("Unknown attachment slot.");
	else if (!Module) OutReason = TEXT("Unknown module identifier.");
	else if (!Module->IsValid()) OutReason = TEXT("Invalid module specifications.");
	else if (Slot->Category != Module->Category) OutReason = TEXT("Module category is incompatible with this slot.");
	else
	{
		InstalledModules.Add(SlotId, ModuleId);
		OutReason = TEXT("Installed: ") + Module->Name;
		return true;
	}
	return false; // Failed requests never mutate installed state.
}

const FSpacecraftModule* FSpacecraftConfiguration::Installed(FName SlotId) const
{
	const FName* Id = InstalledModules.Find(SlotId);
	return Id ? FSpacecraftCatalog::FindModule(*Id) : nullptr;
}

double FSpacecraftConfiguration::TotalDryMassKg() const
{
	double Total = FSpacecraftCatalog::BusMassKg;
	for (const auto& Slot : FSpacecraftCatalog::Slots()) if (const auto* M = Installed(Slot.Id)) Total += M->MassKg;
	return Total;
}

double FSpacecraftConfiguration::TotalRatedPowerW() const
{
	double Total = FSpacecraftCatalog::BusRatedPowerW;
	for (const auto& Slot : FSpacecraftCatalog::Slots()) if (const auto* M = Installed(Slot.Id)) Total += M->RatedPowerW;
	return Total;
}
