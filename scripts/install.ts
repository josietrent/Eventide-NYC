/**
 * install.ts
 *
 * Installs the Noodles to Nightmares mod by extracting dist/Drag-N-Drop.zip
 * over into the GTA III game directory, and ensuring the unflagged runner
 * hard-link exists.
 */

import { join } from "@std/path";
import { say } from "./log.ts";

const gta3Base = Deno.env.get("GTA3");
if (!gta3Base) {
  say.error("GTA3 environment variable is not set in .env");
  Deno.exit(1);
}

const cwd = Deno.cwd();
const dragDropZip = join(cwd, "dist", "Drag-N-Drop.zip");

try {
  const stat = await Deno.stat(dragDropZip);
  if (!stat.isFile) {
    throw new Error("Not a file");
  }
} catch {
  say.error(
    `Drag-N-Drop archive not found at ${dragDropZip}. Run 'deno task build' first.`,
  );
  Deno.exit(1);
}

say.heading(`Installing mod from ${dragDropZip} into ${gta3Base}...`);

// Clean existing modloader/Eventide directory before extraction
const modDestDir = join(gta3Base, "modloader", "Eventide");
try {
  await Deno.remove(modDestDir, { recursive: true });
  say.done("  Cleaned existing modloader/Eventide directory");
} catch {
  // Ignored if directory doesn't exist
}

const cmd = new Deno.Command("tar", {
  args: ["-xf", dragDropZip, "-C", gta3Base],
});
const { success, stderr } = await cmd.output();
if (!success) {
  say.error(
    "Failed to extract Drag-N-Drop.zip via tar:",
    new TextDecoder().decode(stderr),
  );
  Deno.exit(1);
}
say.done("  Extracted Drag-N-Drop package assets into game directory");

/**
 * Lints for loose copies of packaged scripts in `[gta3]/CLEO`. CLEO loads that
 * folder directly, so a stale copy there can shadow the modloader one.
 */
async function findShadowingCleoFiles(
  packagedDir: string,
  looseDir: string,
): Promise<string[]> {
  const shadowing: string[] = [];
  let looseNames: Set<string>;
  try {
    looseNames = new Set<string>();
    for await (const entry of Deno.readDir(looseDir)) {
      if (entry.isFile) looseNames.add(entry.name.toLowerCase());
    }
  } catch {
    return shadowing; // No loose folder, nothing can shadow
  }
  try {
    for await (const entry of Deno.readDir(packagedDir)) {
      if (entry.isFile && looseNames.has(entry.name.toLowerCase())) {
        shadowing.push(join(looseDir, entry.name));
      }
    }
  } catch {
    // Nothing packaged in this folder
  }
  return shadowing;
}

const shadowedFiles = [
  ...await findShadowingCleoFiles(
    join(modDestDir, "cleo"),
    join(gta3Base, "CLEO"),
  ),
  ...await findShadowingCleoFiles(
    join(modDestDir, "cleo", "cleo_text"),
    join(gta3Base, "CLEO", "CLEO_TEXT"),
  ),
];
if (shadowedFiles.length > 0) {
  say.error(
    "Conflict: these files in [gta3]/CLEO duplicate packaged modloader/Eventide files and may be stale:",
  );
  for (const file of shadowedFiles) say.fail(`      ${file}`);
  say.fail("    Remove them so the modloader copies are the ones that run.");
  Deno.exit(1);
}

// Ensure custom strings are installed in CLEO/cleo_text
const customStrings = join(gta3Base, "CLEO", "cleo_text", "strings.fxt");
try {
  const stat = await Deno.stat(customStrings);
  if (stat.isFile) {
    say.done("  Verified custom strings installed in CLEO");
  }
} catch {
  const localBuilt = join(
    cwd,
    "data",
    "CLEO",
    "strings.fxt",
  );
  try {
    if ((await Deno.stat(localBuilt)).isFile) {
      const cleoDir = join(gta3Base, "CLEO");
      await Deno.mkdir(cleoDir, { recursive: true });
      await Deno.copyFile(localBuilt, customStrings);
      say.done("  Copied custom strings into CLEO");
    }
  } catch {
    // Optional
  }
}

// Ensure unflagged runner hard-link exists (bypasses Windows UAC elevation)
const gtaExe = join(gta3Base, "gta3-game.exe");
try {
  await Deno.stat(gtaExe);
} catch {
  try {
    await Deno.link(gtaExe, gtaExe);
    say.done("  Created runner link gta3-game.exe (bypasses UAC)");
  } catch {
    // Ignore if link unsupported
  }
}

say.pass("Noodles to Nightmares mod installation complete");
