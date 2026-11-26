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

constexpr uintptr_t ADDR_HOOK_DRAW_NORTH_CALL =
    0x004A4460; // call CRadar::DrawRadarSprite for NORTH (0x0E)
constexpr uintptr_t ADDR_DRAW_RADAR_SPRITE = 0x004A5EF0;

constexpr uintptr_t ADDR_TRANSFORM_REAL_TO_RADAR = 0x004A50D0;
constexpr uintptr_t ADDR_LIMIT_RADAR_POINT = 0x004A4F30;
constexpr uintptr_t ADDR_CALCULATE_BLIP_ALPHA = 0x004A4F90;
constexpr uintptr_t ADDR_TRANSFORM_RADAR_TO_SCREEN = 0x004A5040;

constexpr uintptr_t ADDR_SPRITE_SET_TEXTURE = 0x0051EA40;
constexpr uintptr_t ADDR_SPRITE_SET_ADDRESSING = 0x0051EAA0;
constexpr uintptr_t ADDR_SPRITE_DRAW = 0x0051ED50;

constexpr uintptr_t ADDR_TXD_ADD_SLOT = 0x005274E0;
constexpr uintptr_t ADDR_TXD_FIND_SLOT = 0x005275D0;
constexpr uintptr_t ADDR_TXD_LOAD = 0x005276B0;
constexpr uintptr_t ADDR_TXD_SET_CURRENT = 0x005278C0;
constexpr uintptr_t ADDR_TXD_PUSH_CURRENT = 0x00527900;
constexpr uintptr_t ADDR_TXD_POP_CURRENT = 0x00527910;
constexpr uintptr_t ADDR_TXD_ADD_REF = 0x00527930;

constexpr uintptr_t ADDR_SCRIPT_SPACE = 0x0074B248; // CTheScripts::ScriptSpace
constexpr uintptr_t ADDR_RS_MAX_WIDTH = 0x008F436C;
constexpr uintptr_t ADDR_RS_MAX_HEIGHT = 0x008F4370;

constexpr size_t VAR_IDX_BARRIER_IND = 5012;      // $BarrierIND
constexpr size_t VAR_IDX_SPRAYSHOP_ACTIVE = 5028; // $SprayShop_Active
constexpr size_t VAR_IDX_BARRIER_COM = 5038;      // $BarrierCOM

constexpr uint8_t EXPECTED_PATCH_BYTES[5] = {0xE9, 0xC2, 0x01, 0x00, 0x00};
constexpr uint8_t EXPECTED_NORTH_CALL_BYTES[5] = {0xE8, 0x8B, 0x1A, 0x00, 0x00};

// RadarSprite enum IDs (GTA III / Sanny Builder)
constexpr uint16_t MAX_RADAR_SPRITE_ID = 32;
constexpr uint16_t SPRITE_BOMB = 2;
constexpr uint16_t SPRITE_SPRAY = 18;
constexpr uint16_t SPRITE_WEAPON = 20;

struct CVector2D {
  float x;
  float y;
};

struct CRect {
  float left;
  float bottom;
  float right;
  float top;
};

struct CRGBA {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
};

struct CSprite2d {
  void *m_pTexture = nullptr;
};

struct SprayShopDef {
  CVector2D pos;
  size_t gateVarIndex; // 0 if always open, else script global == 0 required
  bool checkHeatDuplicate;
};

using DrawRadarSpriteFn = void(__cdecl *)(uint32_t spriteId, float x, float y,
                                          uint32_t alpha);
using TransformVecFn = void(__cdecl *)(CVector2D *out, const CVector2D *in);
using LimitRadarPointFn = float(__cdecl *)(CVector2D *point);
using CalculateBlipAlphaFn = uint8_t(__cdecl *)(float dist);

using SpriteSetTexFn = void(__thiscall *)(CSprite2d *self, const char *name);
using SpriteSetAddrFn = void(__thiscall *)(CSprite2d *self, int32_t addr);
using SpriteDrawFn = void(__thiscall *)(CSprite2d *self, const CRect *rect,
                                        const CRGBA *col);

