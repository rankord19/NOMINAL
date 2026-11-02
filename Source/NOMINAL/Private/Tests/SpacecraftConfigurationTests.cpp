#include "Spacecraft/SpacecraftConfiguration.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Mission/MissionControlGameMode.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpacecraftConfigurationTest, "NOMINAL.Spacecraft.Configuration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSpacecraftConfigurationTest::RunTest(const FString& Parameters)
{
	FSpacecraftConfiguration A, B;
	FString Reason;
	TestEqual(TEXT("Default dry mass includes bus and all modules"), A.TotalDryMassKg(), 408.0);
	TestEqual(TEXT("Default power"), A.TotalRatedPowerW(), 1200.0);
	TSet<FName> Ids;
	for (const auto& Module : FSpacecraftCatalog::Modules())
	{
		TestTrue(TEXT("Catalog has valid SI data"), Module.IsValid());
		TestFalse(TEXT("Stable module identifiers are unique"), Ids.Contains(Module.Id));
		Ids.Add(Module.Id);
		for (const auto& Slot : FSpacecraftCatalog::Slots())
		{
			FSpacecraftConfiguration Candidate;
			const FName Before = Candidate.Installed(Slot.Id)->Id;
			const bool Expected = Slot.Category == Module.Category;
			TestEqual(TEXT("Complete slot/module compatibility matrix"), Candidate.TryInstall(Slot.Id, Module.Id, Reason), Expected);
			TestEqual(TEXT("Rejected installs leave slot unchanged"), Candidate.Installed(Slot.Id)->Id, Expected ? Module.Id : Before);
		}
	}
	TestFalse(TEXT("Unknown slot rejected"), A.TryInstall(TEXT("missing"), TEXT("power.wing2"), Reason));
	TestFalse(TEXT("Unknown module rejected"), A.TryInstall(TEXT("power"), TEXT("missing"), Reason));
	TestTrue(TEXT("Solar replacement accepted"), A.TryInstall(TEXT("power"), TEXT("power.wing2"), Reason));
	TestEqual(TEXT("Replacement removes old mass"), A.TotalDryMassKg(), 373.0);
	TestEqual(TEXT("Power recalculated"), A.TotalRatedPowerW(), 400.0);
	TestTrue(TEXT("Repeated install succeeds without duplication"), A.TryInstall(TEXT("power"), TEXT("power.wing2"), Reason));
	TestEqual(TEXT("Mass does not accumulate"), A.TotalDryMassKg(), 373.0);
	TestEqual(TEXT("Independent config unchanged"), B.TotalDryMassKg(), 408.0);
	FSpacecraftConfiguration Copy = A;
	Copy.TryInstall(TEXT("communications"), TEXT("comms.dish06"), Reason);
	TestEqual(TEXT("Copy modifications are isolated"), A.TotalDryMassKg(), 373.0);
	Copy.TryInstall(TEXT("propulsion"), TEXT("propulsion.compact"), Reason);
	TestEqual(TEXT("All compact dry mass"), Copy.TotalDryMassKg(), 347.0);
	TestEqual(TEXT("Other module replacements preserve generation"), Copy.TotalRatedPowerW(), 400.0);
	FSpacecraftModule Invalid = FSpacecraftCatalog::Modules()[0];
	Invalid.MassKg = -1;
	TestFalse(TEXT("Negative mass rejected"), Invalid.IsValid());
	Invalid.MassKg = std::numeric_limits<double>::quiet_NaN();
	TestFalse(TEXT("Nonfinite mass rejected"), Invalid.IsValid());
	Invalid = FSpacecraftCatalog::Modules()[0];
	Invalid.RatedPowerW = std::numeric_limits<double>::infinity();
	TestFalse(TEXT("Nonfinite power rejected"), Invalid.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpacecraftSessionTest, "NOMINAL.Spacecraft.SessionNavigation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSpacecraftSessionTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	auto* Session = World->SpawnActor<AMissionControlGameMode>();
	if (!TestNotNull(TEXT("Session created"), Session)) { World->DestroyWorld(false); return false; }
	Session->InstallModule(TEXT("power"), TEXT("power.wing2"));
	Session->RequestPhase(EMissionPhase::SpacecraftDesign);
	const FText Phase = Session->PhaseText();
	const FText Log = Session->LogText();
	for (int32 I = 0; I < 3; ++I)
	{
		Session->ShowDesigner();
		Session->ShowOrbitalView(false);
		Session->ShowOrbitalView(true);
		Session->ShowDesigner();
	}
	TestEqual(TEXT("Session preserves installed module through navigation"), Session->GetSpacecraftConfiguration().Installed(TEXT("power"))->Id, FName(TEXT("power.wing2")));
	TestEqual(TEXT("Session preserves totals"), Session->GetSpacecraftConfiguration().TotalDryMassKg(), 373.0);
	TestTrue(TEXT("Navigation preserves mission phase"), Session->PhaseText().EqualTo(Phase));
	TestTrue(TEXT("Navigation preserves mission logs"), Session->LogText().EqualTo(Log));
	World->DestroyWorld(false);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpacecraftPresentationTest, "NOMINAL.Spacecraft.VisualProfiles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSpacecraftPresentationTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FSpacecraftPresentation Presentation;
	TestFalse(TEXT("Missing visual bindings are reported"), Presentation.Initialize(World));
	const TArray<FName> Roles = {TEXT("Designer.Panel"), TEXT("Designer.Dish"), TEXT("Designer.Feed"), TEXT("Designer.Nozzle"), TEXT("Designer.Mount")};
	TArray<AStaticMeshActor*> Parts;
	for (FName Role : Roles)
	{
		auto* Actor = World->SpawnActor<AStaticMeshActor>();
		Actor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
		Actor->Tags.Add(Role);
		Actor->SetActorLocation(FVector(490, 0, 0));
		Parts.Add(Actor);
	}
	TestTrue(TEXT("All roles found"), Presentation.Initialize(World));
	FSpacecraftConfiguration Compact;
	FString Reason;
	Compact.TryInstall(TEXT("power"), TEXT("power.wing2"), Reason);
	Compact.TryInstall(TEXT("communications"), TEXT("comms.dish06"), Reason);
	Compact.TryInstall(TEXT("propulsion"), TEXT("propulsion.compact"), Reason);
	for (int32 I = 0; I < 3; ++I) Presentation.Apply(Compact);
	TestTrue(TEXT("Compact wings hide outer panels"), Parts[0]->IsHidden());
	TestTrue(TEXT("Dish profile does not accumulate scaling"), Parts[1]->GetActorScale3D().Equals(FVector(0.5)));
	TestTrue(TEXT("Engine is narrow, retaining mount depth"), Parts[3]->GetActorScale3D().Equals(FVector(0.5, 0.5, 1)));
	Presentation.Apply(FSpacecraftConfiguration());
	for (const auto* Actor : Parts)
	{
		TestFalse(TEXT("Default restores visibility"), Actor->IsHidden());
		TestTrue(TEXT("Default restores scale"), Actor->GetActorScale3D().Equals(FVector::OneVector));
		TestTrue(TEXT("Default restores location"), Actor->GetActorLocation().Equals(FVector(490, 0, 0)));
	}
	World->DestroyWorld(false);
	return true;
}
#endif
