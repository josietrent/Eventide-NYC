/**
 * build.ts
 *
 * Compiles Sanny Builder scripts and packages mod assets into dist/Drag-N-Drop.zip:
 *
 * Supports interactive bisection and step filtering via:
 * --bisect, -b, --step=<N>, --only=<name>, --skip=<name>
 * and recompiles every Sanny script, ignoring the hash cache, with --force.
 *
 * Model assets are bisected with --skip-asset=<asset|model|model:asset>[,...]
 * where <asset> is dff, txd, or col (e.g. --skip-asset=dinerind:col,dff), and a
 * bare <model> skips all of that model's assets (e.g. --skip-asset=dinerind).
 */

import { basename, dirname, extname, join } from "@std/path";
import { ensureDir } from "@std/fs/ensure-dir";
import { includeTree } from "./includes.ts";
import { say } from "./log.ts";
import { buildMenuTxd, buildSingleTextureTxd } from "./build-menu-txd.ts";
import { buildCyrillicFontAtlas } from "./build-cyrillic-font.ts";

// Bisection / filtering arguments
const args = Deno.args;
const isBisect = args.includes("--bisect") || args.includes("-b");
const forceArg = args.includes("--force");
const onlyArg = args
  .find((a) => a.startsWith("--only="))
  ?.slice(7)
  ?.split(",");
const skipArg = args
  .find((a) => a.startsWith("--skip="))
  ?.slice(7)
  ?.split(",");
const stepArg = args
  .find((a) => a.startsWith("--step="))
  ?.slice(7)
  ?.split(",")
  .map(Number);

const skipAssetArg = args
  .find((a) => a.startsWith("--skip-asset="))
  ?.slice(13)
  ?.split(",")
  .map((s) => s.trim().toLowerCase());

type ModelAsset = "dff" | "txd" | "col";
const assetAnswers = new Map<string, boolean>();