using TxdAddSlotFn = int32_t(__cdecl *)(const char *name);
using TxdFindSlotFn = int32_t(__cdecl *)(const char *name);
using TxdLoadFn = uint8_t(__cdecl *)(int32_t slot, const char *path);
using TxdSetCurrentFn = void(__cdecl *)(int32_t slot);
using TxdPushPopFn = void(__cdecl *)();
using TxdAddRefFn = int32_t(__cdecl *)(int32_t slot);

// 8 Portland bus stop sign locations (world XY matching IPL placements)
constexpr CVector2D kPortlandBusStops[] = {
    {1038.25f, -118.00f},  // Harwood - Fire Station / Head Radio
    {1202.75f, -68.00f},   // Harwood - Car Showroom & Junkyard
    {946.00f, -481.25f},   // Red Light District - Subway Station
    {1024.46f, -484.75f},  // Chinatown / Subway Border
    {881.80f, -1011.60f},  // Callahan Point - Greasy Joe's Diner curb
    {1106.50f, -1107.25f}, // Atlantic Quays - Callahan Bridge Side (Vincenzo's)
    {1274.50f, -954.00f},  // Trenton - Portland Bus Depot
    {1133.25f, -606.00f},  // Portland View - Hospital & Police Station
};

// Pay 'n' Spray locations across Liberty City
constexpr SprayShopDef kSprayShops[] = {
    {{925.00f, -361.00f}, 0, true}, // Portland - Red Light District
    {{380.90f, -493.70f}, VAR_IDX_BARRIER_IND, false}, // Staunton - Newport
    {{-1142.00f, 34.70f}, VAR_IDX_BARRIER_COM, false}, // Shoreside - Pike Creek
};

// Short-range threshold in normalized radar space (LimitRadarPoint clamps at
// > 1.0f)
float g_maxRadarDist = 1.0f;
bool g_showSprites[MAX_RADAR_SPRITE_ID] = {};
bool g_shortRangeSprites[MAX_RADAR_SPRITE_ID] = {};
bool g_drawBusStops = true;
bool g_busShortRange = true;
bool g_drawSprayShops = true;
bool g_sprayShortRange = true;

bool g_patched = false;
uint8_t g_origBytes[5] = {};
bool g_northPatched = false;
uint8_t g_origNorthBytes[5] = {};

char g_iniPath[MAX_PATH] = {};
char g_txdPath[MAX_PATH] = {};

CSprite2d g_busSprite{};
bool g_busLoadAttempted = false;

