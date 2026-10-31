# Mission Core foundation

## Existing project and scope

At base commit `332969a`, the project contained one runtime module (`NOMINAL`),
its bootstrap/header, game/editor build targets, and default configuration.
There were no mission classes, mission modules, automated tests, or tracked
Unreal assets/maps/Blueprints. No existing mission implementation was replaced.

This foundation follows the supplied `NOMINAL_MASTER_DESIGN_2026-11-01.txt`,
particularly sections 2, 13, 14, 16, and 17, and the requested lifecycle scope.
The supplied baseline is currently an untracked local input and is not changed
or added by this feature.

## C++ contract

`FMissionLifecycle` is a plain C++ value type in the existing module. It starts
in `Planning`. Its private phase changes only through `TryTransitionTo`, which
returns false and preserves state for invalid transitions. `CanTransitionTo`
performs the same structural check without mutation.

The initial transition graph is:

- Planning -> SpacecraftDesign -> LaunchPreparation -> Launch -> FlightOperations.
- FlightOperations <-> ScienceOperations, allowing further maneuvers between observations.
- FlightOperations or ScienceOperations -> Completed.
- Any nonterminal phase -> Failed, only on an explicit caller request.
- Completed and Failed have no outgoing transitions.

Repeated phase requests, phase skips, backward prelaunch transitions, and invalid
enum values are rejected. This is the minimal structural graph, not a complete
mission rules engine. It does not advance with time or infer failure from faults.
The caller must evaluate readiness, mission objectives, and viable alternatives
before requesting changes. Completion records that operations ended; it does not
assert full scientific success. Partial results and outcome grading remain open.

No actor, subsystem, UI binding, global mission singleton, ticking, physics,
construction, finances, mission specifications, or assets are introduced.
No new module dependencies or engine configuration changes are required.

## Boundaries and future decisions

The owner of a live mission, readiness checks, outcome criteria, and persistence
are deliberately undecided. Checkpoint restoration for Standard mode is a separate
future operation, not a backward transition or an in-flight spacecraft redesign.
Prelaunch revision flows and detailed flight/science scheduling should be added
when their rules are agreed. This type alone is not playable mission gameplay.

## Validation

Unreal development automation tests use the `NOMINAL.MissionCore` prefix:

- `TransitionMatrix`: all 64 valid-enum source/destination pairs, all invalid
  uint8 enum values in every phase, query purity, rejection without mutation,
  initial state, terminal states, and the full forward lifecycle.
- `OperationsAndIsolation`: repeated flight/science operations, explicit
  completion, terminal-state protection, and independent mission instances.

Build the `NOMINALEditor Win64 Development` target, then run:

```powershell
& '<UE>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' '<repo>\NOMINAL.uproject' '-ExecCmds=Automation RunTests NOMINAL.MissionCore;Quit' '-TestExit=Automation Test Queue Empty' -unattended -nop4 -NullRHI -nosplash
```

Tests are compiled only when `WITH_DEV_AUTOMATION_TESTS` is enabled.

Verified on 2026-11-01 with the installed Unreal Engine 5.8.4:

- `NOMINALEditor Win64 Development` built successfully. The first build exposed
  an automation flag API mismatch, corrected to the installed engine's
  `EAutomationTestFlags_ApplicationContextMask` before the successful build.
- Both `NOMINAL.MissionCore` automation tests passed in unattended NullRHI mode;
  the test process exited with code 0.
- The compiler emitted a warning that installed MSVC 14.52.37025 is newer than
  the preferred 14.50.35717. Editor startup also emitted generic `Condition failed`
  messages before the test run; neither Mission Core test reported errors.
- Packaged game and Shipping builds were not performed.