/** Whether a manifested model's asset is packaged, per --skip-asset or the --bisect prompt. */
function shouldPackageAsset(modelName: string, asset: ModelAsset): boolean {
  const name = modelName.toLowerCase();
  if (
    skipAssetArg?.includes(asset) ||
    skipAssetArg?.includes(name) ||
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
  if ((await logStamp()) !== before) {
    const log = (await Deno.readTextFile(logPath)).trim();
    // An error inside an include names the include, not the input, and the compiler drops the failing line
    if (log) {
      say.error(`Failed to compile: ${inputPath}\n${log}`);
      Deno.exit(1);
    }
  }
  say.pass(`Compiled ${basename(inputPath)}`);
}

/** Locates an x86 MSVC toolchain and Windows 10/11 SDK for building .asi plugins. */
async function findMsvcX86Toolchain(): Promise<{
  clExe: string;
  includes: string[];
  libs: string[];
} | null> {
  const vswhere =
    "C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vswhere.exe";
  let vsPath = "";
  try {
    const out = await new Deno.Command(vswhere, {
      args: [
        "-latest",
        "-products",
        "*",
        "-requires",
        "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
        "-property",
        "installationPath",
      ],
    }).output();
    if (out.success) {
      vsPath =
        new TextDecoder().decode(out.stdout).trim().split(/\r?\n/)[0] || "";
    }
  } catch {
    // vswhere unavailable
  }
  if (!vsPath) return null;

  const msvcBase = join(vsPath, "VC", "Tools", "MSVC");
  const msvcVersions: string[] = [];
  try {
    for await (const entry of Deno.readDir(msvcBase)) {
      if (entry.isDirectory) msvcVersions.push(entry.name);
    }
  } catch {
    return null;
  }
  msvcVersions.sort().reverse();

  let msvcDir = "";
  let clExe = "";
  for (const ver of msvcVersions) {
    const root = join(msvcBase, ver);
    for (const host of ["Hostx64", "Hostx86"]) {
      const candidate = join(root, "bin", host, "x86", "cl.exe");
      try {
        if ((await Deno.stat(candidate)).isFile) {
          msvcDir = root;
          clExe = candidate;
          break;
        }
      } catch {
        // Try next
      }
    }
    if (clExe) break;
  }
  if (!clExe) return null;

  const sdkRoots = [
    "D:\\Windows Kits\\10",
    "C:\\Program Files (x86)\\Windows Kits\\10",
    "C:\\Program Files\\Windows Kits\\10",
  ];
  for (const sdkRoot of sdkRoots) {
    const incBase = join(sdkRoot, "Include");
    const sdkVersions: string[] = [];
    try {
      for await (const entry of Deno.readDir(incBase)) {
        if (entry.isDirectory) sdkVersions.push(entry.name);
      }
    } catch {
      continue;
    }
    sdkVersions.sort().reverse();
    for (const ver of sdkVersions) {
      try {
        if ((await Deno.stat(join(incBase, ver, "um", "windows.h"))).isFile) {
          return {
            clExe,
            includes: [
              join(msvcDir, "include"),
              join(incBase, ver, "ucrt"),
              join(incBase, ver, "um"),
              join(incBase, ver, "shared"),
            ],
            libs: [
              join(msvcDir, "lib", "x86"),
              join(sdkRoot, "Lib", ver, "ucrt", "x86"),
              join(sdkRoot, "Lib", ver, "um", "x86"),
            ],
          };
        }
      } catch {
        // Try next SDK version
      }
    }
  }
  return null;
}

/** Compiles an .asi plugin DLL from C++ source when changed or missing. */
async function compileAsiIfChanged(
  sourcePath: string,
  outputPath: string,
  objDir: string,
  extraLibs: string[] = ["user32.lib"],
  extraDepPaths: string[] = [],
) {
  const depParts = await Promise.all(
    [sourcePath, ...extraDepPaths].map(
      async (p) => `${p}\n${await sha256(await Deno.readFile(p))}`,
    ),
  );
  const hash = await sha256(depParts.join("\n"));
  if (
    !forceArg &&
    (await allExist([outputPath])) &&
    hashCache[sourcePath] === hash
  ) {
    say.skip(`  Up to date: ${basename(outputPath)}`);
    return;
  }

  const toolchain = await findMsvcX86Toolchain();
  if (!toolchain) {
    if (await allExist([outputPath])) {
      say.info(
        `  MSVC x86 not found; reusing prebuilt ${basename(outputPath)}`,
      );
      return;
    }
    say.error(`MSVC x86 toolchain not found and ${outputPath} is missing.`);
    Deno.exit(1);
  }

  await ensureDir(dirname(outputPath));
  await ensureDir(objDir);
  const objPath = join(
    objDir,
    `${basename(sourcePath, extname(sourcePath))}.obj`,
  );
  const args = [
    "/nologo",
    "/LD",
    "/O2",
    "/MT",
    "/EHsc",
    "/W4",
    "/WX",
    ...toolchain.includes.map((dir) => `/I${dir}`),
    sourcePath,
    `/Fe:${outputPath}`,
    `/Fo:${objPath}`,
    "/link",
    ...toolchain.libs.map((dir) => `/LIBPATH:${dir}`),
    ...extraLibs,
  ];
  const out = await new Deno.Command(toolchain.clExe, { args }).output();
  if (!out.success) {
    say.error(
      `Failed to compile ${basename(sourcePath)}:\n` +
        new TextDecoder().decode(out.stdout) +
        new TextDecoder().decode(out.stderr),
    );
    Deno.exit(1);
  }

  say.pass(`Compiled ${basename(outputPath)}`);
  await saveHash(sourcePath, hash);
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
    !forceArg &&
    (await allExist([outputPath])) &&
    hashCache[inputPath] === hash
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

const cwd = Deno.cwd();
const distDir = join(cwd, "dist");
const modName = "Eventide";
const stagingDir = join(distDir, "staging");
const modStagingDir = join(stagingDir, "modloader", modName);
const dragDropZip = join(distDir, "Drag-N-Drop.zip");
const tarExe =
  Deno.build.os === "windows"
    ? (() => {
        const sysTar = join(
          Deno.env.get("SystemRoot") || "C:\\Windows",
          "System32",
          "tar.exe",
        );
        try {
          if (Deno.statSync(sysTar).isFile) return sysTar;
        } catch {
          // Fallback
        }
        return "tar.exe";
      })()
    : "tar";

// Ensure clean staging directory
try {
  await Deno.remove(stagingDir, { recursive: true });
} catch {
  // Ignored if stagingDir does not exist
}
await ensureDir(modStagingDir);

// 0. Preserve existing zip assets (modloader/.data, modloader.ini, movies/GTAtitles.mpg)
try {
  const stat = await Deno.stat(dragDropZip);
  if (stat.isFile) {
    say.info("Preserving existing zip assets from Drag-N-Drop.zip...");
    const cmd = new Deno.Command(tarExe, {
      args: ["-xf", dragDropZip, "-C", stagingDir],
    });
    await cmd.output();
    // Clear out old mod directory in staging so only selected steps are packaged
    try {
      await Deno.remove(modStagingDir, { recursive: true });
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

await ensureDir(modStagingDir);

// Step 1: Main Script
if (shouldRun(1, "main")) {
  say.heading("Step 1 (main): Compiling main script...");
  const mainInput = join(cwd, "main.txt");
  const mainOutput = join(cwd, "main.scm");
  await compileIfChanged(mainInput, mainOutput);

  const dataStagingDir = join(modStagingDir, "data");
  await ensureDir(dataStagingDir);
  await Deno.copyFile(mainOutput, join(dataStagingDir, basename(mainOutput)));
  say.done(`  Packaged modloader/${modName}/data/${basename(mainOutput)}`);
}

// Step 2: Help Thread
if (shouldRun(2, "help")) {
  say.heading("Step 2 (help): Compiling help thread...");
  const helpInput = join(cwd, "threads", "help.txt");
  const helpOutput = join(cwd, "threads", "help.cs");
  await compileIfChanged(helpInput, helpOutput);

  const cleoStagingDir = join(modStagingDir, "CLEO");
  await ensureDir(cleoStagingDir);
  await Deno.copyFile(helpOutput, join(cleoStagingDir, basename(helpOutput)));
  say.done(`  Packaged modloader/${modName}/CLEO/${basename(helpOutput)}`);
}

// Step 2b: Porter Tunnel Guard Thread (launched by help.cs)
if (shouldRun(2, "tunnel") || shouldRun(2, "help")) {
  const tunnelInput = join(cwd, "threads", "tunnel.txt");
  const tunnelOutput = join(cwd, "threads", "tunnel.cm");
  if (await allExist([tunnelInput])) {
    say.heading("Step 2b (tunnel): Compiling tunnel guard thread...");
    await compileIfChanged(tunnelInput, tunnelOutput);

    const cleoStagingDir = join(modStagingDir, "CLEO");
    await ensureDir(cleoStagingDir);
    await Deno.copyFile(
      tunnelOutput,
      join(cleoStagingDir, basename(tunnelOutput)),
    );
    say.done(`  Packaged modloader/${modName}/CLEO/${basename(tunnelOutput)}`);
  }
}

// Step 3: INIT Thread
if (shouldRun(3, "init")) {
  say.heading("Step 3 (init): Compiling INIT thread...");
  const helpInput = join(cwd, "threads", "init.txt");
  const helpOutput = join(cwd, "threads", "init.cm");
  await compileIfChanged(helpInput, helpOutput);

  const cleoStagingDir = join(modStagingDir, "CLEO");
  await ensureDir(cleoStagingDir);
  await Deno.copyFile(helpOutput, join(cleoStagingDir, basename(helpOutput)));
  say.done(`  Packaged modloader/${modName}/CLEO/${basename(helpOutput)}`);
}

// Step 4: Coords Thread
if (shouldRun(4, "coords")) {
  say.heading("Step 4 (coords): Compiling coords thread...");
  const coordsInput = join(cwd, "threads", "coords.txt");
  const coordsOutput = join(cwd, "threads", "coords.cs");
  await compileIfChanged(coordsInput, coordsOutput);

  const cleoStagingDir = join(modStagingDir, "CLEO");
  await ensureDir(cleoStagingDir);
  await Deno.copyFile(
    coordsOutput,
    join(cleoStagingDir, basename(coordsOutput)),
  );
  say.done(`  Packaged modloader/${modName}/CLEO/${basename(coordsOutput)}`);
}

// Step 5: Pistol Mission
if (shouldRun(5, "pistol")) {
  say.heading("Step 5 (pistol): Compiling pistol mission...");
  const pistolInput = join(cwd, "threads", "pistol.txt");
  const pistolOutput = join(cwd, "threads", "pistol.cm");
  await compileIfChanged(pistolInput, pistolOutput);

  const cleoStagingDir = join(modStagingDir, "CLEO");
  await ensureDir(cleoStagingDir);
  await Deno.copyFile(
    pistolOutput,
    join(cleoStagingDir, basename(pistolOutput)),
  );
  say.done(`  Packaged modloader/${modName}/CLEO/${basename(pistolOutput)}`);
}

// Step 6: Car Mission
if (shouldRun(6, "car")) {
  say.heading("Step 6 (car): Compiling car mission...");
  const carInput = join(cwd, "threads", "car.txt");
  const carOutput = join(cwd, "threads", "car.cm");
  await compileIfChanged(carInput, carOutput);

  const cleoStagingDir = join(modStagingDir, "CLEO");
  await ensureDir(cleoStagingDir);
  await Deno.copyFile(carOutput, join(cleoStagingDir, basename(carOutput)));
  say.done(`  Packaged modloader/${modName}/CLEO/${basename(carOutput)}`);
}

// Step 7: Alley Mission
if (shouldRun(7, "alley")) {
  say.heading("Step 7 (alley): Compiling alley mission...");
  const alleyInput = join(cwd, "threads", "alley.txt");
  const alleyOutput = join(cwd, "threads", "alley.cm");
  await compileIfChanged(alleyInput, alleyOutput);

  const cleoStagingDir = join(modStagingDir, "CLEO");
  await ensureDir(cleoStagingDir);
  await Deno.copyFile(alleyOutput, join(cleoStagingDir, basename(alleyOutput)));
  say.done(`  Packaged modloader/${modName}/CLEO/${basename(alleyOutput)}`);
}

// Step 8: Protection Mission
if (shouldRun(8, "protection")) {
  say.heading("Step 8 (protection): Compiling protection mission...");
  const protectionInput = join(cwd, "threads", "protection.txt");
  const protectionOutput = join(cwd, "threads", "protection.cm");
  await compileIfChanged(protectionInput, protectionOutput);

  const cleoStagingDir = join(modStagingDir, "CLEO");
  await ensureDir(cleoStagingDir);
  await Deno.copyFile(
    protectionOutput,
    join(cleoStagingDir, basename(protectionOutput)),
  );
  say.done(
    `  Packaged modloader/${modName}/CLEO/${basename(protectionOutput)}`,
  );
}

// Step 8b: Smuggle Mission
if (shouldRun(8, "smuggle") || shouldRun(8, "protection")) {
  const smuggleInput = join(cwd, "threads", "smuggle.txt");
  const smuggleOutput = join(cwd, "threads", "smuggle.cm");
  if (await allExist([smuggleInput])) {
    say.heading("Step 8b (smuggle): Compiling smuggle mission...");
    await compileIfChanged(smuggleInput, smuggleOutput);

    const cleoStagingDir = join(modStagingDir, "CLEO");
    await ensureDir(cleoStagingDir);
    await Deno.copyFile(
      smuggleOutput,
      join(cleoStagingDir, basename(smuggleOutput)),
    );
    say.done(`  Packaged modloader/${modName}/CLEO/${basename(smuggleOutput)}`);
  }
}

// Step 8c: Statue Mission
if (shouldRun(8, "statue")) {
  const statueInput = join(cwd, "threads", "statue.txt");
  const statueOutput = join(cwd, "threads", "statue.cm");
  if (await allExist([statueInput])) {
    say.heading("Step 8c (statue): Compiling statue mission...");
    await compileIfChanged(statueInput, statueOutput);

    const cleoStagingDir = join(modStagingDir, "CLEO");
    await ensureDir(cleoStagingDir);
    await Deno.copyFile(
      statueOutput,
      join(cleoStagingDir, basename(statueOutput)),
    );
    say.done(`  Packaged modloader/${modName}/CLEO/${basename(statueOutput)}`);
  }
}

// Step 8d: Sap Mission
if (shouldRun(8, "sap")) {
  const sapInput = join(cwd, "threads", "sap.txt");
  const sapOutput = join(cwd, "threads", "sap.cm");
  if (await allExist([sapInput])) {
    say.heading("Step 8d (sap): Compiling sap mission...");
    await compileIfChanged(sapInput, sapOutput);

    const cleoStagingDir = join(modStagingDir, "CLEO");
    await ensureDir(cleoStagingDir);
    await Deno.copyFile(sapOutput, join(cleoStagingDir, basename(sapOutput)));
    say.done(
      "  Packaged modloader/" + modName + "/CLEO/" + basename(sapOutput),
    );
  }
}

// Step 8e: Ashes Mission
if (shouldRun(8, "ashes")) {
  const ashesInput = join(cwd, "threads", "ashes.txt");
  const ashesOutput = join(cwd, "threads", "ashes.cm");
  if (await allExist([ashesInput])) {
    say.heading("Step 8e (ashes): Compiling ashes mission...");
    await compileIfChanged(ashesInput, ashesOutput);

    const cleoStagingDir = join(modStagingDir, "CLEO");
    await ensureDir(cleoStagingDir);
    await Deno.copyFile(
      ashesOutput,
      join(cleoStagingDir, basename(ashesOutput)),
    );
    say.done(
      "  Packaged modloader/" + modName + "/CLEO/" + basename(ashesOutput),
    );
  }
}

// Step 9: World Thread
if (shouldRun(9, "world")) {
  say.heading("Step 9 (world): Compiling world thread...");
  const worldInput = join(cwd, "threads", "world.txt");
  const worldOutput = join(cwd, "threads", "world.cs");
  await compileIfChanged(worldInput, worldOutput);

  const cleoStagingDir = join(modStagingDir, "CLEO");
  await ensureDir(cleoStagingDir);
  await Deno.copyFile(worldOutput, join(cleoStagingDir, basename(worldOutput)));
  say.done(`  Packaged modloader/${modName}/CLEO/${basename(worldOutput)}`);
}

// Step 10: Triangular Blips Thread
if (shouldRun(10, "triblips")) {
  say.heading("Step 10 (triblips): Compiling triblips thread...");
  const triInput = join(cwd, "threads", "triblips.txt");
  const triOutput = join(cwd, "threads", "triblips.cs");
  await compileIfChanged(triInput, triOutput);

  const cleoStagingDir = join(modStagingDir, "CLEO");
  await ensureDir(cleoStagingDir);
  await Deno.copyFile(triOutput, join(cleoStagingDir, basename(triOutput)));
  say.done(`  Packaged modloader/${modName}/CLEO/${basename(triOutput)}`);
}

// Package CLEO custom strings into CLEO/cleo_text (CLEO reads .fxt from here, not modloader)
const stringsSrc = join(cwd, "data", "CLEO", "strings.fxt");
if (await allExist([stringsSrc])) {
  const cleoTextDir = join(stagingDir, "CLEO", "cleo_text");
  await ensureDir(cleoTextDir);
  await Deno.copyFile(stringsSrc, join(cleoTextDir, "strings.fxt"));
  say.done("  Packaged CLEO/cleo_text/strings.fxt");
}

// Step 11: PauseRadio ASI Plugin
if (shouldRun(11, "pauseradio")) {
  say.heading("Step 11 (pauseradio): Compiling PauseRadio.asi...");
  const asiInput = join(cwd, "plugins", "PauseRadio", "PauseRadio.cpp");
  const asiOutput = join(cwd, "plugins", "PauseRadio", "PauseRadio.asi");
  await compileAsiIfChanged(asiInput, asiOutput, join(stagingDir, "obj"));

  await Deno.copyFile(asiOutput, join(modStagingDir, basename(asiOutput)));
  say.done(`  Packaged modloader/${modName}/${basename(asiOutput)}`);
}

// Step 12: PauseMusic ASI Plugin
if (shouldRun(12, "pausemusic")) {
  say.heading("Step 12 (pausemusic): Compiling PauseMusic.asi...");
  const asiInput = join(cwd, "plugins", "PauseMusic", "PauseMusic.cpp");
  const asiOutput = join(cwd, "plugins", "PauseMusic", "PauseMusic.asi");
  const iniInput = join(cwd, "plugins", "PauseMusic", "PauseMusic.ini");
  await compileAsiIfChanged(asiInput, asiOutput, join(stagingDir, "obj"));

  await Deno.copyFile(asiOutput, join(modStagingDir, basename(asiOutput)));
  await Deno.copyFile(iniInput, join(modStagingDir, basename(iniInput)));
  say.done(`  Packaged modloader/${modName}/${basename(asiOutput)}`);
  say.done(`  Packaged modloader/${modName}/${basename(iniInput)}`);
}

// Step 13: MenuLogo ASI Plugin & eventide_menu.txd & Splash Screens
if (shouldRun(13, "menulogo")) {
  say.heading(
    "Step 13 (menulogo): Compiling MenuLogo.asi & Menu/Splash TXDs...",
  );
  const asiInput = join(cwd, "plugins", "MenuLogo", "MenuLogo.cpp");
  const asiOutput = join(cwd, "plugins", "MenuLogo", "MenuLogo.asi");
  const pngInput = join(cwd, "plugins", "MenuLogo", "eventidelogo128.png");
  const titlePngInput = join(cwd, "plugins", "MenuLogo", "gtalogo128.png");
  const txdOutput = join(cwd, "plugins", "MenuLogo", "eventide_menu.txd");
  await compileAsiIfChanged(asiInput, asiOutput, join(stagingDir, "obj"));
  await buildMenuTxd(pngInput, txdOutput, titlePngInput);

  await Deno.copyFile(asiOutput, join(modStagingDir, basename(asiOutput)));
  await Deno.copyFile(txdOutput, join(modStagingDir, basename(txdOutput)));
  say.done(`  Packaged modloader/${modName}/${basename(asiOutput)}`);
  say.done(`  Packaged modloader/${modName}/${basename(txdOutput)}`);

  // Build & package Splash / Startup Loading Screen TXDs into modloader/Eventide/txd/
  const txdStagingDir = join(modStagingDir, "txd");
  await ensureDir(txdStagingDir);

  const splash1Png = join(cwd, "plugins", "MenuLogo", "splash1.png");
  const splash1Txd = join(cwd, "plugins", "MenuLogo", "SPLASH1.txd");
  if (await allExist([splash1Png])) {
    const splashHash = await sha256(await Deno.readFile(splash1Png));
    if (
      forceArg ||
      !(await allExist([splash1Txd])) ||
      hashCache[splash1Txd] !== splashHash
    ) {
      await buildSingleTextureTxd("splash1", splash1Png, splash1Txd);
      await saveHash(splash1Txd, splashHash);
    }
    await Deno.copyFile(splash1Txd, join(txdStagingDir, "SPLASH1.txd"));
    say.done(`  Packaged modloader/${modName}/txd/SPLASH1.txd`);
  }

  const mainsc1Png = join(cwd, "plugins", "MenuLogo", "mainsc1.png");
  const mainsc1Txd = join(cwd, "plugins", "MenuLogo", "mainsc1.txd");
  const mainsc2Txd = join(cwd, "plugins", "MenuLogo", "mainsc2.txd");
  if (await allExist([mainsc1Png])) {
    const mainscHash = await sha256(await Deno.readFile(mainsc1Png));
    if (
      forceArg ||
      !(await allExist([mainsc1Txd])) ||
      hashCache[mainsc1Txd] !== mainscHash
    ) {
      await buildSingleTextureTxd("mainsc1", mainsc1Png, mainsc1Txd);
      await saveHash(mainsc1Txd, mainscHash);
    }
    if (
      forceArg ||
      !(await allExist([mainsc2Txd])) ||
      hashCache[mainsc2Txd] !== mainscHash
    ) {
      await buildSingleTextureTxd("mainsc2", mainsc1Png, mainsc2Txd);
      await saveHash(mainsc2Txd, mainscHash);
    }
    await Deno.copyFile(mainsc1Txd, join(txdStagingDir, "mainsc1.txd"));
    await Deno.copyFile(mainsc2Txd, join(txdStagingDir, "mainsc2.txd"));
    say.done(`  Packaged modloader/${modName}/txd/mainsc1.txd`);
    say.done(`  Packaged modloader/${modName}/txd/mainsc2.txd`);
  }
}

// Step 14: WeaponSprint ASI Plugin
if (shouldRun(14, "weaponsprint")) {
  say.heading("Step 14 (weaponsprint): Compiling WeaponSprint.asi...");
  const asiInput = join(cwd, "plugins", "WeaponSprint", "WeaponSprint.cpp");
  const asiOutput = join(cwd, "plugins", "WeaponSprint", "WeaponSprint.asi");
  await compileAsiIfChanged(asiInput, asiOutput, join(stagingDir, "obj"));

  await Deno.copyFile(asiOutput, join(modStagingDir, basename(asiOutput)));
  say.done(`  Packaged modloader/${modName}/${basename(asiOutput)}`);
}

// Step 15: CyrillicText ASI Plugin & Caveat-Bold Atlas TXD
if (shouldRun(15, "cyrillic")) {
  say.heading(
    "Step 15 (cyrillic): Compiling CyrillicText.asi & Cyrillic TXD...",
  );
  const ttfInput = join(cwd, "plugins", "MenuLogo", "Caveat-Bold.ttf");
  const genScript = join(cwd, "scripts", "build-cyrillic-font.ts");
  const txdOutput = join(
    cwd,
    "plugins",
    "CyrillicText",
    "eventide_cyrillic.txd",
  );
  const headerOutput = join(cwd, "plugins", "CyrillicText", "cyrillic_atlas.h");
  const asiInput = join(cwd, "plugins", "CyrillicText", "CyrillicText.cpp");
  const asiOutput = join(cwd, "plugins", "CyrillicText", "CyrillicText.asi");

  const atlasHash = await sha256(
    `${await sha256(await Deno.readFile(ttfInput))}\n${await sha256(await Deno.readFile(genScript))}`,
  );
  if (
    forceArg ||
    !(await allExist([txdOutput, headerOutput])) ||
    hashCache[txdOutput] !== atlasHash
  ) {
    await buildCyrillicFontAtlas(ttfInput, txdOutput, headerOutput);
    await saveHash(txdOutput, atlasHash);
    say.pass("Generated eventide_cyrillic.txd & cyrillic_atlas.h");
  } else {
    say.skip("  Up to date: eventide_cyrillic.txd");
  }

  await compileAsiIfChanged(
    asiInput,
    asiOutput,
    join(stagingDir, "obj"),
    ["user32.lib"],
    [headerOutput],
  );

  await Deno.copyFile(asiOutput, join(modStagingDir, basename(asiOutput)));
  await Deno.copyFile(txdOutput, join(modStagingDir, basename(txdOutput)));
  say.done(`  Packaged modloader/${modName}/${basename(asiOutput)}`);
  say.done(`  Packaged modloader/${modName}/${basename(txdOutput)}`);
}

// Step 16: EmergencyRadio ASI Plugin
if (shouldRun(16, "emergencyradio")) {
  say.heading("Step 16 (emergencyradio): Compiling EmergencyRadio.asi...");
  const asiInput = join(cwd, "plugins", "EmergencyRadio", "EmergencyRadio.cpp");
  const asiOutput = join(
    cwd,
    "plugins",
    "EmergencyRadio",
    "EmergencyRadio.asi",
  );
  await compileAsiIfChanged(asiInput, asiOutput, join(stagingDir, "obj"));

  await Deno.copyFile(asiOutput, join(modStagingDir, basename(asiOutput)));
  say.done(`  Packaged modloader/${modName}/${basename(asiOutput)}`);
}

// Clean compiler scratch directory before zipping
try {
  await Deno.remove(join(stagingDir, "obj"), { recursive: true });
} catch {
  // Ignored
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

const tarCmd = new Deno.Command(tarExe, {
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
