#define WIN32_LEAN_AND_MEAN
#include "cyrillic_atlas.h"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <stdint.h>
#include <string>
#include <vector>
#include <windows.h>

namespace {

// GTA III v1.0 addresses
constexpr uintptr_t ADDR_FONT_DETAILS = 0x8F317C;
constexpr uintptr_t ADDR_RS_MAX_WIDTH = 0x8F436C;
constexpr uintptr_t ADDR_RS_MAX_HEIGHT = 0x8F4370;

constexpr uintptr_t ADDR_FONT_INITIALISE = 0x500A40;
constexpr uintptr_t ADDR_FONT_SHUTDOWN = 0x500BA0;
constexpr uintptr_t ADDR_FONT_INIT_PER_FRAME = 0x500BE0;
constexpr uintptr_t ADDR_FONT_PRINT_CHAR = 0x500C30;
constexpr uintptr_t ADDR_FONT_PRINT_STRING = 0x500F50;
constexpr uintptr_t ADDR_FONT_GET_NUMBER_LINES = 0x501260;
constexpr uintptr_t ADDR_FONT_GET_TEXT_RECT = 0x5013B0;
constexpr uintptr_t ADDR_FONT_PRINT_STRING_RANGE = 0x501730;
constexpr uintptr_t ADDR_FONT_GET_CHAR_SIZE = 0x501840;
constexpr uintptr_t ADDR_FONT_GET_STRING_WIDTH = 0x5018A0;
constexpr uintptr_t ADDR_FONT_DRAW_FONTS = 0x501B50;

constexpr uintptr_t ADDR_SPRITE_DELETE = 0x51EA00;
constexpr uintptr_t ADDR_SPRITE_SET_TEXTURE = 0x51EA40;
constexpr uintptr_t ADDR_SPRITE_SET_ADDRESSING = 0x51EAA0;
constexpr uintptr_t ADDR_SPRITE_DRAW_UV = 0x51ED90;

constexpr uintptr_t ADDR_TXD_ADD_SLOT = 0x5274E0;
constexpr uintptr_t ADDR_TXD_FIND_SLOT = 0x5275D0;
constexpr uintptr_t ADDR_TXD_LOAD = 0x5276B0;
constexpr uintptr_t ADDR_TXD_REMOVE = 0x527870;
constexpr uintptr_t ADDR_TXD_SET_CURRENT = 0x5278C0;
constexpr uintptr_t ADDR_TXD_PUSH_CURRENT = 0x527900;
constexpr uintptr_t ADDR_TXD_POP_CURRENT = 0x527910;
constexpr uintptr_t ADDR_TXD_ADD_REF = 0x527930;

constexpr uintptr_t ADDR_RW_RENDER_STATE_SET = 0x5A43C0;
constexpr uintptr_t ADDR_RW_RENDER_STATE_GET = 0x5A4410;

constexpr uint32_t RW_RS_TEXTURE_FILTER = 9;
constexpr uint32_t RW_RS_VERTEX_ALPHA_ENABLE = 12;
constexpr uintptr_t RW_FILTER_LINEAR = 2;

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

struct CFontDetails {
  CRGBA color;
  float scaleX;
  float scaleY;
  float slant;
  float slantRefX;
  float slantRefY;
  uint8_t justify;
  uint8_t centre;
  uint8_t rightJustify;
  uint8_t background;
  uint8_t backgroundOnlyText;
  uint8_t prop;
  uint8_t pad1E[2];
  float alphaFade;
  CRGBA backgroundColor;
  float wrapX;
  float centreSize;
  float rightJustifyWrap;
  int16_t style;
  uint8_t pad36[2];
  int32_t bank;
  int16_t dropShadowPosition;
  CRGBA dropColor;
  uint8_t pad42[2];
};

static_assert(offsetof(CFontDetails, color) == 0x00, "color offset");
static_assert(offsetof(CFontDetails, scaleX) == 0x04, "scaleX offset");
static_assert(offsetof(CFontDetails, prop) == 0x1D, "prop offset");
static_assert(offsetof(CFontDetails, style) == 0x34, "style offset");
static_assert(offsetof(CFontDetails, dropShadowPosition) == 0x3C,
              "dropShadowPosition offset");
static_assert(offsetof(CFontDetails, dropColor) == 0x3E, "dropColor offset");
static_assert(sizeof(CFontDetails) == 0x44, "CFontDetails size");

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

struct QueuedGlyph {
  CRect rect;
  CRGBA color;
  float u0;
  float v0;
  float u1;
  float v1;
};

using FontInitialiseFn = void(__cdecl *)();
using FontShutdownFn = void(__cdecl *)();
using FontInitPerFrameFn = void(__cdecl *)();
using FontPrintCharFn = void(__cdecl *)(float x, float y, int16_t character);
using FontPrintStringFn = void(__cdecl *)(float x, float y,
                                          const wchar_t *text);
using FontGetNumLinesFn = int(__cdecl *)(float x, float y, const wchar_t *text);
using FontGetTextRectFn = CRect *(__cdecl *)(CRect * rectOut, float x, float y,
                                             const wchar_t *text);
using FontPrintRangeFn = void(__cdecl *)(float x, float y, const wchar_t *start,
                                         const wchar_t *end, float spwidth);
using FontGetCharSizeFn = float(__cdecl *)(uint16_t c);
using FontGetStringWidthFn = float(__cdecl *)(const wchar_t *str,
                                              bool sentence);
using FontDrawFontsFn = void(__cdecl *)();

using SpriteSetTexFn = void(__thiscall *)(CSprite2d *self, const char *name);
using SpriteSetAddrFn = void(__thiscall *)(CSprite2d *self, int32_t mode);
using SpriteDeleteFn = void(__thiscall *)(CSprite2d *self);
using SpriteDrawUvFn = void(__thiscall *)(CSprite2d *self, const CRect *rect,
                                          const CRGBA *color, float u0,
                                          float v0, float u1, float v1,
                                          float u3, float v3, float u2,
                                          float v2);

using TxdAddSlotFn = int32_t(__cdecl *)(const char *name);
using TxdFindSlotFn = int32_t(__cdecl *)(const char *name);
using TxdLoadFn = bool(__cdecl *)(int32_t slot, const char *filename);
using TxdRemoveFn = void(__cdecl *)(int32_t slot);
using TxdSetCurrentFn = void(__cdecl *)(int32_t slot);
using TxdPushPopFn = void(__cdecl *)();
using TxdAddRefFn = void(__cdecl *)(int32_t slot);

using RwRenderStateSetFn = int32_t(__cdecl *)(uint32_t state, void *value);
using RwRenderStateGetFn = int32_t(__cdecl *)(uint32_t state, void *value);

std::string g_asiDir;
std::string g_txdPath;
std::string g_logPath;

CSprite2d g_cyrSprite;
int32_t g_txdSlot = -1;
bool g_loadAttempted = false;
bool g_fontHooksInstalled = false;
std::vector<QueuedGlyph> g_queuedGlyphs;

FontInitialiseFn g_origInitialise = nullptr;
FontShutdownFn g_origShutdown = nullptr;
FontInitPerFrameFn g_origInitPerFrame = nullptr;
FontPrintCharFn g_origPrintChar = nullptr;
FontPrintStringFn g_origPrintString = nullptr;
FontGetNumLinesFn g_origGetNumberLines = nullptr;
FontGetTextRectFn g_origGetTextRect = nullptr;
FontPrintRangeFn g_origPrintRange = nullptr;
FontGetCharSizeFn g_origGetCharSize = nullptr;
FontGetStringWidthFn g_origGetStringWidth = nullptr;
FontDrawFontsFn g_origDrawFonts = nullptr;

struct HookEntry {
  uintptr_t addr = 0;
  uint8_t origBytes[16] = {};
  size_t patchLen = 0;
  uint8_t *trampoline = nullptr;
  bool active = false;
};

std::vector<HookEntry> g_hooks;

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
    g_txdPath = g_asiDir + "\\eventide_cyrillic.txd";
  } else {
    g_txdPath = "eventide_cyrillic.txd";
  }
  if (!exeDir.empty()) {
    g_logPath = exeDir + "\\CyrillicText.log";
  } else if (!g_asiDir.empty()) {
    g_logPath = g_asiDir + "\\CyrillicText.log";
  } else {
    g_logPath = "CyrillicText.log";
  }
  HANDLE f = CreateFileA(g_logPath.c_str(), GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                         CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (f != INVALID_HANDLE_VALUE) {
    CloseHandle(f);
  }
}

