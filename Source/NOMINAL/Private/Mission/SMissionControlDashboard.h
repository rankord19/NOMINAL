#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Mission/MissionLifecycle.h"

class AMissionControlGameMode;

/** Presentation only: all commands and values belong to the existing session. */
class SMissionControlDashboard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMissionControlDashboard) {}
		SLATE_ARGUMENT(AMissionControlGameMode*, Session)
		SLATE_ATTRIBUTE(EMissionPhase, Phase)
	SLATE_END_ARGS()
	void Construct(const FArguments& Args);
private:
	TWeakObjectPtr<AMissionControlGameMode> Session;
	TAttribute<EMissionPhase> Phase;
};
