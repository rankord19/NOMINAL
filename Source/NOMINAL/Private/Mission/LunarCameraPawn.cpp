#include "Mission/LunarCameraPawn.h"
#include "Camera/CameraComponent.h"

namespace
{
constexpr double MoonRadius = 173740000.0; // cm; NASA LOLA reference sphere, 1737.4 km.
const FVector MoonCenter(0, 0, -183740000.0); // Static visual placement, NOT a mission orbit.
}

ALunarCameraPawn::ALunarCameraPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	SetRootComponent(Camera);
	Camera->FieldOfView = 55.f;
	SetActorLocation(FVector(1161, -1859, 192));
	SetActorRotation((-GetActorLocation()).Rotation());
}

void ALunarCameraPawn::Orbit(double X, double Y)
{
	Yaw = FMath::UnwindDegrees(Yaw + X * 0.2);
	Pitch = FMath::Clamp(Pitch + Y * 0.2, -75.0, 75.0);
}

void ALunarCameraPawn::Pan(double X, double Y)
{
	PanOffset += (-GetActorRightVector() * X + GetActorUpVector() * Y) * Distance * 0.001;
	PanOffset = PanOffset.GetClampedToMaxSize(bMoonFocus ? MoonRadius * 0.2 : 1000.0);
}

void ALunarCameraPawn::Zoom(double Steps)
{
	Distance = FMath::Clamp(Distance * FMath::Exp(-Steps * 0.15),
		bMoonFocus ? MoonRadius * 2.0 : 600.0, bMoonFocus ? MoonRadius * 8.0 : 6000.0);
}

void ALunarCameraPawn::FocusMoon(bool bMoon)
{
	bMoonFocus = bMoon;
	PanOffset = FVector::ZeroVector;
	Yaw = bMoon ? -20.0 : -58.0;
	Pitch = bMoon ? 35.0 : 8.0;
	Distance = bMoon ? MoonRadius * 4.5 : 2200.0;
}

void ALunarCameraPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const FVector Target = (bMoonFocus ? MoonCenter : FVector::ZeroVector) + PanOffset;
	FVector Desired = Target + FRotator(Pitch, Yaw, 0).Vector() * Distance;
	// Keep the presentation camera outside the lunar surface even while changing focus.
	const FVector FromMoon = Desired - MoonCenter;
	if (FromMoon.Size() < MoonRadius + 2000000.0)
		Desired = MoonCenter + FromMoon.GetSafeNormal() * (MoonRadius + 2000000.0);
	const double Alpha = 1.0 - FMath::Exp(-5.0 * FMath::Min(DeltaSeconds, 0.1f));
	FVector Position = FMath::Lerp(GetActorLocation(), Desired, Alpha);
	const FVector Offset = Position - MoonCenter;
	if (Offset.Size() < MoonRadius + 2000000.0)
		Position = MoonCenter + Offset.GetSafeNormal() * (MoonRadius + 2000000.0);
	SetActorLocation(Position);
	SetActorRotation(FQuat::Slerp(GetActorQuat(), (Target - Position).Rotation().Quaternion(), Alpha));
}
