# Character Skins & Model Blueprint (_Eventide III_)

Canonical appearance descriptions, folklore references, and GTA III `.dff` / `.txd` slot mappings for _Eventide III_.

---

## 1. Playable Protagonists & Act 1 → Act 2 Perspective Shift

In Act 1 (Portland), the player controls **Toni**. Blowing the Callahan Barrier at the end of Mission 9 ([`threads/bleed.txt`](../threads/bleed.txt)) represents freeing Toni from her coma, transitioning the player perspective to **Zoey** for Act 2 (Staunton Island) onward.

- **In-Engine Swap Mechanism (`0352` / `0353`):** GTA III supports swapping the player's `.dff` + `.txd` model at runtime via `Char.Undress($scplayer, '<MODEL>')` -> `Streaming.LoadAllModelsNow()` -> `Char.Dress($scplayer)` (already used in [`main.txt`](../main.txt#L89-L90)), backed by a pinned global flag so save reloads restore Zoey's model automatically.

### Toni _(Active in Portland — Act 1 Player Slot: `player`)_
- **Description:** Wore a lime green striped white t-shirt and a pair of black cargo capris. On her feet were a pair of 4-inch high forest green heels—complementing the rest of her ensemble. Per her usual, her face was made-up with black eyeliner, smoky eye-shadow, and black lipstick.
- **GTA III Mapping (`player.dff` / `player.txd`):**
  - **Mesh:** Swap vanilla Claude `player.dff` for a female GTA III ped rig (e.g., `cat.dff` / `female01.dff` / `b_wom2.dff`) or custom female player `.dff` with heeled silhouette.
  - **TXD Reskin:** White tee with lime green stripes, black cargo capris, forest green heels, pale skin, smoky eyeshadow, black eyeliner, black lipstick.

### Zoey _(Act 2+ Player Slot: custom special model e.g. `zoey.dff` / `zoey.txd`)_
- **Book 1 / Wardrobe Description:** Sky blue corduroy pants, black camisole underneath a forest green short-sleeved satin blouse lined with black lace at the hem. At her throat sits a jade-encrusted bird pendant on a small chain attached to a lace choker, paired with matching earrings.
- **Book 2 Shift:** Switches to utilitarian clothing just before descending into the Medical Wing to pull Toni out of her coma.
- **GTA III Mapping (`zoey.dff` / `zoey.txd` via `Char.Undress`):**
  - **Mesh:** Female GTA III ped rig (e.g., `asuka.dff` / `maria.dff` / `ct_wom2.dff`), with optional Book 2 utilitarian variant.
  - **TXD Reskin:** Forest green satin top with black lace hem trim & dark camisole collar, sky blue corduroy pants, jade bird lace choker & earrings (or Book 2 utilitarian attire).

---

## 2. Active Portland Cast (Priority Reskins)

Currently on the playing field in Portland: **Toni** (above), **Marie**, **Pentor**, and **Reinhardt**.

### Marie _(Active in Portland — `#SPECIAL02` / Mission 2 & 6: `misty`)_
- **Description:** Walked out of her wardrobe wearing a light pink floor-length dress. The short sleeves were slightly puffy, with what looked like a crinoline beneath her skirt. The hem had black lace trim and brushed the floor.
- **GTA III Mapping (`misty.dff` / `misty.txd`, or `maria.dff` base):**
  - **TXD Reskin:** Light pink dress fabric with puffy-sleeve shading and black lace hem trim.
  - **`.dff` Note:** Vanilla `misty.dff` wears a mini-skirt; swapping base `.dff` to `maria.dff` / `female03.dff` gives a longer skirt immediately, and a custom `.dff` pass is needed for a true floor-length crinoline bell silhouette.

