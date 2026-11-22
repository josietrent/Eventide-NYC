# Intro Cutscene & Voiced Narration Notes (_Eventide III_)

## 1. 3-Shot Cold Open ([`main.txt`](../main.txt))
Runs once on **New Game** before background mission threads start (`@Driving`, `@CarHealth`, `@Packing`, `@Barrier`), skippable at any point via `Space`, `Enter`, `Left Shift`, or `F`:

1. **Shot 1 — Callahan Bridge & Barrier:** `JumpCut` east of the Portland approach (`1072.0, -952.0, 24.5`) framing `#INDHELIX_BARRIER` (`1027.25, -933.75, 15.0`), then `Camera.SetInterpolationParameters(0.0, 6000)` gliding west across the sealed span (`835.0, -952.0, 48.0`).
2. **Shot 2 — Portland Aerial Pan:** High-altitude `Camera.SetInterpolationParameters(0.0, 6000)` drift from Trenton/Chinatown (`860.0, -780.0, 74.0`) northward toward Harwood (`990.0, -320.0, 68.0`), staying `>= 55 m` clear and well above all 26 Portland Hidden Packages so none are spoiled.
3. **Shot 3 — Portland Beach Arrival:** `JumpCut` above the surf/headland (`972.0, -206.0, 22.0`) framing the Canyonero and shoreline, gliding down and up right in front of the player (`937.48, -251.44, 5.55` -> `933.66, -252.64, 5.15`), followed by an Eventide purple fade (`40, 0, 70`), `Camera.RestoreJumpcut()`, and fade back in with player health set low (`30`) and `BEACH` displayed.

---

## 2. Voiced Narration TODO — _Nowhere Like Home..._ Pier Trigger (Option A)
- **Trigger Location:** Callahan Point pier sphere `(774.45, -880.88, 14.74)` in `:Driving_Loop` ([`main.txt`](../main.txt)).
- **Direction:** Keep the narration **natural and author-written/voiced** (not AI-scripted). When ready:
  1. Place the recorded audio file under `audio/EC/` (e.g., `audio/EC/intro_narration.wav` or `.mp3`).
  2. Wire `play_audio_stream_1channel "audio/EC/<filename>", False, 1.0` and matching timed `Text.Print` subtitle keys in [`:Driving_Loop`](../main.txt) at the `TODO` hook (updating the wait duration to match the clip length while preserving the skip check).
  3. Add those subtitle keys to [`data/CLEO/strings.fxt`](../data/CLEO/strings.fxt).
  4. If distributing custom audio via `Drag-N-Drop.zip`, add an `audio/EC/` packaging step to [`scripts/build.ts`](../scripts/build.ts).
