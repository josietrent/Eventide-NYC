// Console output for every script CLI: colors for routine lines, emoji only for
// the major outcomes (compile, package, install, wait for the game, pass or fail).

import {
  bold,
  cyan,
  dim,
  green,
  red,
  setColorEnabled,
  yellow,
} from "@std/fmt/colors";

export { bold, cyan, dim, green, red, yellow };

// Color only for terminals, as `deno task` piped to a file should stay plain
setColorEnabled(Deno.stdout.isTerminal() && !Deno.noColor);

/** OSC 8 terminal hyperlink: clickable text that opens a URL. */
export function link(url: string, text: string): string {
  return `\x1b]8;;${url}\x1b\\${text}\x1b]8;;\x1b\\`;
}

export const say = {
  /** Plain output, such as report rows */
  text: (message = "") => console.log(message),
  /** A build step title */
  heading: (message: string) => console.log(bold(cyan(message))),
  /** Work in progress or a note */
  info: (message: string) => console.log(cyan(message)),
  /** A finished piece of work */
  done: (message: string) => console.log(green(message)),
  /** Work that was not needed or was left out */
  skip: (message: string) => console.log(dim(message)),
  warn: (message: string, ...rest: unknown[]) =>
    console.warn(yellow(message), ...rest),
  fail: (message: string, ...rest: unknown[]) =>
    console.error(red(message), ...rest),
  /** A major step that is waiting on something outside the script */
  wait: (message: string) => console.log(bold(cyan(`⏳ ${message}`))),
  /** A major step that passed */
  pass: (message: string) => console.log(bold(green(`✅ ${message}`))),
  /** A major step that failed */
  error: (message: string, ...rest: unknown[]) =>
    console.error(bold(red(`❌ ${message}`)), ...rest),
};
