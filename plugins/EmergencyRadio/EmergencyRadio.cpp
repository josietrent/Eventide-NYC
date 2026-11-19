#define WIN32_LEAN_AND_MEAN
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <windows.h>

namespace {

// GTA III v1.0 addresses
constexpr uintptr_t ADDR_MUSIC_MANAGER = 0x8F3964;
constexpr uintptr_t ADDR_AUDIO_MANAGER = 0x880FC0;
constexpr uintptr_t ADDR_SAMPLE_MANAGER = 0x7341E0;
constexpr uintptr_t ADDR_THE_TEXT = 0x941520;
constexpr uintptr_t ADDR_THE_CAMERA = 0x6FACF8;
constexpr uintptr_t ADDR_USER_PAUSE = 0x95CD7C;
constexpr uintptr_t ADDR_CODE_PAUSE = 0x95CDB1;
constexpr uintptr_t ADDR_REPLAY_MODE = 0x95CD5B;
constexpr uintptr_t ADDR_RS_MAX_WIDTH = 0x8F436C;
constexpr uintptr_t ADDR_RS_MAX_HEIGHT = 0x8F4370;

constexpr uintptr_t ADDR_NUM_RETUNE_PRESSES = 0x650B80;
constexpr uintptr_t ADDR_RETUNE_COUNTER = 0x650B84;
constexpr uintptr_t ADDR_CURRENT_STATION_PTR = 0x650B9C;
constexpr uintptr_t ADDR_DISPLAY_TIMER = 0x650BA1;
constexpr uintptr_t ADDR_WINDOW_HANDLE = 0x726768;

// Functions
constexpr uintptr_t ADDR_FIND_PLAYER_VEHICLE = 0x4A10C0;
constexpr uintptr_t ADDR_FIND_PLAYER_PED = 0x4A1150;
constexpr uintptr_t ADDR_TEXT_GET = 0x52C5A0;

constexpr uintptr_t ADDR_FONT_PRINT_STRING = 0x500F50;
constexpr uintptr_t ADDR_FONT_DRAW_FONTS = 0x501B50;
constexpr uintptr_t ADDR_FONT_SET_SCALE = 0x501B80;
constexpr uintptr_t ADDR_FONT_SET_COLOR = 0x501BD0;
constexpr uintptr_t ADDR_FONT_SET_JUSTIFY_OFF = 0x501C80;
constexpr uintptr_t ADDR_FONT_SET_CENTRE_ON = 0x501C90;
constexpr uintptr_t ADDR_FONT_SET_CENTRE_SIZE = 0x501CD0;
constexpr uintptr_t ADDR_FONT_SET_BACKGROUND_OFF = 0x501CF0;
constexpr uintptr_t ADDR_FONT_SET_PROP_ON = 0x501DA0;
constexpr uintptr_t ADDR_FONT_SET_FONT_STYLE = 0x501DB0;

constexpr uintptr_t ADDR_MUSIC_RESET_AFTER_RELOAD = 0x57CF30;
constexpr uintptr_t ADDR_MUSIC_INITIALISE = 0x57CF70;
constexpr uintptr_t ADDR_MUSIC_SET_RADIO_BY_SCRIPT = 0x57D180;
constexpr uintptr_t ADDR_MUSIC_SERVICE_GAME_MODE = 0x57D690;
constexpr uintptr_t ADDR_MUSIC_CHANGE_RADIO_TRACK = 0x57E450;
constexpr uintptr_t ADDR_MUSIC_PLAYER_IN_CAR = 0x57E4B0;
constexpr uintptr_t ADDR_MUSIC_GET_CAR_TUNING = 0x57E530;
constexpr uintptr_t ADDR_MUSIC_GET_NEXT_CAR_TUNING = 0x57E5A0;
constexpr uintptr_t ADDR_MUSIC_USES_POLICE_RADIO = 0x57E6A0;
constexpr uintptr_t ADDR_MUSIC_DISPLAY_RADIO_NAME = 0x57E6D0;

constexpr uintptr_t ADDR_PAD_GET_PAD = 0x492F60;
constexpr uintptr_t ADDR_PAD_CHANGE_STATION = 0x493870;
constexpr uintptr_t ADDR_AUDIO_PLAY_ONE_SHOT = 0x57A500;
constexpr uint16_t SOUND_FRONTEND_RADIO_CHANGE = 0x9F;

// Struct field offsets (validated against GTA III v1.0)
constexpr size_t OFF_MUSIC_NEXT_TRACK = 0x03;
constexpr size_t OFF_MUSIC_PLAYING_TRACK = 0x04;
constexpr size_t OFF_MUSIC_PREV_PLAYER_IN_CAR = 0x09;
constexpr size_t OFF_MUSIC_PLAYER_IN_CAR = 0x0A;
constexpr size_t OFF_MUSIC_RADIO_SET_BY_SCRIPT = 0x94F;

constexpr size_t OFF_AUDIO_WAS_PAUSED = 0x4B0E;
constexpr size_t OFF_CAM_WIDESCREEN = 0x70;

constexpr size_t OFF_ENTITY_STATUS_BYTE = 0x50;
constexpr size_t OFF_ENTITY_MODEL_INDEX = 0x5C;
constexpr size_t OFF_PED_STATE = 0x224;
constexpr size_t OFF_VEH_RADIO_STATION = 0x229;

// Game constants
constexpr uint8_t HEAD_RADIO = 0;
constexpr uint8_t DOUBLE_CLEF = 1;
constexpr uint8_t JAH_RADIO = 2;
constexpr uint8_t RISE_FM = 3;
constexpr uint8_t LIPS_106 = 4;
constexpr uint8_t GAME_FM = 5;
constexpr uint8_t MSX_FM = 6;
constexpr uint8_t FLASHBACK = 7;
constexpr uint8_t CHATTERBOX = 8;
constexpr uint8_t USERTRACK = 9;
constexpr uint8_t POLICE_RADIO = 10;
constexpr uint8_t RADIO_OFF = 11;
constexpr uint8_t STREAMED_SOUND_CITY_AMBIENT = 11;
constexpr uint8_t STREAMED_SOUND_WATER_AMBIENT = 12;

constexpr int16_t MI_AMBULAN = 106;
constexpr int16_t MI_POLICE = 116;

constexpr uint8_t STATUS_WRECKED = 5;
constexpr int32_t PEDSTATE_DRAG_FROM_CAR = 51;
constexpr int32_t PEDSTATE_EXIT_CAR = 54;
constexpr int32_t PEDSTATE_ARRESTED = 56;
constexpr uint8_t REPLAY_MODE_PLAYBACK = 1;
constexpr int16_t FONT_HEADING = 2;

// 18:39 into GAME.wav (in milliseconds)
constexpr int32_t EMERGENCY_GAME_FM_POS_MS = (18 * 60 + 39) * 1000;

struct CRGBA {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
};

using MusicBoolFn = bool(__thiscall *)(void *self);
using MusicVoidFn = void(__thiscall *)(void *self);
using MusicUint8Fn = uint8_t(__thiscall *)(void *self);
using MusicUsesPoliceRadioFn = bool(__thiscall *)(void *self, void *veh);
using MusicSetRadioByScriptFn = void(__thiscall *)(void *self, uint8_t station,
                                                   int32_t pos);
using MusicChangeRadioTrackFn = void(__thiscall *)(void *self, uint8_t track,
                                                   int32_t pos);
using SampleIsMP3AvailFn = bool(__thiscall *)(void *self);

using FindPlayerVehicleFn = void *(__cdecl *)();
using FindPlayerPedFn = void *(__cdecl *)();
using TextGetFn = const wchar_t *(__thiscall *)(void *self, const char *key);

using FontVoidFn = void(__cdecl *)();
using FontScaleFn = void(__cdecl *)(float w, float h);
using FontColorFn = void(__cdecl *)(const CRGBA *color);
using FontFloatFn = void(__cdecl *)(float val);
using FontStyleFn = void(__cdecl *)(int16_t style);
using FontPrintStringFn = void(__cdecl *)(float x, float y, const wchar_t *s);

using PadGetFn = void *(__cdecl *)(int32_t pad);
using PadChangeStationFn = bool(__thiscall *)(void *pad);
using AudioPlayOneShotFn = void(__thiscall *)(void *audioMgr,
                                              int32_t entityIndex,
                                              uint16_t sound, float vol);

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

SampleIsMP3AvailFn g_isMP3Available = nullptr;

MusicBoolFn g_origInitialise = nullptr;
MusicVoidFn g_origResetMusicAfterReload = nullptr;
MusicVoidFn g_origServiceGameMode = nullptr;
MusicBoolFn g_origPlayerInCar = nullptr;
MusicUint8Fn g_origGetNextCarTuning = nullptr;
MusicUsesPoliceRadioFn g_origUsesPoliceRadio = nullptr;
MusicVoidFn g_origDisplayRadioStationName = nullptr;

void *g_lastPlayerVeh = nullptr;
uint8_t g_currentStation = RADIO_OFF;
int32_t g_pendingScrollSteps = 0;
WNDPROC g_origWndProc = nullptr;
HWND g_gameHwnd = nullptr;

const wchar_t kPoliceScannerLabel[] = L"Police Scanner";
const wchar_t kRadioOffLabel[] = L"Radio Off";

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
      g_logPath = full.substr(0, pos) + "\\EmergencyRadio.log";
    } else {
      g_logPath = "EmergencyRadio.log";
    }
  } else {
    g_logPath = "EmergencyRadio.log";
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

LRESULT CALLBACK SubclassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                 LPARAM lParam) {
  if (uMsg == WM_MOUSEWHEEL) {
    const short delta = GET_WHEEL_DELTA_WPARAM(wParam);
    if (delta > 0) {
      g_pendingScrollSteps += 1;
    } else if (delta < 0) {
      g_pendingScrollSteps -= 1;
    }
  }
  return CallWindowProcA(g_origWndProc, hwnd, uMsg, wParam, lParam);
}

