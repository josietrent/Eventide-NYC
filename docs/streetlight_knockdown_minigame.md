# Streetlight & Traffic Signal Knockdown Side-Mission

Design note and implementation reference for the ambient streetlight/traffic-signal knockdown reward system in _Eventide III_.

> [!NOTE]
> **Status:** Implemented (Ambient Free-Roam + Combo Multiplier)  
> **Implementation:** [`plugins/PoleKnockdown/PoleKnockdown.cpp`](../plugins/PoleKnockdown/PoleKnockdown.cpp) (`PoleKnockdown.asi`) + [`threads/poles.txt`](../threads/poles.txt) (`poles.cs`) + [`utils/globals.txt`](../utils/globals.txt) (`$PoleKnockTotal`, `$PoleKnockLastModel`, `$PoleKnockEnabled`).

---

## 1. Concept

Reward the player with cash in free roam for knocking down **streetlights** and **traffic signals** around Liberty City, with a streak/combo multiplier for consecutive hits.

### Implemented Behavior

- **Ambient Free-Roam Bonus + Combo Streak:** Knocking down qualifying poles during freeroam (`$Intro_Passed == 1`, `$onmission == 0`) awards cash immediately:
  - **Standard lampposts** (`lamppost1`, `lamppost2`, `lamppost3`): **\$25** base
  - **Double-arm median (`doublestreetlght1`) & Traffic signals (`trafficlight1`)**: **\$50** base
  - **7-second Combo Window (`TIMERA <= 7000`):** Consecutive knockdowns build a multiplier up to **5×** (up to **\$125** per standard lamp / **\$250** per signal or double-arm lamp), accompanied by a `SoundPartMissionComplete` chime and HUD banner (`POLE1`–`POLE3`, `POLEC` in [`data/CLEO/strings.fxt`](../data/CLEO/strings.fxt)).

---

## 2. Target Models (`generic.IDE` & `object.dat`)

In GTA III's `data/object.dat`, all five target road poles share **Collision Response Case `1` (`lampost`)**, `CollisionDamageEffect = 0`, and an `Uproot Limit` of `50.0`:

| Model ID | Model Name          | `object.dat` Uproot Limit | Collision Response | Base Payout | Notes                          |
| -------- | ------------------- | ------------------------- | ------------------ | ----------- | ------------------------------ |
| `1393`   | `lamppost1`         | `50.0`                    | `1` (`lampost`)    | `\$25`      | Single-arm urban lamppost      |
| `1404`   | `lamppost2`         | `50.0`                    | `1` (`lampost`)    | `\$25`      | Tall highway/arterial lamppost |
| `1345`   | `lamppost3`         | `50.0`                    | `1` (`lampost`)    | `\$25`      | Secondary streetlamp           |
| `1394`   | `doublestreetlght1` | `50.0`                    | `1` (`lampost`)    | `\$50`      | Double-arm median streetlight  |
| `1405`   | `trafficlight1`     | `50.0`                    | `1` (`lampost`)    | `\$50`      | Intersection traffic signal    |

Note: `Streetlamp1` (`1353`), `Streetlamp2` (`1352`), and `lampost_coast` (`1342`) have no `object.dat` entry (non-uprootable static props), and `bollardlight` (`1328`) uses Collision Response `0`.

---

## 3. Why Pure CLEO/SCM Is Not Ideal

GTA III's SCM (`main.scm` / CLEO) has **no built-in opcode** to:

- Enumerate map-placed IPL props (`CDummyObject` -> `CObject` pool entries spawned dynamically near the camera), or
- Detect when a static pole is uprooted (`CPhysical::ApplyCollision` clearing `bIsStatic` at `CEntity + 0x51 & 0x04`) and verify player attribution.

---

## 4. Architecture (ASI + CLEO Hybrid)

Follows the project's established pattern (`plugins/*.cpp` compiled via [`scripts/build.ts`](../scripts/build.ts) into ModLoader `.asi` plugins, paired with CLEO scripts and pinned globals in [`utils/globals.txt`](../utils/globals.txt)):

### A. C++ ASI Plugin (`plugins/PoleKnockdown/PoleKnockdown.cpp`)

1. **Collision Uproot Hook (`CPhysical::ApplyCollision` @ `0x4973A0`):**
   - Inspects collisions where either entity is an upright (`bIsStatic == 1`) target pole (`1345`, `1393`, `1394`, `1404`, `1405`).
   - After `ApplyCollision` executes, verifies `bIsStatic` transitioned `1 -> 0` and attributes the hit to `FindPlayerVehicle()`, `FindPlayerPed()`, or an NPC vehicle bumped by the player within `2500` ms.
2. **Deduplication (`CTheScripts::Process` @ `0x439040`):**
   - Tracks uprooted `CObject*` pointers (`CPools::ms_pObjectPool` @ `0x880E28`, stride `0x19C`) so driving over a fallen pole never double-counts, while clearing entries if the pole streams out or respawns upright (`bIsStatic == 1`).
3. **Bridge to CLEO:**
   - Increments `$PoleKnockTotal` (`5146`) and updates `$PoleKnockLastModel` (`5147`) directly in `CTheScripts::ScriptSpace` (`0x74B248`), respecting `$PoleKnockEnabled` (`5148`, disabled only if set to `2`).

### B. CLEO Script (`threads/poles.txt`)

1. Waits for `Player.IsPlaying($player)`, fade-in, and `Game.HasSaveGameFinished()`, then syncs `lastTotal = $PoleKnockTotal` so loading a savegame never pays out past totals.
2. Monitors `$PoleKnockTotal` in a `wait 0` loop; keeps `lastTotal` synced silently during `$Intro_Passed == 0` or `$onmission == 1` so story mission HUD/text is undisturbed.
3. On freeroam knockdowns, calculates base payout + `7`s combo multiplier (`1x`–`5x`), grants cash via `Player.AddScore($player, payout)`, plays `SoundPartMissionComplete`, and prints `POLE1`/`POLE2`/`POLE3`/`POLEC`.