void InitDefaultsAndConfig(HMODULE hModule) {
  for (bool &show : g_showSprites) {
    show = true;
  }
  g_shortRangeSprites[SPRITE_BOMB] = true;
  g_shortRangeSprites[SPRITE_WEAPON] = true;
  g_shortRangeSprites[SPRITE_SPRAY] = true;
  g_drawBusStops = true;
  g_busShortRange = true;
  g_drawSprayShops = true;
  g_sprayShortRange = true;

  char moduleDir[MAX_PATH] = {};
  if (GetModuleFileNameA(hModule, moduleDir, sizeof(moduleDir)) == 0) {
    return;
  }
  char *slash = nullptr;
  for (char *p = moduleDir; *p != '\0'; ++p) {
    if (*p == '\\' || *p == '/') {
      slash = p;
    }
  }
  if (slash != nullptr) {
    *(slash + 1) = '\0';
    std::snprintf(g_iniPath, sizeof(g_iniPath), "%sShortRangeBlips.ini",
                  moduleDir);
    std::snprintf(g_txdPath, sizeof(g_txdPath), "%seventide_busblip.txd",
                  moduleDir);
  }

  // Optional ShortRangeBlips.ini overrides (plugin works with defaults if
  // absent)
  if (g_iniPath[0] == '\0' ||
      GetFileAttributesA(g_iniPath) == INVALID_FILE_ATTRIBUTES) {
    return;
  }

  // [Display] section (1 = visible, 0 = completely hidden)
  g_drawBusStops =
      GetPrivateProfileIntA("Display", "ShowBusStops", 1, g_iniPath) != 0;
  g_showSprites[SPRITE_WEAPON] =
      GetPrivateProfileIntA("Display", "ShowWeapons", 1, g_iniPath) != 0;
  g_drawSprayShops =
      GetPrivateProfileIntA("Display", "ShowSprayShops", 1, g_iniPath) != 0;
  g_showSprites[SPRITE_SPRAY] = g_drawSprayShops;
  g_showSprites[SPRITE_BOMB] =
      GetPrivateProfileIntA("Display", "ShowBombs", 1, g_iniPath) != 0;

  // [ShortRange] section (1 = short-range within MaxRadarDist, 0 = long-range)
  g_busShortRange =
      GetPrivateProfileIntA("ShortRange", "BusStops", 1, g_iniPath) != 0;
  g_shortRangeSprites[SPRITE_WEAPON] =
      GetPrivateProfileIntA("ShortRange", "Weapons", 1, g_iniPath) != 0;
  g_sprayShortRange =
      GetPrivateProfileIntA("ShortRange", "SprayShops", 1, g_iniPath) != 0;
  g_shortRangeSprites[SPRITE_SPRAY] = g_sprayShortRange;
  g_shortRangeSprites[SPRITE_BOMB] =
      GetPrivateProfileIntA("ShortRange", "Bombs", 1, g_iniPath) != 0;

  // Legacy [Sprites] overrides if explicitly specified in custom user INIs
  if (GetPrivateProfileIntA("Sprites", "Bus", -1, g_iniPath) != -1) {
    g_drawBusStops = GetPrivateProfileIntA("Sprites", "Bus", 1, g_iniPath) != 0;
  }
  if (GetPrivateProfileIntA("Sprites", "SprayShops", -1, g_iniPath) != -1) {
    g_drawSprayShops =
        GetPrivateProfileIntA("Sprites", "SprayShops", 1, g_iniPath) != 0;
  }
  if (GetPrivateProfileIntA("Sprites", "BusShortRange", -1, g_iniPath) != -1) {
    g_busShortRange =
        GetPrivateProfileIntA("Sprites", "BusShortRange", 1, g_iniPath) != 0;
  }
  if (GetPrivateProfileIntA("Sprites", "SprayShortRange", -1, g_iniPath) !=
      -1) {
    g_sprayShortRange =
        GetPrivateProfileIntA("Sprites", "SprayShortRange", 1, g_iniPath) != 0;
  }
  if (GetPrivateProfileIntA("Sprites", "Bomb", -1, g_iniPath) != -1) {
    g_shortRangeSprites[SPRITE_BOMB] =
        GetPrivateProfileIntA("Sprites", "Bomb", 1, g_iniPath) != 0;
  }
  if (GetPrivateProfileIntA("Sprites", "Spray", -1, g_iniPath) != -1) {
    g_shortRangeSprites[SPRITE_SPRAY] =
        GetPrivateProfileIntA("Sprites", "Spray", 1, g_iniPath) != 0;
  }
  if (GetPrivateProfileIntA("Sprites", "Weapon", -1, g_iniPath) != -1) {
    g_shortRangeSprites[SPRITE_WEAPON] =
        GetPrivateProfileIntA("Sprites", "Weapon", 1, g_iniPath) != 0;
  }

  char distBuf[32] = {};
  GetPrivateProfileStringA("General", "MaxRadarDist", "1.0", distBuf,
                           sizeof(distBuf), g_iniPath);
  const float parsed = static_cast<float>(std::atof(distBuf));
  if (parsed > 0.05f && parsed <= 10.0f) {
    g_maxRadarDist = parsed;
  }
}

