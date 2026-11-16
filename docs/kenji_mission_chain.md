# Kenji Mission Chain — Design Blueprint (_Eventide III_)

Full technical and narrative blueprint adapting beats from _Enbenite - Book Two_ into Kenji's 3-mission Portland arc (following Luigi's / Louie's missions), saved for final review before scripting.

---

## 1. Current Story & Script Hooks

- **Trigger Point:** [`$KenjiBlip`](../threads/alley.txt#L295) at `(1004.42, -49.06, 7.44)` (northern Portland / Harwood), currently unlocked in [`threads/alley.txt`](../threads/alley.txt#L295) alongside Mission 4 [`:Affairs`](../main.txt#L371).
- **Preceding Beat:** Louie's warning at the end of [`threads/protection.txt`](../threads/protection.txt#L506): _"Now, make yourself scarce. The cops will be looking for you, so watch yourself. Capiche?"_
- **World State:** Staunton Island is sealed by `#INDHELIX_BARRIER` in [`threads/barriers.txt`](../threads/barriers.txt).
- **Audio Assets Note:** Use samples from the **Cloudcookooland** libraries for eerie audio cues (`play_audio_stream_1channel`, alongside existing `audio/EC/miasma.wav`).

---

## 2. Shared Wiring & Progression (`main.txt` + `utils/globals.txt`)

- **Trigger / Unlock Timing:** Either keep Missions 4 & 5 parallel after [`threads/alley.txt`](../threads/alley.txt#L295), or move `$KenjiBlip` creation to [`threads/protection.txt`](../threads/protection.txt#L521) (`:Protection_Success`) so Mission 5 unlocks strictly after Louie's protection job.
- **New Thread in [`main.txt`](../main.txt):** Add a `:Kenji` loop launching `statue.cm` -> `sap.cm` -> `bleed.cm`.
- **Build Pipeline ([`scripts/build.ts`](../scripts/build.ts)):** Add Steps 11–13 (`statue`, `sap`, `bleed`) to compile `threads/*.txt` -> `CLEO/*.cm` and package into `dist/Drag-N-Drop.zip`.
- **New Pinned Globals (`5104+` in [`utils/globals.txt`](../utils/globals.txt)):**
  - `5104`: `$OnKenjiMission`
  - `5105`: `$KenjiStage` (`0` -> `1` -> `2` -> `3`)
  - `5106`: `$MarieHealthBar`
    _(Ensure `main.txt` references the highest pinned slot or sizes globals cleanly so Sanny's allocator covers all CLEO globals.)_
- **Save Slot Mission Titles (`Stat.RegisterMissionPassed` / `0318` + [`data/CLEO/strings.fxt`](../data/CLEO/strings.fxt)):**
  - `MIS5NAM` -> Mission 5 title
  - `MIS6NAM` -> Mission 6 title
  - `MIS7NAM` -> Mission 7 title

---

## 3. Primary 3-Mission Arc (Selected: `statue` -> `sap` -> `bleed`)

### Mission 5 (`statue.cm`): _"The Living Statue..."_ _(Ch. 1 & Ch. 4 — Marie's Catatonia / Supply Convoy)_

- **Book Parallel:** Marie freezes completely catatonic on her balcony / blocking supply carriages from the Other Side, forcing Zoey to haul her indoors as her episodes grow darker and more volatile.
- **Actors & Models:**
  - `#SPECIAL01` `'KENJI'`
  - `#SPECIAL02` `'MISTY'` _(temporary base-game ped model placeholder for **Marie** until custom character models/bones are added)_
  - Attackers: Triads / `#REDNECK` / `#GANG03` or street thugs
  - Abductor van: `#PONY` or `#RUMPO`
- **Gameplay Loop:**
  1. **Harwood Briefing (`1004.42, -49.06, 7.44`):**
     - Kenji (acting as Pentor or a Court lookout) flags Claude down in Harwood—**Marie** has gone completely catatonic mid-walk, frozen stiff in the open near a supply drop / road (e.g., **Callahan Docks** or the **Trenton supply yard**), and hostile crews have spotted her.
  2. **Race & Hold (`3:00` or urgency timer):**
     - Short urgency prompt/timer (_"Marie's frozen up—get to her before someone grabs her!"_) to reach her location.
     - Marie stands dead-still (`Char.SetObjWaitOnFoot`, frozen-idle anim, `Char.SetProofs` against knockback until the fight starts).
     - Show `$MarieHealthBar` (`CounterDisplay.Bar`), mirroring `$DeadbeatHealthBar` in [`threads/protection.txt`](../threads/protection.txt#L435).
     - Defeat 1–2 waves (foot attackers + arriving `#PONY` / `#RUMPO` abduction crew).
  3. **Dead-Weight Extraction:**
     - Because she is locked stiff like a statue, she cannot walk to your car like in [`threads/pistol.txt`](../threads/pistol.txt#L352).
     - Pull a 4-door vehicle or van within ~3–4m of her -> brief fade (_"You haul her dead-weight into the passenger seat..."_) -> `Char.WarpIntoCarAsPassenger`.
  4. **Drive Home & Twist:**
     - Drive her back to the **Barn hideout** ([`threads/savep.txt`](../threads/savep.txt)) or her Hepburn Heights apartment while shaking a 1-car tail / wanted level.
     - **Ending Cutscene (Ch. 1 & Ch. 4 nod):** Once inside/at the drop-off, she suddenly snaps awake with zero memory of the gap (_"What happened? Why are we here?"_). Trigger an eerie weather flicker (`Weather.ForceNow`) and a faint **Cloudcookooland** audio cue (`play_audio_stream_1channel`) hinting something inside her is changing.

---

### Mission 6 (`sap.cm`): _"Digging for Sap..."_ _(Ch. 10 — Hortic & The Midnight Deadline)_

- **Book Parallel:** After finding the cryptic _"The blood begins where the Sap ends"_ inscription, Hortic bursts into the library and orders Zoey and Marie out into the Tipua Forest to dig up the Sap before the moon reaches its apex so they can reach Toni.
- **Actors & Models:** `#SPECIAL01` `'KENJI'` (Hortic beat), `#COLT45` / `#SHOTGUN` / `#UZI` guards, hidden pickup props/markers.
- **Gameplay Loop:**
  1. **Night-Window Trigger:**
     - Triggers only between `22:00–04:00` (matching `:Affairs`'s clock check in [`main.txt`](../main.txt#L377-L393)), with a tight pre-dawn HUD countdown (`$TIMER`).
  2. **3 Outskirts Sites:**
     - Kenji sends Claude across Portland's wooded/coastal outskirts (**Portland Beach**, **Callahan Point rocks**, **Harwood cliffs/hills**) to recover 3 guarded "Sap" / ritual pickups (`Pickup.Create`).
  3. **Payoff:**
     - Collecting the 3rd piece triggers an abrupt storm/fog shift (`Weather.ForceNow`), an eerie Cloudcookooland audio cue, and a hint pointing toward Toni / the sealed bridge.

---

### Mission 7 (`bleed.cm`): _"The Bleeding Edge..."_ _(Ch. 9 — Toni's Nightmarish Solo Interlude)_

- **Book Parallel:** Toni wakes up alone at a rotting Lewey Lake campsite, tries driving Zoey's dying 2011 Chrysler down Route 30 as _Bela Lugosi's Dead_ warps on the speakers, hits broken road past the shattered tollbooth and _King of the Frosties_ (_Seven Months_ pulsing over the PA), and sprints barefoot to Oak Mountain Drive.
- **Actors & Models:** Pre-damaged `#STALLION` / `#SENTINEL` / `#KURUMA`, `#HELIX_BARRIER` roadblocks, fog ambush peds (bats / pistols / `#CHNSAW`).
- **Gameplay Loop:**
  1. **Atmospheric Bleed-Through Opener:**
     - Triggers in deep night fog (after sleep/save or from `$KenjiBlip` as a "vision / bleed-through") inside a pre-damaged sedan (`Car.SetHealth` low, sluggish/stalling feel) while a warped/slowed **Cloudcookooland** horror track streams via `play_audio_stream_1channel`.
  2. **Shattered Roadblock Gauntlet:**
     - Follow checkpoints past wrecked cars and `#HELIX_BARRIER` / `#INDHELIX_BARRIER` props (mirroring the buckled Route 30 tollbooth), forcing Claude out on foot past an ambushed **Punk Noodles / Chinatown Diner** where hostile peds emerge out of the fog.
  3. **Climax & Bridge Unlock:**
     - Reach the safe house marker to break the bleed-through—optionally setting `$BarrierIND = 0` and clearing the Callahan Bridge `#INDHELIX_BARRIER` in [`threads/barriers.txt`](../threads/barriers.txt) so **Staunton Island** opens as Act 2 begins.

---

## 4. Open Sign-Off Questions (For Review)

1. **Unlock Order:** Should Mission 5 unlock right after Mission 3 (current `$KenjiBlip` in [`threads/alley.txt`](../threads/alley.txt#L295)), or only after Mission 4 ([`threads/protection.txt`](../threads/protection.txt))?
2. **Kenji's Persona:** Should Kenji represent one consistent book figure (e.g., **Hortic**, **Pentor**, or **Meran**), or act as an Eventide-native Harwood fixer?
3. **Mission 5 Drop-off:** Extract frozen Marie to the **Barn hideout** (`savep.txt`) or **Hepburn Heights**?
4. **Staunton Unlock:** Should **Mission 7 (_The Bleeding Edge..._)** set `$BarrierIND = 0` and open Staunton Island at the end?
5. **Titles & Coordinates:** Confirm `"The Living Statue..."`, `"Digging for Sap..."`, `"The Bleeding Edge..."` and finalize exact spawn coordinates for the frozen Marie site, 3 Sap locations, and Bleeding Edge route.

---

## 5. Unused / Alternate Concepts (Backup Pool)

### Alternate A: _"Ashes in the Square"_ _(Ch. 1 & Ch. 7 — Arson & Cornering Reinhardt)_

- **Book Parallel:** No one in Korvus helps during the town-square arson until Zoey and Marie haul water themselves; the next day, Zoey corners Reinhardt in a secluded hall, drives off his lackeys, and beats him half to death.
- **Gameplay Loop:**
  - Rival arsonists/Redcaps torch shops in Chinatown or Little Italy. Steal `#FIRETRUK` from Harwood Fire Station (right near `$KenjiBlip`) to extinguish burning targets or chase down the fleeing Molotov crew.
  - Corner their leader ("Reinhardt") in a tight alley/warehouse, scatter his crew, and use the non-lethal [`$DeadbeatHealthBar`](../threads/protection.txt#L434-L451) mechanic to beat him below 25% health without killing him before an intimidation cutscene.

### Alternate B: _"Border Smugglers"_ _(Ch. 1 Smugglers + Louie's Cop Heat Warning)_

- **Book Parallel:** Zoey tries to get shady town-square smugglers to carry letters across the sealed border to Earth while Meran tightens security.
- **Gameplay Loop:**
  - Ties Louie's cop-heat warning directly into the Staunton barrier in [`threads/barriers.txt`](../threads/barriers.txt).
  - `$KenjiBlip` acts as the smuggler contact near the Harwood/tunnel edge, agreeing to move a package "to the Other Side" only if you clear a dock/tunnel drop and shake a forced 2-star wanted level (`Player.AlterWantedLevelNoDrop`) without leading cops to the drop—ending with the smuggler backing out when the Eventide wards flare.

---

## 6. Side-Notes & Planned Assets

- **Car Dealer Side-Mission:** `$CarDealerBlip` in [`threads/INIT.txt`](../threads/INIT.txt#L139) intentionally uses `Blip.AddSpriteForContactPoint` for a planned _Liberty City Stories_-style Car Dealer side-mission.
- **Character Rename (_Misty -> Marie_) & Future NPC Remodels:**
  - Misty has been renamed to **Marie** across her introduction mission ([`threads/pistol.txt`](../threads/pistol.txt) and [`data/CLEO/strings.fxt`](../data/CLEO/strings.fxt)).
  - `Streaming.LoadSpecialCharacter(..., 'MISTY')` remains as the temporary GTA III `.dff`/`.txd` special-ped placeholder until custom story character models and skeletons/bones are sculpted/commissioned to replace all base-game story NPCs.
