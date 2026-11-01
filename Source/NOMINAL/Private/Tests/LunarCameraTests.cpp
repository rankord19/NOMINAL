#include "Mission/LunarCameraPawn.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLunarCameraTest, "NOMINAL.LunarView.CameraBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLunarCameraTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	ALunarCameraPawn* Pawn = World->SpawnActor<ALunarCameraPawn>();
	if (!TestNotNull(TEXT("Camera pawn can spawn"), Pawn)) { World->DestroyWorld(false); return false; }
	const FVector Center(0, 0, -183740000.0);
	for (bool Moon : {true, false})
	{
		Pawn->FocusMoon(Moon);
		for (int32 Frame = 0; Frame < 360; ++Frame)
		{
			Pawn->Tick(1.f / 60.f);
			TestFalse(TEXT("Camera position stays finite"), Pawn->GetActorLocation().ContainsNaN());
			TestTrue(TEXT("Focus transition stays above lunar surface"), FVector::Dist(Pawn->GetActorLocation(), Center) >= 173740000.0);
		}
		TestTrue(TEXT("Camera reaches selected focus range"), Moon
			? FVector::Dist(Pawn->GetActorLocation(), Center) > 173740000.0 * 2.0
			: FMath::IsNearlyEqual(Pawn->GetActorLocation().Size(), 2200.0, 1.0));
	}
	Pawn->Zoom(1000);
	Pawn->Pan(100000, 100000);
	Pawn->Orbit(100000, 100000);
	for (int32 Frame = 0; Frame < 240; ++Frame) Pawn->Tick(1.f / 60.f);
	TestTrue(TEXT("Extreme input remains bounded near spacecraft"), Pawn->GetActorLocation().Size() <= 1700.0);
	World->DestroyWorld(false);
	return true;
}
#endif
