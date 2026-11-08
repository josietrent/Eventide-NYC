/**
 * includes.ts
 *
 * Static analysis of Sanny `{$INCLUDE path}` directives: the dependency tree of a script, so the build
 * recompiles a script only when it or a file it includes changes, as make does with its prerequisites.
 */

import { dirname, join } from "@std/path";

/** A script and everything it includes. */
export interface IncludeTree {
  /** The script, then its includes in first-seen order, each once. */
  files: string[];
  /** Includes named by a file in the tree that do not exist. */
  missing: string[];
}

/** The text with `/* *\/` and `//` comments cut out, so a commented-out include is not a dependency. */
function withoutComments(text: string): string {
  return text.replace(/\/\*[\s\S]*?\*\//g, "").replace(/\/\/.*$/gm, "");
}

/**
 * The include tree of `path`. An include is resolved against the folder of the file that names it. A loop ends
 * where it closes, and a missing file is listed instead of thrown, so creating it later changes the tree.
 */
export async function includeTree(path: string): Promise<IncludeTree> {
  const files: string[] = [];
  const missing: string[] = [];
  const seen = new Set<string>();

  async function visit(file: string) {
    if (seen.has(file)) return;
    seen.add(file);
    let text: string;
    try {
      text = await Deno.readTextFile(file);
    } catch {
      missing.push(file);
      return;
    }
    files.push(file);
    for (
      const match of withoutComments(text).matchAll(
        /\{\$INCLUDE\s+([^}]+?)\s*\}/gi,
      )
    ) {
      await visit(join(dirname(file), match[1]));
    }
  }

  await visit(path);
  return { files, missing };
}
