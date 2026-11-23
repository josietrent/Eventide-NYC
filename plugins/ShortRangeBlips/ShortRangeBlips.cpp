#define WIN32_LEAN_AND_MEAN
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <windows.h>

namespace {

// GTA III v1.0 addresses (verified against gta3.exe)
constexpr uintptr_t ADDR_HOOK_COORD_SPRITE_JMP = 0x004A4614; // jmp 0x004A47DB
constexpr uintptr_t ADDR_DRAW_RADAR_SPRITE_IN =
    0x004A47DB; // push Y; push X; push eax; call CRadar::DrawRadarSprite
constexpr uintptr_t ADDR_LOOP_NEXT_BLIP = 0x004A47EC; // inc dword ptr [esp+1Ch]

constexpr uint8_t EXPECTED_PATCH_BYTES[5] = {0xE9, 0xC2, 0x01, 0x00, 0x00};

// RadarSprite enum IDs (GTA III / Sanny Builder)
constexpr uint16_t MAX_RADAR_SPRITE_ID = 32;
constexpr uint16_t SPRITE_BOMB = 2;
constexpr uint16_t SPRITE_SPRAY = 18;
constexpr uint16_t SPRITE_WEAPON = 20;

// Short-range threshold in normalized radar space (LimitRadarPoint clamps at
// > 1.0f)
float g_maxRadarDist = 1.0f;
bool g_shortRangeSprites[MAX_RADAR_SPRITE_ID] = {};
bool g_patched = false;
uint8_t g_origBytes[5] = {};
char g_iniPath[MAX_PATH] = {};

void InitDefaultsAndConfig(HMODULE hModule) {
  g_shortRangeSprites[SPRITE_BOMB] = true;
  g_shortRangeSprites[SPRITE_WEAPON] = true;
  g_shortRangeSprites[SPRITE_SPRAY] = false;

  if (GetModuleFileNameA(hModule, g_iniPath, sizeof(g_iniPath)) == 0) {
    return;
  }
  char *slash = nullptr;
  for (char *p = g_iniPath; *p != '\0'; ++p) {
    if (*p == '\\' || *p == '/') {
      slash = p;
    }
  }
  if (slash != nullptr) {
    *(slash + 1) = '\0';
    std::snprintf(slash + 1,
                  sizeof(g_iniPath) -
                      static_cast<size_t>((slash + 1) - g_iniPath),
                  "ShortRangeBlips.ini");
  }

  // Optional ShortRangeBlips.ini overrides (plugin works with defaults if
  // absent)
  if (GetFileAttributesA(g_iniPath) == INVALID_FILE_ATTRIBUTES) {
    return;
  }

  g_shortRangeSprites[SPRITE_BOMB] =
      GetPrivateProfileIntA("Sprites", "Bomb", 1, g_iniPath) != 0;
  g_shortRangeSprites[SPRITE_SPRAY] =
      GetPrivateProfileIntA("Sprites", "Spray", 0, g_iniPath) != 0;
  g_shortRangeSprites[SPRITE_WEAPON] =
      GetPrivateProfileIntA("Sprites", "Weapon", 1, g_iniPath) != 0;

  char distBuf[32] = {};
  GetPrivateProfileStringA("General", "MaxRadarDist", "1.0", distBuf,
                           sizeof(distBuf), g_iniPath);
  const float parsed = static_cast<float>(std::atof(distBuf));
  if (parsed > 0.05f && parsed <= 10.0f) {
    g_maxRadarDist = parsed;
  }
}

bool __stdcall ShouldSkipBlip(uint32_t sprite, float dist) {
  if (sprite >= MAX_RADAR_SPRITE_ID) {
    return false;
  }
  if (!g_shortRangeSprites[sprite]) {
    return false;
  }
  return dist > g_maxRadarDist;
}

__declspec(naked) void Hook_CoordSpriteDraw() {
  __asm {
    pushfd
    push eax
    push ecx
    push edx
    movzx eax, ax
    push dword ptr [esp + 0x3C] // dist stored at [esp+28h] before 0x4A4610 alpha
                             // push (+4) and 4 saved regs (+16)
    push eax
    call ShouldSkipBlip
    test al, al
    pop edx
    pop ecx
    pop eax
    jnz skip_blip
    popfd
    push ADDR_DRAW_RADAR_SPRITE_IN
    ret
  skip_blip:
    popfd
    add esp, 4                       // discard alpha pushed at 0x4A4610
    push ADDR_LOOP_NEXT_BLIP
    ret
  }
}

bool InstallHook() {
  auto *target = reinterpret_cast<uint8_t *>(ADDR_HOOK_COORD_SPRITE_JMP);
  for (size_t i = 0; i < sizeof(EXPECTED_PATCH_BYTES); ++i) {
    if (target[i] != EXPECTED_PATCH_BYTES[i]) {
      return false;
    }
  }

  DWORD oldProtect = 0;
  if (!VirtualProtect(target, sizeof(EXPECTED_PATCH_BYTES),
                      PAGE_EXECUTE_READWRITE, &oldProtect)) {
    return false;
  }

  for (size_t i = 0; i < sizeof(g_origBytes); ++i) {
    g_origBytes[i] = target[i];
  }

  const uintptr_t hookAddr = reinterpret_cast<uintptr_t>(&Hook_CoordSpriteDraw);
  const int32_t rel =
      static_cast<int32_t>(hookAddr - (ADDR_HOOK_COORD_SPRITE_JMP + 5));
  target[0] = 0xE9;
  *reinterpret_cast<int32_t *>(target + 1) = rel;

  VirtualProtect(target, sizeof(EXPECTED_PATCH_BYTES), oldProtect, &oldProtect);
  FlushInstructionCache(GetCurrentProcess(), target,
                        sizeof(EXPECTED_PATCH_BYTES));
  g_patched = true;
  return true;
}

void RemoveHook() {
  if (!g_patched) {
    return;
  }
  auto *target = reinterpret_cast<uint8_t *>(ADDR_HOOK_COORD_SPRITE_JMP);
  DWORD oldProtect = 0;
  if (VirtualProtect(target, sizeof(g_origBytes), PAGE_EXECUTE_READWRITE,
                     &oldProtect)) {
    for (size_t i = 0; i < sizeof(g_origBytes); ++i) {
      target[i] = g_origBytes[i];
    }
    VirtualProtect(target, sizeof(g_origBytes), oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), target, sizeof(g_origBytes));
  }
  g_patched = false;
}

} // namespace

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(hModule);
    InitDefaultsAndConfig(hModule);
    InstallHook();
  } else if (reason == DLL_PROCESS_DETACH) {
    if (lpReserved == nullptr) {
      RemoveHook();
    }
  }
  return TRUE;
}
