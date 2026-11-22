#define WIN32_LEAN_AND_MEAN
#include <cstdio>
#include <cstring>
#include <stdint.h>
#include <string>
#include <windows.h>

namespace {

// GTA III v1.0 addresses
constexpr uintptr_t ADDR_CALL_LOAD_MENU_SPRITE_TEX =
    0x47A406; // inside CMenuManager::LoadAllTextures() ->
              // CSprite2d::SetTexture(name, mask)
constexpr uintptr_t ADDR_CALL_UNLOAD_MENU_TXD =
    0x47A4BC; // inside CMenuManager::UnloadTextures() -> CTxdStore::RemoveTxd()
constexpr uintptr_t ADDR_CALL_DRAW_STD_MENUS =
    0x47AB12; // inside CMenuManager::DrawBackground() ->
              // CMenuManager::DrawStandardMenus()

constexpr uintptr_t ADDR_FN_REMOVE_TXD = 0x527870;
constexpr uintptr_t ADDR_FN_DRAW_STD_MENUS = 0x47AE00;
constexpr uintptr_t ADDR_FN_FADE_IN_ALPHA = 0x48AC60;

constexpr uintptr_t ADDR_RS_MAX_WIDTH = 0x8F436C;  // RsGlobal.maximumWidth
constexpr uintptr_t ADDR_RS_MAX_HEIGHT = 0x8F4370; // RsGlobal.maximumHeight

constexpr uintptr_t ADDR_SPRITE_SET_TEXTURE = 0x51EA40;
constexpr uintptr_t ADDR_SPRITE_SET_TEXTURE_MASK = 0x51EA70;
constexpr uintptr_t ADDR_SPRITE_SET_ADDRESSING = 0x51EAA0;
constexpr uintptr_t ADDR_SPRITE_DELETE = 0x51EA00;
constexpr uintptr_t ADDR_SPRITE_DRAW = 0x51ED50;

constexpr uintptr_t ADDR_TXD_ADD_SLOT = 0x5274E0;
constexpr uintptr_t ADDR_TXD_FIND_SLOT = 0x5275D0;
constexpr uintptr_t ADDR_TXD_LOAD = 0x5276B0;
constexpr uintptr_t ADDR_TXD_SET_CURRENT = 0x5278C0;
constexpr uintptr_t ADDR_TXD_PUSH_CURRENT = 0x527900;
constexpr uintptr_t ADDR_TXD_POP_CURRENT = 0x527910;
constexpr uintptr_t ADDR_TXD_ADD_REF = 0x527930;

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

struct RwRaster {
  void *parent;
  uint8_t *cpPixels;
  uint8_t *palette;
  int32_t width;
  int32_t height;
};

struct RwTexture {
  RwRaster *raster;
};

struct CSprite2d {
  RwTexture *m_pTexture = nullptr;
};

using DrawStdMenusFn = void(__thiscall *)(void *menuMgr);
using FadeInAlphaFn = uint32_t(__thiscall *)(void *menuMgr, uint32_t maxAlpha);
using RemoveTxdFn = void(__cdecl *)(int32_t slot);

using SpriteSetTexFn = void(__thiscall *)(CSprite2d *self, const char *name);
using SpriteSetTexMaskFn = void(__thiscall *)(CSprite2d *self, const char *name,
                                              const char *mask);
using SpriteSetAddrFn = void(__thiscall *)(CSprite2d *self, int32_t mode);
using SpriteDeleteFn = void(__thiscall *)(CSprite2d *self);
using SpriteDrawFn = void(__thiscall *)(CSprite2d *self, const CRect *rect,
                                        const CRGBA *color);

using TxdAddSlotFn = int32_t(__cdecl *)(const char *name);
using TxdFindSlotFn = int32_t(__cdecl *)(const char *name);
using TxdLoadFn = bool(__cdecl *)(int32_t slot, const char *filename);
using TxdSetCurrentFn = void(__cdecl *)(int32_t slot);
using TxdPushPopFn = void(__cdecl *)();
using TxdAddRefFn = void(__cdecl *)(int32_t slot);

CSprite2d g_logoSprite;
int32_t g_evTxdSlot = -1;
bool g_loadAttempted = false;

std::string g_asiDir;
std::string g_txdPath;
std::string g_logPath;

int32_t g_origDrawRel32 = 0;
int32_t g_origRemoveTxdRel32 = 0;
int32_t g_origMenuSetTexRel32 = 0;
bool g_patchedDraw = false;
bool g_patchedRemoveTxd = false;
bool g_patchedMenuSetTex = false;

DrawStdMenusFn g_origDrawStdMenus =
    reinterpret_cast<DrawStdMenusFn>(ADDR_FN_DRAW_STD_MENUS);
RemoveTxdFn g_origRemoveTxd = reinterpret_cast<RemoveTxdFn>(ADDR_FN_REMOVE_TXD);
SpriteSetTexMaskFn g_origMenuSetTexMask =
    reinterpret_cast<SpriteSetTexMaskFn>(ADDR_SPRITE_SET_TEXTURE_MASK);

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

void InitPaths(HMODULE hModule) {
  char buf[MAX_PATH] = {};
  if (GetModuleFileNameA(hModule, buf, MAX_PATH) > 0) {
    std::string full(buf);
    const size_t pos = full.find_last_of("\\/");
    if (pos != std::string::npos) {
      g_asiDir = full.substr(0, pos);
    }
  }
  std::string exeDir;
  if (GetModuleFileNameA(nullptr, buf, MAX_PATH) > 0) {
    std::string full(buf);
    const size_t pos = full.find_last_of("\\/");
    if (pos != std::string::npos) {
      exeDir = full.substr(0, pos);
    }
  }
  if (!g_asiDir.empty()) {
    g_txdPath = g_asiDir + "\\eventide_menu.txd";
  } else {
    g_txdPath = "eventide_menu.txd";
  }
  if (!exeDir.empty()) {
    g_logPath = exeDir + "\\MenuLogo.log";
  } else if (!g_asiDir.empty()) {
    g_logPath = g_asiDir + "\\MenuLogo.log";
  } else {
    g_logPath = "MenuLogo.log";
  }
  HANDLE f = CreateFileA(g_logPath.c_str(), GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                         CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (f != INVALID_HANDLE_VALUE) {
    CloseHandle(f);
  }
}

int32_t EnsureEvTxdLoaded() {
  if (g_evTxdSlot != -1) {
    return g_evTxdSlot;
  }
  if (GetFileAttributesA(g_txdPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
    return -1;
  }

  auto findSlot = reinterpret_cast<TxdFindSlotFn>(ADDR_TXD_FIND_SLOT);
  auto addSlot = reinterpret_cast<TxdAddSlotFn>(ADDR_TXD_ADD_SLOT);
  auto loadTxd = reinterpret_cast<TxdLoadFn>(ADDR_TXD_LOAD);
  auto addRef = reinterpret_cast<TxdAddRefFn>(ADDR_TXD_ADD_REF);

  int32_t evSlot = findSlot("evmenu");
  if (evSlot == -1) {
    evSlot = addSlot("evmenu");
  }
  if (evSlot != -1 && loadTxd(evSlot, g_txdPath.c_str())) {
    addRef(evSlot);
    g_evTxdSlot = evSlot;
    return g_evTxdSlot;
  }

  WriteLog("Failed to load eventide_menu.txd");
  return -1;
}

void EnsureLogoLoaded() {
  if (g_logoSprite.m_pTexture != nullptr || g_loadAttempted) {
    return;
  }
  g_loadAttempted = true;

  auto findSlot = reinterpret_cast<TxdFindSlotFn>(ADDR_TXD_FIND_SLOT);
  auto setCurrent = reinterpret_cast<TxdSetCurrentFn>(ADDR_TXD_SET_CURRENT);
  auto pushCurrent = reinterpret_cast<TxdPushPopFn>(ADDR_TXD_PUSH_CURRENT);
  auto popCurrent = reinterpret_cast<TxdPushPopFn>(ADDR_TXD_POP_CURRENT);
  auto setTex = reinterpret_cast<SpriteSetTexFn>(ADDR_SPRITE_SET_TEXTURE);
  auto setTexMask =
      reinterpret_cast<SpriteSetTexMaskFn>(ADDR_SPRITE_SET_TEXTURE_MASK);
  auto setAddr = reinterpret_cast<SpriteSetAddrFn>(ADDR_SPRITE_SET_ADDRESSING);

  // 1. Prefer eventidelogo128 inside menu.txd if a custom menu.txd includes it
  const int32_t menuSlot = findSlot("menu");
  if (menuSlot != -1) {
    pushCurrent();
    setCurrent(menuSlot);
    setTex(&g_logoSprite, "eventidelogo128");
    if (g_logoSprite.m_pTexture == nullptr) {
      setTexMask(&g_logoSprite, "eventidelogo128", "eventidelogo128m");
    }
    popCurrent();
  }

  // 2. Otherwise load standalone eventide_menu.txd next to MenuLogo.asi
  if (g_logoSprite.m_pTexture == nullptr) {
    const int32_t evSlot = EnsureEvTxdLoaded();
    if (evSlot != -1) {
      pushCurrent();
      setCurrent(evSlot);
      setTex(&g_logoSprite, "eventidelogo128");
      if (g_logoSprite.m_pTexture == nullptr) {
        setTexMask(&g_logoSprite, "eventidelogo128", "eventidelogo128m");
      }
      popCurrent();
    }
  }

  if (g_logoSprite.m_pTexture != nullptr) {
    setAddr(&g_logoSprite, 4);
    char msg[256] = {};
    const RwRaster *raster = g_logoSprite.m_pTexture->raster;
    std::snprintf(msg, sizeof(msg), "Loaded eventidelogo128 (%dx%d)",
                  raster ? raster->width : 0, raster ? raster->height : 0);
    WriteLog(msg);
  } else {
    WriteLog("eventidelogo128 texture not found");
  }
}

void UnloadLogo() {
  auto spriteDelete = reinterpret_cast<SpriteDeleteFn>(ADDR_SPRITE_DELETE);
  if (g_logoSprite.m_pTexture != nullptr) {
    spriteDelete(&g_logoSprite);
  }
  if (g_evTxdSlot != -1) {
    g_origRemoveTxd(g_evTxdSlot);
    g_evTxdSlot = -1;
  }
  g_loadAttempted = false;
}

void DrawBottomRightLogo(void *menuMgr) {
  EnsureLogoLoaded();
  if (g_logoSprite.m_pTexture == nullptr) {
    return;
  }

  const int32_t screenW = *reinterpret_cast<const int32_t *>(ADDR_RS_MAX_WIDTH);
  const int32_t screenH =
      *reinterpret_cast<const int32_t *>(ADDR_RS_MAX_HEIGHT);
  if (screenW <= 0 || screenH <= 0) {
    return;
  }

  // Keep native texture aspect ratio in screen pixels
  float aspect = 1.0f;
  const RwRaster *raster = g_logoSprite.m_pTexture->raster;
  if (raster && raster->width > 0 && raster->width <= 4096 &&
      raster->height > 0 && raster->height <= 4096) {
    aspect =
        static_cast<float>(raster->width) / static_cast<float>(raster->height);
  }

  // Centered 4:3 menu panel bounds (matches GTA3.WidescreenFix
  // FrontendAspectRatio=auto)
  const float w = static_cast<float>(screenW);
  const float h = static_cast<float>(screenH);
  const float panelW = (w > h * (4.0f / 3.0f)) ? (h * (4.0f / 3.0f)) : w;
  const float panelLeft = (w - panelW) * 0.5f;

  // Match the bottom-right gold Rockstar badge height/Y-alignment and place
  // cleanly to its left
  const float logoH = h * (34.0f / 448.0f);
  const float logoW = logoH * aspect;
  const float goldLeft = panelLeft + panelW * (583.5f / 640.0f);
  const float gap = h * (7.0f / 448.0f);

  CRect rect;
  rect.right = goldLeft - gap;
  rect.left = rect.right - logoW;
  rect.top = h * (395.5f / 448.0f);
  rect.bottom = rect.top + logoH;

  auto fadeInAlpha = reinterpret_cast<FadeInAlphaFn>(ADDR_FN_FADE_IN_ALPHA);
  uint32_t alpha = fadeInAlpha(menuMgr, 255);
  if (alpha > 255) {
    alpha = 255;
  }

  CRGBA color = {255, 255, 255, static_cast<uint8_t>(alpha)};
  auto spriteDraw = reinterpret_cast<SpriteDrawFn>(ADDR_SPRITE_DRAW);
  spriteDraw(&g_logoSprite, &rect, &color);
}

void __fastcall Hook_MenuLoadSetTexMask(CSprite2d *self, void * /*edx*/,
                                        const char *name, const char *mask) {
  if (name != nullptr && std::strcmp(name, "gtaLogo128") == 0) {
    const int32_t evSlot = EnsureEvTxdLoaded();
    if (evSlot != -1) {
      auto setCurrent = reinterpret_cast<TxdSetCurrentFn>(ADDR_TXD_SET_CURRENT);
      auto pushCurrent = reinterpret_cast<TxdPushPopFn>(ADDR_TXD_PUSH_CURRENT);
      auto popCurrent = reinterpret_cast<TxdPushPopFn>(ADDR_TXD_POP_CURRENT);
      auto setTex = reinterpret_cast<SpriteSetTexFn>(ADDR_SPRITE_SET_TEXTURE);

      pushCurrent();
      setCurrent(evSlot);
      setTex(self, "gtaLogo128");
      popCurrent();

      if (self->m_pTexture != nullptr) {
        const RwRaster *raster = self->m_pTexture->raster;
        char msg[256] = {};
        std::snprintf(msg, sizeof(msg),
                      "Replaced gtaLogo128 from eventide_menu.txd (%dx%d)",
                      raster ? raster->width : 0, raster ? raster->height : 0);
        WriteLog(msg);
        return;
      }
    }
  }
  g_origMenuSetTexMask(self, name, mask);
}

void __fastcall Hook_DrawStdMenus(void *self, void * /*edx*/) {
  g_origDrawStdMenus(self);
  DrawBottomRightLogo(self);
}

void __cdecl Hook_RemoveMenuTxd(int32_t slot) {
  UnloadLogo();
  g_origRemoveTxd(slot);
}

bool PatchCallRel32(uintptr_t callAddr, int32_t newRel32) {
  DWORD oldProtect = 0;
  void *target = reinterpret_cast<void *>(callAddr + 1);
  if (!VirtualProtect(target, sizeof(int32_t), PAGE_EXECUTE_READWRITE,
                      &oldProtect)) {
    return false;
  }
  *reinterpret_cast<int32_t *>(target) = newRel32;
  VirtualProtect(target, sizeof(int32_t), oldProtect, &oldProtect);
  FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void *>(callAddr),
                        5);
  return true;
}

bool InstallHooks() {
  if (*reinterpret_cast<const uint8_t *>(ADDR_CALL_DRAW_STD_MENUS) != 0xE8) {
    WriteLog("InstallHooks FAILED: expected 0xE8 at ADDR_CALL_DRAW_STD_MENUS");
    return false;
  }
  g_origDrawRel32 =
      *reinterpret_cast<const int32_t *>(ADDR_CALL_DRAW_STD_MENUS + 1);
  g_origDrawStdMenus = reinterpret_cast<DrawStdMenusFn>(
      ADDR_CALL_DRAW_STD_MENUS + 5 + g_origDrawRel32);
  const int32_t drawRel32 =
      static_cast<int32_t>(reinterpret_cast<uintptr_t>(&Hook_DrawStdMenus) -
                           (ADDR_CALL_DRAW_STD_MENUS + 5));
  g_patchedDraw = PatchCallRel32(ADDR_CALL_DRAW_STD_MENUS, drawRel32);

  if (*reinterpret_cast<const uint8_t *>(ADDR_CALL_UNLOAD_MENU_TXD) == 0xE8) {
    g_origRemoveTxdRel32 =
        *reinterpret_cast<const int32_t *>(ADDR_CALL_UNLOAD_MENU_TXD + 1);
    g_origRemoveTxd = reinterpret_cast<RemoveTxdFn>(ADDR_CALL_UNLOAD_MENU_TXD +
                                                    5 + g_origRemoveTxdRel32);
    const int32_t remRel32 =
        static_cast<int32_t>(reinterpret_cast<uintptr_t>(&Hook_RemoveMenuTxd) -
                             (ADDR_CALL_UNLOAD_MENU_TXD + 5));
    g_patchedRemoveTxd = PatchCallRel32(ADDR_CALL_UNLOAD_MENU_TXD, remRel32);
  }

  if (*reinterpret_cast<const uint8_t *>(ADDR_CALL_LOAD_MENU_SPRITE_TEX) ==
      0xE8) {
    g_origMenuSetTexRel32 =
        *reinterpret_cast<const int32_t *>(ADDR_CALL_LOAD_MENU_SPRITE_TEX + 1);
    g_origMenuSetTexMask = reinterpret_cast<SpriteSetTexMaskFn>(
        ADDR_CALL_LOAD_MENU_SPRITE_TEX + 5 + g_origMenuSetTexRel32);
    const int32_t setTexRel32 = static_cast<int32_t>(
        reinterpret_cast<uintptr_t>(&Hook_MenuLoadSetTexMask) -
        (ADDR_CALL_LOAD_MENU_SPRITE_TEX + 5));
    g_patchedMenuSetTex =
        PatchCallRel32(ADDR_CALL_LOAD_MENU_SPRITE_TEX, setTexRel32);
  }

  WriteLog((g_patchedDraw && g_patchedMenuSetTex)
               ? "InstallHooks OK"
               : "InstallHooks partial/failed");
  return g_patchedDraw;
}

void RemoveHooks() {
  if (g_patchedDraw) {
    PatchCallRel32(ADDR_CALL_DRAW_STD_MENUS, g_origDrawRel32);
    g_patchedDraw = false;
  }
  if (g_patchedRemoveTxd) {
    PatchCallRel32(ADDR_CALL_UNLOAD_MENU_TXD, g_origRemoveTxdRel32);
    g_patchedRemoveTxd = false;
  }
  if (g_patchedMenuSetTex) {
    PatchCallRel32(ADDR_CALL_LOAD_MENU_SPRITE_TEX, g_origMenuSetTexRel32);
    g_patchedMenuSetTex = false;
  }
}

} // namespace

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(hModule);
    InitPaths(hModule);
    InstallHooks();
  } else if (reason == DLL_PROCESS_DETACH) {
    if (lpReserved == nullptr) {
      RemoveHooks();
    }
  }
  return TRUE;
}
