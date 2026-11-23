# Streetlight & Traffic Signal Knockdown Side-Mission + Traffic Law System

Design note and implementation reference for the ambient streetlight/traffic-signal knockdown reward system, always-on AI driver taming, and post-Staunton "Be Good" traffic-law enforcement in _Eventide III_.

> [!NOTE]
> **Status:** Implemented (Ambient Free-Roam + Combo Multiplier, Always-On AI Taming, Post-Staunton Traffic Law)  
> **Implementation:** [`plugins/TrafficLaw/TrafficLaw.cpp`](../plugins/TrafficLaw/TrafficLaw.cpp) (`TrafficLaw.asi`) + [`threads/poles.txt`](../threads/poles.txt) (`poles.cs`) + [`utils/globals.txt`](../utils/globals.txt) (`$PoleKnockTotal`, `$PoleKnockLastModel`, `$PoleKnockEnabled`, `$TrafficLawActive`, `$TrafficLawEvent`, `$TrafficLawFine`, `$TrafficLawValue`).

---

## 1. Concept

1. **Ambient Streetlight & Fire Hydrant Knockdown Minigame:** Reward the player with cash in free roam for knocking down **streetlights**, **park lights**, **traffic signals**, and **fire hydrants** around Liberty City, with a streak/combo multiplier for consecutive hits.
2. **Always-On AI Driver Taming:** Constantly calms civilian AI drivers regardless of story completion so traffic behaves cleanly.
3. **Post-Staunton "Be Good" Traffic Law Enforcement:** Once Staunton is completed (`$BarrierCOM == 0`, `$Intro_Passed == 1`, `$onmission == 0`), knocking down poles or damaging city property near cops suppresses the minigame payout and issues fines/wanted stars, alongside speeding, red-light, and reckless hit-and-run enforcement.

### Implemented Behavior

- **Ambient Free-Roam Bonus + Combo Streak:** Knocking down qualifying poles or fire hydrants during freeroam (`$Intro_Passed == 1`, `$onmission == 0`) awards cash immediately (unless cop-witnessed after Staunton is complete):
  - **Standard lampposts & park/coastal lights** (`lamppost1`, `lamppost2`, `lamppost3`, `Streetlamp1`, `lampost_coast`): **\$25** base
  - **Double-arm median (`doublestreetlght1`), Double park light (`Streetlamp2`), Traffic signals (`trafficlight1`), & Fire hydrants (`fire_hydrant`)**: **\$50** base
  - **7-second Combo Window (`TIMERA <= 7000`):** Consecutive knockdowns build a multiplier up to **5×** (up to **\$125** / **\$250** per hit), accompanied by a `SoundPartMissionComplete` chime and HUD banner (`POLE1`–`POLE6`, `POLEC` in [`data/CLEO/strings.fxt`](../data/CLEO/strings.fxt)).
- **Post-Staunton Property Damage & Traffic Laws (`TLAW1`–`TLAW8`):**
  - If `$TrafficLawActive == 1`, damaging tracked poles or city property (`fire_hydrant`, `fencesmallb`, `bollardlight`, `parkingmeter`, `bin1`, etc.) within cop sight/radius cancels the minigame payout, deducts a fine, and sets at least `1★`.
  - Speeding (`35` mph street / `55` mph bridge & tunnel with grace buffers), running red lights, and high-speed forward crashes issue cop-witnessed (`1★` + fine) or traffic-camera citations (3 strikes in 90s escalate to `1★`).

---

## 2. Target Knockdown Models (`generic.IDE` & `object.dat`)

| Model ID | Model Name          | Base Payout | Notes                          |
| -------- | ------------------- | ----------- | ------------------------------ |
| `1322`   | `fire_hydrant`      | `\$50`      | Fire hydrant                   |
| `1342`   | `lampost_coast`     | `\$25`      | Coastal lamppost               |
| `1345`   | `lamppost3`         | `\$25`      | Secondary streetlamp           |
| `1352`   | `Streetlamp2`       | `\$50`      | Double-arm park light          |
| `1353`   | `Streetlamp1`       | `\$25`      | Single-arm park light          |
| `1393`   | `lamppost1`         | `\$25`      | Single-arm urban lamppost      |
| `1394`   | `doublestreetlght1` | `\$50`      | Double-arm median streetlight  |
| `1404`   | `lamppost2`         | `\$25`      | Tall highway/arterial lamppost |
| `1405`   | `trafficlight1`     | `\$50`      | Intersection traffic signal    |

