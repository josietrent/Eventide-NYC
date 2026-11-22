# Civilian Drivers Exit & Flee on Road Vehicle Hits

Design specification and canonical GTA III zone/neighborhood reference for _Eventide III_.

> [!NOTE]
> **Status:** Implemented in [`FleeOnShot.cpp`](../plugins/FleeOnShot/FleeOnShot.cpp) (packaged as `FleeOnShot.asi`)
> **Scope:** Portland (Phase 1) — **Staunton Island** & **Shoreside Vale** deferred to TODO below.

---

## 1. Confirmed Gameplay Decisions

When the player fires at and strikes an occupied **road vehicle** inside an active Portland neighborhood/corridor:

1. **Vehicle Stops:** Instead of speeding away in panic, the road vehicle immediately decelerates/brakes to a stop (`STATUS_ABANDONED`, `CAutoPilot` cruise speed `0`, `CAR_MISSION_NONE` / `TEMPACT_WAIT`, handbrake + rapid velocity damping so it stops cleanly even on slopes).
2. **Civilian Occupants Only:** Applies **only when the driver is a random civilian** (`PEDTYPE_CIVMALE` / `PEDTYPE_CIVFEMALE`, excluding `MISSION_CHAR` / `MISSION_VEHICLE`). Police, emergency services, and gang vehicles keep vanilla/aggressive AI.
3. **Driver & Passengers Exit and Flee:** Both the civilian driver **and all passengers** exit the vehicle (`OBJECTIVE_LEAVE_CAR`) and run away on foot (`bFleeAfterExitingCar = true` -> `OBJECTIVE_FLEE_ON_FOOT_TILL_SAFE`).
4. **Nearby On-Foot Pedestrians:** Nearby pedestrians on foot **do not flee** from road-vehicle hits (`CPed::CheckForGunShots` suppressed for nearby non-fleeing civilians during the hit window), **but still vocalize/scream** (`CPed::Say(SOUND_PED_FLEE_SPRINT)` with per-ped cooldown).
5. **Zone Check:** Gated on the **hit vehicle's world coordinates** so neighborhood behavior stays consistent around border streets even if the player fires from across a boundary.

---

## 2. Active Portland Regions (Phase 1)

| Region / Neighborhood                       | Source Zone                  | Bounding Box (`X`, `Y`)                      | Notes                                                                                                                                                                                       |
| ------------------------------------------- | ---------------------------- | -------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Harwood (West of Junkyard)**              | `HARWOOD` (clipped)          | `X [746.0..1120.0]`, `Y [-78.2..322.7]`      | Stops just west of Junkyard entrance `(1120.6, 27.9)` / crusher `(1140.3, 50.1)`                                                                                                            |
| **Red Light District**                      | `REDLIGH`                    | `X [745.4..1065.4]`, `Y [-463.6..-282.6]`    | Full zone                                                                                                                                                                                   |
| **Portland View**                           | `S_VIEW`                     | `X [1066.1..1363.6]`, `Y [-741.8..-512.8]`   | Full zone                                                                                                                                                                                   |
| **Portland Harbor**                         | `PORT_E`                     | `X [1363.7..1815.7]`, `Y [-1069.7..-613.6]`  | Full zone (East Docks)                                                                                                                                                                      |
| **Callahan Point**                          | `PORT_W`                     | `X [751.7..1065.7]`, `Y [-1178.2..-958.7]`   | Full zone (Southwest Docks, contains `FISHFAC`)                                                                                                                                             |
| **Atlantic Quays**                          | `PORT_S`                     | `X [1065.9..1501.9]`, `Y [-1251.5..-1069.9]` | Full zone (South Docks)                                                                                                                                                                     |
| **Big S-Road & St. Mark's Bistro Corridor** | `LITTLEI` / `EASTBAY` border | `X [1310.0..1495.0]`, `Y [-512.3..-120.0]`   | Covers Marco's/St. Mark's Bistro `(1329.7, -465.7)`, north ridge road (`X = 1370.8`), S-curve road (`ind_customroad067`, `1345.8, -233.8`), and Mansion approach (`1428..1490, -220..-150`) |

---

## 3. Complete GTA III Neighborhood & Zone Reference (`gta3.zon` + `american.gxt`)

### Portland

| Neighborhood Name            | Zone Code | Type | Bounds (`X`, `Y`)                            |
| ---------------------------- | --------- | ---- | -------------------------------------------- |
| **Atlantic Quays**           | `PORT_S`  | `0`  | `X [1065.9..1501.9]`, `Y [-1251.5..-1069.9]` |
| **Callahan Bridge**          | `ROADBR1` | `1`  | `X [617.4..1065.4]`, `Y [-958.3..-908.3]`    |
| **Callahan Point**           | `PORT_W`  | `0`  | `X [751.7..1065.7]`, `Y [-1178.2..-958.7]`   |
| **Chinatown**                | `CHINA`   | `0`  | `X [745.4..1065.4]`, `Y [-908.3..-463.7]`    |
| **Harwood**                  | `HARWOOD` | `0`  | `X [746.0..1389.0]`, `Y [-78.2..322.7]`      |
| **Hepburn Heights**          | `TOWERS`  | `0`  | `X [745.4..1065.4]`, `Y [-282.4..-78.8]`     |
| **Portland** _(Island Wide)_ | `IND_ZON` | `0`  | `X [617.2..1902.7]`, `Y [-1329.7..434.1]`    |
| **Portland Beach**           | `EASTBAY` | `0`  | `X [1389.4..1797.6]`, `Y [-613.5..199.6]`    |
| **Portland Harbor**          | `PORT_E`  | `0`  | `X [1363.7..1815.7]`, `Y [-1069.7..-613.6]`  |
| **Portland View**            | `S_VIEW`  | `0`  | `X [1066.1..1363.6]`, `Y [-741.8..-512.8]`   |
| **Red Light District**       | `REDLIGH` | `0`  | `X [745.4..1065.4]`, `Y [-463.6..-282.6]`    |
| **Saint Mark's**             | `LITTLEI` | `0`  | `X [1065.9..1388.9]`, `Y [-512.3..-78.3]`    |
| **Trenton**                  | `PORT_I`  | `0`  | `X [1065.9..1363.4]`, `Y [-1069.8..-742.1]`  |

