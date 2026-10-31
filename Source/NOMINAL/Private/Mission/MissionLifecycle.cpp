#include "Mission/MissionLifecycle.h"

bool FMissionLifecycle::IsTerminal() const
{
	return Phase == EMissionPhase::Completed || Phase == EMissionPhase::Failed;
}

bool FMissionLifecycle::CanTransitionTo(EMissionPhase NextPhase) const
{
	if (IsTerminal())
	{
		return false;
	}

	// This records an externally determined outcome; no subsystem fault implies failure here.
	if (NextPhase == EMissionPhase::Failed)
	{
		return true;
	}

	switch (Phase)
	{
	case EMissionPhase::Planning:
		return NextPhase == EMissionPhase::SpacecraftDesign;
	case EMissionPhase::SpacecraftDesign:
		return NextPhase == EMissionPhase::LaunchPreparation;
	case EMissionPhase::LaunchPreparation:
		return NextPhase == EMissionPhase::Launch;
	case EMissionPhase::Launch:
		return NextPhase == EMissionPhase::FlightOperations;
	case EMissionPhase::FlightOperations:
		return NextPhase == EMissionPhase::ScienceOperations || NextPhase == EMissionPhase::Completed;
	case EMissionPhase::ScienceOperations:
		return NextPhase == EMissionPhase::FlightOperations || NextPhase == EMissionPhase::Completed;
	default:
		return false;
	}
}

bool FMissionLifecycle::TryTransitionTo(EMissionPhase NextPhase)
{
	if (!CanTransitionTo(NextPhase))
	{
		return false;
	}

	Phase = NextPhase;
	return true;
}
