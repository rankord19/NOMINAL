#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "LunarCameraPawn.generated.h"

/** Presentation camera only. No orbital state, time advancement, or spacecraft motion. */
UCLASS()
class NOMINAL_API ALunarCameraPawn : public APawn
{
	GENERATED_BODY()
public:
	ALunarCameraPawn();
	virtual void Tick(float DeltaSeconds) override;
	void Orbit(double X, double Y);
	void Pan(double X, double Y);
	void Zoom(double Steps);
	void FocusMoon(bool bMoon);
private:
	UPROPERTY() TObjectPtr<class UCameraComponent> Camera;
	bool bMoonFocus = false;
	double Yaw = -58.0;
	double Pitch = 8.0;
	double Distance = 2200.0;
	FVector PanOffset = FVector::ZeroVector;
};
