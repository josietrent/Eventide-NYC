# The Eventide in NYC

Build, install, and run tasks for this repo. Paths for Sanny Builder and the game come from `.env`.

- `deno task build`: runs `scripts/build.ts`. Compiles `main.txt` to `main.scm` with Sanny Builder (`SANNY` in `.env`) and packages `dist/Drag-N-Drop.zip`. Only the main compile step runs.
- `deno task install`: runs build, then `scripts/install.ts`, which extracts the zip into the `GTA3` folder from `.env`.
- `deno task run`: runs install, then `scripts/run.ts`, which launches `gta3-game.exe` from the `GTA3` folder and waits for it to exit. It prints the exit verdict from the game logs and does not record telemetry.
- `.env` holds `SANNY` and `GTA3`.