// Minimal 32-bit x86 instruction length decoder for function prologues.
// Returns 0 on unsupported instructions so we fail safely instead of corrupting
// code.
size_t DecodeX86InsnLen(const uint8_t *p) {
  size_t i = 0;
  bool opSize16 = false;
  bool addrSize16 = false;

  // Legacy prefixes
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
      if (mod == 0 && rm == 6)
        return 3;
      if (mod == 1)
        return 2;
      if (mod == 2)
        return 3;
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
    if (mod == 1)
      len += 1;
    else if (mod == 2)
      len += 4;
    return len;
  };

  const uint8_t op = p[i++];
  const size_t imm32Size = opSize16 ? 2 : 4;

  // Two-byte 0F opcodes common in prologues (movzx, movsx, jcc rel32, etc.)
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

  // Single-byte register push/pop, nop, ret
  if ((op >= 0x50 && op <= 0x5F) || op == 0x90 || op == 0xC3) {
    return i;
  }

  // push imm8 / jcc rel8 / jmp rel8
  if (op == 0x6A || op == 0xEB || (op >= 0x70 && op <= 0x7F)) {
    return i + 1;
  }

  // push imm32 / call rel32 / jmp rel32
  if (op == 0x68 || op == 0xE8 || op == 0xE9) {
    return i + imm32Size;
  }

  // mov r32, imm32 (B8..BF) / mov r8, imm8 (B0..B7)
  if (op >= 0xB8 && op <= 0xBF) {
    return i + imm32Size;
  }
  if (op >= 0xB0 && op <= 0xB7) {
    return i + 1;
  }

  // mov eax, [moffs32] / mov [moffs32], eax (A0..A3)
  if (op >= 0xA0 && op <= 0xA3) {
    return i + (addrSize16 ? 2 : 4);
  }

  // AL/EAX immediate ALU ops
  if (op == 0x04 || op == 0x0C || op == 0x14 || op == 0x1C || op == 0x24 ||
      op == 0x2C || op == 0x34 || op == 0x3C || op == 0xA8) {
    return i + 1;
  }
  if (op == 0x05 || op == 0x0D || op == 0x15 || op == 0x1D || op == 0x25 ||
      op == 0x2D || op == 0x35 || op == 0x3D || op == 0xA9) {
    return i + imm32Size;
  }

  // Standard ModR/M ALU / mov / lea / x87 fld/fstp
  if ((op <= 0x3B && (op & 7) <= 3) || op == 0x63 || op == 0x84 || op == 0x85 ||
      (op >= 0x88 && op <= 0x8D) || op == 0x8F || (op >= 0xD8 && op <= 0xDF)) {
    return i + modrmLen(&p[i]);
  }

  // Group 1 / shift / test / mov r/m, imm
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

  // If another ASI already placed a JMP rel32 at addr, chain directly to its
  // target
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
    // Reject short relative jumps inside the stolen prologue bytes
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

  // Relocate any CALL/JMP rel32 inside copied prologue instructions
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

  // Append JMP back to addr + copyLen
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
    if (!it->active)
      continue;
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

