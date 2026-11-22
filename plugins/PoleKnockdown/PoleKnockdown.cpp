#define WIN32_LEAN_AND_MEAN
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <windows.h>

namespace {

// GTA III v1.0 globals and functions (verified against gta3.exe)
constexpr uintptr_t ADDR_SCRIPT_SPACE = 0x74B248; // CTheScripts::ScriptSpace
constexpr uintptr_t ADDR_TIME_MS = 0x885B48; // CTimer::m_snTimeInMilliseconds
constexpr uintptr_t ADDR_OBJECT_POOL_PTR = 0x880E28; // CPools::ms_pObjectPool

constexpr uintptr_t ADDR_SCRIPTS_PROCESS = 0x439040; // CTheScripts::Process
constexpr uintptr_t ADDR_PHYSICAL_APPLY_COLLISION =
    0x4973A0; // CPhysical::ApplyCollision
constexpr uintptr_t ADDR_FIND_PLAYER_VEHICLE = 0x4A10C0; // FindPlayerVehicle
constexpr uintptr_t ADDR_FIND_PLAYER_PED = 0x4A1150;     // FindPlayerPed
constexpr uintptr_t ADDR_MUSIC_INITIALISE =
    0x57CF70; // cMusicManager::Initialise

// CPool layout & CObjectPool stride (verified at 0x4BBDF0 / 0x4BBE4A)
constexpr size_t OFF_POOL_OBJECTS = 0x00;
constexpr size_t OFF_POOL_BYTEMAP = 0x04;
constexpr size_t OFF_POOL_SIZE = 0x08;
constexpr size_t OBJECT_POOL_STRIDE = 0x19C;

// CEntity offsets (verified against gta3.exe v1.0)
constexpr size_t OFF_ENTITY_FLAGS_50 = 0x50;    // m_nType:3 | m_nStatus:5
constexpr size_t OFF_ENTITY_FLAGS_51 = 0x51;    // bit 2 (0x04) = bIsStatic
constexpr size_t OFF_ENTITY_MODEL_INDEX = 0x5C; // int16_t

constexpr uint8_t ENTITY_TYPE_MASK = 0x07;
constexpr uint8_t ENTITY_TYPE_VEHICLE = 2;
constexpr uint8_t ENTITY_TYPE_OBJECT = 4;
constexpr uint8_t ENTITY_FLAG_IS_STATIC = 0x04;

// Target uprootable streetlight & traffic signal model IDs (generic.IDE /
// object.dat)
constexpr int16_t MI_LAMPPOST3 = 1345;
constexpr int16_t MI_LAMPPOST1 = 1393;
constexpr int16_t MI_DOUBLESTREETLGHT1 = 1394;
constexpr int16_t MI_LAMPPOST2 = 1404;
constexpr int16_t MI_TRAFFICLIGHT1 = 1405;

// Pinned global variable slots in utils/globals.txt
constexpr size_t SLOT_POLE_TOTAL = 5146;
constexpr size_t SLOT_POLE_LAST_MODEL = 5147;
constexpr size_t SLOT_POLE_ENABLED = 5148;
constexpr int32_t POLE_MODE_DISABLED = 2;

// Window (ms) in which an NPC vehicle rammed by the player still credits a
// knockdown
constexpr uint32_t BILLIARD_ATTRIBUTION_MS = 2500;

using MusicBoolFn = bool(__thiscall *)(void *self);
using ScriptsProcessFn = void(__cdecl *)();
using ApplyCollisionFn = bool(__thiscall *)(void *self, void *entityB,
                                            void *colPoint, float *impulseA,
                                            float *impulseB);
using FindPlayerVehicleFn = void *(__cdecl *)();
using FindPlayerPedFn = void *(__cdecl *)();

struct HookEntry {
  uintptr_t addr = 0;
  uint8_t origBytes[16] = {};
  size_t patchLen = 0;
  uint8_t *trampoline = nullptr;
  bool active = false;
};

struct CountedPole {
  void *obj = nullptr;
  int16_t modelIndex = 0;
};

struct BumpedVehicle {
  void *veh = nullptr;
  uint32_t untilMs = 0;
};

std::string g_logPath;
std::vector<HookEntry> g_hooks;
bool g_deferredInstalled = false;

MusicBoolFn g_origInitialise = nullptr;
ScriptsProcessFn g_origScriptsProcess = nullptr;
ApplyCollisionFn g_origApplyCollision = nullptr;

std::vector<CountedPole> g_countedPoles;
std::vector<BumpedVehicle> g_bumpedVehicles;

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
  (void)hModule;
  char buf[MAX_PATH] = {};
  if (GetModuleFileNameA(nullptr, buf, MAX_PATH) > 0) {
    std::string full(buf);
    const size_t pos = full.find_last_of("\\/");
    if (pos != std::string::npos) {
      g_logPath = full.substr(0, pos) + "\\PoleKnockdown.log";
    } else {
      g_logPath = "PoleKnockdown.log";
    }
  } else {
    g_logPath = "PoleKnockdown.log";
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

template <typename T> inline const T &FieldAt(const void *base, size_t off) {
  return *reinterpret_cast<const T *>(reinterpret_cast<uintptr_t>(base) + off);
}

inline int32_t *ScriptGlobalPtr(size_t slot) {
  return reinterpret_cast<int32_t *>(ADDR_SCRIPT_SPACE + slot * 4);
}

inline uint32_t TimeMs() {
  return *reinterpret_cast<const uint32_t *>(ADDR_TIME_MS);
}

inline bool IsTargetPoleModel(int16_t modelIndex) {
  return modelIndex == MI_LAMPPOST1 || modelIndex == MI_LAMPPOST2 ||
         modelIndex == MI_LAMPPOST3 || modelIndex == MI_DOUBLESTREETLGHT1 ||
         modelIndex == MI_TRAFFICLIGHT1;
}

inline bool IsEntityType(const void *entity, uint8_t type) {
  return (FieldAt<uint8_t>(entity, OFF_ENTITY_FLAGS_50) & ENTITY_TYPE_MASK) ==
         type;
}

inline bool IsEntityStatic(const void *entity) {
  return (FieldAt<uint8_t>(entity, OFF_ENTITY_FLAGS_51) &
          ENTITY_FLAG_IS_STATIC) != 0;
}

bool IsUprightTargetPole(const void *entity, int16_t *outModel) {
  if (!entity || !IsEntityType(entity, ENTITY_TYPE_OBJECT) ||
      !IsEntityStatic(entity)) {
    return false;
  }
  const int16_t mi = FieldAt<int16_t>(entity, OFF_ENTITY_MODEL_INDEX);
  if (!IsTargetPoleModel(mi)) {
    return false;
  }
  if (outModel) {
    *outModel = mi;
  }
  return true;
}

void RecordBumpedVehicle(void *veh, uint32_t nowMs) {
  if (!veh) {
    return;
  }
  const uint32_t untilMs = nowMs + BILLIARD_ATTRIBUTION_MS;
  for (auto &entry : g_bumpedVehicles) {
    if (entry.veh == veh) {
      entry.untilMs = untilMs;
      return;
    }
  }
  g_bumpedVehicles.push_back({veh, untilMs});
}

bool WasRecentlyBumpedByPlayer(const void *veh, uint32_t nowMs) {
  if (!veh) {
    return false;
  }
  for (const auto &entry : g_bumpedVehicles) {
    if (entry.veh == veh && static_cast<int32_t>(entry.untilMs - nowMs) > 0) {
      return true;
    }
  }
  return false;
}

bool IsPoleAlreadyCounted(const void *pole) {
  for (const auto &entry : g_countedPoles) {
    if (entry.obj == pole) {
      return true;
    }
  }
  return false;
}

void PruneTrackedState() {
  const uint32_t nowMs = TimeMs();
  g_bumpedVehicles.erase(
      std::remove_if(g_bumpedVehicles.begin(), g_bumpedVehicles.end(),
                     [nowMs](const BumpedVehicle &e) {
                       return static_cast<int32_t>(e.untilMs - nowMs) <= 0;
                     }),
      g_bumpedVehicles.end());

  if (g_countedPoles.empty()) {
    return;
  }

  const uintptr_t pool =
      *reinterpret_cast<const uintptr_t *>(ADDR_OBJECT_POOL_PTR);
  if (pool == 0) {
    g_countedPoles.clear();
    return;
  }

  const uintptr_t objectsBase = FieldAt<uintptr_t>(
      reinterpret_cast<const void *>(pool), OFF_POOL_OBJECTS);
  const uint8_t *byteMap = FieldAt<const uint8_t *>(
      reinterpret_cast<const void *>(pool), OFF_POOL_BYTEMAP);
  const int32_t poolSize =
      FieldAt<int32_t>(reinterpret_cast<const void *>(pool), OFF_POOL_SIZE);

  if (!objectsBase || !byteMap || poolSize <= 0) {
    g_countedPoles.clear();
    return;
  }

  const uintptr_t poolEnd =
      objectsBase + static_cast<uintptr_t>(poolSize) * OBJECT_POOL_STRIDE;

  g_countedPoles.erase(
      std::remove_if(
          g_countedPoles.begin(), g_countedPoles.end(),
          [&](const CountedPole &entry) {
            const uintptr_t p = reinterpret_cast<uintptr_t>(entry.obj);
            if (p < objectsBase || p >= poolEnd) {
              return true;
            }
            const uintptr_t offset = p - objectsBase;
            if (offset % OBJECT_POOL_STRIDE != 0) {
              return true;
            }
            const int32_t idx =
                static_cast<int32_t>(offset / OBJECT_POOL_STRIDE);
            if (idx < 0 || idx >= poolSize || (byteMap[idx] & 0x80) != 0) {
              return true;
            }
            if (!IsEntityType(entry.obj, ENTITY_TYPE_OBJECT)) {
              return true;
            }
            if (FieldAt<int16_t>(entry.obj, OFF_ENTITY_MODEL_INDEX) !=
                entry.modelIndex) {
              return true;
            }
            // If the pole re-streamed upright (static again), allow future
            // knockdown
            if (IsEntityStatic(entry.obj)) {
              return true;
            }
            return false;
          }),
      g_countedPoles.end());
}

bool __fastcall Hook_ApplyCollision(void *self, uintptr_t, void *entityB,
                                    void *colPoint, float *impulseA,
                                    float *impulseB) {
  if (!self || !entityB) {
    return g_origApplyCollision(self, entityB, colPoint, impulseA, impulseB);
  }

  void *pole = nullptr;
  void *hitter = nullptr;
  int16_t poleModel = 0;

  if (IsUprightTargetPole(entityB, &poleModel)) {
    pole = entityB;
    hitter = self;
  } else if (IsUprightTargetPole(self, &poleModel)) {
    pole = self;
    hitter = entityB;
  } else if (IsEntityType(self, ENTITY_TYPE_VEHICLE) &&
             IsEntityType(entityB, ENTITY_TYPE_VEHICLE)) {
    auto findPlayerVeh =
        reinterpret_cast<FindPlayerVehicleFn>(ADDR_FIND_PLAYER_VEHICLE);
    void *playerVeh = findPlayerVeh();
    if (playerVeh) {
      if (self == playerVeh) {
        RecordBumpedVehicle(entityB, TimeMs());
      } else if (entityB == playerVeh) {
        RecordBumpedVehicle(self, TimeMs());
      }
    }
  }

  const bool result =
      g_origApplyCollision(self, entityB, colPoint, impulseA, impulseB);

  if (pole && !IsEntityStatic(pole) &&
      *ScriptGlobalPtr(SLOT_POLE_ENABLED) != POLE_MODE_DISABLED) {
    auto findPlayerPed =
        reinterpret_cast<FindPlayerPedFn>(ADDR_FIND_PLAYER_PED);
    void *playerPed = findPlayerPed();
    if (playerPed) {
      auto findPlayerVeh =
          reinterpret_cast<FindPlayerVehicleFn>(ADDR_FIND_PLAYER_VEHICLE);
      void *playerVeh = findPlayerVeh();
      const uint32_t nowMs = TimeMs();
      const bool byPlayer = (playerVeh && hitter == playerVeh) ||
                            (hitter == playerPed) ||
                            (IsEntityType(hitter, ENTITY_TYPE_VEHICLE) &&
                             WasRecentlyBumpedByPlayer(hitter, nowMs));

      if (byPlayer && !IsPoleAlreadyCounted(pole)) {
        g_countedPoles.push_back({pole, poleModel});
        *ScriptGlobalPtr(SLOT_POLE_TOTAL) += 1;
        *ScriptGlobalPtr(SLOT_POLE_LAST_MODEL) =
            static_cast<int32_t>(poleModel);
      }
    }
  }

  return result;
}

void __cdecl Hook_ScriptsProcess() {
  g_origScriptsProcess();
  PruneTrackedState();
}

void InstallDeferredHooks() {
  if (g_deferredInstalled) {
    return;
  }
  g_deferredInstalled = true;

  const bool okCol = InstallDetour(ADDR_PHYSICAL_APPLY_COLLISION,
                                   &Hook_ApplyCollision, &g_origApplyCollision);
  const bool okScr = InstallDetour(ADDR_SCRIPTS_PROCESS, &Hook_ScriptsProcess,
                                   &g_origScriptsProcess);

  WriteLog(okCol ? "ApplyCollision hook (0x4973A0) OK"
                 : "ApplyCollision hook (0x4973A0) FAILED");
  WriteLog(okScr ? "ScriptsProcess hook (0x439040) OK"
                 : "ScriptsProcess hook (0x439040) FAILED");
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
