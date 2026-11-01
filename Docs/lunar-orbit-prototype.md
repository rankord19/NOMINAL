# Lunar orbital visual prototype

This is a static visual study, not an orbital simulation or an approved spacecraft
design. The existing Mission Core is unchanged. Mission Control and the 3D canvas
share the same game mode and lifecycle instance: switching views never changes the
mission phase or clears its log. The dedicated level is `/Game/LunarPrototype/L_LunarOrbit`.

## Build and launch

Use the primary checkout `D:\GitHub\NOMINAL` or agent checkout
`D:\GitHub\NOMINAL-Astra`, containing `codex/lunar-orbit-prototype` (or main after
merge). Git LFS is required for the committed Unreal assets. In PowerShell:

```powershell
$NominalCheckout = 'D:\GitHub\NOMINAL'
$NominalEngine = 'D:\Epic Games\UE_5.8'
$NominalProject = Join-Path $NominalCheckout 'NOMINAL.uproject'
git -C $NominalCheckout lfs pull
& (Join-Path $NominalEngine 'Engine\Build\BatchFiles\Build.bat') NOMINALEditor Win64 Development "-Project=$NominalProject" -WaitMutex -NoHotReloadFromIDE
```

After the build succeeds, open the project in Unreal Editor and press Play (not
Simulate). The new map is the editor startup and game default map. Or launch:

```powershell
& (Join-Path $NominalEngine 'Engine\Binaries\Win64\UnrealEditor.exe') $NominalProject -game -windowed -ResX=1600 -ResY=900
```

1. Mission Control opens first. Click **Enter lunar view**.
2. Use **Spacecraft focus** or **Moon focus** to reset framing and smoothly change focus.
3. Drag with the left button in the selected **Orbit drag** or **Pan drag** mode.
   Right drag also orbits; middle drag also pans. Scroll the wheel to zoom.
4. **Capture PNG** saves a real rendered image under `Saved/Screenshots/LunarView*.png`.
5. **Mission Control** returns to the existing phase controls and log.

Pointer gestures are handled only by the canvas, not by the toolbar or Mission
Control panel. The log and phase persist across view changes, but not application
restarts. No separate mock state machine, level travel, or mission reset is used.

## Assets and provenance

