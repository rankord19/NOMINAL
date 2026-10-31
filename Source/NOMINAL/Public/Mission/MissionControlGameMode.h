#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Mission/MissionLifecycle.h"
#include "MissionControlGameMode.generated.h"

class SWidget;

/** One local prototype session. No persistence or scientific outcome evaluation. */
UCLASS()
class NOMINAL_API AMissionControlGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	AMissionControlGameMode();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	bool RequestPhase(EMissionPhase Next);
	void AdvancePhase();
	FText PhaseText() const;
	FText LogText() const;
private:
	FMissionLifecycle Lifecycle;
	TArray<FString> Entries;
	TSharedPtr<SWidget> ControlWidget;
};
