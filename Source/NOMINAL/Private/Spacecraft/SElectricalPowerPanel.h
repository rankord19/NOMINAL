#pragma once
#include "Widgets/SCompoundWidget.h"
#include "Spacecraft/ElectricalPowerSystem.h"

/** Displays the session's actual model; no simulated state or time integration in Slate. */
class SElectricalPowerPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SElectricalPowerPanel) {}
		SLATE_ARGUMENT(FElectricalPowerSystem*, Model)
		SLATE_ARGUMENT(const FSpacecraftConfiguration*, Configuration)
	SLATE_END_ARGS()
	void Construct(const FArguments& Args);
private:
	FText Summary() const;
	FText Alerts() const;
	FText LoadText(FName Id) const;
	FElectricalPowerSystem* Model = nullptr;
	const FSpacecraftConfiguration* Configuration = nullptr;
	FString LastStep = TEXT("Paused. Only Advance buttons change stored energy.");
};
