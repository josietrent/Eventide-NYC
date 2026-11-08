/**
 * build.ts
 *
 * Compiles Sanny Builder scripts and packages mod assets into dist/Drag-N-Drop.zip:
 * - modloader/Noodles/data/main.scm (Main script)
 * - modloader/Noodles/cleo/*.cm (Mission threads)
 * - modloader/Noodles/models/gta3.img/ (*.dff, *.txd, *.col models)
 * - modloader/Noodles/data/maps/ (Map placements and IDE/IPL)
 * - modloader/Noodles/noodles_custom.txt (Map manifest directives)
 * - modloader/Noodles/txd/ (UI, splash, and loading screens)
 * - modloader/Noodles/cleo/cleo_text/UL_TEXT.fxt (Text strings)
 * - CLEO/sockets.cleo & CLEO/CLEO_PLUGINS/sockets.cleo (UDP Telemetry Plugin)
 *
 * Supports interactive bisection and step filtering via:
 * --bisect, -b, --step=<N>, --only=<name>, --skip=<name>
 * and recompiles every Sanny script, ignoring the hash cache, with --force.
 *
 * Model assets are bisected with --skip-asset=<asset|model|model:asset>[,...]
 * where <asset> is dff, txd, or col (e.g. --skip-asset=dinerind:col,dff), and a
 * bare <model> skips all of that model's assets (e.g. --skip-asset=dinerind).
 * A skipped COL is also commented out of the staged noodles_custom.txt. With
 * --bisect, each manifested model asset is prompted for instead.
 */

import { basename, dirname, extname, join } from "@std/path";
import { ensureDir } from "@std/fs/ensure-dir";
import { includeTree } from "./includes.ts";
import { capitalCase } from "change-case";
import { say } from "./log.ts";

// Bisection / filtering arguments
const args = Deno.args;
const isBisect = args.includes("--bisect") || args.includes("-b");
const forceArg = args.includes("--force");
const onlyArg = args.find((a) => a.startsWith("--only="))?.slice(7)?.split(",");
const skipArg = args.find((a) => a.startsWith("--skip="))?.slice(7)?.split(",");
const stepArg = args.find((a) => a.startsWith("--step="))?.slice(7)?.split(",")
  .map(Number);

const skipAssetArg = args.find((a) => a.startsWith("--skip-asset="))?.slice(13)
  ?.split(",").map((s) => s.trim().toLowerCase());

type ModelAsset = "dff" | "txd" | "col";
const assetAnswers = new Map<string, boolean>();

/** Whether a manifested model's asset is packaged, per --skip-asset or the --bisect prompt. */
function shouldPackageAsset(modelName: string, asset: ModelAsset): boolean {
  const name = modelName.toLowerCase();
  if (
    skipAssetArg?.includes(asset) || skipAssetArg?.includes(name) ||
    skipAssetArg?.includes(`${name}:${asset}`)
  ) {
    return false;
  }
  if (!isBisect) return true;
  const key = `${name}:${asset}`;
  let answer = assetAnswers.get(key);
  if (answer === undefined) {
    const res = prompt(
      `Package ${asset.toUpperCase()} for ${modelName}? [Y/n]:`,
    );
    answer = res === null || res.trim().toLowerCase() !== "n";
    assetAnswers.set(key, answer);
  }
  return answer;
}

function shouldRun(stepNum: number, stepName: string): boolean {
  if (stepArg && !stepArg.includes(stepNum)) return false;
  if (onlyArg && !onlyArg.includes(stepName)) return false;
  if (skipArg && skipArg.includes(stepName)) return false;
  if (isBisect) {
    const res = prompt(`Build Step ${stepNum} (${stepName})? [Y/n]:`);
    return res === null || res.trim().toLowerCase() !== "n";
  }
  return true;
}

/** Compiles a script using the Sanny Builder CLI. */
async function compile(inputPath: string, outputPath: string) {
  const sannyExe = Deno.env.get("SANNY") || "sanny.exe";
  // The CLI exits 0 on a compile error and writes it to compile.log beside sanny.exe instead
  const logPath = join(dirname(sannyExe), "compile.log");
  const logStamp = async () => {
    try {
      return (await Deno.stat(logPath)).mtime?.getTime();
    } catch {
      return undefined;
    }
  };
  const before = await logStamp();
  const command = new Deno.Command(sannyExe, {
    args: [
      "--no-splash",
      "--mode",
      "gta3_sbl",
      "--compile",
      inputPath,
      outputPath,
    ],
    stdout: "inherit",
  });
  const { success } = await command.output();
  if (!success) {
    say.error(`Failed to compile: ${inputPath}`);
    Deno.exit(1);
  }
  if (await logStamp() !== before) {
    const log = (await Deno.readTextFile(logPath)).trim();
    // An error inside an include names the include, not the input, and the compiler drops the failing line
    if (log) {
      say.error(`Failed to compile: ${inputPath}\n${log}`);
      Deno.exit(1);
    }
  }
  say.pass(`Compiled ${basename(inputPath)}`);
}

