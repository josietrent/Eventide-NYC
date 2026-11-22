#define WIN32_LEAN_AND_MEAN
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <windows.h>

namespace {

// GTA III v1.0 globals and pools
constexpr uintptr_t ADDR_TIME_MS = 0x885B48;
constexpr uintptr_t ADDR_PED_POOL_PTR = 0x8F2C60;
constexpr uintptr_t VTABLE_CAUTOMOBILE = 0x600C1C;

// GTA III v1.0 functions
constexpr uintptr_t ADDR_FIND_PLAYER_VEHICLE = 0x4A10C0;
constexpr uintptr_t ADDR_FIND_PLAYER_PED = 0x4A1150;
constexpr uintptr_t ADDR_PED_CHECK_FOR_GUNSHOTS = 0x4D47D0;
constexpr uintptr_t ADDR_PED_SET_OBJECTIVE_ENT = 0x4D83E0;
constexpr uintptr_t ADDR_PED_SAY = 0x4E5A10;
constexpr uintptr_t ADDR_AUTOMOBILE_PROCESS_CONTROL = 0x531470;
constexpr uintptr_t ADDR_VEHICLE_INFLICT_DAMAGE = 0x551950;
constexpr uintptr_t ADDR_MUSIC_INITIALISE = 0x57CF70;

// CPool layout & CPed pool stride (sizeof(CPlayerPed) = 0x5F0)
constexpr size_t OFF_POOL_OBJECTS = 0x00;
constexpr size_t OFF_POOL_BYTEMAP = 0x04;
constexpr size_t OFF_POOL_SIZE = 0x08;
constexpr size_t PED_POOL_STRIDE = 0x5F0;

// CEntity / CPhysical offsets
constexpr size_t OFF_ENTITY_VTABLE = 0x00;
constexpr size_t OFF_ENTITY_POS = 0x34;          // float[3] inside m_matrix.pos
constexpr size_t OFF_ENTITY_STATUS_BYTE = 0x50;  // m_nType:3 | m_nStatus:5
constexpr size_t OFF_PHYSICAL_MOVE_SPEED = 0x78; // float[3]

// CVehicle offsets
constexpr size_t OFF_VEH_AUTOPILOT_CAR_MISSION = 0x15A;      // uint8_t
constexpr size_t OFF_VEH_AUTOPILOT_TEMP_ACTION = 0x15B;      // uint8_t
constexpr size_t OFF_VEH_AUTOPILOT_TIME_TEMP_ACTION = 0x15C; // uint32_t
constexpr size_t OFF_VEH_AUTOPILOT_CRUISE_SPEED = 0x164;     // uint8_t
constexpr size_t OFF_VEH_DRIVER = 0x1A4;                     // CPed*
constexpr size_t OFF_VEH_PASSENGERS = 0x1A8;                 // CPed*[8]
constexpr size_t OFF_VEH_MAX_PASSENGERS = 0x1CC;             // uint8_t
constexpr size_t OFF_VEH_GAS_PEDAL = 0x1EC;                  // float
constexpr size_t OFF_VEH_BRAKE_PEDAL = 0x1F0;                // float
constexpr size_t OFF_VEH_CREATED_BY = 0x1F4;                 // uint8_t
constexpr size_t OFF_VEH_FLAGS_1F5 = 0x1F5;                  // uint8_t
constexpr size_t OFF_VEH_HEALTH = 0x200;                     // float
constexpr size_t OFF_VEH_CLASS = 0x284;                      // uint32_t

// CPed offsets
constexpr size_t OFF_PED_FLAGS_158 =
    0x158; // uint8_t (bit 7 = bFleeAfterExitingCar)
constexpr size_t OFF_PED_CREATED_BY = 0x160; // uint8_t
constexpr size_t OFF_PED_OBJECTIVE = 0x164;  // uint32_t
constexpr size_t OFF_PED_STATE = 0x224;      // uint32_t
constexpr size_t OFF_PED_HEALTH = 0x2C0;     // float
constexpr size_t OFF_PED_VEHICLE = 0x310;    // CVehicle*
constexpr size_t OFF_PED_IN_VEHICLE = 0x314; // uint8_t
constexpr size_t OFF_PED_TYPE = 0x32C;       // uint32_t

// Game constants (validated against GTA III v1.0 & re3)
constexpr uint8_t STATUS_ABANDONED = 4;
constexpr uint8_t STATUS_WRECKED = 5;

constexpr uint32_t VEHICLE_AUTOMOBILE = 0;
constexpr uint8_t MISSION_VEHICLE = 2;
constexpr uint8_t VEH_FLAG_HANDBRAKE = 0x20;
constexpr uint8_t MISSION_NONE = 0;
constexpr uint8_t TEMPACT_WAIT = 1;

constexpr uint8_t PED_FLAG_FLEE_AFTER_EXITING_CAR = 0x80;
constexpr uint8_t MISSION_CHAR = 2;
constexpr uint32_t OBJECTIVE_FLEE_ON_FOOT_TILL_SAFE = 2;
constexpr uint32_t OBJECTIVE_LEAVE_CAR = 13;
constexpr uint32_t PEDSTATE_DIE = 48;
constexpr uint32_t PEDSTATE_DEAD = 49;
constexpr uint32_t PEDTYPE_CIVMALE = 4;
constexpr uint32_t PEDTYPE_CIVFEMALE = 5;

constexpr uint16_t SOUND_PED_FLEE_SPRINT = 110;

// Weapon types (eWeaponType)
constexpr uint32_t WEAPONTYPE_COLT45 = 2;
constexpr uint32_t WEAPONTYPE_UZI = 3;
constexpr uint32_t WEAPONTYPE_SHOTGUN = 4;
constexpr uint32_t WEAPONTYPE_AK47 = 5;
constexpr uint32_t WEAPONTYPE_M16 = 6;
constexpr uint32_t WEAPONTYPE_SNIPERRIFLE = 7;
constexpr uint32_t WEAPONTYPE_UZI_DRIVEBY = 19;

// Tuning parameters
constexpr float NEARBY_SCREAM_RADIUS = 28.0f;
constexpr float NEARBY_SCREAM_Z_TOL = 8.0f;
constexpr float GUNSHOT_SUPPRESS_RADIUS = 70.0f;
constexpr uint32_t PED_SCREAM_COOLDOWN_MS = 2500;
constexpr uint32_t GUNSHOT_SUPPRESS_MS = 1200;
constexpr uint32_t STOP_CAR_MAX_MS = 2500;
constexpr uint32_t STOP_CAR_MIN_HOLD_MS = 1200;
constexpr float STOP_SPEED_EPSILON_SQ = 0.0004f;

using MusicBoolFn = bool(__thiscall *)(void *self);
using VehicleInflictDamageFn = void(__thiscall *)(void *self, void *damagedBy,
                                                  uint32_t weaponType,
                                                  float damage);
using AutomobileProcessControlFn = void(__thiscall *)(void *self);
using PedCheckForGunShotsFn = void *(__thiscall *)(void *self);
using PedSetObjectiveEntFn = void(__thiscall *)(void *self, uint32_t objective,
                                                void *entity);
using PedSayFn = void(__thiscall *)(void *self, uint16_t audio);
using FindPlayerVehicleFn = void *(__cdecl *)();
using FindPlayerPedFn = void *(__cdecl *)();

struct HookEntry {
  uintptr_t addr = 0;
  uint8_t origBytes[16] = {};
  size_t patchLen = 0;
  uint8_t *trampoline = nullptr;
  bool active = false;
};

struct StopCarEntry {
  void *veh = nullptr;
  uint32_t startMs = 0;
  uint32_t lastTickMs = 0;
};

struct PedCooldownEntry {
  void *ped = nullptr;
  uint32_t untilMs = 0;
};

struct Vec3 {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};

std::string g_logPath;
std::vector<HookEntry> g_hooks;
bool g_deferredInstalled = false;

MusicBoolFn g_origInitialise = nullptr;
VehicleInflictDamageFn g_origVehicleInflictDamage = nullptr;
AutomobileProcessControlFn g_origAutomobileProcessControl = nullptr;
PedCheckForGunShotsFn g_origPedCheckForGunShots = nullptr;

std::vector<StopCarEntry> g_stopCars;
std::vector<PedCooldownEntry> g_pedCooldowns;
uint32_t g_suppressUntilMs = 0;
Vec3 g_suppressPos{};

void WriteLog(const char *line) {
  if (g_logPath.empty() || !line) {
    return;
  }
  HANDLE f = CreateFileA(g_logPath.c_str(), FILE_APPEND_DATA,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                         OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (f == INVALID_HANDLE_VALUE) {
    return;
  }
  DWORD written = 0;
  WriteFile(f, line, static_cast<DWORD>(std::strlen(line)), &written, nullptr);
  WriteFile(f, "\r\n", 2, &written, nullptr);
  CloseHandle(f);
}

void InitLogPath(HMODULE hModule) {
  char buf[MAX_PATH] = {};
  if (GetModuleFileNameA(hModule, buf, MAX_PATH) > 0) {
    std::string full(buf);
    const size_t pos = full.find_last_of("\\/");
    if (pos != std::string::npos) {
      g_logPath = full.substr(0, pos) + "\\FleeOnShot.log";
    } else {
      g_logPath = "FleeOnShot.log";
    }
  } else {
    g_logPath = "FleeOnShot.log";
  }
  HANDLE f = CreateFileA(g_logPath.c_str(), GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                         CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (f != INVALID_HANDLE_VALUE) {
    CloseHandle(f);
  }
}

size_t DecodeX86InsnLen(const uint8_t *p) {
  size_t i = 0;
  bool opSize16 = false;
  bool addrSize16 = false;

  for (int k = 0; k < 4; ++k) {
    const uint8_t b = p[i];
    if (b == 0x66) {
      opSize16 = true;
      ++i;
    } else if (b == 0x67) {
      addrSize16 = true;
      ++i;
    } else if (b == 0xF0 || b == 0xF2 || b == 0xF3 || b == 0x26 || b == 0x2E ||
               b == 0x36 || b == 0x3E || b == 0x64 || b == 0x65) {
      ++i;
    } else {
      break;
    }
  }

  auto modrmLen = [&](const uint8_t *m) -> size_t {
    const uint8_t modrm = m[0];
    const uint8_t mod = (modrm >> 6) & 3;
    const uint8_t rm = modrm & 7;
    if (addrSize16) {
      if (mod == 0 && rm == 6) {
        return 3;
      }
      if (mod == 1) {
        return 2;
      }
      if (mod == 2) {
        return 3;
      }
      return 1;
    }
    size_t len = 1;
    if (mod != 3 && rm == 4) {
      const uint8_t sib = m[1];
      ++len;
      if (mod == 0 && (sib & 7) == 5) {
        len += 4;
      }
    } else if (mod == 0 && rm == 5) {
      len += 4;
    }
    if (mod == 1) {
      len += 1;
    } else if (mod == 2) {
      len += 4;
    }
    return len;
  };

  const uint8_t op = p[i++];
  const size_t imm32Size = opSize16 ? 2 : 4;

  if (op == 0x0F) {
    const uint8_t op2 = p[i++];
    if (op2 >= 0x80 && op2 <= 0x8F) {
      return i + 4;
    }
    if (op2 == 0xB6 || op2 == 0xB7 || op2 == 0xBE || op2 == 0xBF ||
        op2 == 0xAF || op2 == 0x40 || (op2 >= 0x41 && op2 <= 0x4F)) {
      return i + modrmLen(&p[i]);
    }
    return 0;
  }

  if ((op >= 0x50 && op <= 0x5F) || op == 0x90 || op == 0xC3) {
    return i;
  }

  if (op == 0x6A || op == 0xEB || (op >= 0x70 && op <= 0x7F)) {
    return i + 1;
  }

  if (op == 0x68 || op == 0xE8 || op == 0xE9) {
    return i + imm32Size;
  }

  if (op >= 0xB8 && op <= 0xBF) {
    return i + imm32Size;
  }
  if (op >= 0xB0 && op <= 0xB7) {
    return i + 1;
  }

  if (op >= 0xA0 && op <= 0xA3) {
    return i + (addrSize16 ? 2 : 4);
  }

  if (op == 0x04 || op == 0x0C || op == 0x14 || op == 0x1C || op == 0x24 ||
      op == 0x2C || op == 0x34 || op == 0x3C || op == 0xA8) {
    return i + 1;
  }
  if (op == 0x05 || op == 0x0D || op == 0x15 || op == 0x1D || op == 0x25 ||
      op == 0x2D || op == 0x35 || op == 0x3D || op == 0xA9) {
    return i + imm32Size;
  }

  if ((op <= 0x3B && (op & 7) <= 3) || op == 0x63 || op == 0x84 || op == 0x85 ||
      (op >= 0x88 && op <= 0x8D) || op == 0x8F || (op >= 0xD8 && op <= 0xDF)) {
    return i + modrmLen(&p[i]);
  }

  if (op == 0x80 || op == 0x82 || op == 0x83 || op == 0xC0 || op == 0xC1 ||
      op == 0xC6) {
    return i + modrmLen(&p[i]) + 1;
  }
  if (op == 0x81 || op == 0xC7) {
    return i + modrmLen(&p[i]) + imm32Size;
  }
  if (op == 0xD0 || op == 0xD1 || op == 0xD2 || op == 0xD3) {
    return i + modrmLen(&p[i]);
  }
  if (op == 0xF6) {
    const uint8_t reg = (p[i] >> 3) & 7;
    return i + modrmLen(&p[i]) + (reg <= 1 ? 1 : 0);
  }
  if (op == 0xF7) {
    const uint8_t reg = (p[i] >> 3) & 7;
    return i + modrmLen(&p[i]) + (reg <= 1 ? imm32Size : 0);
  }
  if (op == 0xFE || op == 0xFF) {
    return i + modrmLen(&p[i]);
  }

  return 0;
}

template <typename FnPtr>
bool InstallDetour(uintptr_t addr, void *hookFn, FnPtr *outOrig) {
  const uint8_t *src = reinterpret_cast<const uint8_t *>(addr);

  if (src[0] == 0xE9) {
    const int32_t existingRel = *reinterpret_cast<const int32_t *>(src + 1);
    const uintptr_t existingTarget = addr + 5 + existingRel;
    *outOrig = reinterpret_cast<FnPtr>(existingTarget);

    HookEntry entry;
    entry.addr = addr;
    entry.patchLen = 5;
    std::memcpy(entry.origBytes, src, 5);

    DWORD oldProt = 0;
    if (!VirtualProtect(reinterpret_cast<void *>(addr), 5,
                        PAGE_EXECUTE_READWRITE, &oldProt)) {
      return false;
    }
    *reinterpret_cast<int32_t *>(const_cast<uint8_t *>(src) + 1) =
        static_cast<int32_t>(reinterpret_cast<uintptr_t>(hookFn) - (addr + 5));
    VirtualProtect(reinterpret_cast<void *>(addr), 5, oldProt, &oldProt);
    FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void *>(addr),
                          5);
    entry.active = true;
    g_hooks.push_back(entry);
    return true;
  }

  size_t copyLen = 0;
  while (copyLen < 5) {
    const size_t insnLen = DecodeX86InsnLen(src + copyLen);
    if (insnLen == 0 || copyLen + insnLen > 16) {
      char msg[128] = {};
      std::snprintf(
          msg, sizeof(msg), "Unsupported prologue at 0x%08X (byte 0x%02X)",
          static_cast<unsigned>(addr), static_cast<unsigned>(src[copyLen]));
      WriteLog(msg);
      return false;
    }
    const uint8_t first = src[copyLen];
    if (first == 0xEB || (first >= 0x70 && first <= 0x7F)) {
      char msg[128] = {};
      std::snprintf(msg, sizeof(msg),
                    "Unexpected short jump in prologue at 0x%08X",
                    static_cast<unsigned>(addr));
      WriteLog(msg);
      return false;
    }
    copyLen += insnLen;
  }

  uint8_t *tramp = static_cast<uint8_t *>(VirtualAlloc(
      nullptr, 32, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
  if (!tramp) {
    return false;
  }

  std::memcpy(tramp, src, copyLen);

  size_t off = 0;
  while (off < copyLen) {
    const size_t insnLen = DecodeX86InsnLen(src + off);
    if ((src[off] == 0xE8 || src[off] == 0xE9) && insnLen == 5) {
      const int32_t origRel = *reinterpret_cast<const int32_t *>(src + off + 1);
      const uintptr_t absTarget = addr + off + 5 + origRel;
      *reinterpret_cast<int32_t *>(tramp + off + 1) = static_cast<int32_t>(
          absTarget - (reinterpret_cast<uintptr_t>(tramp) + off + 5));
    }
    off += insnLen;
  }

  tramp[copyLen] = 0xE9;
  *reinterpret_cast<int32_t *>(tramp + copyLen + 1) = static_cast<int32_t>(
      (addr + copyLen) - (reinterpret_cast<uintptr_t>(tramp) + copyLen + 5));
  FlushInstructionCache(GetCurrentProcess(), tramp, copyLen + 5);

  HookEntry entry;
  entry.addr = addr;
  entry.patchLen = copyLen;
  entry.trampoline = tramp;
  std::memcpy(entry.origBytes, src, copyLen);

  DWORD oldProt = 0;
  if (!VirtualProtect(reinterpret_cast<void *>(addr), copyLen,
                      PAGE_EXECUTE_READWRITE, &oldProt)) {
    VirtualFree(tramp, 0, MEM_RELEASE);
    return false;
  }

  uint8_t *dst = const_cast<uint8_t *>(src);
  dst[0] = 0xE9;
  *reinterpret_cast<int32_t *>(dst + 1) =
      static_cast<int32_t>(reinterpret_cast<uintptr_t>(hookFn) - (addr + 5));
  for (size_t k = 5; k < copyLen; ++k) {
    dst[k] = 0x90;
  }
  VirtualProtect(reinterpret_cast<void *>(addr), copyLen, oldProt, &oldProt);
  FlushInstructionCache(GetCurrentProcess(), dst, copyLen);

  entry.active = true;
  g_hooks.push_back(entry);
  *outOrig = reinterpret_cast<FnPtr>(tramp);
  return true;
}

void RemoveAllHooks() {
  for (auto it = g_hooks.rbegin(); it != g_hooks.rend(); ++it) {
    if (!it->active) {
      continue;
    }
    DWORD oldProt = 0;
    if (VirtualProtect(reinterpret_cast<void *>(it->addr), it->patchLen,
                       PAGE_EXECUTE_READWRITE, &oldProt)) {
      std::memcpy(reinterpret_cast<void *>(it->addr), it->origBytes,
                  it->patchLen);
      VirtualProtect(reinterpret_cast<void *>(it->addr), it->patchLen, oldProt,
                     &oldProt);
      FlushInstructionCache(GetCurrentProcess(),
                            reinterpret_cast<void *>(it->addr), it->patchLen);
    }
    if (it->trampoline) {
      VirtualFree(it->trampoline, 0, MEM_RELEASE);
      it->trampoline = nullptr;
    }
    it->active = false;
  }
  g_hooks.clear();
}

inline uint32_t GetTimeMs() {
  return *reinterpret_cast<const uint32_t *>(ADDR_TIME_MS);
}

template <typename T> inline T &FieldAt(void *base, size_t off) {
  return *reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(base) + off);
}

template <typename T> inline const T &FieldAt(const void *base, size_t off) {
  return *reinterpret_cast<const T *>(reinterpret_cast<uintptr_t>(base) + off);
}

inline uint8_t GetEntityStatus(const void *entity) {
  return static_cast<uint8_t>(
      FieldAt<uint8_t>(entity, OFF_ENTITY_STATUS_BYTE) >> 3);
}

inline void SetEntityStatus(void *entity, uint8_t status) {
  uint8_t &b = FieldAt<uint8_t>(entity, OFF_ENTITY_STATUS_BYTE);
  b = static_cast<uint8_t>((b & 0x07u) | ((status & 0x1Fu) << 3));
}

inline Vec3 GetEntityPos(const void *entity) {
  const float *p = &FieldAt<float>(entity, OFF_ENTITY_POS);
  return Vec3{p[0], p[1], p[2]};
}

inline bool IsFirearmWeapon(uint32_t weaponType) {
  return (weaponType >= WEAPONTYPE_COLT45 &&
          weaponType <= WEAPONTYPE_SNIPERRIFLE) ||
         weaponType == WEAPONTYPE_UZI_DRIVEBY;
}

inline bool IsCivilianPedType(uint32_t pedType) {
  return pedType == PEDTYPE_CIVMALE || pedType == PEDTYPE_CIVFEMALE;
}

inline bool IsPedAlive(const void *ped) {
  if (!ped) {
    return false;
  }
  const uint32_t state = FieldAt<uint32_t>(ped, OFF_PED_STATE);
  if (state == PEDSTATE_DIE || state == PEDSTATE_DEAD) {
    return false;
  }
  return FieldAt<float>(ped, OFF_PED_HEALTH) > 0.0f;
}

inline bool IsRoadAutomobile(const void *veh) {
  if (!veh) {
    return false;
  }
  return FieldAt<uintptr_t>(veh, OFF_ENTITY_VTABLE) == VTABLE_CAUTOMOBILE &&
         FieldAt<uint32_t>(veh, OFF_VEH_CLASS) == VEHICLE_AUTOMOBILE;
}

inline bool IsVehicleHealthy(const void *veh) {
  return veh && GetEntityStatus(veh) != STATUS_WRECKED &&
         FieldAt<float>(veh, OFF_VEH_HEALTH) > 0.0f;
}

// Phase 1 active Portland neighborhoods & corridors
// (docs/peds_flee_road_vehicles.md)
bool IsInActivePortlandZone(float x, float y) {
  // 1. Harwood (West of Junkyard)
  if (x >= 746.0f && x <= 1120.0f && y >= -78.2f && y <= 322.7f) {
    return true;
  }
  // 2. Red Light District (REDLIGH)
  if (x >= 745.4f && x <= 1065.4f && y >= -463.6f && y <= -282.6f) {
    return true;
  }
  // 3. Portland View (S_VIEW)
  if (x >= 1066.1f && x <= 1363.6f && y >= -741.8f && y <= -512.8f) {
    return true;
  }
  // 4. Portland Harbor (PORT_E)
  if (x >= 1363.7f && x <= 1815.7f && y >= -1069.7f && y <= -613.6f) {
    return true;
  }
  // 5. Callahan Point (PORT_W)
  if (x >= 751.7f && x <= 1065.7f && y >= -1178.2f && y <= -958.7f) {
    return true;
  }
  // 6. Atlantic Quays (PORT_S)
  if (x >= 1065.9f && x <= 1501.9f && y >= -1251.5f && y <= -1069.9f) {
    return true;
  }
  // 7. Big S-Road & St. Mark's Bistro Corridor (LITTLEI / EASTBAY border)
  if (x >= 1310.0f && x <= 1495.0f && y >= -512.3f && y <= -120.0f) {
    return true;
  }
  return false;
}

void TrackStopVehicle(void *veh, uint32_t now) {
  for (StopCarEntry &entry : g_stopCars) {
    if (entry.veh == veh) {
      entry.startMs = now;
      entry.lastTickMs = now;
      return;
    }
  }
  if (g_stopCars.size() >= 32) {
    g_stopCars.erase(g_stopCars.begin());
  }
  g_stopCars.push_back(StopCarEntry{veh, now, now});
}

void RemoveStopVehicle(size_t index) {
  g_stopCars.erase(g_stopCars.begin() + static_cast<std::ptrdiff_t>(index));
}

bool TryAcquireScreamCooldown(void *ped, uint32_t now) {
  for (size_t i = g_pedCooldowns.size(); i > 0; --i) {
    const size_t idx = i - 1;
    const uint32_t until = g_pedCooldowns[idx].untilMs;
    // Also purge stale entries on timer rewind (save reload)
    if (now >= until || (until - now) > PED_SCREAM_COOLDOWN_MS * 2) {
      g_pedCooldowns.erase(g_pedCooldowns.begin() +
                           static_cast<std::ptrdiff_t>(idx));
    }
  }
  for (PedCooldownEntry &entry : g_pedCooldowns) {
    if (entry.ped == ped) {
      return false;
    }
  }
  if (g_pedCooldowns.size() >= 64) {
    g_pedCooldowns.erase(g_pedCooldowns.begin());
  }
  g_pedCooldowns.push_back(PedCooldownEntry{ped, now + PED_SCREAM_COOLDOWN_MS});
  return true;
}

void MakeNearbyPedsScream(const Vec3 &center, void *playerPed, uint32_t now) {
  void *pedPool = *reinterpret_cast<void **>(ADDR_PED_POOL_PTR);
  if (!pedPool) {
    return;
  }

  uint8_t *objects = FieldAt<uint8_t *>(pedPool, OFF_POOL_OBJECTS);
  const uint8_t *byteMap = FieldAt<const uint8_t *>(pedPool, OFF_POOL_BYTEMAP);
  const int32_t size = FieldAt<int32_t>(pedPool, OFF_POOL_SIZE);
  if (!objects || !byteMap || size <= 0) {
    return;
  }

  auto pedSay = reinterpret_cast<PedSayFn>(ADDR_PED_SAY);
  const float maxDistSq = NEARBY_SCREAM_RADIUS * NEARBY_SCREAM_RADIUS;

  for (int32_t i = 0; i < size; ++i) {
    if ((byteMap[i] & 0x80u) != 0) {
      continue;
    }
    void *ped = objects + static_cast<size_t>(i) * PED_POOL_STRIDE;
    if (ped == playerPed) {
      continue;
    }
    if (FieldAt<uint8_t>(ped, OFF_PED_IN_VEHICLE) != 0 ||
        FieldAt<void *>(ped, OFF_PED_VEHICLE) != nullptr) {
      continue;
    }
    if (!IsPedAlive(ped)) {
      continue;
    }
    if (FieldAt<uint8_t>(ped, OFF_PED_CREATED_BY) == MISSION_CHAR ||
        !IsCivilianPedType(FieldAt<uint32_t>(ped, OFF_PED_TYPE))) {
      continue;
    }

    const Vec3 pos = GetEntityPos(ped);
    const float dz = pos.z - center.z;
    if (dz < -NEARBY_SCREAM_Z_TOL || dz > NEARBY_SCREAM_Z_TOL) {
      continue;
    }
    const float dx = pos.x - center.x;
    const float dy = pos.y - center.y;
    if (dx * dx + dy * dy > maxDistSq) {
      continue;
    }

    if (TryAcquireScreamCooldown(ped, now)) {
      pedSay(ped, SOUND_PED_FLEE_SPRINT);
    }
  }
}

void ForceVehicleStopAndBail(void *veh, void *driver, void *const *passengers,
                             uint8_t maxPassengers, void *playerPed,
                             uint32_t now) {
  SetEntityStatus(veh, STATUS_ABANDONED);

  FieldAt<uint8_t>(veh, OFF_VEH_AUTOPILOT_CAR_MISSION) = MISSION_NONE;
  FieldAt<uint8_t>(veh, OFF_VEH_AUTOPILOT_TEMP_ACTION) = TEMPACT_WAIT;
  FieldAt<uint32_t>(veh, OFF_VEH_AUTOPILOT_TIME_TEMP_ACTION) =
      now + STOP_CAR_MAX_MS;
  FieldAt<uint8_t>(veh, OFF_VEH_AUTOPILOT_CRUISE_SPEED) = 0;
  FieldAt<float>(veh, OFF_VEH_GAS_PEDAL) = 0.0f;
  FieldAt<float>(veh, OFF_VEH_BRAKE_PEDAL) = 1.0f;
  FieldAt<uint8_t>(veh, OFF_VEH_FLAGS_1F5) |= VEH_FLAG_HANDBRAKE;

  TrackStopVehicle(veh, now);

  auto setObjectiveEnt =
      reinterpret_cast<PedSetObjectiveEntFn>(ADDR_PED_SET_OBJECTIVE_ENT);

  auto commandBail = [&](void *ped) {
    if (!ped || ped == playerPed || !IsPedAlive(ped)) {
      return;
    }
    FieldAt<uint8_t>(ped, OFF_PED_FLAGS_158) |= PED_FLAG_FLEE_AFTER_EXITING_CAR;
    if (FieldAt<uint32_t>(ped, OFF_PED_OBJECTIVE) != OBJECTIVE_LEAVE_CAR) {
      setObjectiveEnt(ped, OBJECTIVE_LEAVE_CAR, veh);
    }
  };

  commandBail(driver);
  void *curDriver = FieldAt<void *>(veh, OFF_VEH_DRIVER);
  if (curDriver && curDriver != driver) {
    commandBail(curDriver);
  }
  const uint8_t cap = maxPassengers > 8 ? 8 : maxPassengers;
  for (uint8_t i = 0; i < cap; ++i) {
    commandBail(passengers[i]);
  }
}

void __fastcall Hook_VehicleInflictDamage(void *self, uintptr_t,
                                          void *damagedBy, uint32_t weaponType,
                                          float damage) {
  auto findPlayerPed = reinterpret_cast<FindPlayerPedFn>(ADDR_FIND_PLAYER_PED);
  auto findPlayerVeh =
      reinterpret_cast<FindPlayerVehicleFn>(ADDR_FIND_PLAYER_VEHICLE);
  void *playerPed = findPlayerPed();
  void *playerVeh = findPlayerVeh();

  bool triggerCustom = false;
  void *origDriver = nullptr;
  void *origPassengers[8] = {};
  uint8_t maxPass = 0;
  Vec3 vehPos{};

  if (IsRoadAutomobile(self) && IsVehicleHealthy(self) &&
      FieldAt<uint8_t>(self, OFF_VEH_CREATED_BY) != MISSION_VEHICLE &&
      IsFirearmWeapon(weaponType) && damagedBy != nullptr &&
      (damagedBy == playerPed || damagedBy == playerVeh)) {
    origDriver = FieldAt<void *>(self, OFF_VEH_DRIVER);
    if (origDriver && origDriver != playerPed && IsPedAlive(origDriver) &&
        FieldAt<uint8_t>(origDriver, OFF_PED_CREATED_BY) != MISSION_CHAR &&
        IsCivilianPedType(FieldAt<uint32_t>(origDriver, OFF_PED_TYPE))) {
      vehPos = GetEntityPos(self);
      if (IsInActivePortlandZone(vehPos.x, vehPos.y)) {
        triggerCustom = true;
        maxPass = FieldAt<uint8_t>(self, OFF_VEH_MAX_PASSENGERS);
        if (maxPass > 8) {
          maxPass = 8;
        }
        void *const *passArray = &FieldAt<void *>(self, OFF_VEH_PASSENGERS);
        for (uint8_t i = 0; i < maxPass; ++i) {
          origPassengers[i] = passArray[i];
        }
      }
    }
  }

  g_origVehicleInflictDamage(self, damagedBy, weaponType, damage);

  if (triggerCustom && IsVehicleHealthy(self)) {
    const uint32_t now = GetTimeMs();
    ForceVehicleStopAndBail(self, origDriver, origPassengers, maxPass,
                            playerPed, now);

    // Prevent nearby on-foot civilian peds from fleeing due to the same
    // gunshot, while still making nearby civilians vocalize a scream.
    g_suppressUntilMs = now + GUNSHOT_SUPPRESS_MS;
    g_suppressPos = vehPos;
    MakeNearbyPedsScream(vehPos, playerPed, now);
  }
}

void __fastcall Hook_AutomobileProcessControl(void *self, uintptr_t) {
  size_t matchIndex = static_cast<size_t>(-1);
  for (size_t i = 0; i < g_stopCars.size(); ++i) {
    if (g_stopCars[i].veh == self) {
      matchIndex = i;
      break;
    }
  }

  if (matchIndex == static_cast<size_t>(-1)) {
    g_origAutomobileProcessControl(self);
    return;
  }

  const uint32_t now = GetTimeMs();
  StopCarEntry &entry = g_stopCars[matchIndex];

  auto findPlayerPed = reinterpret_cast<FindPlayerPedFn>(ADDR_FIND_PLAYER_PED);
  void *playerPed = findPlayerPed();
  void *driver = FieldAt<void *>(self, OFF_VEH_DRIVER);

  const bool timerInvalid = now < entry.startMs ||
                            (now - entry.lastTickMs > 5000) ||
                            (now - entry.startMs > STOP_CAR_MAX_MS);
  const bool newDriverTookOver =
      driver != nullptr &&
      (driver == playerPed ||
       ((FieldAt<uint8_t>(driver, OFF_PED_FLAGS_158) &
         PED_FLAG_FLEE_AFTER_EXITING_CAR) == 0 &&
        FieldAt<uint32_t>(driver, OFF_PED_OBJECTIVE) != OBJECTIVE_LEAVE_CAR));

  if (!IsVehicleHealthy(self) || timerInvalid || newDriverTookOver) {
    RemoveStopVehicle(matchIndex);
    g_origAutomobileProcessControl(self);
    return;
  }

  SetEntityStatus(self, STATUS_ABANDONED);
  FieldAt<uint8_t>(self, OFF_VEH_AUTOPILOT_CAR_MISSION) = MISSION_NONE;
  FieldAt<uint8_t>(self, OFF_VEH_AUTOPILOT_TEMP_ACTION) = TEMPACT_WAIT;
  FieldAt<uint8_t>(self, OFF_VEH_AUTOPILOT_CRUISE_SPEED) = 0;
  FieldAt<float>(self, OFF_VEH_GAS_PEDAL) = 0.0f;
  FieldAt<float>(self, OFF_VEH_BRAKE_PEDAL) = 1.0f;
  FieldAt<uint8_t>(self, OFF_VEH_FLAGS_1F5) |= VEH_FLAG_HANDBRAKE;

  uint32_t dtMs = now > entry.lastTickMs ? (now - entry.lastTickMs) : 16;
  if (dtMs > 100) {
    dtMs = 100;
  }
  entry.lastTickMs = now;

  float damp = 1.0f - static_cast<float>(dtMs) / 240.0f;
  if (damp < 0.70f) {
    damp = 0.70f;
  }

  float *moveSpeed = &FieldAt<float>(self, OFF_PHYSICAL_MOVE_SPEED);
  moveSpeed[0] *= damp;
  moveSpeed[1] *= damp;
  if (moveSpeed[0] * moveSpeed[0] + moveSpeed[1] * moveSpeed[1] <
      STOP_SPEED_EPSILON_SQ) {
    moveSpeed[0] = 0.0f;
    moveSpeed[1] = 0.0f;
  }

  g_origAutomobileProcessControl(self);

  // STATUS_ABANDONED inside ProcessControl resets brake/handbrake to 0.2/false
  // before exit finishes; re-apply full braking so cars don't roll down hills.
  FieldAt<float>(self, OFF_VEH_BRAKE_PEDAL) = 1.0f;
  FieldAt<uint8_t>(self, OFF_VEH_FLAGS_1F5) |= VEH_FLAG_HANDBRAKE;

  const float *postSpeed = &FieldAt<float>(self, OFF_PHYSICAL_MOVE_SPEED);
  const float speedSq2D =
      postSpeed[0] * postSpeed[0] + postSpeed[1] * postSpeed[1];
  const uint32_t elapsed = now - entry.startMs;
  if (speedSq2D < STOP_SPEED_EPSILON_SQ && elapsed >= STOP_CAR_MIN_HOLD_MS &&
      FieldAt<void *>(self, OFF_VEH_DRIVER) == nullptr) {
    RemoveStopVehicle(matchIndex);
  }
}

void *__fastcall Hook_PedCheckForGunShots(void *self, uintptr_t) {
  void *shooter = g_origPedCheckForGunShots(self);
  if (!shooter || g_suppressUntilMs == 0) {
    return shooter;
  }

  const uint32_t now = GetTimeMs();
  if (now > g_suppressUntilMs ||
      (g_suppressUntilMs - now) > GUNSHOT_SUPPRESS_MS * 2) {
    g_suppressUntilMs = 0;
    return shooter;
  }

  auto findPlayerPed = reinterpret_cast<FindPlayerPedFn>(ADDR_FIND_PLAYER_PED);
  auto findPlayerVeh =
      reinterpret_cast<FindPlayerVehicleFn>(ADDR_FIND_PLAYER_VEHICLE);
  if (shooter != findPlayerPed() && shooter != findPlayerVeh()) {
    return shooter;
  }

  if (FieldAt<uint8_t>(self, OFF_PED_IN_VEHICLE) != 0 ||
      FieldAt<void *>(self, OFF_PED_VEHICLE) != nullptr) {
    return shooter;
  }

  if (FieldAt<uint8_t>(self, OFF_PED_CREATED_BY) == MISSION_CHAR ||
      !IsCivilianPedType(FieldAt<uint32_t>(self, OFF_PED_TYPE))) {
    return shooter;
  }

  if ((FieldAt<uint8_t>(self, OFF_PED_FLAGS_158) &
       PED_FLAG_FLEE_AFTER_EXITING_CAR) != 0 ||
      FieldAt<uint32_t>(self, OFF_PED_OBJECTIVE) ==
          OBJECTIVE_FLEE_ON_FOOT_TILL_SAFE) {
    return shooter;
  }

  const Vec3 pos = GetEntityPos(self);
  const float dx = pos.x - g_suppressPos.x;
  const float dy = pos.y - g_suppressPos.y;
  if (dx * dx + dy * dy <= GUNSHOT_SUPPRESS_RADIUS * GUNSHOT_SUPPRESS_RADIUS) {
    return nullptr;
  }

  return shooter;
}

void InstallDeferredHooks() {
  if (g_deferredInstalled) {
    return;
  }
  g_deferredInstalled = true;

  bool ok = true;
  ok &= InstallDetour(ADDR_VEHICLE_INFLICT_DAMAGE, &Hook_VehicleInflictDamage,
                      &g_origVehicleInflictDamage);
  ok &= InstallDetour(ADDR_AUTOMOBILE_PROCESS_CONTROL,
                      &Hook_AutomobileProcessControl,
                      &g_origAutomobileProcessControl);
  ok &= InstallDetour(ADDR_PED_CHECK_FOR_GUNSHOTS, &Hook_PedCheckForGunShots,
                      &g_origPedCheckForGunShots);

  WriteLog(ok ? "FleeOnShot hooks installed OK"
              : "FleeOnShot hooks partial/failed");
}

bool __fastcall Hook_Initialise(void *self, uintptr_t) {
  const bool ok = g_origInitialise(self);
  InstallDeferredHooks();
  return ok;
}

bool InstallBootHook() {
  const bool ok =
      InstallDetour(ADDR_MUSIC_INITIALISE, &Hook_Initialise, &g_origInitialise);
  WriteLog(ok ? "Boot hook (cMusicManager::Initialise) OK"
              : "Boot hook (cMusicManager::Initialise) FAILED");
  return ok;
}

} // namespace

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(hModule);
    InitLogPath(hModule);
    InstallBootHook();
  } else if (reason == DLL_PROCESS_DETACH) {
    if (lpReserved == nullptr) {
      RemoveAllHooks();
    }
  }
  return TRUE;
}
