#include "Spacecraft/ElectricalPowerSystem.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Mission/MissionControlGameMode.h"
#include "Engine/World.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEPSEnergyTest, "NOMINAL.EPS.EnergyAndLoads",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEPSEnergyTest::RunTest(const FString& Parameters)
{
	FSpacecraftConfiguration Config;
	FElectricalPowerSystem Power(Config), Independent(Config);
	TestEqual(TEXT("Default storage"), Power.CapacityWh(), 500.0);
	TestEqual(TEXT("Initial test charge is 50 percent"), Power.StoredEnergyWh(), 250.0);
	TestEqual(TEXT("Rated installed generation"), Power.RatedGenerationW(), 1200.0);
	TestEqual(TEXT("Sunlight generation"), Power.AvailableGenerationW(), 1200.0);
	TestEqual(TEXT("Sum of requested loads"), Power.RequestedConsumptionW(), 850.0);
	TestEqual(TEXT("Actual active consumption"), Power.ActiveConsumptionW(), 850.0);
	TestEqual(TEXT("Positive balance"), Power.NetPowerW(), 350.0);
	TestTrue(TEXT("Advance accepted"), Power.Advance(600));
	TestTrue(TEXT("Known charging interval in Wh"), FMath::IsNearlyEqual(Power.StoredEnergyWh(), 250.0 + 350.0/6.0, 1e-9));
	Power.Advance(3600);
	TestEqual(TEXT("Charging capped at capacity"), Power.StoredEnergyWh(), 500.0);
	Power.SetIllumination(EEPSIllumination::Eclipse);
	TestEqual(TEXT("Eclipse generation is zero"), Power.AvailableGenerationW(), 0.0);
	TestEqual(TEXT("Rated capacity unaffected by eclipse"), Power.RatedGenerationW(), 1200.0);
	TestEqual(TEXT("Negative balance"), Power.NetPowerW(), -850.0);
	Power.Advance(600);
	TestTrue(TEXT("Known discharge interval in Wh"), FMath::IsNearlyEqual(Power.StoredEnergyWh(), 500.0 - 850.0/6.0, 1e-9));
	TestTrue(TEXT("Science demand switchable"), Power.SetLoadEnabled(TEXT("instrument"), false));
	TestEqual(TEXT("Consumption drops by instrument rating"), Power.ActiveConsumptionW(), 350.0);
	const double Before = Power.StoredEnergyWh();
	Power.Advance(600);
	TestTrue(TEXT("Lower demand slows discharge"), FMath::IsNearlyEqual(Power.StoredEnergyWh(), Before - 350.0/6.0, 1e-9));
	Power.SetLoadEnabled(TEXT("transmitter"), false);
	TestEqual(TEXT("Essential-only draw"), Power.ActiveConsumptionW(), 100.0);
	TestFalse(TEXT("Cannot disable avionics"), Power.SetLoadEnabled(TEXT("avionics"), false));
	Power.SetLoadEnabled(TEXT("instrument"), true);
	TestTrue(TEXT("Load ratings configurable"), Power.SetLoadWatts(TEXT("instrument"), 200));
	TestEqual(TEXT("Consumption derived from configured active loads"), Power.ActiveConsumptionW(), 300.0);
	TestEqual(TEXT("Independent instance retains initial charge"), Independent.StoredEnergyWh(), 250.0);
	TestEqual(TEXT("Independent instance retains load states"), Independent.ActiveConsumptionW(), 850.0);
	const double BeforeInvalid = Power.StoredEnergyWh();
	const double TimeBeforeInvalid = Power.ElapsedSeconds();
	TestFalse(TEXT("Negative interval rejected"), Power.Advance(-1));
	TestFalse(TEXT("NaN interval rejected"), Power.Advance(std::numeric_limits<double>::quiet_NaN()));
	TestFalse(TEXT("Infinite interval rejected"), Power.Advance(std::numeric_limits<double>::infinity()));
	TestTrue(TEXT("Zero interval is a no-op"), Power.Advance(0));
	TestFalse(TEXT("Invalid condition rejected"), Power.SetIllumination(static_cast<EEPSIllumination>(255)));
	TestFalse(TEXT("Unknown load rejected"), Power.SetLoadEnabled(TEXT("missing"), true));
	TestFalse(TEXT("Negative load rating rejected"), Power.SetLoadWatts(TEXT("instrument"), -100));
	TestFalse(TEXT("Nonfinite rating rejected"), Power.SetLoadWatts(TEXT("instrument"), std::numeric_limits<double>::infinity()));
	TestEqual(TEXT("Invalid commands preserve energy"), Power.StoredEnergyWh(), BeforeInvalid);
	TestEqual(TEXT("Invalid commands preserve time"), Power.ElapsedSeconds(), TimeBeforeInvalid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEPSSheddingTest, "NOMINAL.EPS.DepletionAndRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEPSSheddingTest::RunTest(const FString& Parameters)
{
	FSpacecraftConfiguration Config;
	FString Reason;
	Config.TryInstall(TEXT("power"), TEXT("power.wing2"), Reason);
	FElectricalPowerSystem Power(Config);
	TestEqual(TEXT("Compact panel generation"), Power.AvailableGenerationW(), 400.0);
	TestEqual(TEXT("Compact deficit"), Power.NetPowerW(), -450.0);
	Power.Advance(2000); // 250 Wh / 450 W * 3600 s/h
	TestEqual(TEXT("Exact depletion event"), Power.StoredEnergyWh(), 0.0);
	TestTrue(TEXT("Avionics has first priority"), Power.GetLoads()[0].IsPowered());
	TestTrue(TEXT("Transmitter fits remaining source"), Power.GetLoads()[1].IsPowered());
	TestFalse(TEXT("Science shed"), Power.GetLoads()[2].IsPowered());
	TestEqual(TEXT("Shed consumption excludes unavailable load"), Power.ActiveConsumptionW(), 350.0);
	TestEqual(TEXT("Requested consumption remains visible"), Power.RequestedConsumptionW(), 850.0);
	Power.Advance(600);
	TestTrue(TEXT("Surplus after shedding recharges battery"), FMath::IsNearlyEqual(Power.StoredEnergyWh(), 50.0/6.0, 1e-9));
	TestTrue(TEXT("Protection stays latched between steps"), Power.HasShedLoads());
	Power.RestoreRequestedLoads();
	TestEqual(TEXT("Operator can restore using remaining energy"), Power.ActiveConsumptionW(), 850.0);
	Power.SetIllumination(EEPSIllumination::Eclipse);
	Power.Advance(3600);
	TestEqual(TEXT("Energy cannot fall below zero"), Power.StoredEnergyWh(), 0.0);
	TestEqual(TEXT("Zero source and empty battery power no loads"), Power.ActiveConsumptionW(), 0.0);
	TestTrue(TEXT("Loss of critical power is represented"), Power.GetLoads()[0].bShed);
	Power.RestoreRequestedLoads();
	TestEqual(TEXT("Restore cannot create power"), Power.ActiveConsumptionW(), 0.0);
	Power.SetIllumination(EEPSIllumination::Sunlight);
	Power.RestoreRequestedLoads();
	TestEqual(TEXT("Restore respects source at zero energy"), Power.ActiveConsumptionW(), 350.0);
	Power.SetLoadEnabled(TEXT("instrument"), false);
	TestFalse(TEXT("Commanded off is distinct from requested-but-unavailable"), Power.HasShedLoads());
	Power.Advance(3600);
	TestEqual(TEXT("Recovery charges without free reset"), Power.StoredEnergyWh(), 50.0);
	TestTrue(TEXT("Low energy alert threshold"), Power.IsLowEnergy());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEPSReplacementTest, "NOMINAL.EPS.ConfigurationReplacement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEPSReplacementTest::RunTest(const FString& Parameters)
{
	FSpacecraftConfiguration Config;
	FElectricalPowerSystem Power(Config);
	FString Reason;
	TestEqual(TEXT("Default dry mass includes battery"), Config.TotalDryMassKg(), 428.0);
	Power.SetIllumination(EEPSIllumination::Eclipse);
	Power.Advance(600);
	const double BeforeSwap = Power.StoredEnergyWh();
	TestTrue(TEXT("Large battery accepted"), Config.TryInstall(TEXT("storage"), TEXT("storage.1000"), Reason));
	Power.SyncConfiguration(Config);
	TestEqual(TEXT("Large battery mass"), Config.TotalDryMassKg(), 443.0);
	TestEqual(TEXT("New capacity"), Power.CapacityWh(), 1000.0);
	TestEqual(TEXT("Larger battery does not refill"), Power.StoredEnergyWh(), BeforeSwap);
	Power.SyncConfiguration(Config);
	TestEqual(TEXT("Repeated synchronization does not recharge"), Power.StoredEnergyWh(), BeforeSwap);
	Config.TryInstall(TEXT("power"), TEXT("power.wing2"), Reason);
	Power.SyncConfiguration(Config);
	TestEqual(TEXT("Solar replacement changes rated capacity"), Power.RatedGenerationW(), 400.0);
	TestEqual(TEXT("Eclipse remains zero after array replacement"), Power.AvailableGenerationW(), 0.0);
	TestEqual(TEXT("Solar replacement preserves charge"), Power.StoredEnergyWh(), BeforeSwap);
	TestEqual(TEXT("Combined replacements update mass"), Config.TotalDryMassKg(), 408.0);
	Config.TryInstall(TEXT("power"), TEXT("power.wing6"), Reason);
	Power.SyncConfiguration(Config);
	Power.SetIllumination(EEPSIllumination::Sunlight);
	Power.Advance(10000);
	TestEqual(TEXT("Large battery fills normally"), Power.StoredEnergyWh(), 1000.0);
	Config.TryInstall(TEXT("storage"), TEXT("storage.500"), Reason);
	Power.SyncConfiguration(Config);
	TestEqual(TEXT("Smaller battery clamps absolute energy"), Power.StoredEnergyWh(), 500.0);
	Config.TryInstall(TEXT("storage"), TEXT("storage.1000"), Reason);
	Power.SyncConfiguration(Config);
	TestEqual(TEXT("Swapping back cannot recover discarded energy"), Power.StoredEnergyWh(), 500.0);
	TestFalse(TEXT("Battery cannot occupy solar slot"), Config.TryInstall(TEXT("power"), TEXT("storage.500"), Reason));
	FSpacecraftModule Invalid = *FSpacecraftCatalog::FindModule(TEXT("storage.500"));
	Invalid.UsableCapacityWh = 0;
	TestFalse(TEXT("Zero battery capacity invalid"), Invalid.IsValid());
	Invalid.UsableCapacityWh = std::numeric_limits<double>::quiet_NaN();
	TestFalse(TEXT("NaN battery capacity invalid"), Invalid.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEPSDeterminismTest, "NOMINAL.EPS.DurationEquivalence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEPSDeterminismTest::RunTest(const FString& Parameters)
{
	FSpacecraftConfiguration Config;
	FString Reason;
	Config.TryInstall(TEXT("power"), TEXT("power.wing2"), Reason);
	for (EEPSIllumination Condition : {EEPSIllumination::Sunlight, EEPSIllumination::Eclipse})
		for (double Duration : {600.0, 2000.0, 7200.0, 72000.0})
		{
			FElectricalPowerSystem Single(Config);
			Single.SetIllumination(Condition);
			Single.Advance(Duration);
			for (double Step : {0.5, 60.0, 600.0})
			{
				FElectricalPowerSystem Split(Config);
				Split.SetIllumination(Condition);
				for (double Time = 0; Time < Duration; Time += Step)
				{
					Split.SyncConfiguration(Config); // Must not reset protection or charge.
					Split.Advance(FMath::Min(Step, Duration - Time));
				}
				TestTrue(TEXT("Equivalent duration preserves energy within floating-point tolerance"), FMath::IsNearlyEqual(Split.StoredEnergyWh(), Single.StoredEnergyWh(), 1e-7));
				TestEqual(TEXT("Equivalent duration preserves active consumption"), Split.ActiveConsumptionW(), Single.ActiveConsumptionW());
				TestEqual(TEXT("Equivalent duration preserves elapsed time"), Split.ElapsedSeconds(), Single.ElapsedSeconds());
				for (int32 I = 0; I < Single.GetLoads().Num(); ++I)
					TestEqual(TEXT("Equivalent duration preserves availability"), Split.GetLoads()[I].bShed, Single.GetLoads()[I].bShed);
			}
		}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEPSSessionTest, "NOMINAL.EPS.SessionAndFlightLock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEPSSessionTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	auto* Session = World->SpawnActor<AMissionControlGameMode>();
	if (!TestNotNull(TEXT("Session created"), Session)) { World->DestroyWorld(false); return false; }
	Session->InstallModule(TEXT("power"), TEXT("power.wing2"));
	Session->InstallModule(TEXT("storage"), TEXT("storage.1000"));
	TestEqual(TEXT("Designer changes feed EPS"), Session->GetEPS().RatedGenerationW(), 400.0);
	TestEqual(TEXT("Designer changes battery capacity"), Session->GetEPS().CapacityWh(), 1000.0);
	Session->GetEPS().SetIllumination(EEPSIllumination::Eclipse);
	Session->GetEPS().Advance(600);
	const double Energy = Session->GetEPS().StoredEnergyWh();
	Session->ShowEPS();
	Session->ShowDesigner();
	Session->ShowOrbitalView(false);
	Session->ShowEPS();
	TestEqual(TEXT("Navigation preserves energy"), Session->GetEPS().StoredEnergyWh(), Energy);
	for (EMissionPhase Phase : {EMissionPhase::SpacecraftDesign, EMissionPhase::LaunchPreparation, EMissionPhase::Launch, EMissionPhase::FlightOperations})
		Session->RequestPhase(Phase);
	TestFalse(TEXT("Flight refit locked"), Session->CanEditSpacecraft());
	TestTrue(TEXT("Flight battery swap rejected"), Session->InstallModule(TEXT("storage"), TEXT("storage.500")).StartsWith(TEXT("REJECTED")));
	TestTrue(TEXT("Flight solar swap rejected"), Session->InstallModule(TEXT("power"), TEXT("power.wing6")).StartsWith(TEXT("REJECTED")));
	TestEqual(TEXT("Rejected refit preserves capacity"), Session->GetEPS().CapacityWh(), 1000.0);
	TestEqual(TEXT("Rejected refit preserves energy"), Session->GetEPS().StoredEnergyWh(), Energy);
	const FText Phase = Session->PhaseText(), Log = Session->LogText();
	Session->GetEPS().Advance(3600);
	TestTrue(TEXT("Power loss is represented"), Session->GetEPS().HasShedLoads());
	TestTrue(TEXT("Power loss does not cause mission failure"), Session->PhaseText().EqualTo(Phase));
	TestTrue(TEXT("EPS does not alter lifecycle log"), Session->LogText().EqualTo(Log));
	World->DestroyWorld(false);
	return true;
}
#endif
