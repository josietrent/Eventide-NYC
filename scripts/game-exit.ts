/**
 * game-exit.ts
 *
 * Tells a crash from a normal quit of GTA III by the game's logs. The process exit code cannot: WinMain returns
 * the wParam of the last window message, and "quit game" ends the loop with `RsGlobal.quit` instead of WM_QUIT,
 * so a normal quit exits with an arbitrary code (30193, for one). CLEO writes `--Game Shutdown--` and SCRLog
 * writes `Logging finished` only on a clean shutdown, and ModLoader logs an unhandled exception.
 */

/** The text of each log written by this launch; "" when the game did not write it. */
export interface ExitLogs {
  cleo: string;
  scrlog: string;
  modloader: string;
}

export interface ExitDiagnosis {
  /** False when no log was written, so a crash can be neither ruled in nor out. */
  known: boolean;
  crashed: boolean;
  signs: string[];
  /** The ModLoader line naming the fault address, when there is one. */
  exception?: string;
}

export function diagnoseExit(logs: ExitLogs): ExitDiagnosis {
  if (!logs.cleo && !logs.scrlog) {
    return { known: false, crashed: false, signs: [] };
  }
  const signs: string[] = [];
  if (logs.cleo && !/--Game Shutdown--/.test(logs.cleo)) {
    signs.push("cleo.log has no shutdown marker");
  }
  if (logs.scrlog && !/Logging finished/.test(logs.scrlog)) {
    signs.push("scrlog.log has no footer");
  }
  const exception = logs.modloader.split(/\r?\n/).findLast((line) =>
    line.startsWith("Unhandled exception")
  );
  if (exception) signs.push("modloader.log has an unhandled exception");
  return { known: true, crashed: signs.length > 0, signs, exception };
}