/** Hex SHA-256 of a string or bytes. */
async function sha256(data: string | Uint8Array<ArrayBuffer>): Promise<string> {
  const digest = await crypto.subtle.digest(
    "SHA-256",
    typeof data === "string" ? new TextEncoder().encode(data) : data,
  );
  return Array.from(new Uint8Array(digest))
    .map((b) => b.toString(16).padStart(2, "0"))
    .join("");
}

/** Hashes a script together with every file it `{$INCLUDE}`s, recursively (see includes.ts). */
async function hashScript(path: string): Promise<string> {
  const tree = await includeTree(path);
  const parts = await Promise.all(
    tree.files.map(async (file) => `${file}\n${await Deno.readTextFile(file)}`),
  );
  return await sha256(
    [...parts, ...tree.missing.map((file) => `missing:${file}`)].join("\n"),
  );
}

const cachePath = join(Deno.cwd(), "dist", ".build-cache.json");
let hashCache: Record<string, string> = {};
try {
  hashCache = JSON.parse(await Deno.readTextFile(cachePath));
} catch {
  // No cache yet; everything compiles
}

async function allExist(paths: string[]): Promise<boolean> {
  for (const path of paths) {
    try {
      await Deno.stat(path);
    } catch {
      return false;
    }
  }
  return true;
}

async function saveHash(key: string, hash: string) {
  hashCache[key] = hash;
  await ensureDir(dirname(cachePath));
  await Deno.writeTextFile(cachePath, JSON.stringify(hashCache, null, 2));
}

/**
 * Like make: compiles only when the hash of the script and the files it includes changed, or the output is
 * missing. A file the script does not include never makes it stale: global slots are pinned in variables.txt,
 * which main and every mission include, so a mission does not depend on main.txt.
 */
async function compileIfChanged(inputPath: string, outputPath: string) {
  const hash = await hashScript(inputPath);
  if (
    !forceArg && await allExist([outputPath]) && hashCache[inputPath] === hash
  ) {
    say.skip(`  Up to date: ${basename(inputPath)}`);
    return;
  }

  await compile(inputPath, outputPath);
  await saveHash(inputPath, hash);
}

/** Recursively copies src to dest, following symlinks so staging holds real files. */
async function copyFollowingSymlinks(src: string, dest: string) {
  const stat = await Deno.stat(src);
  if (stat.isDirectory) {
    await ensureDir(dest);
    for await (const entry of Deno.readDir(src)) {
      await copyFollowingSymlinks(
        join(src, entry.name),
        join(dest, entry.name),
      );
    }
  } else {
    await Deno.copyFile(src, dest);
  }
}

/**
 * Writes each manifested model's `*.pos` coordinates into data/maps/noodles/custom.ipl.
 * A `*.pos` file in the model's folder wins; without one, the IPL line is left as is.
 * IDE lines carry no coordinates, so custom.ide is never touched.
 */