---

## 3. Why Pure CLEO/SCM Is Not Ideal

GTA III's SCM (`main.scm` / CLEO) has **no built-in opcode** to:

- Enumerate map-placed IPL props (`CDummyObject` -> `CObject` pool entries spawned dynamically near the camera),
- Detect when a static pole or city prop is uprooted/damaged (`CPhysical::ApplyCollision` clearing `bIsStatic` at `CEntity + 0x51 & 0x04` or setting `bHasBeenDamaged` at `CObject + 0x175 & 0x08`) and verify player attribution, or
- Inspect `CTrafficLights` states and AI `CAutopilot` parameters directly.

---

## 4. Architecture (ASI + CLEO Hybrid)

Follows the project's established pattern (`plugins/*.cpp` compiled via [`scripts/build.ts`](../scripts/build.ts) into ModLoader `.asi` plugins, paired with CLEO scripts and pinned globals in [`utils/globals.txt`](../utils/globals.txt)):

### A. C++ ASI Plugin (`plugins/TrafficLaw/TrafficLaw.cpp`)

1. **Collision Uproot & Damage Hook (`CPhysical::ApplyCollision` @ `0x4973A0`):**
   - Inspects collisions where either entity is an intact target pole or tracked city prop (`fencesmallb` `1279`, `fire_hydrant` `1322`, `bollardlight` `1328`, `parkingmeter` `1346`, `bin1` `1347`, `roadworkbarrier1` `1349`, `bussign1` `1350`, `noparkingsign1` `1354`, `phonesign` `1355`, `wastebin` `1356`, `strtbarrier01` `1359`, `trafficcone` `1360`, `postbox1` `1396`, `parkbench1` `1398`).
   - After `ApplyCollision` executes, verifies `bIsStatic` transitioned `1 -> 0` or `bHasBeenDamaged` transitioned `0 -> 1` and attributes the hit to `FindPlayerVehicle()`, `FindPlayerPed()`, or an NPC vehicle bumped by the player within `2500` ms.
   - Also detects post-Staunton at-fault forward high-speed vehicle collisions against occupied NPC vehicles.
2. **Always-On AI Driver Taming (`CAutomobile::ProcessControl` @ `0x531470`):**
   - Runs regardless of story progress for ambient civilian traffic (`STATUS_SIMPLE`/`STATUS_PHYSICS`, `CAR_MISSION_CRUISE`, non-mission, non-siren): prevents road-rage `AVOID_CARS`/`PLOUGH_THROUGH` flips, caps cruise/traffic speed, and suppresses reversing at red traffic lights.
3. **Frame Tick (`CTheScripts::Process` @ `0x439040`):**
   - Prunes counted `CObject*` pointers (`CPools::ms_pObjectPool` @ `0x880E28`, stride `0x19C`) so fallen props never double-count while allowing re-streamed props to reset.
   - Evaluates post-Staunton speeding and `CTrafficLights` red-light violations when `$TrafficLawActive == 1`.
4. **Bridge to CLEO:**
   - Updates `$PoleKnockTotal` (`5146`) / `$PoleKnockLastModel` (`5147`) and queues `$TrafficLawEvent` (`5154`), `$TrafficLawFine` (`5155`), and `$TrafficLawValue` (`5156`) in `CTheScripts::ScriptSpace` (`0x74B248`).

### B. CLEO Script (`threads/poles.txt`)

1. Waits for `Player.IsPlaying($player)`, fade-in, and `Game.HasSaveGameFinished()`, then syncs `lastTotal = $PoleKnockTotal` so loading a savegame never pays out past totals.
2. Maintains `$TrafficLawActive` (`1` when `$BarrierCOM == 0`, `$Intro_Passed == 1`, and `$onmission == 0`; `0` otherwise) and processes queued `$TrafficLawEvent` fines (clamped to current cash) and `TLAW1`–`TLAW8` banners.
3. Monitors `$PoleKnockTotal` in a `wait 0` loop; keeps `lastTotal` synced silently during `$Intro_Passed == 0` or `$onmission == 1`, and awards freeroam pole/hydrant knockdown cash + `7`s combo multipliers (`POLE1`–`POLE6`, `POLEC`).
