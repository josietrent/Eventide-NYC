# Public & Hired Transport (Taxis, Cabbies & Buses)

Design feasibility and technical specification for rideable passenger transit in _Eventide III_ (GTA III v1.0 ASI plugin).

> [!NOTE]
> **Status:** Portland Bus Transit implemented (`plugins/BusTransit/BusTransit.cpp`, 8 Portland stops + short-range blue `B` blips); Staunton Island candidate notes below; rideable taxi/cabbie hooks pending.
> **Target Environment:** GTA III v1.0 (`gta3.exe`), standalone `.asi` hook reusing entity/autopilot/font primitives already mapped in [`TrafficLaw.cpp`](../plugins/TrafficLaw/TrafficLaw.cpp) and [`FleeOnShot.cpp`](../plugins/FleeOnShot/FleeOnShot.cpp).

---

## 1. Taxis & Cabbies

### Supported Vehicles (`data/default.ide`)

| Vehicle Name | Model ID | Model / TXD | Notes                                            |
| ------------ | -------- | ----------- | ------------------------------------------------ |
| **Taxi**     | `110`    | `taxi`      | Standard 4-door cab                              |
| **Cabbie**   | `128`    | `cabbie`    | Checker-style 4-door cab                         |
| **Borgnine** | `148`    | `borgnine`  | Unique 4-door bloodstained cab (optional toggle) |

Only applies to **occupied random-traffic vehicles** (`CREATED_BY != MISSION_VEHICLE`, driver alive and not `MISSION_CHAR`) so mission vehicles and parked cabs still behave normally when jacked.

### Gameplay Flow

1. **Hailing & Boarding as Passenger**
   - Player presses/holds a dedicated hail/passenger key (e.g., whistle/horn or secondary interact key, or short-press vs hold on Enter) near an occupied Taxi/Cabbie.
   - Hook sets `CAutopilot` `TEMPACT_WAIT` (`0x15B` / `0x15C`) + brake/handbrake so the cab pulls/stops cleanly, suppresses normal driver jacking, and assigns the player `OBJECTIVE_ENTER_CAR_AS_PASSENGER` into the rear/passenger seat.
2. **Destination Menu (No Vanilla Map Waypoint)**
   - Since GTA III v1.0 has no pause-map custom waypoint, render an in-cab destination list via `CFont` (same HUD font pipeline used in [`TrafficLaw.cpp`](../plugins/TrafficLaw/TrafficLaw.cpp)):
     - Curated roadside landmarks per island (Safehouses, Hospitals, Contacts, Docks, Airport, etc.).
     - Gate Staunton and Shoreside destinations by reading `$BarrierIND` (`5012`) and `$BarrierCOM` (`5038`) from `CTheScripts::ScriptSpace` (`0x74B248`) so cabs never route into locked bridges/tunnels.
     - Include a **Free Ride / Stop Here** option for spontaneous drop-offs anywhere along a street.
3. **Driving, Hurry Mode & Trip Skip**
   - **Normal Ride:** Set `CAutopilot` `m_nCarMission` (`0x15A`) to `MISSION_GOTOCOORDS` / `MISSION_GOTOCOORDS_ACCURATE` with `m_nDrivingStyle` (`0x159`) obeying traffic/lights and standard cruise speed (`0x164`).
   - **Hurry Mode:** Toggle driving style to ignore lights / weave around cars and raise cruise speed (with extra fare multiplier).
   - **Trip Skip:** Fade camera out, call `CStreaming::LoadScene` at the roadside drop-off coordinate, place the cab on the curb node, and fade back in.
4. **Meter, Fare & Bail Logic**
   - Track distance traveled or elapsed trip time; deduct from player cash on drop-off or early exit.
   - If the player exits without enough cash, clear fare and apply a 1-star wanted level.
5. **Clean Exit**
   - On exit (`OBJECTIVE_LEAVE_CAR`), wait until player clears the door, restore driver control (`MISSION_CRUISE`), and reset cab status so it doesn't stall in traffic.

### Taxi Edge Cases