### Pentor, an Elf _(Active in Portland — `#SPECIAL01`: `kenji`)_
- **Role & Description:** Head cook (beginning as the castle's demure and shy head cook in Book 1, coming into his own in Book 2 while helping free Toni from her coma). A tall elf wearing a crisp white chef's coat, apron, and chef's toque, standing regally with arms crossed behind him.
- **GTA III Mapping (`kenji.dff` / `kenji.txd`):**
  - **TXD Reskin:** Crisp white double-breasted chef's coat, white apron ties/straps, pale elven complexion & brows.
  - **`.dff` Note:** Needs a `.dff` pass for a tall **chef's toque**, **apron skirt/panel**, and **pointy elf ears** on a tall male frame.

### Reinhardt, a Redcap _(Active in Portland — `#GANG05`: `gang05`, Mission 8 [`threads/ashes.txt`](../threads/ashes.txt))_
- **Folklore Appearance (Anglo-Scottish Border Tradition):**
  - **Stature & Build:** Short, stocky, and thickset weathered old goblin/elf with a heavily built frame and immense supernatural strength.
  - **Face & Hair:** Large, glowing fiery-red eyes, prominent razor-sharp teeth, and long grisly grey hair streaming over his shoulders; thin, sinewy hands tipped with sharp talon-like nails.
  - **The Cap:** Signature **crimson / blood-soaked cap or cowl** on his head (traditionally stained in victims' blood along the Borders).
  - **Clothing & Footwear:** Faded, tattered dark coat/tunic over rugged Border garments and heavy **iron-shod / iron-tipped boots** (which belie his impossible speed). Classic accounts also place a pikestaff in his left hand.
- **GTA III Mapping (`gang05.dff` / `gang05.txd` — `chico` texture):**
  - **TXD Reskin:** Blood-stained crimson red cap/bandana, fiery glowing red eyes, scarred/grizzled face with sharp grin & grey hair/whiskers, weathered tattered dark jacket (`D` gang logo removed/replaced with worn leather/cloth), and heavy dark iron-shod boots.
  - **`.dff` Note:** Vanilla `#GANG05` already wears a red cap/bandana and dark jacket, making it a strong immediate TXD match; an optional `.dff` pass can make him shorter/stockier with grisly shoulder-length hair and iron boot caps.

---

## 3. Act 2+ / Upcoming Cast

### King Meran, an Elf _(Replaces Salvatore: `frankie`)_
- **Description:** Wore an opulent black leather coat with lapels encrusted with various gems, sleeves rolled up to expose his forearms. Beneath the coat, a clean white linen shirt clung to his upper body with the top ties loosened at his collarbone, exposing his hairless chest. Tight tan pants stuffed into sleek black knee-high boots. Left eye has a striking cerulean blue iris; right eye is split vertically by a scar—half pine-green, half amethyst—under a hooded brow bone. Pointy angular elf ears; hair as silver as a full moon, lustrous and falling past his shoulders. Forearms bear short faint scars alongside longer, pronounced combat scars.
- **GTA III Mapping (`frankie.dff` / `frankie.txd`):**
  - **TXD Reskin:** Opulent black leather coat with jeweled/gem-encrusted lapels, rolled sleeves showing scarred forearms, crisp white open-collar linen chest/shirt, tight tan trousers, sleek black knee-high boots, moon-silver hair, cerulean left eye, scarred split green/amethyst right eye.
  - **`.dff` Note:** Vanilla `frankie.dff` already has Salvatore's open black coat + exposed shirt chest + suit-pants silhouette; `.dff` pass needed later for **rolled sleeves**, **pointy elf ears**, and **long silver hair past the shoulders**.

### Hortic _(Upcoming `#SPECIAL` / Ped Slot)_
- **Description:** A 4-foot-tall miniature devil, horns and all—resembling a classic 1940s shoulder-devil cartoon scaled up to four feet—carrying a pitchfork.
- **GTA III Mapping:**
  - **`.dff` Requirement:** Cannot be achieved via TXD reskin alone; requires a dedicated custom `.dff` (scaled 4-ft rig, horns, tail/wings if desired, and a pitchfork weapon/prop replacing `#BAT` or attached prop).

---

## 4. Next Implementation Steps (Portland Priority)
1. Extract vanilla `player.txd`, `misty.txd` / `maria.txd`, `kenji.txd`, and `gang05.txd` (plus candidate female `.dff`s for Toni/Marie) from `GTA3/models/gta3.img`.
2. Build & wire a `models` packaging step in [`scripts/build.ts`](../scripts/build.ts) copying `data/models/gta3.img/*` -> `modloader/Eventide/models/gta3.img/`.
3. Reskin the 4 active Portland characters (**Toni**, **Marie**, **Pentor**, **Reinhardt**) first, then add **Zoey**'s Act 2 swap (`Char.Undress` / `Char.Dress`) into [`threads/bleed.txt`](../threads/bleed.txt) and [`main.txt`](../main.txt).
