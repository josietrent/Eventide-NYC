#define WIN32_LEAN_AND_MEAN
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <windows.h>

namespace {

// GTA III v1.0 globals and addresses (verified against gta3.exe)
constexpr uintptr_t ADDR_NASTY_GAME = 0x5F4DD4; // CGame::nastyGame (uint8_t)
constexpr uintptr_t ADDR_MI_MONEY = 0x5F5A08;   // MI_MONEY (uint16_t)

// GTA III v1.0 functions
constexpr uintptr_t ADDR_PICKUPS_GENERATE_NEW_ONE = 0x4304B0;
constexpr uintptr_t ADDR_PED_CREATE_DEAD_PED_MONEY = 0x433490;
constexpr uintptr_t ADDR_WORLD_FIND_GROUND_Z = 0x4B3AE0;
constexpr uintptr_t ADDR_MUSIC_INITIALISE = 0x57CF70;
constexpr uintptr_t ADDR_GAME_RAND = 0x5A41D0;

// CEntity / CPed offsets (verified against gta3.exe v1.0)
constexpr size_t OFF_ENTITY_POS = 0x34;         // float[3]
constexpr size_t OFF_ENTITY_MODEL_INDEX = 0x5C; // int16_t
constexpr size_t OFF_PED_CREATED_BY = 0x160;    // uint8_t
constexpr size_t OFF_PED_IN_VEHICLE = 0x314;    // uint8_t

// Game constants
constexpr int16_t MI_MEDIC = 5;
constexpr uint8_t MISSION_CHAR = 2;
constexpr uint8_t PICKUP_MONEY = 7;
constexpr uint16_t INVALID_MODEL_INDEX = 0xFFFF;

// Paramedic cash drop tuning:
// - 100% drop rate on foot (skips MISSION_CHAR & bInVehicle like vanilla)
// - Higher payout range: $60-$150 base across 2-4 pickups (~$60-$175 with
// pickup jitter)
// - Higher jackpot chance: 1 in 12 (~8.3% vs vanilla civilian 1.67%) -> $750
// base (~$750-$875 total)
constexpr int PARAMEDIC_CASH_MIN = 60;
constexpr int PARAMEDIC_CASH_MAX = 150;
constexpr int PARAMEDIC_JACKPOT_MOD = 12; // 1 in 12
constexpr int PARAMEDIC_JACKPOT_CASH = 750;
constexpr float PICKUP_SPAWN_RADIUS = 1.5f;
constexpr float PICKUP_Z_OFFSET = 0.5f;
constexpr float TWO_PI = 6.283185307f;

struct Vec3 {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};

using MusicBoolFn = bool(__thiscall *)(void *self);
using CreateDeadPedMoneyFn = void(__thiscall *)(void *self);
using GenerateNewOneFn = int32_t(__cdecl *)(float x, float y, float z,
                                            uint32_t modelIndex, uint32_t type,
                                            uint32_t ammoOrMoney);
using FindGroundZFn = float(__cdecl *)(float x, float y, float z,
                                       bool *outFound);
using GameRandFn = int(__cdecl *)();

struct HookEntry {
  uintptr_t addr = 0;
  uint8_t origBytes[16] = {};
  size_t patchLen = 0;
  uint8_t *trampoline = nullptr;
  bool active = false;
};

std::string g_logPath;
std::vector<HookEntry> g_hooks;
bool g_deferredInstalled = false;

MusicBoolFn g_origInitialise = nullptr;
CreateDeadPedMoneyFn g_origCreateDeadPedMoney = nullptr;

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
      g_logPath = full.substr(0, pos) + "\\ParamedicCash.log";
    } else {
      g_logPath = "ParamedicCash.log";
    }
  } else {
    g_logPath = "ParamedicCash.log";
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

inline int GameRand() { return reinterpret_cast<GameRandFn>(ADDR_GAME_RAND)(); }

void SpawnParamedicCash(const void *ped) {
  if (*reinterpret_cast<const uint8_t *>(ADDR_NASTY_GAME) == 0) {
    return;
  }
  if (FieldAt<uint8_t>(ped, OFF_PED_CREATED_BY) == MISSION_CHAR) {
    return;
  }
  if (FieldAt<uint8_t>(ped, OFF_PED_IN_VEHICLE) != 0) {
    return;
  }

  const uint16_t moneyModel =
      *reinterpret_cast<const uint16_t *>(ADDR_MI_MONEY);
  if (moneyModel == INVALID_MODEL_INDEX) {
    return;
  }

  const bool isJackpot = (GameRand() % PARAMEDIC_JACKPOT_MOD) == 0;
  const int totalBase =
      isJackpot
          ? PARAMEDIC_JACKPOT_CASH
          : (PARAMEDIC_CASH_MIN +
             (GameRand() % (PARAMEDIC_CASH_MAX - PARAMEDIC_CASH_MIN + 1)));

  const int numPickups = totalBase / 40 + 1;
  const int perPickup = totalBase / numPickups;

  const float *pos = &FieldAt<float>(ped, OFF_ENTITY_POS);
  auto findGroundZ = reinterpret_cast<FindGroundZFn>(ADDR_WORLD_FIND_GROUND_Z);
  auto generateNewOne =
      reinterpret_cast<GenerateNewOneFn>(ADDR_PICKUPS_GENERATE_NEW_ONE);

  for (int i = 0; i < numPickups; ++i) {
    const float angle =
        (static_cast<float>(GameRand() & 0xFF) / 256.0f) * TWO_PI;
    const float px = pos[0] + PICKUP_SPAWN_RADIUS * std::sin(angle);
    const float py = pos[1] + PICKUP_SPAWN_RADIUS * std::cos(angle);
    bool found = false;
    float gz = findGroundZ(px, py, pos[2], &found);
    if (!found) {
      continue;
    }
    gz += PICKUP_Z_OFFSET;
    const uint32_t amount = static_cast<uint32_t>(perPickup + (GameRand() & 7));
    generateNewOne(px, py, gz, static_cast<uint32_t>(moneyModel), PICKUP_MONEY,
                   amount);
  }
}

void __fastcall Hook_CreateDeadPedMoney(void *self, uintptr_t) {
  if (!self) {
    return;
  }
  const int16_t mi = FieldAt<int16_t>(self, OFF_ENTITY_MODEL_INDEX);
  if (mi == MI_MEDIC) {
    SpawnParamedicCash(self);
    return;
  }
  g_origCreateDeadPedMoney(self);
}

void InstallDeferredHooks() {
  if (g_deferredInstalled) {
    return;
  }
  g_deferredInstalled = true;

  const bool ok =
      InstallDetour(ADDR_PED_CREATE_DEAD_PED_MONEY, &Hook_CreateDeadPedMoney,
                    &g_origCreateDeadPedMoney);
  WriteLog(ok ? "ParamedicCash hook (CPed::CreateDeadPedMoney @ 0x433490) OK"
              : "ParamedicCash hook FAILED");
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
