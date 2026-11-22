# Streetlight & Traffic Signal Knockdown Side-Mission

Design note and proposed implementation architecture for a streetlight/traffic-signal knockdown side-mission or ambient reward system in _Eventide III_.

> [!NOTE]
> **Status:** Proposed (not yet implemented)  
> **Feasibility:** High — safe and straightforward using a small C++ `.asi` helper paired with CLEO, matching the existing architecture in [`plugins/FleeOnShot/FleeOnShot.cpp`](../plugins/FleeOnShot/FleeOnShot.cpp).

---

## 1. Concept

Reward the player with cash (either passively in free roam or as a timed/tiered side-mission) for knocking down **streetlights** and **traffic signals** around Liberty City.

### Possible Modes
1. **Ambient Free-Roam Bonus:** Every uprooted pole awards a small cash payout (with an optional combo/streak multiplier when poles are knocked down within a short window).
2. **Dedicated Side-Mission:** Triggered from a parked vehicle or contact marker; gives the player a countdown (or quota) to knock down as many poles as possible before time expires, paying out per pole plus tier/combo bonuses.

---

## 2. Target Models (`generic.IDE` & `object.dat`)

In GTA III's `data/object.dat`, standard breakable/uprootable poles share **Collision Response Case `1` (`lampost`)** and an `Uproot Limit` of `50.0`:

| Model ID | Model Name | `object.dat` Uproot Limit | Collision Response | Notes |
| --- | --- | --- | --- | --- |
| `1393` | `lamppost1` | `50.0` | `1` (`lampost`) | Single-arm urban lamppost |
| `1404` | `lamppost2` | `50.0` | `1` (`lampost`) | Tall highway/arterial lamppost |
| `1345` | `lamppost3` | `50.0` | `1` (`lampost`) | Secondary streetlamp |
| `1394` | `doublestreetlght1` | `50.0` | `1` (`lampost`) | Double-arm median streetlight |
| `1405` | `trafficlight1` | `50.0` | `1` (`lampost`) | Intersection traffic signal |

Optional secondary targets (depending on whether smaller park/bollard lamps should count):
- `1352` (`Streetlamp2`), `1353` (`Streetlamp1`), `1342` (`lampost_coast`), `1328` (`bollardlight`).

---

## 3. Why Pure CLEO/SCM Is Not Ideal

GTA III's SCM (`main.scm` / CLEO) has **no built-in opcode** to:
- Enumerate map-placed IPL props (`CDummyObject` -> `CObject` pool entries spawned dynamically near the camera), or
- Track a "streetlights/traffic lights knocked down" counter (no native stat exists).

Attempting to track map props purely via `World.GetClosestObjectOfType` in CLEO would miss simultaneously knocked poles (e.g., plowing through multiple median `doublestreetlght1`s or intersection `trafficlight1`s at once) and cannot cleanly check whether the player caused the uprooting.

---

## 4. Likely Implementation (ASI + CLEO Hybrid)

Follow the project's established pattern (`plugins/*.cpp` compiled via [`scripts/build.ts`](../scripts/build.ts) into ModLoader `.asi` plugins, paired with CLEO scripts and pinned globals in [`utils/globals.txt`](../utils/globals.txt)):

### A. C++ ASI Plugin (`plugins/PoleKnockdown/PoleKnockdown.cpp`)
1. **Detection Mechanism:**
   - **Option 1 (Event Hook — cleanest):** Hook `CObject::ObjectDamage` / object uproot handling in `gta3.exe` v1.0, filter by target model index (`1393`, `1404`, `1345`, `1394`, `1405`), verify the pole transitioned from upright/static to uprooted/dynamic (`!bIsStatic`) and that the damage/collision source is the player or player-driven vehicle (`FindPlayerPed` / `FindPlayerVehicle`).
   - **Option 2 (Pool Scan fallback):** Each frame (hooked inside a main update/process tick), walk `CPools::ms_pObjectPool`, track active `CObject*` pointers of the target model IDs, and detect `upright -> uprooted` transitions within proximity of the player / player vehicle.
2. **Deduplication:**
   - Track already-counted `CObject*` addresses (cleared when the pointer is recycled/deleted or leaves the pool) so scraping or driving back over a fallen pole never double-counts.
3. **Bridge to CLEO:**
   - Pin two/three slots in [`utils/globals.txt`](../utils/globals.txt) (e.g., `$PoleKnockEnabled`, `$PoleKnockTotal`, `$PoleKnockLastType`) and write directly to `CTheScripts::ScriptSpace` from the ASI whenever a qualifying pole falls.

### B. CLEO Script (`threads/poles.txt` or mission module)
1. Reads `$PoleKnockTotal` changes in a lightweight `wait 0` loop.
2. When active (either always-on or during the side-mission):
   - Awards cash via `Player.AddScore($player, reward)` (scaling by pole type or active streak timer).
   - Displays GXT feedback / HUD counter (`Hud.DisplayCounterWithString` / `Text.PrintWithNumberNow`).
   - Manages side-mission start/fail/pass conditions, timer (`Hud.DisplayTimer`), and cleanup (`$onmission`).

---

## 5. Open Design Questions

- **Ambient bonus vs. formal timed side-mission** (or both: e.g., unlockable ambient bonus after completing a timed challenge).
- **Eligible targets:** Only the 5 major road poles (`1393`, `1394`, `1404`, `1405`, `1345`), or also park lamps / illuminated bollards?
- **Payout & combo rules:** Flat \$ per pole vs. combo window multiplier, and whether on-foot explosives/heavy weapons count or only vehicle collisions.