void InstallWindowHook() {
  if (g_origWndProc) {
    return;
  }
  HWND hwnd = *reinterpret_cast<HWND *>(ADDR_WINDOW_HANDLE);
  if (hwnd) {
    g_gameHwnd = hwnd;
    g_origWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(
        hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(SubclassWndProc)));
    if (g_origWndProc) {
      WriteLog("Mouse wheel WindowProc subclass installed OK");
    }
  }
}

void RemoveWindowHook() {
  if (g_origWndProc && g_gameHwnd) {
    SetWindowLongPtrA(g_gameHwnd, GWLP_WNDPROC,
                      reinterpret_cast<LONG_PTR>(g_origWndProc));
    g_origWndProc = nullptr;
    g_gameHwnd = nullptr;
  }
}

void RemoveAllHooks() {
  RemoveWindowHook();
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

void ResolveMP3AvailableFn() {
  const uint8_t *p = reinterpret_cast<const uint8_t *>(ADDR_MUSIC_INITIALISE);
  for (size_t i = 0; i < 0x200; ++i) {
    if (p[i] == 0xB9 &&
        *reinterpret_cast<const uint32_t *>(p + i + 1) == ADDR_SAMPLE_MANAGER) {
      if (p[i + 5] == 0xE8) {
        const int32_t rel = *reinterpret_cast<const int32_t *>(p + i + 6);
        const uintptr_t target = ADDR_MUSIC_INITIALISE + i + 5 + 5 + rel;
        g_isMP3Available = reinterpret_cast<SampleIsMP3AvailFn>(target);
        char msg[96] = {};
        std::snprintf(msg, sizeof(msg),
                      "Resolved IsMP3RadioChannelAvailable at 0x%08X",
                      static_cast<unsigned>(target));
        WriteLog(msg);
        return;
      }
    }
  }
  WriteLog("Fallback: using direct cSampleManager field for MP3 availability");
}

bool IsMP3ChannelAvailable() {
  if (g_isMP3Available != nullptr) {
    return g_isMP3Available(reinterpret_cast<void *>(ADDR_SAMPLE_MANAGER));
  }
  // cSampleManager::m_nMP3StationPresent offset 0x4B20 in GTA III v1.0
  const uint8_t val =
      *reinterpret_cast<const uint8_t *>(ADDR_SAMPLE_MANAGER + 0x4B20);
  return val != 0;
}

inline uint8_t &MusicU8(void *self, size_t off) {
  return *reinterpret_cast<uint8_t *>(reinterpret_cast<uintptr_t>(self) + off);
}

inline uint8_t GetEntityStatus(const void *entity) {
  return static_cast<uint8_t>(
      *reinterpret_cast<const uint8_t *>(reinterpret_cast<uintptr_t>(entity) +
                                         OFF_ENTITY_STATUS_BYTE) >>
      3);
}

inline int16_t GetEntityModelIndex(const void *entity) {
  return *reinterpret_cast<const int16_t *>(
      reinterpret_cast<uintptr_t>(entity) + OFF_ENTITY_MODEL_INDEX);
}

inline int32_t GetPedState(const void *ped) {
  return *reinterpret_cast<const int32_t *>(reinterpret_cast<uintptr_t>(ped) +
                                            OFF_PED_STATE);
}

inline uint8_t &VehRadioStation(void *veh) {
  return *reinterpret_cast<uint8_t *>(reinterpret_cast<uintptr_t>(veh) +
                                      OFF_VEH_RADIO_STATION);
}

uint8_t AdvanceRadioStation(uint8_t current, int32_t presses, bool mp3Avail) {
  static constexpr uint8_t kStationsWithMP3[] = {
      HEAD_RADIO, DOUBLE_CLEF, JAH_RADIO,  RISE_FM,   LIPS_106,     GAME_FM,
      MSX_FM,     FLASHBACK,   CHATTERBOX, USERTRACK, POLICE_RADIO, RADIO_OFF};
  static constexpr uint8_t kStationsNoMP3[] = {
      HEAD_RADIO, DOUBLE_CLEF, JAH_RADIO,  RISE_FM,      LIPS_106, GAME_FM,
      MSX_FM,     FLASHBACK,   CHATTERBOX, POLICE_RADIO, RADIO_OFF};

  const uint8_t *list = mp3Avail ? kStationsWithMP3 : kStationsNoMP3;
  const int32_t count = static_cast<int32_t>(mp3Avail ? sizeof(kStationsWithMP3)
                                                      : sizeof(kStationsNoMP3));

  int32_t idx = count - 1; // Default to RADIO_OFF if unknown
  for (int32_t i = 0; i < count; ++i) {
    if (list[i] == current) {
      idx = i;
      break;
    }
  }

  int32_t nextIdx = (idx + presses) % count;
  if (nextIdx < 0) {
    nextIdx += count;
  }
  return list[nextIdx];
}

bool __fastcall Hook_PlayerInCar(void *self, uintptr_t) {
  if (g_origPlayerInCar(self)) {
    return true;
  }

  auto findPlayerVeh =
      reinterpret_cast<FindPlayerVehicleFn>(ADDR_FIND_PLAYER_VEHICLE);
  void *veh = findPlayerVeh();
  if (!veh || GetEntityModelIndex(veh) != MI_AMBULAN) {
    return false;
  }

  auto findPlayerPed = reinterpret_cast<FindPlayerPedFn>(ADDR_FIND_PLAYER_PED);
  void *ped = findPlayerPed();
  if (!ped) {
    return false;
  }

  const int32_t pedState = GetPedState(ped);
  if (pedState == PEDSTATE_DRAG_FROM_CAR || pedState == PEDSTATE_EXIT_CAR ||
      pedState == PEDSTATE_ARRESTED) {
    return false;
  }

  if (GetEntityStatus(veh) == STATUS_WRECKED) {
    return false;
  }

  return true;
}

bool __fastcall Hook_UsesPoliceRadio(void *, uintptr_t, void *) {
  // Returning false allows full radio station selection (including Police
  // Scanner) in police/military cars instead of locking them to POLICE_RADIO.
  return false;
}

uint8_t __fastcall Hook_GetNextCarTuning(void *, uintptr_t) {
  auto findPlayerVeh =
      reinterpret_cast<FindPlayerVehicleFn>(ADDR_FIND_PLAYER_VEHICLE);
  void *veh = findPlayerVeh();
  if (!veh) {
    return RADIO_OFF;
  }

  int32_t &numRetunePresses =
      *reinterpret_cast<int32_t *>(ADDR_NUM_RETUNE_PRESSES);
  uint8_t &station = VehRadioStation(veh);

  if (numRetunePresses != 0) {
    station =
        AdvanceRadioStation(station, numRetunePresses, IsMP3ChannelAvailable());
    numRetunePresses = 0;
  }
  g_currentStation = station;
  return station;
}

void __fastcall Hook_DisplayRadioStationName(void *self, uintptr_t) {
  const bool paused =
      (*reinterpret_cast<const uint8_t *>(ADDR_USER_PAUSE) != 0) ||
      (*reinterpret_cast<const uint8_t *>(ADDR_CODE_PAUSE) != 0);
  const bool wideScreen = *reinterpret_cast<const uint8_t *>(
                              ADDR_THE_CAMERA + OFF_CAM_WIDESCREEN) != 0;
  const bool replayPlaying = *reinterpret_cast<const uint8_t *>(
                                 ADDR_REPLAY_MODE) == REPLAY_MODE_PLAYBACK;

  const bool inCar = Hook_PlayerInCar(self, 0);
  uint8_t &displayTimer = *reinterpret_cast<uint8_t *>(ADDR_DISPLAY_TIMER);

  if (paused || wideScreen || replayPlaying || (!inCar && displayTimer == 0)) {
    return;
  }

  const wchar_t *&currentStation =
      *reinterpret_cast<const wchar_t **>(ADDR_CURRENT_STATION_PTR);

  if (inCar && MusicU8(self, OFF_MUSIC_PLAYER_IN_CAR) != 0 &&
      MusicU8(self, OFF_MUSIC_PREV_PLAYER_IN_CAR) == 0) {
    currentStation = nullptr;
  }

  uint8_t curRadio =
      inCar ? MusicU8(self, OFF_MUSIC_NEXT_TRACK) : g_currentStation;
  if (curRadio == STREAMED_SOUND_CITY_AMBIENT ||
      curRadio == STREAMED_SOUND_WATER_AMBIENT) {
    curRadio = RADIO_OFF;
  } else if (curRadio > STREAMED_SOUND_WATER_AMBIENT) {
    return;
  }

  const int32_t numRetunePresses =
      *reinterpret_cast<const int32_t *>(ADDR_NUM_RETUNE_PRESSES);
  const bool mp3Avail = IsMP3ChannelAvailable();
  const uint8_t targetStation =
      inCar ? AdvanceRadioStation(curRadio, numRetunePresses, mp3Avail)
            : curRadio;

  static const char *const kStationTextKeys[9] = {
      "FEA_FM0", "FEA_FM1", "FEA_FM2", "FEA_FM3", "FEA_FM4",
      "FEA_FM5", "FEA_FM6", "FEA_FM7", "FEA_FM8"};

  const wchar_t *label = nullptr;
  auto textGet = reinterpret_cast<TextGetFn>(ADDR_TEXT_GET);
  void *theText = reinterpret_cast<void *>(ADDR_THE_TEXT);

  if (targetStation < 9) {
    label = textGet(theText, kStationTextKeys[targetStation]);
  } else if (targetStation == USERTRACK) {
    if (!mp3Avail) {
      return;
    }
    label = textGet(theText, "FEA_FM9");
  } else if (targetStation == POLICE_RADIO) {
    label = kPoliceScannerLabel;
  } else if (targetStation == RADIO_OFF) {
    label = kRadioOffLabel;
  } else {
    return;
  }

  if (currentStation != label ||
      (MusicU8(self, OFF_MUSIC_NEXT_TRACK) == USERTRACK &&
       MusicU8(self, OFF_MUSIC_PLAYING_TRACK) != USERTRACK)) {
    currentStation = label;
    displayTimer = 60;
  } else {
    if (displayTimer == 0) {
      return;
    }
    --displayTimer;
  }

  const int32_t screenW = *reinterpret_cast<const int32_t *>(ADDR_RS_MAX_WIDTH);
  const int32_t screenH =
      *reinterpret_cast<const int32_t *>(ADDR_RS_MAX_HEIGHT);
  if (screenW <= 0 || screenH <= 0) {
    return;
  }

  const float scaleX = static_cast<float>(screenW) / 640.0f;
  const float scaleY = static_cast<float>(screenH) / 448.0f;
  const float posX = static_cast<float>(screenW) * 0.5f;
  const float posY = 22.0f * scaleY;

  auto setJustifyOff = reinterpret_cast<FontVoidFn>(ADDR_FONT_SET_JUSTIFY_OFF);
  auto setBgOff = reinterpret_cast<FontVoidFn>(ADDR_FONT_SET_BACKGROUND_OFF);
  auto setScale = reinterpret_cast<FontScaleFn>(ADDR_FONT_SET_SCALE);
  auto setPropOn = reinterpret_cast<FontVoidFn>(ADDR_FONT_SET_PROP_ON);
  auto setFontStyle = reinterpret_cast<FontStyleFn>(ADDR_FONT_SET_FONT_STYLE);
  auto setCentreOn = reinterpret_cast<FontVoidFn>(ADDR_FONT_SET_CENTRE_ON);
  auto setCentreSize = reinterpret_cast<FontFloatFn>(ADDR_FONT_SET_CENTRE_SIZE);
  auto setColor = reinterpret_cast<FontColorFn>(ADDR_FONT_SET_COLOR);
  auto printString =
      reinterpret_cast<FontPrintStringFn>(ADDR_FONT_PRINT_STRING);
  auto drawFonts = reinterpret_cast<FontVoidFn>(ADDR_FONT_DRAW_FONTS);

  setJustifyOff();
  setBgOff();
  setScale(0.8f * scaleX, 1.35f * scaleY);
  setPropOn();
  setFontStyle(FONT_HEADING);
  setCentreOn();
  setCentreSize(static_cast<float>(screenW));

  const CRGBA shadow{0, 0, 0, 255};
  setColor(&shadow);
  printString(posX + 2.0f, posY + 2.0f, currentStation);

  const CRGBA fg = numRetunePresses != 0 ? CRGBA{102, 133, 143, 255}
                                         : CRGBA{147, 196, 211, 255};
  setColor(&fg);
  printString(posX, posY, currentStation);
  drawFonts();
}

void __fastcall Hook_ServiceGameMode(void *self, uintptr_t) {
  auto findPlayerVeh =
      reinterpret_cast<FindPlayerVehicleFn>(ADDR_FIND_PLAYER_VEHICLE);
  void *veh = findPlayerVeh();
  const bool inCar = Hook_PlayerInCar(self, 0);

  if (inCar && veh) {
    if (veh != g_lastPlayerVeh) {
      g_lastPlayerVeh = veh;

      const int16_t model = GetEntityModelIndex(veh);
      const bool radioSetByScript =
          MusicU8(self, OFF_MUSIC_RADIO_SET_BY_SCRIPT) != 0;

      if ((model == MI_POLICE || model == MI_AMBULAN) && !radioSetByScript) {
        auto setRadioByScript = reinterpret_cast<MusicSetRadioByScriptFn>(
            ADDR_MUSIC_SET_RADIO_BY_SCRIPT);
        setRadioByScript(self, GAME_FM, EMERGENCY_GAME_FM_POS_MS);

        *reinterpret_cast<int32_t *>(ADDR_RETUNE_COUNTER) = 0;
        *reinterpret_cast<int32_t *>(ADDR_NUM_RETUNE_PRESSES) = 0;

        MusicU8(self, OFF_MUSIC_PLAYER_IN_CAR) = 0;
        VehRadioStation(veh) = GAME_FM;
        g_currentStation = GAME_FM;
      } else if (g_currentStation != RADIO_OFF && !radioSetByScript) {
        // Seamless transition into car: vehicle tunes to our on-foot station!
        VehRadioStation(veh) = g_currentStation;
        MusicU8(self, OFF_MUSIC_NEXT_TRACK) = g_currentStation;
      } else {
        g_currentStation = VehRadioStation(veh);
      }
    } else {
      g_currentStation = VehRadioStation(veh);
    }

    if (g_pendingScrollSteps != 0) {
      *reinterpret_cast<int32_t *>(ADDR_NUM_RETUNE_PRESSES) +=
          g_pendingScrollSteps;
      *reinterpret_cast<int32_t *>(ADDR_RETUNE_COUNTER) = 30;
      auto playOneShot =
          reinterpret_cast<AudioPlayOneShotFn>(ADDR_AUDIO_PLAY_ONE_SHOT);
      playOneShot(reinterpret_cast<void *>(ADDR_AUDIO_MANAGER), 0,
                  SOUND_FRONTEND_RADIO_CHANGE, 1.0f);
      g_pendingScrollSteps = 0;
    }
  } else {
    // Player is on foot!
    g_lastPlayerVeh = nullptr;
    InstallWindowHook();

    // Query both the user's bound radio button/key from the Controls screen
    // and mouse wheel scroll steps
    auto padGet = reinterpret_cast<PadGetFn>(ADDR_PAD_GET_PAD);
    auto padChangeStation =
        reinterpret_cast<PadChangeStationFn>(ADDR_PAD_CHANGE_STATION);
    void *pad = padGet(0);
    const bool padRetune = pad && padChangeStation(pad);

    // Mouse scroll wheel on foot is reserved for weapon switching in GTA III.
    // Discard any mouse wheel steps on foot so weapon cycling is never hijacked.
    g_pendingScrollSteps = 0;
    int32_t deltaPresses = padRetune ? 1 : 0;

    if (deltaPresses != 0) {
      const bool mp3Avail = IsMP3ChannelAvailable();
      g_currentStation =
          AdvanceRadioStation(g_currentStation, deltaPresses, mp3Avail);

      *reinterpret_cast<int32_t *>(ADDR_RETUNE_COUNTER) = 30;
      *reinterpret_cast<int32_t *>(ADDR_NUM_RETUNE_PRESSES) = 0;
      *reinterpret_cast<uint8_t *>(ADDR_DISPLAY_TIMER) = 60;

      auto playOneShot =
          reinterpret_cast<AudioPlayOneShotFn>(ADDR_AUDIO_PLAY_ONE_SHOT);
      playOneShot(reinterpret_cast<void *>(ADDR_AUDIO_MANAGER), 0,
                  SOUND_FRONTEND_RADIO_CHANGE, 1.0f);
    }

    // Direct streaming track when on foot:
    // If g_currentStation != RADIO_OFF, keep streaming the chosen radio station
    if (g_currentStation != RADIO_OFF) {
      MusicU8(self, OFF_MUSIC_NEXT_TRACK) = g_currentStation;
      if (MusicU8(self, OFF_MUSIC_PLAYING_TRACK) != g_currentStation) {
        auto changeTrack = reinterpret_cast<MusicChangeRadioTrackFn>(
            ADDR_MUSIC_CHANGE_RADIO_TRACK);
        changeTrack(self, g_currentStation, 0);
        MusicU8(self, OFF_MUSIC_PLAYING_TRACK) = g_currentStation;
      }
    }
  }

  g_origServiceGameMode(self);

  // If on foot and radio is active, prevent next track from decaying to ambient
  if (!inCar && g_currentStation != RADIO_OFF) {
    MusicU8(self, OFF_MUSIC_NEXT_TRACK) = g_currentStation;
  }
}

void __fastcall Hook_ResetMusicAfterReload(void *self, uintptr_t) {
  g_lastPlayerVeh = nullptr;
  g_currentStation = RADIO_OFF;
  g_pendingScrollSteps = 0;
  *reinterpret_cast<const wchar_t **>(ADDR_CURRENT_STATION_PTR) = nullptr;
  g_origResetMusicAfterReload(self);
}

void InstallDeferredHooks() {
  if (g_deferredInstalled) {
    return;
  }
  g_deferredInstalled = true;

  ResolveMP3AvailableFn();
  InstallWindowHook();

  bool ok = true;
  ok &=
      InstallDetour(ADDR_MUSIC_RESET_AFTER_RELOAD, &Hook_ResetMusicAfterReload,
                    &g_origResetMusicAfterReload);
  ok &= InstallDetour(ADDR_MUSIC_SERVICE_GAME_MODE, &Hook_ServiceGameMode,
                      &g_origServiceGameMode);
  ok &= InstallDetour(ADDR_MUSIC_PLAYER_IN_CAR, &Hook_PlayerInCar,
                      &g_origPlayerInCar);
  ok &= InstallDetour(ADDR_MUSIC_GET_NEXT_CAR_TUNING, &Hook_GetNextCarTuning,
                      &g_origGetNextCarTuning);
  ok &= InstallDetour(ADDR_MUSIC_USES_POLICE_RADIO, &Hook_UsesPoliceRadio,
                      &g_origUsesPoliceRadio);
  ok &= InstallDetour(ADDR_MUSIC_DISPLAY_RADIO_NAME,
                      &Hook_DisplayRadioStationName,
                      &g_origDisplayRadioStationName);

  WriteLog(ok ? "EmergencyRadio hooks installed OK"
              : "EmergencyRadio hooks partial/failed");
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