Source maps: [NASA SVS CGI Moon Kit](https://svs.gsfc.nasa.gov/4720/).
Credit: NASA's Scientific Visualization Studio; Ernie Wright (USRA), Noah Petro
(NASA/GSFC); LRO LROC camera team and LOLA instrument team. No NASA endorsement is
implied. These maps have no third-party copyright restriction identified on the
source page; usage was checked against the linked
[NASA media guidelines](https://www.nasa.gov/nasa-brand-center/images-and-media/).
No logos, astronaut likenesses, or unverified marketplace assets are included.

Inputs retrieved from NASA on 2026-11-01:

| Source | Dimensions | SHA-256 |
|---|---|---|
| [2019 LROC color TIFF](https://svs.gsfc.nasa.gov/vis/a000000/a004700/a004720/lroc_color_poles_8k.tif) | 8192 x 4096 | `4af8b0cd4d50c30851359d98e7e72040240dd8d03256b58b345b5b76e9edb4ef` |
| [LOLA unsigned elevation TIFF](https://svs.gsfc.nasa.gov/vis/a000000/a004700/a004720/ldem_16_uint.tif) | 5760 x 2880 | `45a2b32d56e81ed30db07fead8abc842b249b6511219d9ca2c53f81bc2dc5d62` |

The color map is white-balanced/gamma-adjusted visualization imagery, including
lower-resolution polar fill. NASA explicitly advises using the original source
data for scientific work. It is not calibrated material reflectance.

`prepare_lunar_sources.py` converts the color TIFF to PNG, decodes elevation as
`sample * 0.5 - 10000` meters relative to the 1737.4 km reference radius, generates
a 512 x 256 segmented sphere with sampled relief, and derives tangent-space normals
from the elevation gradients with no height multiplier. Polar gradients are clamped
for numerical stability. This rendering pipeline is not validated for navigation,
terrain measurement, or scientific analysis.

The spacecraft uses engine primitives and an original generated parabolic dish.
Solar-cell grid and foil micro-normal textures are original deterministic procedural
art. Bus, radiator, antenna, arrays, and propulsion pieces are temporary visual
models with plausible-looking proportions only. Their dimensions, material values,
mass, power, performance, and engineering compatibility are not validated.

All runtime assets are committed under `Content/LunarPrototype`:

- `L_LunarOrbit.umap`
- `SM_Moon.uasset`, `SM_Dish.uasset`
- `M_Moon.uasset`, `M_ThermalFoil.uasset`, `M_Aluminum.uasset`,
  `M_Radiator.uasset`, `M_Graphite.uasset`, `M_SolarCells.uasset`
- `T_MoonColor.uasset`, `T_MoonNormal.uasset`, `T_FoilNormal.uasset`, `T_SolarCells.uasset`

The TIFF downloads and intermediate OBJ/PNG files stay in ignored
`Saved/LunarSources`; Unreal assets contain the imported data, so downloads and
Python packages are not required to play. To regenerate, download those exact
TIFFs there, install Pillow and NumPy into `Saved/python` using the engine's bundled
Python, run `Scripts/prepare_lunar_sources.py`, then run UnrealEditor-Cmd with
`-run=pythonscript -script=<absolute path to Scripts/build_lunar_level.py>`.
The generator rebuilds its dedicated level's presentation actors; do not use it to
preserve hand edits to that level. Python is editor tooling only, not a game dependency.

## Scale, rendering, and boundaries

- Unreal centimeters and double-precision FVector camera calculations are used.
  The Moon's reference radius is 1737.4 km. The temporary spacecraft is near the
  local origin; Moon center is `(0, 0, -183740000)` cm. The nominal 100 km offset
  is visual staging, not an approved mission altitude or a propagated orbit.
- Camera position and rotation interpolate smoothly. Zoom/pan are bounded, and
  focus transitions keep the camera at least 20 km above the reference sphere.
- A single directional sun uses 100,000 lux and 0.53-degree angular diameter.
  Fixed exposure is EV100 14.5. These are presentation settings, not photometric
  validation. No atmospheric scattering, fog, sky light, or artificial fill light
  is added. Unlit sides can therefore remain very dark as the camera moves.
- PBR materials, movable spacecraft components, and dynamic spacecraft shadows
  are used. Lunar terrain uses geometry and normal relief; planetary shadow casting
  is disabled to avoid giant-mesh shadow problems. Crater self-shadowing/eclipses
  are not fully modeled.
- Lumen GI is disabled in this level; it adds little to this isolated sunlit scene.
  Nanite is not enabled: a single modest-resolution lunar mesh is adequate for this
  pass, and does not pretend to provide close-surface terrain detail.
- No spacecraft movement, orbital mechanics, construction, finances, experiments,
  star catalog, or additional campaign decisions are implemented.

## Verification and remaining limits

UE 5.8.4 Editor Win64 Development build passed. The two existing Mission Core tests
and `NOMINAL.LunarView.CameraBounds` passed. The new test checks finite positions,
surface clearance during focus transitions, focus ranges, and bounded extreme input.
An initial test-world double-initialization crash was corrected before passing.

Standalone rendering at 1600 x 900 was inspected in Unreal: level loading, both
focus views, orbit drag, pan drag, wheel zoom, returning to Mission Control, advancing
the existing lifecycle, and retaining phase/log state when re-entering 3D were checked.
Actual engine screenshots: [spacecraft](Images/lunar-spacecraft.png) and
[Moon](Images/lunar-moon.png). Their Failed label reflects the live lifecycle state
of the capture session, not a rendering error.

The original near-black spacecraft problem was corrected by fixing Python rotation
argument ordering, explicit sunlight direction, dynamic component lighting, and
material response. The dish is now visible from both sides. Initial framing places
the craft against dark space with a curved lunar limb instead of a full terrain
background. Moon focus fits the globe with margin and shows a terminator.

This is still a placeholder visual prototype, not photorealism. Primitive spacecraft
edges and simplified attachments remain conspicuous. Global lunar textures soften
when viewed close to the surface; dark grazing-angle relief artifacts remain
visible near the lunar limb. No packaging, Shipping build, multiplayer, VR, or broad GPU/resolution
compatibility testing was performed. MSVC 14.52 is newer than the engine-preferred
14.50 toolchain; Unreal startup also emits generic pre-test automation errors, while
the selected NOMINAL tests themselves pass without errors or warnings.