- **Curb Nodes Only:** `MISSION_GOTOCOORDS` only navigates loaded road paths (`CPathFind`). Every preset destination coordinate must sit on a valid street/curb node, never inside an alley or compound.
- **Anti-Stuck Watchdog:** If cab velocity stays near zero outside red lights for several seconds, briefly switch to `TEMPACT_REVERSE_STRAIGHT` or warp forward onto the next path node.

---

## 2. Buses & Coaches

### Supported Vehicles (`data/default.ide`)

| Vehicle Name | Model ID | Model / TXD | Notes                                                                                                                           |
| ------------ | -------- | ----------- | ------------------------------------------------------------------------------------------------------------------------------- |
| **Bus**      | `121`    | `bus`       | City transit bus (`big` class)                                                                                                  |
| **Coach**    | `127`    | `coach`     | Intercity / tour coach (`big` class); in vanilla GTA III, only `COACH` has partial NPC-boarding logic when driven by the player |

### Feasibility & Design Differences vs Taxis

Making Buses/Coaches work as real transit is **also feasible via ASI**, and is actually **simpler in UI** (no destination menu required) but **simpler-yet-stricter in routing and vehicle physics**:

1. **Fixed Loop Routes Instead of Arbitrary Waypoints**
   - Define 1–2 fixed clockwise/counter-clockwise street loops per island (and an express cross-island line once `$BarrierIND == 0` / `$BarrierCOM == 0`), each built from ordered roadside bus-stop + turn-guide coordinates.
   - Because `CPathFind` can take awkward shortcuts or fail tight turns with long `big`-class chassis, supplying **per-turn guidepoints** between major stops keeps `MISSION_GOTOCOORDS` reliable.
2. **Bus Stops & Boarding Window**
   - Mark physical stops (using existing GTA III `bussign1` signs on Staunton/Shoreside plus spawned `bussign1` stops across Portland) with short-range blue `B` blips rendered by [`ShortRangeBlips.cpp`](../plugins/ShortRangeBlips/ShortRangeBlips.cpp).
   - When an active route bus reaches a stop coordinate, engage `TEMPACT_WAIT` for ~5–8 seconds so the player (and optionally nearby civilians) can board as a passenger or step off cleanly.
   - **Flat Fare:** Charge a cheap flat ticket (e.g. `$2`–`$5`) on boarding rather than a distance meter.
3. **In-Bus Controls**
   - **Stay On:** Bus continues stop-to-stop indefinitely along its loop.
   - **Request Next Stop / Get Off:** Player signals exit (or uses a Trip Skip to jump straight to the next stop on the loop) and receives `OBJECTIVE_LEAVE_CAR` when stopped.
4. **Seat & Door Constraints (`bus.dff` vs `coach.dff`)**
   - Although `CVehicle::m_pPassengers[8]` (`0x1A8`) holds up to 8 passenger pointers, actual enter/exit behavior depends on each model's `m_nMaxPassengers` (`0x1CC`) and DFF door/seat dummies (`ped_frontseat` / rear seats).
   - **Player:** Can enter via standard `OBJECTIVE_ENTER_CAR_AS_PASSENGER` into the front passenger seat (or be cleanly seated into `m_pPassengers[]` if `m_nMaxPassengers` / DFF dummies are full).
   - **NPC Riders (Optional):** Easiest to support on `COACH` or by placing boarded peds directly into `m_pPassengers[]` while hiding/re-showing them at stops rather than forcing multiple peds through a single front-door dummy.
5. **Streaming & Virtual Schedule**
   - GTA III despawns ambient traffic beyond ~100–150m, so distant buses cannot physically drive around an empty map.
   - Run a lightweight **virtual loop clock** per route; whenever the player is near a bus stop and a virtual bus is due, spawn (or hijack a nearby ambient bus at) the upstream road node approaching that stop so buses reliably arrive on schedule without fighting pool limits. Once the player boards, GTA III keeps that bus loaded for the whole ride.

### Bus Stops - Portland

Vanilla GTA III places **0** `bussign1` (`1350`) props in Portland (`indust*` IPLs). Adding 8 stops at these landmarks forms a full loop around the island (IPL `Z = sidewalk_Z + 1.27`, centered on each sidewalk per COL/IPL measurements):

