# Staunton Island (Act 2) — Design Blueprint (_Eventide III_)

Design plan for Act 2 following the Act 1 Portland finale ([`threads/bleed.txt`](../threads/bleed.txt)) and the transition to Staunton Island (`$BarrierIND == 0`), aligned with [`character_skins.md`](character_skins.md), [`kenji_mission_chain.md`](kenji_mission_chain.md), and _Book One_ lore.

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

Both existing ContactPoints unlock immediately when Staunton opens (`$BarrierIND == 0`), including existing post-`bleed` savegames, managed via a one-time `$StauntonContactsCreated` guard in [`threads/world.txt`](../threads/world.txt):

| Blip                                             | Landmark / Model                                                     | World Coords (`X, Y, Z`) | COL Floor `Z` | Sphere Radius | Globals (`utils/globals.txt`)                        |
| ------------------------------------------------ | -------------------------------------------------------------------- | ------------------------ | ------------- | ------------- | ---------------------------------------------------- |
| `RadarSprite.Asuka`                              | **`kb_scrap_6`** plaza (`avery` TXD)                                 | `287.92, -658.24, 26.17` | `25.13`       | `1.5`         | `$AsukaBlip` (`5138`), `$AsukaSphere` (`5139`)       |
| `RadarSprite.Ray`                                | **City Hall** steps (`townhallbig`, `newtownhall` TXD)               | `151.72, -757.56, 29.18` | `28.14`       | `1.5`         | `$RayBlip` (`5140`), `$RaySphere` (`5141`)           |
| _TBD (`Catalina` / `Love` / `Colombian` / etc.)_ | **Bedford Point Cathedral** alcove (`Clnm_cthdrlfcde`, `church` TXD) | `11.54, -1105.86, 26.36` | `~25.32`      | `1.5`         | _Next free slots after `5144` (`utils/globals.txt`)_ |

- **Mission Hook Safety:** Because spawning is gated by `$StauntonContactsCreated == 0`, upcoming mission threads can safely remove and restore their blips/spheres during missions without [`threads/world.txt`](../threads/world.txt) recreating duplicates mid-mission.

---

## 3. Act 2 World Infrastructure (In Place)

- **Staunton Safehouse:** Aspatria / Newport Condo (`513.55, -627.24, 16.66`) in [`threads/savest.txt`](../threads/savest.txt) with courtyard Health + Armor pickups ([`utils/assets.txt`](../utils/assets.txt)) and east pool walkway weapon package rewards ([`threads/packages.txt`](../threads/packages.txt)).
- **Newport Pay 'n' Spray:** `376.25, -496.5` → `385.6, -490.8` (`$SprayGarageCOM`), activated whenever `$BarrierIND == 0` in [`threads/world.txt`](../threads/world.txt).
- **Stadium Parking Lot:** 14 varied CarGenerators (`#SENTINEL`, `#STINGER`, `#CHEETAH`, `#BANSHEE`, etc.) across exact `comlandnew4.dff` stalls ([`utils/lot_cars.txt`](../utils/lot_cars.txt)).
- **Shoreside Lockout (`$BarrierCOM == 1`):** Enforced by [`threads/barrier_com.txt`](../threads/barrier_com.txt) with disabled western Porter Tunnel and Shoreside Lift Bridge traffic paths (`SwitchRoadsOff`).

---

## 4. Proposed Act 2 Narrative & Mission Structure

- **Protagonist Shift (Act 2 Opener):** Transition perspective from **Toni** to **Zoey** (`Char.Undress` / `Char.Dress`, persisted via a pinned global across save reloads per [`character_skins.md`](character_skins.md)).
- **Track A — `RadarSprite.Asuka` (`287.92, -658.24, 26.17`, `kb_scrap_6` Plaza):**
  1. **M1 (Tower Contact / Reconnaissance):** Establish Zoey's footing in Staunton, handle early lockdown patrols, and pick up leads after Portland's finale.
  2. **M2 (Court Supply / Sabotage):** Intercept or recover contraband across Staunton's piers/construction yard while shaking hostile pursuers.
  3. **M3 (Architect Escalation):** Nighttime extraction/confrontation at the plaza tower converging toward the Council arc.
- **Track B — `RadarSprite.Ray` (`151.72, -757.56, 29.18`, City Hall Steps):**
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
