#include "Spacecraft/ElectricalPowerSystem.h"

FElectricalPowerSystem::FElectricalPowerSystem(const FSpacecraftConfiguration& Configuration)
{
	Loads = {
		{TEXT("avionics"), TEXT("Essential avionics"), 100, true},
		{TEXT("transmitter"), TEXT("Communications transmitter"), 250, false},
		{TEXT("instrument"), TEXT("Illustrative science instrument"), 500, false}
	};
	RatedW = Configuration.TotalRatedPowerW();
	Capacity = Configuration.TotalBatteryCapacityWh();
	Energy = Capacity * 0.5; // Initial test condition only; never applied on module replacement.
}

void FElectricalPowerSystem::SyncConfiguration(const FSpacecraftConfiguration& Configuration)
{
	RatedW = Configuration.TotalRatedPowerW();
	Capacity = Configuration.TotalBatteryCapacityWh();
	Energy = FMath::Min(Energy, Capacity); // Absolute Wh preserved; removed capacity discards excess energy.
	ShedAtEmptyBattery();
}

bool FElectricalPowerSystem::SetIllumination(EEPSIllumination Condition)
{
	if (Condition != EEPSIllumination::Sunlight && Condition != EEPSIllumination::Eclipse) return false;
	Illumination = Condition;
	ShedAtEmptyBattery();
	return true;
}

bool FElectricalPowerSystem::SetLoadEnabled(FName Id, bool bEnabled)
{
	FEPSLoad* Load = Loads.FindByPredicate([Id](const auto& L) { return L.Id == Id; });
	if (!Load || Load->bCritical) return false;
	Load->bRequested = bEnabled;
	// Switching demand is not a protection reset. Recovery is explicit via RestoreRequestedLoads.
	ShedAtEmptyBattery();
	return true;
}

bool FElectricalPowerSystem::SetLoadWatts(FName Id, double Watts)
{
	FEPSLoad* Load = Loads.FindByPredicate([Id](const auto& L) { return L.Id == Id; });
	if (!Load || !FMath::IsFinite(Watts) || Watts <= 0) return false;
	double Total = Watts;
	for (const auto& L : Loads) if (L.Id != Id) Total += L.Watts;
	if (!FMath::IsFinite(Total)) return false;
	Load->Watts = Watts;
	ShedAtEmptyBattery();
	return true;
}

void FElectricalPowerSystem::RestoreRequestedLoads()
{
	for (auto& Load : Loads) Load.bShed = false;
	ShedAtEmptyBattery(); // Restoration cannot power unsupported loads at zero energy.
}

double FElectricalPowerSystem::RequestedConsumptionW() const
{
	double Total = 0;
	for (const auto& Load : Loads) if (Load.bRequested) Total += Load.Watts;
	return Total;
}

double FElectricalPowerSystem::ActiveConsumptionW() const
{
	double Total = 0;
	for (const auto& Load : Loads) if (Load.IsPowered()) Total += Load.Watts;
	return Total;
}

bool FElectricalPowerSystem::HasShedLoads() const
{
	return Loads.ContainsByPredicate([](const auto& Load) { return Load.bRequested && Load.bShed; });
}

void FElectricalPowerSystem::ShedAtEmptyBattery()
{
	if (Energy > 0) return;
	double RemainingW = AvailableGenerationW();
	bool CriticalUnavailable = false;
	for (auto& Load : Loads)
	{
		if (!Load.bRequested) continue;
		if (CriticalUnavailable || Load.Watts > RemainingW) Load.bShed = true;
		if (Load.IsPowered()) RemainingW -= Load.Watts;
		if (Load.bCritical && !Load.IsPowered()) CriticalUnavailable = true;
	}
}

bool FElectricalPowerSystem::Advance(double Seconds)
{
	if (!FMath::IsFinite(Seconds) || Seconds < 0 || !FMath::IsFinite(Elapsed + Seconds)) return false;
	if (Seconds == 0) return true;
	Elapsed += Seconds;
	double RemainingSeconds = Seconds;
	double NetW = NetPowerW();
	if (NetW < 0)
	{
		const double SecondsToEmpty = (Energy / -NetW) * 3600.0;
		if (RemainingSeconds + 1e-9 < SecondsToEmpty)
		{
			Energy = FMath::Max(0.0, Energy + NetW * (RemainingSeconds / 3600.0));
			return true;
		}
		// Resolve depletion at its exact event time, then integrate the remaining interval
		// with the shed loads. No per-call reset or frame-sized approximation.
		Energy = 0;
		RemainingSeconds = FMath::Max(0.0, RemainingSeconds - SecondsToEmpty);
		ShedAtEmptyBattery();
		NetW = NetPowerW();
	}
	if (NetW > 0)
	{
		const double SecondsToFull = ((Capacity - Energy) / NetW) * 3600.0;
		Energy = RemainingSeconds >= SecondsToFull ? Capacity : Energy + NetW * (RemainingSeconds / 3600.0);
	}
	return true;
}