const CyrillicGlyphMetric *FindCyrillicGlyph(uint16_t cp) {
  size_t lo = 0;
  size_t hi = kCyrillicGlyphCount;
  while (lo < hi) {
    const size_t mid = lo + (hi - lo) / 2;
    const uint16_t midCp = kCyrillicGlyphs[mid].codepoint;
    if (midCp == cp) {
      return &kCyrillicGlyphs[mid];
    }
    if (midCp < cp) {
      lo = mid + 1;
    } else {
      hi = mid;
    }
  }
  return nullptr;
}

// Decodes widened UTF-8 sequences (e.g. from III.VC.CLEO reading UTF-8 .fxt
// byte-by-byte) into real UTF-16 codepoints when supported Cyrillic/punctuation
// sequences are present. Returns nullptr if the string needs no conversion.
const wchar_t *MaybeDecodeWidenedUtf8(const wchar_t *src, size_t maxLen,
                                      size_t *outLen = nullptr) {
  if (!src) {
    return nullptr;
  }

  bool needsDecode = false;
  size_t scan = 0;
  while (scan < maxLen && src[scan] != L'\0') {
    const uint16_t w0 = static_cast<uint16_t>(src[scan]);
    if (w0 >= 0x00D0 && w0 <= 0x00D3 && scan + 1 < maxLen) {
      const uint16_t w1 = static_cast<uint16_t>(src[scan + 1]);
      if (w1 >= 0x0080 && w1 <= 0x00BF) {
        needsDecode = true;
        break;
      }
    } else if (w0 == 0x00C2 && scan + 1 < maxLen) {
      const uint16_t w1 = static_cast<uint16_t>(src[scan + 1]);
      if (w1 == 0x00AB || w1 == 0x00BB) {
        needsDecode = true;
        break;
      }
    } else if (w0 == 0x00E2 && scan + 2 < maxLen) {
      const uint16_t w1 = static_cast<uint16_t>(src[scan + 1]);
      const uint16_t w2 = static_cast<uint16_t>(src[scan + 2]);
      if ((w1 == 0x0080 && (w2 == 0x0093 || w2 == 0x0094)) ||
          (w1 == 0x0084 && w2 == 0x0096)) {
        needsDecode = true;
        break;
      }
    }
    ++scan;
  }

  if (!needsDecode) {
    return nullptr;
  }

  constexpr size_t kBufSize = 1024;
  constexpr size_t kRingSlots = 8;
  static wchar_t ring[kRingSlots][kBufSize];
  static size_t ringIdx = 0;

  wchar_t *dst = ring[ringIdx];
  ringIdx = (ringIdx + 1) % kRingSlots;

  size_t i = 0;
  size_t o = 0;
  while (i < maxLen && src[i] != L'\0' && o + 1 < kBufSize) {
    const uint16_t w0 = static_cast<uint16_t>(src[i]);
    if (w0 >= 0x00D0 && w0 <= 0x00D3 && i + 1 < maxLen) {
      const uint16_t w1 = static_cast<uint16_t>(src[i + 1]);
      if (w1 >= 0x0080 && w1 <= 0x00BF) {
        const uint16_t cp =
            static_cast<uint16_t>(((w0 & 0x1F) << 6) | (w1 & 0x3F));
        if (FindCyrillicGlyph(cp) != nullptr) {
          dst[o++] = static_cast<wchar_t>(cp);
          i += 2;
          continue;
        }
      }
    } else if (w0 == 0x00C2 && i + 1 < maxLen) {
      const uint16_t w1 = static_cast<uint16_t>(src[i + 1]);
      if (w1 == 0x00AB || w1 == 0x00BB) {
        dst[o++] = static_cast<wchar_t>(w1);
        i += 2;
        continue;
      }
    } else if (w0 == 0x00E2 && i + 2 < maxLen) {
      const uint16_t w1 = static_cast<uint16_t>(src[i + 1]);
      const uint16_t w2 = static_cast<uint16_t>(src[i + 2]);
      if (w1 == 0x0080 && (w2 == 0x0093 || w2 == 0x0094)) {
        dst[o++] = static_cast<wchar_t>(w2 == 0x0093 ? 0x2013 : 0x2014);
        i += 3;
        continue;
      }
      if (w1 == 0x0084 && w2 == 0x0096) {
        dst[o++] = static_cast<wchar_t>(0x2116);
        i += 3;
        continue;
      }
    }
    dst[o++] = src[i++];
  }
  dst[o] = L'\0';
  if (outLen) {
    *outLen = o;
  }
  return dst;
}

