# Staunton Island (Act 2) — Design Blueprint (_Eventide III_)

Design plan for Act 2 following the Act 1 Portland finale ([`threads/bleed.txt`](../threads/bleed.txt)) and the transition to Staunton Island (`$BarrierIND == 0`), aligned with [`character_skins.md`](character_skins.md), [`kenji_mission_chain.md`](kenji_mission_chain.md), and _Book One_ / _Book Two_ lore.

---

## 1. ContactPoint RadarSprite Audit

Currently active sprites in _Eventide III_:

- **Story Contacts:** `Joey` (`main.txt`), `Luigi` (`pistol.txt`, `alley.txt`, `main.txt`), `Kenji` (`main.txt`), `Tony` (`INIT.txt` car dealer), `Savehouse` (`INIT.txt`, `savest.txt`).
- **Utility / Mission-Local:** `Spray` (Pay 'n' Spray during wanted checks), `Love` (transient coordinate blip in `INIT.txt` on Punk Noodles).

Unused ContactPoint & Gang sprites available for Act 2 / Act 3 (`enums.txt` L639–665):

- **Contact Sprites:** `Asuka`, `Ray`, `Salvatore` (`main.old.txt` only), `Catalina`, `EightBall`, `Love` (no persistent ContactPoint).
- **Gang Sprites:** `Diablo`, `Yardie`, `Hood` (`Yakuza`, `Triad`, `Mafia`, `Colombian` also exist).

---

## 2. Staunton Act 2 Contact Points (Live on `$BarrierIND == 0`)

Both existing ContactPoints unlock immediately when Staunton opens (`$BarrierIND == 0`), including existing post-`bleed` savegames, managed via `$StauntonContactsCreated` in [`threads/world.txt`](../threads/world.txt) (`0` → `2` on fresh unlock, and `1` → `2` migration for older savegames that spawned Asuka at `287.92, -658.24, 26.17`):

| Blip                                             | Landmark / Model                                                     | World Coords (`X, Y, Z`)  | COL Floor `Z` | Sphere Radius | Globals (`utils/globals.txt`)                        |
| ------------------------------------------------ | -------------------------------------------------------------------- | ------------------------- | ------------- | ------------- | ---------------------------------------------------- |
| `RadarSprite.Asuka`                              | **AMCo. Headquarters** glass entrance (Torrington)                   | `245.59, -1207.57, 26.67` | `~25.63`      | `1.5`         | `$AsukaBlip` (`5138`), `$AsukaSphere` (`5139`)       |
| `RadarSprite.Ray`                                | **City Hall** steps (`townhallbig`, `newtownhall` TXD)               | `151.72, -757.56, 29.18`  | `28.14`       | `1.5`         | `$RayBlip` (`5140`), `$RaySphere` (`5141`)           |
| _TBD (`Catalina` / `Love` / `Colombian` / etc.)_ | **Bedford Point Cathedral** alcove (`Clnm_cthdrlfcde`, `church` TXD) | `11.54, -1105.86, 26.36`  | `~25.32`      | `1.5`         | _Next free slots after `5145` (`utils/globals.txt`)_ |

- **Mission Hook Safety:** Because spawning is gated by `$StauntonContactsCreated` (`0`/`1` → `2`), upcoming mission threads can safely remove and restore their blips/spheres during missions without [`threads/world.txt`](../threads/world.txt) recreating duplicates mid-mission.

---

## 3. Act 2 World Infrastructure (In Place)

