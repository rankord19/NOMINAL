#pragma once
#include "CoreMinimal.h"

enum class ESpacecraftCategory : uint8 { Power, Communications, Propulsion };

/** Illustrative SI specifications, not validated mission hardware. */
struct NOMINAL_API FSpacecraftModule
{
	FName Id;
	FString Name;
	ESpacecraftCategory Category;
	double MassKg = 0;
	double RatedPowerW = 0;
	double AntennaDiameterM = 0;
	double ThrustN = 0;
	double SpecificImpulseS = 0;
	bool IsValid() const;
};

struct NOMINAL_API FSpacecraftSlot
{
	FName Id;
	FString Name;
	ESpacecraftCategory Category;
	FName DefaultModule;
};

/** Immutable catalog, independent of Slate and visual assets. */
class NOMINAL_API FSpacecraftCatalog
{
public:
	static const TArray<FSpacecraftModule>& Modules();
	static const TArray<FSpacecraftSlot>& Slots();
	static const FSpacecraftModule* FindModule(FName Id);
	static const FSpacecraftSlot* FindSlot(FName Id);
	static constexpr double BusMassKg = 300.0;
	static constexpr double BusRatedPowerW = 0.0;
};

/** Value-owned installed IDs; copies and independent instances do not share mutable state. */
class NOMINAL_API FSpacecraftConfiguration
{
public:
	FSpacecraftConfiguration();
	bool TryInstall(FName SlotId, FName ModuleId, FString& OutReason);
	const FSpacecraftModule* Installed(FName SlotId) const;
	double TotalDryMassKg() const;
	double TotalRatedPowerW() const;
private:
	TMap<FName, FName> InstalledModules;
};