---

### TODO — Staunton Island & Shoreside Vale (Future Work)

> [!TODO]
> Revisit and pick which **Staunton Island** and **Shoreside Vale** neighborhoods get this behavior once Act 2 and Act 3 scripting begins.

#### Staunton Island

| Neighborhood Name                   | Zone Code | Type | Bounds (`X`, `Y`)                           |
| ----------------------------------- | --------- | ---- | ------------------------------------------- |
| **Aspatria**                        | `STADIUM` | `0`  | `X [-225.8..116.2]`, `Y [-412.6..160.5]`    |
| **Bedford Point**                   | `SHOPING` | `0`  | `X [-224.4..199.6]`, `Y [-1672.0..-1004.5]` |
| **Belleville Park**                 | `PARK`    | `0`  | `X [-121.6..199.3]`, `Y [-1003.1..-413.1]`  |
| **Callahan Bridge**                 | `ROADBR2` | `1`  | `X [444.8..614.9]`, `Y [-958.3..-908.3]`    |
| **Fort Staunton**                   | `CONSTRU` | `0`  | `X [239.9..614.3]`, `Y [-411.6..-61.6]`     |
| **Liberty Campus**                  | `UNIVERS` | `0`  | `X [117.3..239.3]`, `Y [-411.6..-61.6]`     |
| **Newport**                         | `COM_EAS` | `0`  | `X [200.1..615.1]`, `Y [-1059.2..-412.2]`   |
| **Rockford**                        | `HOSPI_2` | `0`  | `X [117.2..615.2]`, `Y [-61.1..268.9]`      |
| **Staunton Island** _(Island Wide)_ | `COM_ZON` | `0`  | `X [-265.5..615.5]`, `Y [-1720.0..367.3]`   |
| **Torrington**                      | `YAKUSA`  | `0`  | `X [199.8..577.8]`, `Y [-1672.4..-1059.9]`  |

#### Shoreside Vale

| Neighborhood Name                  | Zone Code | Type | Bounds (`X`, `Y`)                                                                             |
| ---------------------------------- | --------- | ---- | --------------------------------------------------------------------------------------------- |
| **Cedar Grove**                    | `SWANKS`  | `0`  | `X [-867.2..-266.9]`, `Y [93.4..650.1]`                                                       |
| **Cochrane Dam**                   | `BIG_DAM` | `0`  | `X [-1394.5..-867.5]`, `Y [93.4..704.5]` _(plus `WEE_DAM` `[-1238.6..-910.4, 306.8..504.6]`)_ |
| **Francis Intl. Airport**          | `AIRPORT` | `0`  | `X [-1633.0..-468.6]`, `Y [-1344.7..-268.4]`                                                  |
| **Pike Creek**                     | `SUB_IND` | `0`  | `X [-1407.6..-812.3]`, `Y [-268.0..92.8]`                                                     |
| **Shoreside Vale** _(Island Wide)_ | `SUB_ZON` | `0`  | `X [-1644.6..-266.9]`, `Y [-1351.4..1206.3]` _(plus `SUB_ZO2`, `SUB_ZO3`)_                    |
| **Wichita Gardens**                | `PROJECT` | `0`  | `X [-811.8..-371.0]`, `Y [-268.1..92.7]`                                                      |

---

## 4. Implementation Architecture (`plugins/FleeOnShot/FleeOnShot.cpp`)

1. **Vehicle Damage Hook (`CVehicle::InflictDamage` @ `0x551950`):**
   - Verifies `CAutomobile` road vehicle (`vtable == 0x600C1C`, `VEHICLE_AUTOMOBILE`), `m_nCreatedBy != MISSION_VEHICLE`, damaged by player (`FindPlayerPed` / `FindPlayerVehicle`) with a firearm (`Colt45`, `Uzi`, `Shotgun`, `AK47`, `M16`, `SniperRifle`, `UziDriveby`), alive random civilian driver (`PEDTYPE_CIVMALE` / `PEDTYPE_CIVFEMALE`), and hit vehicle within an active Portland zone.
   - Forces `STATUS_ABANDONED`, stops `CAutoPilot`, commands driver and all living non-player passengers to exit with `bFleeAfterExitingCar = true` and `CPed::SetObjective(OBJECTIVE_LEAVE_CAR, veh)`.
   - Triggers `CPed::Say(SOUND_PED_FLEE_SPRINT)` on nearby on-foot random civilians within `28.0` units (with a `2500 ms` per-ped cooldown) without assigning any flee objective.
2. **Per-Frame Deceleration Hook (`CAutomobile::ProcessControl` @ `0x531470`):**
   - Smoothly damps horizontal `m_vecMoveSpeed` and holds full brake + handbrake until the hit vehicle comes to a complete stop (`< 0.02` speed) and stays anchored while occupants exit, even on hills.
3. **Gunshot Flee Suppression Hook (`CPed::CheckForGunShots` @ `0x4D47D0`):**
   - Suppresses `EVENT_GUNSHOT` flee reactions for `1200 ms` after a qualifying vehicle hit on nearby (`<= 70.0` units) on-foot civilian peds who are not exiting a vehicle, so bystanders scream without fleeing.