- **Staunton Safehouse:** Aspatria / Newport Condo (`513.55, -627.24, 16.66`) in [`threads/savest.txt`](../threads/savest.txt) with courtyard Health + Armor pickups ([`utils/assets.txt`](../utils/assets.txt)) and east pool walkway weapon package rewards ([`threads/packages.txt`](../threads/packages.txt)).
- **Newport Pay 'n' Spray:** `376.25, -496.5` → `385.6, -490.8` (`$SprayGarageCOM`), activated whenever `$BarrierIND == 0` in [`threads/world.txt`](../threads/world.txt).
- **Torrington Pool Health Pickup (`$HEAL_COM6`):** `242.83, -1117.11, 21.11` in [`utils/assets.txt`](../utils/assets.txt) and [`threads/world.txt`](../threads/world.txt), right near AMCo. HQ.
- **Stadium Parking Lot:** 14 varied CarGenerators (`#SENTINEL`, `#STINGER`, `#CHEETAH`, `#BANSHEE`, etc.) across exact `comlandnew4.dff` stalls ([`utils/lot_cars.txt`](../utils/lot_cars.txt)).
- **Shoreside Lockout (`$BarrierCOM == 1`):** Enforced by [`threads/barrier_com.txt`](../threads/barrier_com.txt) with disabled western Porter Tunnel and Shoreside Lift Bridge traffic paths (`SwitchRoadsOff`).

---

## 4. Proposed Act 2 Narrative & Mission Structure

- **Protagonist Shift (Act 2 Opener):** Transition perspective from **Toni** to **Zoey** (`Char.Undress` / `Char.Dress`, persisted via a pinned global across save reloads per [`character_skins.md`](character_skins.md)).

### Track A — `RadarSprite.Asuka` (AmCo. CEO — AMCo. HQ, `245.59, -1207.57, 26.67`)

- **Premise & Lore Tie-In (_Book One_, _Book Two_, & Act 1):**
  - In Act 1 ([`threads/pistol.txt`](../threads/pistol.txt) / [`strings.fxt`](../data/CLEO/strings.fxt)), **Marie** works at one of Portland's **AmCo.** storefronts (_"Marie isn't here right now, so the store is still closed"_), with weapons/contraband stashed behind the shop.
  - **Asuka** is the ruthless, hyper-polished **CEO of AmCo.** operating out of the Torrington glass tower (`245.59, -1207.57, 26.67`) _(final Eventide / Book character replacement TBD)_.
  - Across _Book One_ and _Book Two_, _Enbenite_ exposure mutates the human genome while smugglers ferry letters, Sap reserves, and illicit goods across the sealed border between Earth and the Other Side (Korvus). Within _Eventide III_'s Liberty City allegory, **AmCo.**'s retail supply chain—from Portland corner stores like Marie's branch up to Staunton corporate logistics—serves as a corporate front moving _Enbenite_-tainted cargo, Sap stock, and quiet payouts for the Council / King Meran's inner circle.
  - Marie's escalating catatonic episodes (`statue.txt`, `sap.txt`) put her branch directly in the bleed-through crossfire; now that the Callahan Barrier is blown and Toni lies in a coma, Asuka recruits Zoey as off-book corporate troubleshooter to scrub leaks, seize hijacked freight, and lock down AmCo.'s pipeline before City Hall or the cultists blow the lid off what the company actually ships.
- **Proposed AmCo. CEO Mission Chain (4 Missions):**
  1. **A1 — Closed Storefront (Branch Audit & Van Intercept):** Following the Callahan Bridge breach, LCPD investigators and opportunistic crews are seizing AmCo.'s unmarked Portland-to-Staunton stock manifests—including shipment records from **Marie's Portland store**. Intercept the fleeing courier vans and rogue inspectors across Staunton before the manifest reaches City Hall.
  2. **A2 — Mineral Freight (_Enbenite_ / Sap Hijack):** Cult cells and rival smugglers ambush an armored AmCo. transport carrying refined Sap / _Enbenite_-reactive canisters near the docks/Aspatria yards. Fight through the ambush, recover the freight intact, and escort it back to AMCo. HQ's loading bay.
  3. **A3 — Executive Privilege (Whistleblower & Boardroom Leak):** A terrified AmCo. logistics officer tied to the cross-border smuggling runs plans to flip to Ray's City Hall contacts. Tail the whistleblower to their secret Torrington/Bedford Point meet, eliminate both the turncoat and their armed detail, and burn the ledger before backup arrives.
  4. **A4 — Hostile Merger (Tower Defense / Convergence):** Rival mercenaries and Redcap-backed remnants try to seize AMCo.'s Staunton distribution hub and blackmail the board. Defend the AMCo. plaza and dismantle their staging crew, earning Asuka's black-budget dossier and security clearance that feed into the **Medical Wing** push to rescue Toni and break into **Shoreside Vale**.

