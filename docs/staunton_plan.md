# Staunton Island (Act 2) — Design Blueprint (_Eventide III_)

Design plan for Act 2 following the Act 1 Portland finale ([`threads/bleed.txt`](../threads/bleed.txt)) and the transition to Staunton Island (`$BarrierIND == 0`), aligned with [`character_skins.md`](character_skins.md) and [`kenji_mission_chain.md`](kenji_mission_chain.md).

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

Both ContactPoints unlock immediately when Staunton opens (`$BarrierIND == 0`), including existing post-`bleed` savegames, managed via a one-time `$StauntonContactsCreated` guard in [`threads/world.txt`](../threads/world.txt):

| Blip | Landmark / Model | World Coords (`X, Y, Z`) | COL Floor `Z` | Sphere Radius | Globals (`utils/globals.txt`) |
|---|---|---|---|---|---|
| `RadarSprite.Asuka` | **`kb_scrap_6`** plaza (`avery` TXD) | `287.92, -658.24, 26.17` | `25.13` | `1.5` | `$AsukaBlip` (`5138`), `$AsukaSphere` (`5139`) |
| `RadarSprite.Ray` | **City Hall** steps (`townhallbig`, `newtownhall` TXD) | `151.72, -757.56, 29.18` | `28.14` | `1.5` | `$RayBlip` (`5140`), `$RaySphere` (`5141`) |

- **Mission Hook Safety:** Because spawning is gated by `$StauntonContactsCreated == 0`, upcoming mission threads for Asuka and Ray can safely remove and restore their blips/spheres during missions without [`threads/world.txt`](../threads/world.txt) recreating duplicates mid-mission.

---

## 3. Act 2 World Infrastructure (In Place)

- **Staunton Safehouse:** Aspatria / Newport Condo (`513.55, -627.24, 16.66`) in [`threads/savest.txt`](../threads/savest.txt) with courtyard Health + Armor pickups ([`utils/assets.txt`](../utils/assets.txt)) and east pool walkway weapon package rewards ([`threads/packages.txt`](../threads/packages.txt)).
- **Newport Pay 'n' Spray:** `376.25, -496.5` → `385.6, -490.8` (`$SprayGarageCOM`), activated whenever `$BarrierIND == 0` in [`threads/world.txt`](../threads/world.txt).
- **Stadium Parking Lot:** 14 varied CarGenerators (`#SENTINEL`, `#STINGER`, `#CHEETAH`, `#BANSHEE`, etc.) across exact `comlandnew4.dff` stalls ([`utils/lot_cars.txt`](../utils/lot_cars.txt)).
- **Shoreside Lockout (`$BarrierCOM == 1`):** Enforced by [`threads/barrier_com.txt`](../threads/barrier_com.txt) with disabled western Porter Tunnel traffic paths (`SwitchRoadsOff`).

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