| #   | Landmark / Stop                                        | Zone      | `bussign1` Placement (`X, Y, Z`) | Heading                            | Sidewalk Basis & Why This Stop                                                                                                                 |
| --- | ------------------------------------------------------ | --------- | -------------------------------- | ---------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------- |
| 1   | **Harwood — Fire Station / Head Radio**                | `HARWOOD` | `1038.25, -118.00, 5.20`         | `90°` (`qz=0.707107, qw=0.707107`) | Centered on west sidewalk (`X=1036.0..1040.5`, `ind_customroad073` `Z=3.933`), serving Fire Station, Head Radio & Borgnine                     |
| 2   | **Harwood — Car Showroom & Junkyard**                  | `HARWOOD` | `1202.75, -68.00, 11.12`         | `90°` (`qz=0.707107, qw=0.707107`) | Centered on east sidewalk (`X=1201.0..1204.5`, `rd_road1b50` `Z=9.851`) outside Easy Credit Autos Showroom & Harwood Junkyard                  |
| 3   | **Red Light District — Subway Station**                | `REDLIGH` | `946.00, -481.25, 15.20`         | `0°` (`qz=0, qw=1`)                | Centered on north sidewalk (`Y=-483.5..-479.0`, `rd_tjunction22` `Z=13.933`), serving Red Light District & Portland Subway                     |
| 4   | **Chinatown — Subway / Boulevard Border**              | `CHINA`   | `1024.46, -484.75, 15.20`        | `0°` (`qz=0, qw=1`)                | Centered on south sidewalk (`Y=-487.0..-482.5`, `roadincustmn` `Z=13.933`) outside `isubntrnce_b` stairs & Chinatown north strip               |
| 5   | **Callahan Point — Greasy Joe's Diner (Under Bridge)** | `PORT_W`  | `881.80, -1011.60, 5.38`         | `90°` (`qz=0.707107, qw=0.707107`) | Inside the Callahan Point curb (`ind_land026` `Z=4.11`) outside Greasy Joe's Diner (adjacent COL seam at `Y=-1012.5` sealed in `indust.col`)   |
| 6   | **Atlantic Quays — Callahan Bridge Side (Vincenzo's)** | `PORT_S`  | `1106.50, -1107.25, 12.09`       | `0°` (`qz=0, qw=1`)                | Centered on south sidewalk (`Y=-1105.0..-1109.5`, `rd_road1a50` `Z=10.82`) near Vincenzo's warehouse & docks (`MafiaGen2` `1113.61, -1113.61`) |
| 7   | **Trenton — Portland Bus Depot**                       | `PORT_I`  | `1274.50, -954.00, 15.20`        | `0°` (`qz=0, qw=1`)                | Centered on sidewalk (`Y=-957.0..-951.0`, `rd_road1a50` `Z=13.933`) outside the Bus Depot entrance wall & East Docks                           |
| 8   | **Portland View — Hospital & Police Station**          | `S_VIEW`  | `1133.25, -606.00, 15.20`        | `90°` (`qz=0.707107, qw=0.707107`) | Centered on east sidewalk (`X=1131.0..1135.5`, `rd_road1a5` `Z=13.933`) outside Sweeney General Hospital & Police Station                      |

### Bus Stops - Staunton Island (Candidates)

Unlike Portland, vanilla Staunton (`comSW.ipl`) already places **2** `bussign1` (`1350`) props facing each other along the main Bedford Point avenue (`rd_Road2A50`, `Z=24.98`, curb sign `Z=26.65`):

- **Existing Vanilla Northbound Stop (East Curb):** `52.25, -1574.06, 26.65` (serves Love Media / Bedford Point Subway `subway_footbridge` at `148.42, -1583.88`)
- **Existing Vanilla Southbound Stop (West Curb):** `31.76, -1518.88, 26.65` (serves Liberty Tree / Courthouse block)

Notice Staunton's road-tile dimensions from `generic.col` / `commer.col`:

- `rd_Road1A50` / `com_1way50`: `±10.0m` total half-width (curb around `±5.5m`, sidewalk center `±7.75m`).
- `rd_Road2A50`: `±15.0m` total half-width (curb around `±10.5m`, sidewalk center `±12.75m`).
- `rd_Road3A50`: `±20.0m` total half-width (curb around `±15.5m`, sidewalk center `±17.75m`).
- **One-way Boulevards:** Staunton has twin one-way spines (`X=-83.19` SB / `X=-63.19` NB around Belleville Park; `X=-158.19` SB / `X=-138.19` NB by the Opera House; `X=406.82` SB / `X=426.82` NB through Newport & Torrington). A Staunton loop should respect one-way lanes so `CAutopilot` never fights opposing traffic.

Recommended **8–10 stop candidates** covering every Staunton zone (`gta3.zon` level `2`):

| #   | District (`gta3.zon`) | Landmark Candidate                            | Road Tile & Reference (`X, Y, Z`)                             | Proposed Curb / Sign Area (`X, Y, Z`)              | Notes & Connections                                                                                                          |
| --- | --------------------- | --------------------------------------------- | ------------------------------------------------------------- | -------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------- |
| 1   | `HOSPI_2` (Rockford)  | **Carson General Hospital (North Ave)**       | `rd_Road2A50` @ `196.81, 83.57, 14.98` (E–W)                  | `~187.50, 71.00, 16.65` (S curb)                   | Directly outside `com_hospital` (`187.48, 19.32`), near Porter Tunnel / Ferry plaza                                          |
| 2   | `UNIVERS` (Campus)    | **Liberty Campus & Rockford/Campus Subway**   | `com_cust_roads48` @ `251.73, -56.40, 19.79`                  | `~241.50, -56.40, 21.46` (W curb)                  | Steps from `subntrnce_n2` (`231.66, -50.83`) & `subntrnce_n` (`237.97, -103.10`); alternative on `106.81, -151.43` West      |
| 3   | `STADIUM` (Aspatria)  | **Liberty Memorial Coliseum (Stadium)**       | `rd_Road2A50` @ `-159.98, -18.44, 14.98` (West road)          | `~-147.25, -18.44, 16.65` (E curb)                 | Outside `Clnm_stadium` (`-18.83, -99.88`); pairs well with Campus West gate (`106.81, -151.43`)                              |
| 4   | `CONSTRU` (Ft. St.)   | **Fort Staunton Construction Site**           | `rd_Road1A50` @ `451.83, -266.40, 20.21` (N–S)                | `~444.10, -279.50, 21.88` (W curb)                 | Along `cons_build2base` (`399.76, -279.55`) & pedestrian tunnel (`X=444.13`)                                                 |
| 5   | `PARK` (Belleville)   | **Belleville Park Fire Station** _(alt)_      | `rd_Road2A50` @ `-103.19, -441.43, 14.98`                     | `~-90.50, -441.43, 16.65` (E curb)                 | Beside `firestation` (`-62.81, -435.34`) & west side of Belleville Park                                                      |
| 6   | `PARK` (Belleville)   | **Belleville Park Subway (`sign2sub`)**       | `com_1way20` @ `-83.19` SB / `-63.19` NB, `-661.43, 24.98`    | `~-90.90` SB / `~-55.50` NB (`Z=26.65`)            | Directly flanks Belleville Park Subway entrance (`-72.62, -658.32`) and central mall overpass (`mallB`)                      |
| 7   | `COM_EAS` (Newport)   | **Newport Ammu-Nation & Multistory Car Park** | `rd_Road2A50` @ `321.81..371.81, -696.43, 24.98`              | `~347.00, -709.00, 26.65` (S curb)                 | Right in front of `com_munation` (`346.96, -714.06`); near Multistory Car Park & central cross-town avenue                   |
| 8   | `COM_EAS` (Newport)   | **Asuka's Condo / Newport Safehouse & Docks** | `com_1way50` @ `426.82` NB (`406.82` SB), `-656.43, 24.98`    | `~434.50, -650.00, 26.65` (E curb NB)              | Serves Staunton Condo Savehouse (`513.55, -627.24`) & Callahan Bridge ramp; `Pay 'n' Spray` (`380.90, -493.70`) is 1 block N |
| 9   | `YAKUSA` (Torrington) | **Staunton Police HQ & Museum**               | `rd_Road3A50` @ `296.81, -1111.43, 24.98`                     | `~314.50, -1116.43, 26.65` (E curb)                | 6-lane boulevard outside `police_com` (`356.81, -1116.43`) & `museum` (`362.61, -987.92`)                                    |
| 10  | `YAKUSA` (Torrington) | **Kenji's Casino**                            | `com_1way10`/`50` @ `406.81, -1376.43, 24.98` (SB only)       | `~414.50, -1376.00, 26.65` (E curb)                | West facade of `casino` (`464.15, -1389.54`); note `426.82` NB lane ends north of here, so `536.82` carries NB traffic       |
| 11  | `SHOPING` (Bedford)   | **Bedford Point / Courthouse & Subway**       | `rd_Road2A50` @ `41.82, -1506..-1556, 24.98`                  | **Vanilla:** `52.25, -1574.06` / `31.76, -1518.88` | Reuse existing `comSW.ipl` signs; South stop sits ~95m west of `subway_footbridge` (`148.42, -1583.88`)                      |
| 12  | `SHOPING` (Bedford)   | **Opera House / Art Gallery** _(optional)_    | `com_1way50` @ `-158.19` SB / `-138.19` NB, `-1281.43, 24.98` | `~-165.90` SB / `~-130.50` NB (`Z=26.65`)          | West commercial strip serving `opera_house` (`-81.78, -1148.05`) & `artglry_stn` (`-42.80, -1295.47`)                        |

> **Next Step (when expanding to Staunton):** Gate Staunton & cross-bridge routes on `$BarrierIND == 0` (slot `5012` in `CTheScripts::ScriptSpace`, already mapped in `ShortRangeBlips.cpp` & `TrafficLaw.cpp`), pick 8 stops that trace a clean loop matching the `com_1way*` directions (e.g. **Clockwise**: Belleville Park NB -> Stadium -> Hospital -> Campus Subway -> Fort Staunton -> Newport/Condo -> Torrington Police HQ -> Casino -> Bedford Point SB -> Opera House), and verify exact COL sidewalks + lamppost clearance before placing `bussign1` props in `data/maps/`.

---

## 3. Shared GTA III v1.0 Primitives Already Mapped in Repo

All core vehicle, ped, camera, HUD, and script-space interfaces needed for both systems are already validated in [`TrafficLaw.cpp`](../plugins/TrafficLaw/TrafficLaw.cpp) and [`FleeOnShot.cpp`](../plugins/FleeOnShot/FleeOnShot.cpp):

- **Pools & Player Lookup:** `ADDR_PED_POOL_PTR` (`0x8F2C60`), `ADDR_VEH_POOL_PTR` (`0x9430DC`), `FindPlayerPed` (`0x4A1150`), `FindPlayerVehicle` (`0x4A10C0`)
- **Script Globals:** `CTheScripts::ScriptSpace` (`0x74B248`) for `$BarrierIND` / `$BarrierCOM` / `$onmission`
- **Vehicle & Autopilot Control:** `OFF_ENTITY_MODEL_INDEX` (`0x5C`), `OFF_VEH_DRIVER` (`0x1A4`), `OFF_VEH_PASSENGERS` (`0x1A8`), `OFF_VEH_MAX_PASSENGERS` (`0x1CC`), `OFF_VEH_AUTOPILOT_DRIVING_STYLE` (`0x159`), `OFF_VEH_AUTOPILOT_CAR_MISSION` (`0x15A`), `OFF_VEH_AUTOPILOT_TEMP_ACTION` (`0x15B`), `OFF_VEH_AUTOPILOT_CRUISE_SPEED` (`0x164`)
- **Ped Objectives:** `CPed::SetObjective` (`0x4D83E0`), `OFF_PED_OBJECTIVE` (`0x164`), `OFF_PED_VEHICLE` (`0x310`), `OFF_PED_IN_VEHICLE` (`0x314`)
- **UI Rendering:** `CFont` print/draw/color/scale helpers (`0x500F50`..`0x501E70`)
