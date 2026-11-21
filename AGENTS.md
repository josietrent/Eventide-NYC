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
