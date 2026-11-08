/**
 * run.ts
 *
 * Launches GTA III from the installation directory specified in .env, waits for it to exit, and captures the
 * telemetry that sockets.cleo streams meanwhile (see telemetry.ts) to dist/telemetry/run-<stamp>.jsonl.
 * `--events` also prints each packet and the exit verdict as event lines (see run-events.ts).
 */

import { dirname, join } from "@std/path";
import { diagnoseExit } from "./game-exit.ts";
import { say } from "./log.ts";
import { eventLine, type RunEvent } from "./run-events.ts";

const gta3Base = Deno.env.get("GTA3");
if (!gta3Base) {
  say.error("GTA3 environment variable is not set in .env");
  Deno.exit(1);
}

const exePath = join(gta3Base, "gta3.exe");

try {
  const stat = await Deno.stat(exePath);
  if (!stat.isFile) {
    throw new Error("Not a file");
  }
} catch {
  say.error(`Executable not found at ${exePath}`);
  Deno.exit(1);
}

// Prefer unflagged hardlink runner to bypass HKLM UAC elevation prompt
const runnerExe = join(gta3Base, "gta3-game.exe");
let targetExe = exePath;

try {
  const stat = await Deno.stat(runnerExe);
  if (stat.isFile) targetExe = runnerExe;
} catch {
  // Fall back to gta3.exe
}

say.info(`Launching Grand Theft Auto III: ${targetExe}`);

const command = new Deno.Command(targetExe, {
  cwd: gta3Base,
});

const launchedAt = Date.now();
const child = command.spawn();
say.wait("Waiting for the game to exit");
const status = await child.status;

reportExit(status.code);

/** A game log's text, or "" when it is missing or was last written before this launch. */
async function freshLog(...path: string[]): Promise<string> {
  const file = join(gta3Base!, ...path);
  try {
    const modified = (await Deno.stat(file)).mtime?.getTime() ?? 0;
    return modified >= launchedAt ? await Deno.readTextFile(file) : "";
  } catch {
    return "";
  }
}

/** Says whether the game quit or crashed, by its logs: the exit code is not a signal (see game-exit.ts). */
async function reportExit(code: number) {
  const diagnosis = diagnoseExit({
    cleo: await freshLog("cleo.log"),
    scrlog: await freshLog("scrlog.log"),
    modloader: await freshLog("modloader", "modloader.log"),
  });
  if (!diagnosis.known) {
    say.skip(
      `The game exited with code ${code}; it wrote no log, so a crash cannot be ruled in or out`,
    );
  } else if (diagnosis.crashed) {
    say.error(`The game crashed: ${diagnosis.signs.join("; ")}`);
    if (diagnosis.exception) say.fail(`  ${diagnosis.exception}`);
  } else {
    say.pass("The game exited normally");
    if (code !== 0) {
      say.skip(
        `Exit code ${code} is the last window message's wParam, not an error`,
      );
    }
  }
}

/** The channel's DEBUG_* name, or its number. */
function channelName(channel: number): string {
  return names.get(channel) ?? `CHANNEL_${channel}`;
}

function describe(event: TimedPacket): string {
  const time = `+${(event.t / 1000).toFixed(1)}s`.padEnd(9);
  if (event.type === "debug") {
    return `${time}${channelName(event.channel)} ${event.kind} ${event.value}`;
  }
  const [x, y, z] = event.pos.map((n) => n.toFixed(1));
  return `${time}${event.type.toUpperCase()} ${x}, ${y}, ${z} h${
    event.heading.toFixed(0)
  }`;
}

function report(summary: Summary, path: string) {
  if (summary.packets === 0) {
    say.skip("No telemetry received. Is sockets.cleo installed?");
    return;
  }
  say.heading("Telemetry");
  const channels = [...summary.channels].sort(([a], [b]) => a - b);
  for (const [channel, stats] of channels) {
    const range = stats.min === stats.max
      ? ""
      : `  range ${stats.min}..${stats.max}`;
    say.text(
      `${
        channelName(channel).padEnd(26)
      }${stats.kind}  n=${stats.count}  last=${stats.last}${range}`,
    );
  }
  say.text(
    `Positions ${summary.positions}, F8 captures ${summary.captures}` +
      (summary.lastPosition ? `, last ${describe(summary.lastPosition)}` : ""),
  );
  say.skip("Last events:");
  for (const event of summary.recent) say.skip(`  ${describe(event)}`);
  say.skip(`Log: ${path}`);
}
