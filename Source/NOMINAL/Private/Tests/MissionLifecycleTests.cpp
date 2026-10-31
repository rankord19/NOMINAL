#include "Mission/MissionLifecycle.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionLifecycleTransitionsTest,
	"NOMINAL.MissionCore.TransitionMatrix",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FMissionLifecycleTransitionsTest::RunTest(const FString& Parameters)
{
	using P = EMissionPhase;
	const P Phases[] = { P::Planning, P::SpacecraftDesign, P::LaunchPreparation,
		P::Launch, P::FlightOperations, P::ScienceOperations, P::Completed, P::Failed };

	// Independent specification: rows are sources, columns are destinations in Phases order.
	const bool Allowed[8][8] = {
		{ false, true,  false, false, false, false, false, true  },
		{ false, false, true,  false, false, false, false, true  },
		{ false, false, false, true,  false, false, false, true  },
		{ false, false, false, false, true,  false, false, true  },
		{ false, false, false, false, false, true,  true,  true  },
		{ false, false, false, false, true,  false, true,  true  },
		{ false, false, false, false, false, false, false, false },
		{ false, false, false, false, false, false, false, false }
	};

	FMissionLifecycle Source;
	TestTrue(TEXT("New mission starts in planning"), Source.GetPhase() == P::Planning);
	for (int32 Row = 0; Row < 8; ++Row)
	{
		if (Phases[Row] == P::Failed)
		{
			Source = FMissionLifecycle();
			TestTrue(TEXT("Reach failed state"), Source.TryTransitionTo(P::Failed));
		}
		else if (Row > 0)
		{
			TestTrue(TEXT("Reach next lifecycle phase"), Source.TryTransitionTo(Phases[Row]));
		}

		TestEqual(TEXT("Only completed and failed are terminal"), Source.IsTerminal(), Row >= 6);
		for (int32 Column = 0; Column < 8; ++Column)
		{
			FMissionLifecycle Mission = Source;
			const FString Context = FString::Printf(TEXT("Transition %d -> %d"), Row, Column);
			TestEqual(Context + TEXT(" validation"), Mission.CanTransitionTo(Phases[Column]), Allowed[Row][Column]);
			TestTrue(Context + TEXT(" query preserves state"), Mission.GetPhase() == Phases[Row]);
			TestEqual(Context + TEXT(" result"), Mission.TryTransitionTo(Phases[Column]), Allowed[Row][Column]);
			TestTrue(Context + TEXT(" resulting state"),
				Mission.GetPhase() == (Allowed[Row][Column] ? Phases[Column] : Phases[Row]));
		}

		for (int32 InvalidValue = 8; InvalidValue <= 255; ++InvalidValue)
		{
			const P InvalidPhase = static_cast<P>(InvalidValue);
			TestFalse(TEXT("Reject invalid phase query"), Source.CanTransitionTo(InvalidPhase));
			TestFalse(TEXT("Reject invalid phase transition"), Source.TryTransitionTo(InvalidPhase));
			TestTrue(TEXT("Invalid request preserves state"), Source.GetPhase() == Phases[Row]);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionLifecycleOperationsTest,
	"NOMINAL.MissionCore.OperationsAndIsolation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FMissionLifecycleOperationsTest::RunTest(const FString& Parameters)
{
	using P = EMissionPhase;
	FMissionLifecycle Mission;
	FMissionLifecycle OtherMission;
	const P Route[] = { P::SpacecraftDesign, P::LaunchPreparation, P::Launch,
		P::FlightOperations, P::ScienceOperations, P::FlightOperations,
		P::ScienceOperations, P::Completed };
	for (P Next : Route)
	{
		TestTrue(TEXT("Operations can alternate before explicit completion"), Mission.TryTransitionTo(Next));
	}
	TestTrue(TEXT("Completed mission is terminal"), Mission.IsTerminal());
	TestFalse(TEXT("Completion cannot be overwritten by failure"), Mission.TryTransitionTo(P::Failed));
	TestTrue(TEXT("Other mission remains independent"), OtherMission.GetPhase() == P::Planning);
	return true;
}

#endif
