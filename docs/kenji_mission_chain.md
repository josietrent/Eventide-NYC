# Kenji Mission Chain — Design Notes (*Eventide III*)

Saved brainstorming notes adapting beats from *Enbenite - Book Two* into the next mission arc after Luigi's missions.

---

## Current Story & Script Hooks

- **Trigger:** `$KenjiBlip` at `(1004.42, -49.06, 7.44)` (northern Portland / Harwood), unlocked in [`threads/alley.txt`](../threads/alley.txt#L295).
- **Preceding Beat:** Louie's warning at the end of [`threads/protection.txt`](../threads/protection.txt#L506): *"Now, make yourself scarce. The cops will be looking for you, so watch yourself. Capiche?"*
- **World State:** Staunton Island is sealed by `#INDHELIX_BARRIER` in [`threads/barriers.txt`](../threads/barriers.txt).
- **Audio Assets Note:** Use samples from the **Cloudcookooland** libraries for eerie audio cues (`play_audio_stream_1channel`).
- **Open Design Questions:**
  - Decide who Kenji represents in the adaptation (e.g., **Pentor**, **Hortic**, or a new Harwood contact).
  - Exact spawn coordinates for the frozen Misty encounter, the 3 Sap pickup sites, and the Nightmare/Bleeding Edge route.

---

## Primary 3-Mission Arc (Selected: #2 → #4 → #5)

### Mission 5: "The Living Statue" *(Ch. 1 & Ch. 4 — Marie's Catatonia / Supply Convoy)*

- **Book Parallel:** Marie freezes completely catatonic on her balcony / blocking supply carriages from the Other Side, forcing Zoey to haul her indoors as her episodes grow darker and more volatile.
- **Story Hook:** Kenji (acting as Pentor or a Court lookout) flags Claude down in Harwood—**Misty** has gone completely catatonic mid-walk, frozen stiff in the open near a supply drop / road (e.g., Callahan Docks or the Trenton supply yard), and rival thugs / Redcaps / Triads have spotted her.
- **Gameplay Loop:**
  1. **Race to the Spot:**
     - Short urgency prompt/timer (*"Misty's frozen up—get to her before someone grabs her!"*) to reach her location.
     - Misty stands dead-still (`Char.SetObjWaitOnFoot`, with a frozen-idle anim and `Char.SetProofs` against knockback until cleared) while enemies close in.
  2. **Hold the Line (Protect Misty):**
     - Show a `MISTY` HUD health bar (`CounterDisplay.Bar`), mirroring `$DeadbeatHealthBar` in [`threads/protection.txt`](../threads/protection.txt#L435).
     - Fend off 1–2 waves of attackers trying to gun her down or abduct her in a van (`#PONY` / `#RUMPO`).
  3. **Haul Her Back ("Recruit Ogres" / Transport):**
     - Because she is locked stiff like a statue, she cannot walk to your car like in [`threads/pistol.txt`](../threads/pistol.txt#L352).
     - Bring a 4-door vehicle or van within ~3–4m of her; brief fade (*"You haul her dead-weight into the passenger seat..."*) and warp her inside (`Char.WarpIntoCarAsPassenger`).
  4. **Extraction & Twist:**
     - Drive her back to the **Barn hideout** ([`threads/savep.txt`](../threads/savep.txt)) or her Hepburn Heights apartment while shaking a pursuing car or wanted heat.
     - **Ending Cutscene (Ch. 1 & Ch. 4 nod):** Once inside/at the drop-off, she suddenly snaps awake with zero memory of the gap (*"What happened? Why are we here?"*). Trigger an eerie weather flicker (`Weather.ForceNow`) and a faint Cloudcookooland audio cue hinting something inside her is changing.

---

### Mission 6: "Digging for Sap" *(Ch. 10 — Hortic & The Midnight Deadline)*

- **Book Parallel:** After finding the cryptic *"The blood begins where the Sap ends"* inscription, Hortic bursts into the library and orders Zoey and Marie out into the Tipua Forest to dig up the Sap before the moon reaches its apex so they can reach Toni.
- **Gameplay Loop:**
  1. **Night-Window Trigger:** Only starts during a late-night time window (similar to the `3:00–7:00` check in `threads/protection.txt`), or requires completing the run before `00:00` / `03:00` with a HUD countdown timer.
  2. **Kenji as Hortic:** Sends Claude across Portland's wooded/coastal outskirts (**Portland Beach**, **Callahan Point rocks**, **Harwood hills**) to recover 3 hidden "Sap" / ritual pickups guarded by armed hostiles/dogs/thugs before dawn.
  3. **Payoff:** Collecting the final piece triggers a sudden weather shift (`Weather.ForceNow`) and an eerie Cloudcookooland audio cue / voice line hinting at Toni's rescue arc—setting up the Staunton bridge unlock.

---

### Mission 7: "The Bleeding Edge" *(Ch. 9 — Toni's Nightmarish Solo Interlude)*

- **Book Parallel:** Toni wakes up alone at a rotting Lewey Lake campsite, tries driving Zoey's dying 2011 Chrysler down Route 30 as *Bela Lugosi's Dead* warps on the speakers, hits broken road past the shattered tollbooth and *King of the Frosties* (*Seven Months* pulsing over the PA), and sprints barefoot to Oak Mountain Drive.
- **Gameplay Loop:**
  1. **Atmospheric Solo Horror Sequence:** Triggered after a sleep/save or from `$KenjiBlip` as a "vision / bleed-through."
  2. **Dying Car & Warped Audio:** Put the player in a pre-damaged sedan (`Car.SetHealth` low, sluggish/stalling feel) in thick night fog, playing a custom slowed/distorted track via `play_audio_stream_1channel`.
  3. **Roadblock Gauntlet:** Block the main road with `#HELIX_BARRIER` / `#INDHELIX_BARRIER` props and wrecked cars (mirroring the buckled Route 30 tollbooth), forcing Claude out on foot past a "corrupted diner" ambush (e.g., Punk Noodles or the Chinatown / Saint Mark's diner) where hostile peds emerge out of the fog before reaching the safe house marker.
  4. **Bridge-Unlock Potential:** Can serve as the climax that shatters the Portland `#INDHELIX_BARRIER` and opens Staunton Island.

---

## Unused / Alternate Concepts (Backup Pool)

### Alternate A: "Ashes in the Square" *(Ch. 1 & Ch. 7 — Arson & Cornering Reinhardt)*
- **Book Parallel:** No one in Korvus helps during the town-square arson until Zoey and Marie haul water themselves; the next day, Zoey corners Reinhardt in a secluded hall, drives off his lackeys, and beats him half to death.
- **Gameplay Loop:**
  - Rival arsonists/Redcaps torch shops in Chinatown or Little Italy. Steal `#FIRETRUK` from Harwood Fire Station (right near `$KenjiBlip`) to extinguish burning targets or chase down the fleeing Molotov crew.
  - Corner their leader ("Reinhardt") in a tight alley/warehouse, scatter his crew, and use the non-lethal [`$DeadbeatHealthBar`](../threads/protection.txt#L434-L451) mechanic to beat him below 25% health without killing him before an intimidation cutscene.

### Alternate B: "Border Smugglers" *(Ch. 1 Smugglers + Louie's Cop Heat Warning)*
- **Book Parallel:** Zoey tries to get shady town-square smugglers to carry letters across the sealed border to Earth while Meran tightens security.
- **Gameplay Loop:**
  - Ties Louie's cop-heat warning directly into the Staunton barrier in [`threads/barriers.txt`](../threads/barriers.txt).
  - `$KenjiBlip` acts as the smuggler contact near the Harwood/tunnel edge, agreeing to move a package "to the Other Side" only if you clear a dock/tunnel drop and shake a forced 2-star wanted level (`Player.AlterWantedLevelNoDrop`) without leading cops to the drop—ending with the smuggler backing out when the Eventide wards flare.

---

## Side-Note: Related Planned Feature
- `$CarDealerBlip` in [`threads/INIT.txt`](../threads/INIT.txt#L139) uses `Blip.AddSpriteForContactPoint` intentionally for a future **Car Dealer side-mission** (inspired by *Liberty City Stories*).
