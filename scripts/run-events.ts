/**
 * run-events.ts
 *
 * The lines `scripts/run.ts --events` prints on stdout for a program that launched it, such as the mission
 * editor: one JSON event per line behind a prefix, among the ordinary build and run output.
 */

import type { TimedPacket } from "./telemetry.ts";

export const EVENT_PREFIX = "@telemetry ";

export type RunEvent =
  /** The `DEBUG_*` channel names from constants.txt, by channel number. */
  | { kind: "names"; names: Record<string, string> }
  | { kind: "launch"; exe: string }
  /** One datagram from the game: a position, an F8 capture, or a debug reading. */
  | { kind: "packet"; event: TimedPacket }
  | {
    kind: "exit";
    code: number;
    /** False when the game wrote no log, so a crash is neither ruled in nor out. */
    known: boolean;
    crashed: boolean;
    signs: string[];
    exception?: string;
  };

export function eventLine(event: RunEvent): string {
  return EVENT_PREFIX + JSON.stringify(event);
}

/** The event on a line of run.ts output, or `null` for any other line. */
export function parseEventLine(line: string): RunEvent | null {
  if (!line.startsWith(EVENT_PREFIX)) return null;
  try {
    const event = JSON.parse(line.slice(EVENT_PREFIX.length));
    return typeof event?.kind === "string" ? event as RunEvent : null;
  } catch {
    return null;
  }
}
