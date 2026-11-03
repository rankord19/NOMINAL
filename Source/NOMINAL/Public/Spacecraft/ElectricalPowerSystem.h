#pragma once
#include "CoreMinimal.h"
#include "Spacecraft/SpacecraftConfiguration.h"

enum class EEPSIllumination : uint8 { Sunlight, Eclipse };

/** Array order defines shedding priority. Values are illustrative constant-power loads. */
struct NOMINAL_API FEPSLoad
{
	FName Id;
	FString Name;
	double Watts;
	bool bCritical;
	bool bRequested = true;
	bool bShed = false;
	bool IsPowered() const { return bRequested && !bShed; }
};

/** Explicit-time ideal energy model. No world, tick, rendering, UI or mission outcome dependency. */
class NOMINAL_API FElectricalPowerSystem
{
public:
	explicit FElectricalPowerSystem(const FSpacecraftConfiguration& Configuration);
	void SyncConfiguration(const FSpacecraftConfiguration& Configuration);
	bool SetIllumination(EEPSIllumination Condition);
	bool SetLoadEnabled(FName Id, bool bEnabled);
	bool SetLoadWatts(FName Id, double Watts);
	void RestoreRequestedLoads();
	bool Advance(double Seconds);
	const TArray<FEPSLoad>& GetLoads() const { return Loads; }
	EEPSIllumination GetIllumination() const { return Illumination; }
	double RatedGenerationW() const { return RatedW; }
	double AvailableGenerationW() const { return Illumination == EEPSIllumination::Sunlight ? RatedW : 0; }
	double RequestedConsumptionW() const;
	double ActiveConsumptionW() const;
	double NetPowerW() const { return AvailableGenerationW() - ActiveConsumptionW(); }
	double CapacityWh() const { return Capacity; }
	double StoredEnergyWh() const { return Energy; }
	double ChargeFraction() const { return Capacity > 0 ? Energy / Capacity : 0; }
	double ElapsedSeconds() const { return Elapsed; }
	bool HasShedLoads() const;
	bool IsLowEnergy() const { return ChargeFraction() <= 0.2; }
private:
	void ShedAtEmptyBattery();
	TArray<FEPSLoad> Loads;
	EEPSIllumination Illumination = EEPSIllumination::Sunlight;
	double RatedW = 0;
	double Capacity = 0;
	double Energy = 0;
	double Elapsed = 0;
};