void EnsureBusSpriteLoaded() {
  if (g_busSprite.m_pTexture != nullptr || g_busLoadAttempted) {
    return;
  }
  g_busLoadAttempted = true;

  if (g_txdPath[0] == '\0' ||
      GetFileAttributesA(g_txdPath) == INVALID_FILE_ATTRIBUTES) {
    return;
  }

  const auto findSlot = reinterpret_cast<TxdFindSlotFn>(ADDR_TXD_FIND_SLOT);
  const auto addSlot = reinterpret_cast<TxdAddSlotFn>(ADDR_TXD_ADD_SLOT);
  const auto loadTxd = reinterpret_cast<TxdLoadFn>(ADDR_TXD_LOAD);
  const auto addRef = reinterpret_cast<TxdAddRefFn>(ADDR_TXD_ADD_REF);
  const auto pushCurrent =
      reinterpret_cast<TxdPushPopFn>(ADDR_TXD_PUSH_CURRENT);
  const auto popCurrent = reinterpret_cast<TxdPushPopFn>(ADDR_TXD_POP_CURRENT);
  const auto setCurrent =
      reinterpret_cast<TxdSetCurrentFn>(ADDR_TXD_SET_CURRENT);
  const auto setTex = reinterpret_cast<SpriteSetTexFn>(ADDR_SPRITE_SET_TEXTURE);
  const auto setAddr =
      reinterpret_cast<SpriteSetAddrFn>(ADDR_SPRITE_SET_ADDRESSING);

  int32_t slot = findSlot("evbus");
  if (slot == -1) {
    slot = addSlot("evbus");
  }
  if (slot != -1 && loadTxd(slot, g_txdPath) != 0) {
    addRef(slot);
    pushCurrent();
    setCurrent(slot);
    setTex(&g_busSprite, "busblip");
    popCurrent();
  }

  if (g_busSprite.m_pTexture != nullptr) {
    setAddr(&g_busSprite, 3); // rwTEXTUREADDRESSCLAMP
  }
}

void DrawBusStopBlips() {
  if (!g_drawBusStops) {
    return;
  }
  EnsureBusSpriteLoaded();
  if (g_busSprite.m_pTexture == nullptr) {
    return;
  }

  const int32_t maxW = *reinterpret_cast<const int32_t *>(ADDR_RS_MAX_WIDTH);
  const int32_t maxH = *reinterpret_cast<const int32_t *>(ADDR_RS_MAX_HEIGHT);
  if (maxW <= 0 || maxH <= 0) {
    return;
  }

  const float halfW = 8.0f * (static_cast<float>(maxW) / 640.0f);
  const float halfH = 8.0f * (static_cast<float>(maxH) / 448.0f);

  const auto realToRadar =
      reinterpret_cast<TransformVecFn>(ADDR_TRANSFORM_REAL_TO_RADAR);
  const auto limitPoint =
      reinterpret_cast<LimitRadarPointFn>(ADDR_LIMIT_RADAR_POINT);
  const auto calcAlpha =
      reinterpret_cast<CalculateBlipAlphaFn>(ADDR_CALCULATE_BLIP_ALPHA);
  const auto radarToScreen =
      reinterpret_cast<TransformVecFn>(ADDR_TRANSFORM_RADAR_TO_SCREEN);
  const auto spriteDraw = reinterpret_cast<SpriteDrawFn>(ADDR_SPRITE_DRAW);

  for (const CVector2D &worldPos : kPortlandBusStops) {
    CVector2D radarPos{};
    realToRadar(&radarPos, &worldPos);
    const float dist = limitPoint(&radarPos);
    if (g_busShortRange && dist > g_maxRadarDist) {
      continue;
    }
    const uint8_t alpha = calcAlpha(dist);
    if (alpha == 0) {
      continue;
    }

    CVector2D screenPos{};
    radarToScreen(&screenPos, &radarPos);

    const CRect rect{
        screenPos.x - halfW,
        screenPos.y + halfH,
        screenPos.x + halfW,
        screenPos.y - halfH,
    };
    const CRGBA color{255, 255, 255, alpha};
    spriteDraw(&g_busSprite, &rect, &color);
  }
}

void DrawSprayShopBlips() {
  if (!g_drawSprayShops) {
    return;
  }

  const auto *scriptSpace =
      reinterpret_cast<const int32_t *>(ADDR_SCRIPT_SPACE);
  const auto realToRadar =
      reinterpret_cast<TransformVecFn>(ADDR_TRANSFORM_REAL_TO_RADAR);
  const auto limitPoint =
      reinterpret_cast<LimitRadarPointFn>(ADDR_LIMIT_RADAR_POINT);
  const auto calcAlpha =
      reinterpret_cast<CalculateBlipAlphaFn>(ADDR_CALCULATE_BLIP_ALPHA);
  const auto radarToScreen =
      reinterpret_cast<TransformVecFn>(ADDR_TRANSFORM_RADAR_TO_SCREEN);
  const auto drawRadarSprite =
      reinterpret_cast<DrawRadarSpriteFn>(ADDR_DRAW_RADAR_SPRITE);

  for (const SprayShopDef &shop : kSprayShops) {
    if (shop.gateVarIndex != 0 && scriptSpace[shop.gateVarIndex] != 0) {
      continue;
    }
    if (shop.checkHeatDuplicate && scriptSpace[VAR_IDX_SPRAYSHOP_ACTIVE] != 0) {
      continue;
    }

    CVector2D radarPos{};
    realToRadar(&radarPos, &shop.pos);
    const float dist = limitPoint(&radarPos);
    if (g_sprayShortRange && dist > g_maxRadarDist) {
      continue;
    }
    const uint8_t alpha = calcAlpha(dist);
    if (alpha == 0) {
      continue;
    }

    CVector2D screenPos{};
    radarToScreen(&screenPos, &radarPos);
    drawRadarSprite(SPRITE_SPRAY, screenPos.x, screenPos.y, alpha);
  }
}

