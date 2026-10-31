# Mission Control prototype

The default game mode owns one FMissionLifecycle per play session and presents
a native Slate interface. Mission Core is unchanged. Advance requests the next
phase on the forward route; Request failure explicitly requests Failed. Every
request uses TryTransitionTo and records ACCEPTED or REJECTED. Terminal requests
remain available to demonstrate rejection. The scrollable log retains 100 entries.
Restart the game or Play session to reset. These are prototype controls, not
readiness checks or scientific outcome rules.

## Launch on this workstation

Build from PowerShell:

```powershell
& 'D:\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' NOMINALEditor Win64 Development '-Project=D:\GitHub\NOMINAL-Astra\NOMINAL.uproject' -WaitMutex -NoHotReloadFromIDE
```

Then launch:

```powershell
& 'D:\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe' 'D:\GitHub\NOMINAL-Astra\NOMINAL.uproject' -game -windowed -ResX=1280 -ResY=720
```

Alternatively open the project in the editor and press Play (not Simulate), on a
map without a GameMode override. The existing engine default map is retained.
Use Advance six times to reach Completed, then again to see a rejected request.
Restart and use Request failure to inspect Failed. Close standalone with Alt+F4.

## Validation on 2026-11-01

- UE 5.8.4 NOMINALEditor Win64 Development build passed.
- Existing Mission Core automation: 2 passed, 0 failed, process exit 0.
- Standalone rendered interface inspected at 1280x720: title, phase, buttons,
  successful Planning -> Spacecraft Design transition, Failed state, and rejected
  terminal requests were visible in the log.
- Completed rendering, editor PIE, packaging, and other resolutions not tested.
- Installed MSVC remains newer than the engine-preferred toolchain.

No physics, mission specifications, financial systems, persistence, or visual
assets are introduced. This is a local single-player prototype; multiplayer is
not supported. The master design was added unchanged in a separate commit.