async function syncPositions() {
  const iplPath = join(cwd, "data", "maps", "noodles", "custom.ipl");
  const manifestText = await Deno.readTextFile(
    join(cwd, "models", "manifest.csv"),
  );
  const iplText = await Deno.readTextFile(iplPath);
  const eol = iplText.includes("\r\n") ? "\r\n" : "\n";
  const iplLines = iplText.split(/\r?\n/);

  const rows = manifestText.trim().split(/\r?\n/);
  const header = rows[0].split(",").map((s) => s.trim());
  for (const rowText of rows.slice(1)) {
    const cells = rowText.split(",").map((s) => s.trim());
    if (cells.length < header.length) continue;
    const entry: Record<string, string> = {};
    header.forEach((k, idx) => (entry[k] = cells[idx]));
    if (!entry.dff) continue;

    const folderPath = join(cwd, "models", entry.folder);
    let posFile: string | undefined;
    try {
      for await (const f of Deno.readDir(folderPath)) {
        if (f.isFile && f.name.toLowerCase().endsWith(".pos")) {
          posFile = f.name;
          break;
        }
      }
    } catch {
      continue;
    }
    if (!posFile) continue;

    const coords = (await Deno.readTextFile(join(folderPath, posFile)))
      .trim().split(/[\s,]+/).map(Number);
    if (coords.length < 3 || coords.slice(0, 3).some(Number.isNaN)) {
      say.warn(`  Warning: ${posFile} is not "x y z"; skipped`);
      continue;
    }
    const [x, y, z] = coords.map((n) => n.toFixed(6));

    const modelName = basename(entry.dff, extname(entry.dff)).toLowerCase();
    const instLine =
      /^(\s*#?\s*\d+\s*,\s*(\S+?)\s*,\s*)[-+\d.eE]+(\s*,\s*)[-+\d.eE]+(\s*,\s*)[-+\d.eE]+(.*)$/;
    for (let i = 0; i < iplLines.length; i++) {
      const m = iplLines[i].match(instLine);
      if (!m || m[2].toLowerCase() !== modelName) continue;
      const synced = `${m[1]}${x}${m[3]}${y}${m[4]}${z}${m[5]}`;
      if (synced !== iplLines[i]) {
        iplLines[i] = synced;
        say.info(`  custom.ipl ${modelName} <- ${posFile}`);
      }
    }
  }

  const synced = iplLines.join(eol);
  if (synced !== iplText) await Deno.writeTextFile(iplPath, synced);
}

function formatMissionName(name: string): string {
  name = capitalCase(name);
  return name.startsWith("Side")
    ? name.replace(/^Side/, "Side:")
    : name.replace(/^(\d+)/, "$1:");
}

const cwd = Deno.cwd();
const distDir = join(cwd, "dist");
const stagingDir = join(distDir, ".staging");
const noodlesStagingDir = join(stagingDir, "modloader", "Noodles");
const dragDropZip = join(distDir, "Drag-N-Drop.zip");

// Ensure clean staging directory
try {
  await Deno.remove(stagingDir, { recursive: true });
} catch {
  // Ignored if stagingDir does not exist
}
await ensureDir(stagingDir);

// 0. Preserve existing zip assets (modloader/.data, modloader.ini, movies/GTAtitles.mpg)
try {
  const stat = await Deno.stat(dragDropZip);
  if (stat.isFile) {
    say.info("Preserving existing zip assets from Drag-N-Drop.zip...");
    const cmd = new Deno.Command("tar", {
      args: ["-xf", dragDropZip, "-C", stagingDir],
    });
    await cmd.output();
    // Clear out old Noodles mod directory in staging so only selected steps are packaged
    try {
      await Deno.remove(noodlesStagingDir, { recursive: true });
    } catch {
      // Ignored
    }
    // Remove obsolete loose CLEO scripts from the zip; the modloader copies are the ones that run
    try {
      for await (const entry of Deno.readDir(join(stagingDir, "CLEO"))) {
        if (entry.isFile && extname(entry.name) === ".cs") {
          await Deno.remove(join(stagingDir, "CLEO", entry.name));
        }
      }
    } catch {
      // Ignored if the zip has no CLEO folder
    }
    // Remove obsolete loose root models directory if present in zip
    try {
      await Deno.remove(join(stagingDir, "models"), { recursive: true });
    } catch {
      // Ignored
    }
  }
} catch {
  // Archive does not exist yet
}

await ensureDir(noodlesStagingDir);

// Step 1: Main Script
if (shouldRun(1, "main")) {
  say.heading("Step 1 (main): Compiling main script...");
  const mainInput = join(cwd, "main.txt");
  const mainOutput = join(cwd, "main.scm");
  await compileIfChanged(mainInput, mainOutput);

  const dataStagingDir = join(noodlesStagingDir, "data");
  await ensureDir(dataStagingDir);
  await Deno.copyFile(mainOutput, join(dataStagingDir, "main.scm"));
  say.done("  Packaged modloader/Noodles/data/main.scm");
}

// Packaging archive into dist/Drag-N-Drop.zip
say.heading("Packaging dist/Drag-N-Drop.zip...");
await ensureDir(distDir);
const topLevelEntries: string[] = [];
for await (const entry of Deno.readDir(stagingDir)) {
  topLevelEntries.push(entry.name);
}

if (topLevelEntries.length === 0) {
  say.error("No assets in staging directory to package.");
  Deno.exit(1);
}

const tarCmd = new Deno.Command("tar", {
  args: ["-a", "-cf", dragDropZip, "-C", stagingDir, ...topLevelEntries],
});
const { success: tarSuccess, stderr: tarStderr } = await tarCmd.output();
if (!tarSuccess) {
  say.error(
    "Failed to create Drag-N-Drop.zip:",
    new TextDecoder().decode(tarStderr),
  );
  Deno.exit(1);
}

// Clean up staging directory
try {
  await Deno.remove(stagingDir, { recursive: true });
} catch {
  // Ignored
}

say.pass("dist/Drag-N-Drop.zip built");
