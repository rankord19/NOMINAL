#pragma once

#include "CoreTypes.h"

/** Coarse lifecycle only; orbital events and scientific outcomes belong to other systems. */
enum class EMissionPhase : uint8
{
	Planning,
	SpacecraftDesign,
	LaunchPreparation,
	Launch,
	FlightOperations,
	ScienceOperations,
	Completed,
	Failed
};

/**
 * Explicit, caller-driven lifecycle for one mission. No ticking or gameplay dependencies.
 * The caller must evaluate readiness and outcomes before requesting a transition.
 * Completed means operations ended, not that every scientific objective succeeded.
 * Failed must only be requested after the caller has ruled out viable alternatives.
 */
class NOMINAL_API FMissionLifecycle
{
public:
	EMissionPhase GetPhase() const { return Phase; }
	bool IsTerminal() const;

	/** Structural validation only. Invalid enum values and same-phase requests are rejected. */
	bool CanTransitionTo(EMissionPhase NextPhase) const;

	/** Returns false without changing state when the transition is not allowed. */
	bool TryTransitionTo(EMissionPhase NextPhase);

private:
	EMissionPhase Phase = EMissionPhase::Planning;
};