bool NeedsCustomStringWidth(const wchar_t *s) {
  if (!s) {
    return false;
  }
  for (; *s != L'\0'; ++s) {
    const uint16_t ch = static_cast<uint16_t>(*s);
    if (ch >= 0x00E1u || FindCyrillicGlyph(ch) != nullptr) {
      return true;
    }
  }
  return false;
}

void EnsureTextureLoaded() {
  if (g_cyrSprite.m_pTexture != nullptr || g_loadAttempted) {
    return;
  }
  g_loadAttempted = true;

  if (GetFileAttributesA(g_txdPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
    WriteLog("eventide_cyrillic.txd not found next to CyrillicText.asi");
    return;
  }

  auto findSlot = reinterpret_cast<TxdFindSlotFn>(ADDR_TXD_FIND_SLOT);
  auto addSlot = reinterpret_cast<TxdAddSlotFn>(ADDR_TXD_ADD_SLOT);
  auto loadTxd = reinterpret_cast<TxdLoadFn>(ADDR_TXD_LOAD);
  auto addRef = reinterpret_cast<TxdAddRefFn>(ADDR_TXD_ADD_REF);
  auto pushCurrent = reinterpret_cast<TxdPushPopFn>(ADDR_TXD_PUSH_CURRENT);
  auto popCurrent = reinterpret_cast<TxdPushPopFn>(ADDR_TXD_POP_CURRENT);
  auto setCurrent = reinterpret_cast<TxdSetCurrentFn>(ADDR_TXD_SET_CURRENT);
  auto setTex = reinterpret_cast<SpriteSetTexFn>(ADDR_SPRITE_SET_TEXTURE);
  auto setAddr = reinterpret_cast<SpriteSetAddrFn>(ADDR_SPRITE_SET_ADDRESSING);

  int32_t slot = findSlot("evcyr");
  if (slot == -1) {
    slot = addSlot("evcyr");
  }
  if (slot != -1 && loadTxd(slot, g_txdPath.c_str())) {
    addRef(slot);
    g_txdSlot = slot;
    pushCurrent();
    setCurrent(slot);
    setTex(&g_cyrSprite, "cyrillic");
    popCurrent();
  }

  if (g_cyrSprite.m_pTexture != nullptr) {
    // CLAMP (3) avoids any edge wrap
    setAddr(&g_cyrSprite, 3);
    const RwRaster *r = g_cyrSprite.m_pTexture->raster;
    char msg[128] = {};
    std::snprintf(msg, sizeof(msg), "Loaded eventide_cyrillic.txd (%dx%d)",
                  r ? r->width : 0, r ? r->height : 0);
    WriteLog(msg);
  } else {
    WriteLog("Failed to load cyrillic texture from eventide_cyrillic.txd");
  }
}

void UnloadTexture() {
  g_queuedGlyphs.clear();
  if (g_cyrSprite.m_pTexture != nullptr) {
    auto spriteDelete = reinterpret_cast<SpriteDeleteFn>(ADDR_SPRITE_DELETE);
    spriteDelete(&g_cyrSprite);
  }
  if (g_txdSlot != -1) {
    auto removeTxd = reinterpret_cast<TxdRemoveFn>(ADDR_TXD_REMOVE);
    removeTxd(g_txdSlot);
    g_txdSlot = -1;
  }
  g_loadAttempted = false;
}

void __cdecl Hook_InitPerFrame() {
  g_queuedGlyphs.clear();
  g_origInitPerFrame();
}

void __cdecl Hook_Shutdown() {
  UnloadTexture();
  g_origShutdown();
}

float __cdecl Hook_GetCharSize(uint16_t c) {
  const uint16_t cp = static_cast<uint16_t>(c + 0x20u);
  const CyrillicGlyphMetric *g = FindCyrillicGlyph(cp);
  if (g != nullptr) {
    const auto *details =
        reinterpret_cast<const CFontDetails *>(ADDR_FONT_DETAILS);
    const float units = details->prop ? g->advanceUnits : 22.0f;
    return units * details->scaleX;
  }
  return g_origGetCharSize(c);
}

void __cdecl Hook_PrintChar(float x, float y, int16_t character) {
  const uint16_t cp = static_cast<uint16_t>(character) + 0x20u;
  const CyrillicGlyphMetric *g = FindCyrillicGlyph(cp);
  if (g == nullptr) {
    g_origPrintChar(x, y, character);
    return;
  }

  const int32_t screenW = *reinterpret_cast<const int32_t *>(ADDR_RS_MAX_WIDTH);
  const int32_t screenH =
      *reinterpret_cast<const int32_t *>(ADDR_RS_MAX_HEIGHT);
  if (screenW <= 0 || screenH <= 0 || x <= 0.0f ||
      x > static_cast<float>(screenW) || y <= 0.0f ||
      y > static_cast<float>(screenH)) {
    return;
  }

  const auto *details =
      reinterpret_cast<const CFontDetails *>(ADDR_FONT_DETAILS);
  const float sx = details->scaleX;
  const float sy =
      (details->style == 1) ? (details->scaleY * 0.8f) : details->scaleY;

  CRect rect;
  rect.left = x + g->xOffUnits * sx;
  rect.top = y + g->yOffUnits * sy;
  rect.right = rect.left + g->quadWUnits * sx;
  rect.bottom = rect.top + g->quadHUnits * sy;

  if (details->dropShadowPosition != 0) {
    const float shX = static_cast<float>(details->dropShadowPosition) *
                      (static_cast<float>(screenW) / 640.0f);
    const float shY = static_cast<float>(details->dropShadowPosition) *
                      (static_cast<float>(screenH) / 448.0f);
    CRect shadowRect;
    shadowRect.left = rect.left + shX;
    shadowRect.top = rect.top + shY;
    shadowRect.right = rect.right + shX;
    shadowRect.bottom = rect.bottom + shY;
    g_queuedGlyphs.push_back(
        {shadowRect, details->dropColor, g->u0, g->v0, g->u1, g->v1});
  }

  g_queuedGlyphs.push_back({rect, details->color, g->u0, g->v0, g->u1, g->v1});
}

void __cdecl Hook_PrintString(float x, float y, const wchar_t *text) {
  const wchar_t *decoded = MaybeDecodeWidenedUtf8(text, 1024);
  g_origPrintString(x, y, decoded ? decoded : text);
}

int __cdecl Hook_GetNumberLines(float x, float y, const wchar_t *text) {
  const wchar_t *decoded = MaybeDecodeWidenedUtf8(text, 1024);
  return g_origGetNumberLines(x, y, decoded ? decoded : text);
}

CRect *__cdecl Hook_GetTextRect(CRect *rectOut, float x, float y,
                                const wchar_t *text) {
  const wchar_t *decoded = MaybeDecodeWidenedUtf8(text, 1024);
  return g_origGetTextRect(rectOut, x, y, decoded ? decoded : text);
}

void __cdecl Hook_PrintRange(float x, float y, const wchar_t *start,
                             const wchar_t *end, float spwidth) {
  if (start && end && end > start) {
    const size_t len = static_cast<size_t>(end - start);
    size_t outLen = 0;
    const wchar_t *decoded = MaybeDecodeWidenedUtf8(start, len, &outLen);
    if (decoded) {
      g_origPrintRange(x, y, decoded, decoded + outLen, spwidth);
      return;
    }
  }
  g_origPrintRange(x, y, start, end, spwidth);
}

float __cdecl Hook_GetStringWidth(const wchar_t *str, bool sentence) {
  const wchar_t *decoded = MaybeDecodeWidenedUtf8(str, 1024);
  const wchar_t *target = decoded ? decoded : str;
  if (!target) {
    return 0.0f;
  }
  if (!NeedsCustomStringWidth(target)) {
    return g_origGetStringWidth(target, sentence);
  }

  // Vanilla CFont::GetStringWidth inlines CFont::Size[style][c] out-of-bounds
  // for characters >= 0xE1, so compute width via Hook_GetCharSize instead.
  float width = 0.0f;
  for (const wchar_t *s = target; *s != L'\0' && (*s != L' ' || sentence);) {
    if (*s == L'~') {
      ++s;
      while (*s != L'\0' && *s != L'~') {
        ++s;
      }
      if (*s == L'~') {
        ++s;
      }
      continue;
    }
    const uint16_t ch = static_cast<uint16_t>(*s);
    if (ch >= 0x20u) {
      width += Hook_GetCharSize(static_cast<uint16_t>(ch - 0x20u));
    }
    ++s;
  }
  return width;
}

void __cdecl Hook_DrawFonts() {
  g_origDrawFonts();
  if (g_queuedGlyphs.empty()) {
    return;
  }

  EnsureTextureLoaded();
  if (g_cyrSprite.m_pTexture != nullptr) {
    auto rsGet = reinterpret_cast<RwRenderStateGetFn>(ADDR_RW_RENDER_STATE_GET);
    auto rsSet = reinterpret_cast<RwRenderStateSetFn>(ADDR_RW_RENDER_STATE_SET);
    auto drawUv = reinterpret_cast<SpriteDrawUvFn>(ADDR_SPRITE_DRAW_UV);

    void *prevAlpha = nullptr;
    void *prevFilter = nullptr;
    rsGet(RW_RS_VERTEX_ALPHA_ENABLE, &prevAlpha);
    rsGet(RW_RS_TEXTURE_FILTER, &prevFilter);

    rsSet(RW_RS_VERTEX_ALPHA_ENABLE, reinterpret_cast<void *>(1));
    rsSet(RW_RS_TEXTURE_FILTER, reinterpret_cast<void *>(RW_FILTER_LINEAR));

    for (const auto &q : g_queuedGlyphs) {
      drawUv(&g_cyrSprite, &q.rect, &q.color, q.u0, q.v0, q.u1, q.v0, q.u0,
             q.v1, q.u1, q.v1);
    }

    rsSet(RW_RS_TEXTURE_FILTER, prevFilter);
    rsSet(RW_RS_VERTEX_ALPHA_ENABLE, prevAlpha);
  }
  g_queuedGlyphs.clear();
}

void InstallDeferredFontHooks() {
  if (g_fontHooksInstalled) {
    return;
  }
  g_fontHooksInstalled = true;

  bool ok = true;
  ok &= InstallDetour(ADDR_FONT_SHUTDOWN, &Hook_Shutdown, &g_origShutdown);
  ok &= InstallDetour(ADDR_FONT_INIT_PER_FRAME, &Hook_InitPerFrame,
                      &g_origInitPerFrame);
  ok &= InstallDetour(ADDR_FONT_PRINT_CHAR, &Hook_PrintChar, &g_origPrintChar);
  ok &= InstallDetour(ADDR_FONT_PRINT_STRING, &Hook_PrintString,
                      &g_origPrintString);
  ok &= InstallDetour(ADDR_FONT_GET_NUMBER_LINES, &Hook_GetNumberLines,
                      &g_origGetNumberLines);
  ok &= InstallDetour(ADDR_FONT_GET_TEXT_RECT, &Hook_GetTextRect,
                      &g_origGetTextRect);
  ok &= InstallDetour(ADDR_FONT_PRINT_STRING_RANGE, &Hook_PrintRange,
                      &g_origPrintRange);
  ok &= InstallDetour(ADDR_FONT_GET_CHAR_SIZE, &Hook_GetCharSize,
                      &g_origGetCharSize);
  ok &= InstallDetour(ADDR_FONT_GET_STRING_WIDTH, &Hook_GetStringWidth,
                      &g_origGetStringWidth);
  ok &= InstallDetour(ADDR_FONT_DRAW_FONTS, &Hook_DrawFonts, &g_origDrawFonts);

  WriteLog(ok ? "Font hooks installed OK" : "Font hooks partial/failed");
}

void __cdecl Hook_Initialise() {
  g_origInitialise();
  InstallDeferredFontHooks();
  EnsureTextureLoaded();
}

bool InstallBootHook() {
  // Hook only CFont::Initialise at DLL_PROCESS_ATTACH so ASI loaders/patches
  // (SilentPatch, WidescreenFix, CLEO) finish their pattern scans untouched.
  const bool ok =
      InstallDetour(ADDR_FONT_INITIALISE, &Hook_Initialise, &g_origInitialise);
  WriteLog(ok ? "Boot hook (CFont::Initialise) OK"
              : "Boot hook (CFont::Initialise) FAILED");
  return ok;
}

} // namespace

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(hModule);
    InitPaths(hModule);
    InstallBootHook();
  } else if (reason == DLL_PROCESS_DETACH) {
    if (lpReserved == nullptr) {
      RemoveAllHooks();
    }
  }
  return TRUE;
}
