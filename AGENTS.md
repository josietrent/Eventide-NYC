# The Eventide in NYC

Build, install, and run tasks for this repo. Paths for Sanny Builder and the game come from `.env`.

- `deno task build`: runs `scripts/build.ts`. Compiles `main.txt` to `main.scm` with Sanny Builder (`SANNY` in `.env`) and packages `dist/Drag-N-Drop.zip`. Only the main compile step runs.
- `deno task install`: runs build, then `scripts/install.ts`, which extracts the zip into the `GTA3` folder from `.env`.
- `deno task run`: runs install, then `scripts/run.ts`, which launches `gta3.exe` from the `GTA3` folder and waits for it to exit. It prints the exit verdict from the game logs and does not record telemetry.
- `.env` holds `SANNY` and `GTA3`.

## Rules

- **Always use the vanilla `gta3.exe`:** Never create, rename, hard-link, or launch alternate executables (such as `gta3-game.exe`) in the game directory or scripts. Direct3D wrappers, ASI loaders, and ModLoader depend on the exact `gta3.exe` executable name.
- **Persistent Spheres Only:** Never use `Player.LocateAnyMeans3D(..., True)` (or other `Locate*` opcodes) to spawn mission spheres, as they flicker whenever the loop has a non-zero `wait` or conditional checks. Always pass `False` for the sphere flag, spawn a persistent sphere with `Sphere.Create(...)`, and delete it with `Sphere.Remove(...)` on trigger, completion, and failure cleanup.
- **Verify World Geometry Against IDE/IPL/COL Files:** Always verify world placement coordinates, ground `Z` elevations, cliff/water boundaries, and object footprints against GTA III's `data/maps/*.ide`, `*.ipl`, and `*.col` files (using `GTA3` from `.env`) rather than guessing.
- **Double-Check Cutscene Headings & Facing:** Always verify character and player headings in every cutscene (opening, outro, and skip branches): in GTA III, headings are counter-clockwise degrees from North (`0.0 = +Y North`, `90.0 = -X West`, `180.0 = -Y South`, `270.0 = +X East`). To prevent freshly spawned or objective-reset NPCs from turning away during fades, position both actors first, clear objectives (`Char.SetObjNoObj`), apply `Char.SetHeading` across a `wait 0`, call `Char.SetObjWaitOnFoot`, and call `Char.TurnToFacePlayer(npc, $player)` so both `m_fRotationCur` and `m_fRotationDest` lock toward the player.
- **CLEO-Only for World Spawns (Savegame Compatibility):** Never add new pickups, blips, parked vehicles, or car generators to `main.scm` (`utils/vehicles.txt`, `utils/assets.txt`, etc.), because changing the byte size of `main.scm` invalidates existing savegames. Always add new freeroam pickups, blips, garages, and car generators strictly via CLEO in `threads/world.txt` (`world.cs`), protected by a one-time global/initialization check (e.g. `if $...Created == 0 ...`), keeping `main.scm` frozen and savegames 100% compatible.