### Track B — `RadarSprite.Ray` (`151.72, -757.56, 29.18`, City Hall Steps)

1. **M1 (Council Audience / Audit):** Political/undercover errand tied to City Hall officials (infiltration / tailing).
2. **M2 (Medical Wing / Suppression):** High-heat rescue lead pointing toward Toni's coma state and King Meran (`frankie`).
3. **M3 (Staunton Finale → Unlock Shoreside):** Coordinated strike lifting the Council's Shoreside lockdown (`$BarrierCOM = 0`), restoring the Lift Bridge and western Porter Tunnel branch (`barrier_com.txt`).

---

## 5. Track C — Cult & Apocalypse Chain (Bedford Point Cathedral)

Per _Book One_ lore, the Staunton unrest serves as a staging ground for the coming **Apocalypse**, driven on the streets by fanatical **human-sympathizing cult cells** exploiting the _Enbenite_ bleed-through while Toni remains trapped in her coma.

- **Contact Location:** Recessed doorway alcove of **Bedford Point Cathedral** (`1606 Clnm_cthdrlfcde`, `church` TXD) at `11.54, -1105.86, 26.36` (`COL` floor `~25.32`).
- **Contact Giver ("Father Grigori" Archetype):**
  - An unhinged, shotgun-toting parish keeper / former cleric squatting inside the Cathedral precincts who watches the cult's nocturnal rituals overrun Staunton.
  - Speaks in apocalyptic sermons, treats Zoey as an unlikely instrument to cleanse the congregation's hijackers, and can fight alongside her during siege set pieces (like Grigori in Ravenholm).
  - _Open Choice:_ Exact character name, `#SPECIAL` slot / priest-style ped `.dff`+`.txd` reskin, and which unused `RadarSprite` (`Catalina`, `Love`, `Colombian`, etc.) marks the Cathedral steps.
- **Proposed Cult Mission Arc (3–4 Missions):**
  1. **C1 — Cleansing the Steps (Parish Purge):** Fanatic human-sympathizer acolytes descend on the Cathedral plaza and nearby streets; clear the courtyard, hunt down fleeing zealot messengers before they rally backups, and establish the Cathedral as a holdout.
  2. **C2 — False Relics (Convoy Interception):** Cult enforcers are moving corrupted _Enbenite_/Sap caches across Staunton (Torrington / Liberty Campus / docks) to anchor their ritual sites. Intercept and destroy the moving cell vehicles and their armed escorts before they reach their drop points.
  3. **C3 — Heretic Congregation (Underground / Shrines):** Strike multiple synchronized cult ritual cells scattered around Staunton under forced night-fog (`miasma` weather/audio), fighting raging melee & shotgun/Molotov zealots before a ritual timer expires.
  4. **C4 — Choir of Ash (Apocalypse Omen Finale):** A full-scale cult assault converges on Bedford Point Cathedral; defend the doors shoulder-to-shoulder with the Father Grigori-like keeper (or push out to break their ringmaster), uncovering concrete proof that these fanatics are merely heralds preparing the **Apocalypse** ahead of Act 3 (Shoreside Vale).
- **Gameplay & Asset Notes:**
  - Fanatic enemy squads can reuse aggressive street-gang ped slots (`#GANG04` / `#GANG07` / `#CRIMINAL01` or dedicated cult reskins) armed with bats, shotguns, Uzis, and Molotovs/flamethrowers (`WeaponType.Molotov` / `#MOLOTOV`) to feel distinct from Portland's Mafia/Triad crews.
  - Pair mission climaxes with `Weather.ForceNow(WeatherType.Foggy)` / night clock + `miasma.wav` stingers and restore `Weather.Release()` / `WeatherType.Foggy` cleanly on pass and fail.