void __cdecl Hook_DrawNorthAndBusStops(uint32_t spriteId, float x, float y,
                                       uint32_t alpha) {
  DrawBusStopBlips();
  DrawSprayShopBlips();
  const auto origDrawRadarSprite =
      reinterpret_cast<DrawRadarSpriteFn>(ADDR_DRAW_RADAR_SPRITE);
  origDrawRadarSprite(spriteId, x, y, alpha);
}

bool __stdcall ShouldSkipBlip(uint32_t sprite, float dist) {
  if (sprite >= MAX_RADAR_SPRITE_ID) {
    return false;
  }
  if (!g_showSprites[sprite]) {
    return true;
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

bool PatchCallOrJump5(uintptr_t addr, const uint8_t expected[5], uint8_t opcode,
                      uintptr_t destAddr, uint8_t origOut[5]) {
  auto *target = reinterpret_cast<uint8_t *>(addr);
  for (size_t i = 0; i < 5; ++i) {
    if (target[i] != expected[i]) {
      return false;
    }
  }

  DWORD oldProtect = 0;
  if (!VirtualProtect(target, 5, PAGE_EXECUTE_READWRITE, &oldProtect)) {
    return false;
  }

  for (size_t i = 0; i < 5; ++i) {
    origOut[i] = target[i];
  }

  const int32_t rel = static_cast<int32_t>(destAddr - (addr + 5));
  target[0] = opcode;
  *reinterpret_cast<int32_t *>(target + 1) = rel;

  VirtualProtect(target, 5, oldProtect, &oldProtect);
  FlushInstructionCache(GetCurrentProcess(), target, 5);
  return true;
}

void RestoreBytes5(uintptr_t addr, const uint8_t origBytes[5]) {
  auto *target = reinterpret_cast<uint8_t *>(addr);
  DWORD oldProtect = 0;
  if (VirtualProtect(target, 5, PAGE_EXECUTE_READWRITE, &oldProtect)) {
    for (size_t i = 0; i < 5; ++i) {
      target[i] = origBytes[i];
    }
    VirtualProtect(target, 5, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), target, 5);
  }
}

void InstallHooks() {
  g_patched = PatchCallOrJump5(
      ADDR_HOOK_COORD_SPRITE_JMP, EXPECTED_PATCH_BYTES, 0xE9,
      reinterpret_cast<uintptr_t>(&Hook_CoordSpriteDraw), g_origBytes);
  g_northPatched = PatchCallOrJump5(
      ADDR_HOOK_DRAW_NORTH_CALL, EXPECTED_NORTH_CALL_BYTES, 0xE8,
      reinterpret_cast<uintptr_t>(&Hook_DrawNorthAndBusStops),
      g_origNorthBytes);
}

void RemoveHooks() {
  if (g_patched) {
    RestoreBytes5(ADDR_HOOK_COORD_SPRITE_JMP, g_origBytes);
    g_patched = false;
  }
  if (g_northPatched) {
    RestoreBytes5(ADDR_HOOK_DRAW_NORTH_CALL, g_origNorthBytes);
    g_northPatched = false;
  }
}

} // namespace

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(hModule);
    InitDefaultsAndConfig(hModule);
    InstallHooks();
  } else if (reason == DLL_PROCESS_DETACH) {
    if (lpReserved == nullptr) {
      RemoveHooks();
    }
  }
  return TRUE;
}
