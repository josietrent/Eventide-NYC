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

// GTA III v1.0 addresses
constexpr uintptr_t ADDR_MUSIC_MANAGER = 0x8F3964;
constexpr uintptr_t ADDR_AUDIO_MANAGER = 0x880FC0;
constexpr uintptr_t ADDR_SAMPLE_MANAGER = 0x7341E0;
constexpr uintptr_t ADDR_SAMPLE_MUSIC_VOL =
    ADDR_SAMPLE_MANAGER + 0x01;                                    // 0..127
constexpr uintptr_t ADDR_SAMPLE_INIT = ADDR_SAMPLE_MANAGER + 0x56; // uint8_t
constexpr uintptr_t ADDR_DIG_DRIVER = 0x8F1A24;                    // HDIGDRIVER
constexpr uintptr_t ADDR_THE_TEXT = 0x941520;
constexpr uintptr_t ADDR_THE_CAMERA = 0x6FACF8;
constexpr uintptr_t ADDR_GAME_STATE = 0x8F5838;         // int32_t (9 = game)
constexpr uintptr_t ADDR_MENU_ACTIVE = 0x8F5AE9;        // uint8_t
constexpr uintptr_t ADDR_ACTIVATE_SAVE_MENU = 0x8F5E2B; // uint8_t
constexpr uintptr_t ADDR_MENU_SCREEN = 0x8F5F20;        // int32_t
constexpr uintptr_t ADDR_USER_PAUSE = 0x95CD7C;         // uint8_t
constexpr uintptr_t ADDR_CODE_PAUSE = 0x95CDB1;         // uint8_t
constexpr uintptr_t ADDR_REPLAY_MODE = 0x95CD5B;
constexpr uintptr_t ADDR_RS_MAX_WIDTH = 0x8F436C;
constexpr uintptr_t ADDR_RS_MAX_HEIGHT = 0x8F4370;
constexpr uintptr_t ADDR_COLLISION_IN_MEMORY = 0x941514; // 1=IND, 2=COM, 3=SUB
constexpr uintptr_t ADDR_WORLD_PLAYERS = 0x9412F0;   // CPlayerInfo Players[]
constexpr uintptr_t ADDR_PLAYER_IN_FOCUS = 0x95CD61; // uint8_t

constexpr uintptr_t ADDR_NUM_RETUNE_PRESSES = 0x650B80;
constexpr uintptr_t ADDR_RETUNE_COUNTER = 0x650B84;
constexpr uintptr_t ADDR_CURRENT_STATION_PTR = 0x650B9C;
constexpr uintptr_t ADDR_DISPLAY_TIMER = 0x650BA1;

// Miles Sound System (Mss32.dll) IAT pointers in GTA III v1.0
constexpr uintptr_t IAT_AIL_OPEN_STREAM = 0x61D5C8;
constexpr uintptr_t IAT_AIL_STREAM_MS_POSITION = 0x61D5CC;
constexpr uintptr_t IAT_AIL_CLOSE_STREAM = 0x61D5D0;
constexpr uintptr_t IAT_AIL_PAUSE_STREAM = 0x61D5FC;
constexpr uintptr_t IAT_AIL_SET_STREAM_LOOP_COUNT = 0x61D65C;
constexpr uintptr_t IAT_AIL_SERVICE_STREAM = 0x61D660;
constexpr uintptr_t IAT_AIL_START_STREAM = 0x61D664;
constexpr uintptr_t IAT_AIL_SET_STREAM_MS_POSITION = 0x61D668;
constexpr uintptr_t IAT_AIL_SET_STREAM_VOLUME = 0x61D66C;
constexpr uintptr_t IAT_AIL_STREAM_STATUS = 0x61D674;

// Functions
constexpr uintptr_t ADDR_FIND_PLAYER_VEHICLE = 0x4A10C0;
constexpr uintptr_t ADDR_FIND_PLAYER_PED = 0x4A1150;
constexpr uintptr_t ADDR_TEXT_GET = 0x52C5A0;
constexpr uintptr_t ADDR_SAMPLE_IS_MP3_AVAILABLE = 0x566490;
constexpr uintptr_t ADDR_SAMPLE_TERMINATE = 0x566DC0;
constexpr uintptr_t ADDR_SAMPLE_SET_STREAMED_VOL = 0x5681D0;
constexpr uintptr_t ADDR_AUDIO_SERVICE = 0x57AA60;

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
constexpr uintptr_t ADDR_MUSIC_SERVICE_AMBIENCE = 0x57DCB0;
constexpr uintptr_t ADDR_MUSIC_SERVICE_VOLUME = 0x57E100;
constexpr uintptr_t ADDR_MUSIC_SERVICE_TRACK = 0x57E130;
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
constexpr size_t SIZEOF_PLAYER_INFO = 0x13C;
constexpr size_t OFF_PLAYERINFO_WB_STATE =
    0xD8; // uint8_t (0=playing, 1=wasted, 2=busted)

constexpr size_t OFF_MUSIC_NEXT_TRACK = 0x03;
constexpr size_t OFF_MUSIC_PLAYING_TRACK = 0x04;
constexpr size_t OFF_MUSIC_PREV_PLAYER_IN_CAR = 0x09;
constexpr size_t OFF_MUSIC_PLAYER_IN_CAR = 0x0A;
constexpr size_t OFF_MUSIC_RADIO_SET_BY_SCRIPT = 0x94F;

constexpr size_t OFF_CAM_WIDESCREEN = 0x70;
constexpr size_t OFF_PAD_DISABLE_CONTROLS = 0xDF;

constexpr size_t OFF_ENTITY_POS = 0x34; // float[3] m_matrix.pos
constexpr size_t OFF_ENTITY_STATUS_BYTE = 0x50;
constexpr size_t OFF_ENTITY_MODEL_INDEX = 0x5C;
constexpr size_t OFF_PED_STATE = 0x224;
constexpr size_t OFF_PED_HEALTH = 0x2C0; // float
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
constexpr uint8_t WLSU_RADIO = 13;

constexpr int32_t GS_PLAYING_GAME = 9;
constexpr int32_t MENUPAGE_SOUND_SETTINGS = 5;
constexpr int32_t LEVEL_INDUSTRIAL = 1;
constexpr int32_t LEVEL_COMMERCIAL = 2;
constexpr int32_t SMP_DONE = 2;

constexpr uint8_t WBSTATE_WASTED = 1;
constexpr uint8_t WBSTATE_BUSTED = 2;

constexpr int16_t MI_AMBULAN = 106;
constexpr int16_t MI_POLICE = 116;

constexpr uint8_t STATUS_WRECKED = 5;
constexpr int32_t PEDSTATE_DIE = 48;
constexpr int32_t PEDSTATE_DEAD = 49;
constexpr int32_t PEDSTATE_DRAG_FROM_CAR = 51;
constexpr int32_t PEDSTATE_EXIT_CAR = 54;
constexpr int32_t PEDSTATE_ARRESTED = 56;
constexpr uint8_t REPLAY_MODE_PLAYBACK = 1;
constexpr int16_t FONT_HEADING = 2;

// 18:39 into GAME.wav (in milliseconds)
constexpr int32_t EMERGENCY_GAME_FM_POS_MS = (18 * 60 + 39) * 1000;

// WLSU 88.5 transmitter coordinates (rooftop antenna on Liberty Campus /
// newuni_gz)
constexpr float WLSU_TX_X = 179.6f;
constexpr float WLSU_TX_Y = -300.0f;

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
using AudioServiceFn = void(__thiscall *)(void *self);
using SampleTerminateFn = void(__thiscall *)(void *self);
using SampleIsMP3AvailFn = bool(__thiscall *)(void *self);
using SampleSetStreamedVolFn = void(__thiscall *)(void *self, uint8_t vol,
                                                  uint8_t pan,
                                                  uint8_t isAmbience,
                                                  int32_t channel);

using AilOpenStreamFn = uint32_t(__stdcall *)(uint32_t digDriver,
                                              const char *filename,
                                              int32_t streamMem);
using AilCloseStreamFn = void(__stdcall *)(uint32_t stream);
using AilPauseStreamFn = void(__stdcall *)(uint32_t stream, int32_t onoff);
using AilServiceStreamFn = void(__stdcall *)(uint32_t stream, int32_t fillup);
using AilStartStreamFn = void(__stdcall *)(uint32_t stream);
using AilStreamStatusFn = int32_t(__stdcall *)(uint32_t stream);
using AilSetStreamLoopCountFn = void(__stdcall *)(uint32_t stream,
                                                  int32_t count);
using AilStreamMsPositionFn = void(__stdcall *)(uint32_t stream,
                                                int32_t *totalMs,
                                                int32_t *currentMs);
using AilSetStreamMsPositionFn = void(__stdcall *)(uint32_t stream,
                                                   int32_t posMs);
using AilSetStreamVolumeFn = void(__stdcall *)(uint32_t stream, int32_t volume);
using AilLastErrorFn = const char *(__stdcall *)();

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

std::string g_asiDir;
std::string g_exeDir;
std::string g_logPath;
std::vector<HookEntry> g_hooks;
bool g_deferredInstalled = false;

CRITICAL_SECTION g_wlsuLock;
bool g_wlsuLockInit = false;
volatile bool g_shuttingDown = false;
bool g_windowInactive = false;
bool g_focusLost = false;

MusicBoolFn g_origInitialise = nullptr;
MusicVoidFn g_origResetMusicAfterReload = nullptr;
MusicVoidFn g_origServiceGameMode = nullptr;
MusicVoidFn g_origServiceAmbience = nullptr;
MusicBoolFn g_origPlayerInCar = nullptr;
MusicUint8Fn g_origGetCarTuning = nullptr;
MusicUint8Fn g_origGetNextCarTuning = nullptr;
MusicUsesPoliceRadioFn g_origUsesPoliceRadio = nullptr;
MusicVoidFn g_origDisplayRadioStationName = nullptr;
AudioServiceFn g_origAudioService = nullptr;
SampleTerminateFn g_origSampleTerminate = nullptr;

void *g_lastPlayerVeh = nullptr;
uint8_t g_currentStation = RADIO_OFF;
bool g_emergencyStationApplied = false;
bool g_wasWastedOrBusted = false;
bool g_onMansionRoof = false;
int32_t g_onFootRadioFade = 100;
DWORD g_lastFadeTick = 0;
int32_t g_pendingScrollSteps = 0;
int32_t g_wrappedRetuneBonus = 0;
WNDPROC g_origWndProc = nullptr;
HWND g_gameHwnd = nullptr;

// WLSU 88.5 dual-stream Miles state
uint32_t g_wlsuStream = 0;
uint32_t g_wlsuStaticStream = 0;
bool g_wlsuPaused = false;
int32_t g_wlsuLastProgVol = -1;
int32_t g_wlsuLastStaticVol = -1;
int32_t g_wlsuSavedPosMs = 0;
int32_t g_wlsuTotalMs = 0;
bool g_wlsuHasSavedPos = false;
uint64_t g_wlsuActivePlayMs = 0;
uint64_t g_wlsuAnchorPlayMs = 0;
DWORD g_wlsuLastServiceTick = 0;
DWORD g_wlsuProgRetryTick = 0;
DWORD g_wlsuStaticRetryTick = 0;
DWORD g_wlsuResolveRetryTick = 0;
std::string g_wlsuProgPath;
std::string g_wlsuStaticPath;

const wchar_t kPoliceScannerLabel[] = L"Police Scanner";
const wchar_t kRadioOffLabel[] = L"Radio Off";
const wchar_t kWlsuLabel[] = L"WLSU 88.5";

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

std::string DirOfPath(const std::string &fullPath) {
  const size_t pos = fullPath.find_last_of("\\/");
  if (pos == std::string::npos) {
    return std::string();
  }
  return fullPath.substr(0, pos);
}

void InitLogPath(HMODULE hModule) {
  char buf[MAX_PATH] = {};
  if (GetModuleFileNameA(hModule, buf, MAX_PATH) > 0) {
    g_asiDir = DirOfPath(buf);
  }
  if (GetModuleFileNameA(nullptr, buf, MAX_PATH) > 0) {
    g_exeDir = DirOfPath(buf);
  }
  if (!g_exeDir.empty()) {
    g_logPath = g_exeDir + "\\EmergencyRadio.log";
  } else if (!g_asiDir.empty()) {
    g_logPath = g_asiDir + "\\EmergencyRadio.log";
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

std::string ResolveAudioAsset(const char *fileName) {
  if (!fileName || !fileName[0]) {
    return std::string();
  }
  if (!g_asiDir.empty()) {
    const std::string cands[] = {
        g_asiDir + "\\audio\\EC\\" + fileName,
        g_asiDir + "\\audio\\" + fileName,
        g_asiDir + "\\" + fileName,
    };
    for (const auto &cand : cands) {
      if (GetFileAttributesA(cand.c_str()) != INVALID_FILE_ATTRIBUTES) {
        return cand;
      }
    }
  }
  if (!g_exeDir.empty()) {
    const std::string cands[] = {
        g_exeDir + "\\audio\\EC\\" + fileName,
        g_exeDir + "\\audio\\" + fileName,
    };
    for (const auto &cand : cands) {
      if (GetFileAttributesA(cand.c_str()) != INVALID_FILE_ATTRIBUTES) {
        return cand;
      }
    }
  }
  const std::string relCands[] = {
      std::string("audio\\EC\\") + fileName,
      std::string("audio\\") + fileName,
  };
  for (const auto &rel : relCands) {
    if (GetFileAttributesA(rel.c_str()) != INVALID_FILE_ATTRIBUTES) {
      return rel;
    }
  }
  return std::string();
}

void EnsureAudioPathsResolved(DWORD now) {
  if (!g_wlsuProgPath.empty() && !g_wlsuStaticPath.empty()) {
    return;
  }
  if (now < g_wlsuResolveRetryTick) {
    return;
  }
  g_wlsuResolveRetryTick = now + 1000;
  if (g_wlsuProgPath.empty()) {
    g_wlsuProgPath = ResolveAudioAsset("WLSU.mp3");
    if (!g_wlsuProgPath.empty()) {
      std::string msg = "Resolved WLSU program: " + g_wlsuProgPath;
      WriteLog(msg.c_str());
    }
  }
  if (g_wlsuStaticPath.empty()) {
    g_wlsuStaticPath = ResolveAudioAsset("WLSU_static.wav");
    if (!g_wlsuStaticPath.empty()) {
      std::string msg = "Resolved WLSU static: " + g_wlsuStaticPath;
      WriteLog(msg.c_str());
    }
  }
}

inline uint32_t AilOpenStream(uint32_t digDriver, const char *filename) {
  auto fn = *reinterpret_cast<const AilOpenStreamFn *>(IAT_AIL_OPEN_STREAM);
  return (fn && digDriver && filename) ? fn(digDriver, filename, 0) : 0;
}

inline void AilCloseStream(uint32_t stream) {
  auto fn = *reinterpret_cast<const AilCloseStreamFn *>(IAT_AIL_CLOSE_STREAM);
  if (fn && stream) {
    fn(stream);
  }
}

inline void AilPauseStream(uint32_t stream, int32_t pause) {
  auto fn = *reinterpret_cast<const AilPauseStreamFn *>(IAT_AIL_PAUSE_STREAM);
  if (fn && stream) {
    fn(stream, pause);
  }
}

inline void AilStartStream(uint32_t stream) {
  auto fn = *reinterpret_cast<const AilStartStreamFn *>(IAT_AIL_START_STREAM);
  if (fn && stream) {
    fn(stream);
  }
}

inline void AilServiceStream(uint32_t stream, int32_t fillup) {
  auto fn =
      *reinterpret_cast<const AilServiceStreamFn *>(IAT_AIL_SERVICE_STREAM);
  if (fn && stream) {
    fn(stream, fillup);
  }
}

inline int32_t AilStreamStatus(uint32_t stream) {
  auto fn = *reinterpret_cast<const AilStreamStatusFn *>(IAT_AIL_STREAM_STATUS);
  return (fn && stream) ? fn(stream) : 0;
}

inline void AilSetStreamLoopCount(uint32_t stream, int32_t count) {
  auto fn = *reinterpret_cast<const AilSetStreamLoopCountFn *>(
      IAT_AIL_SET_STREAM_LOOP_COUNT);
  if (fn && stream) {
    fn(stream, count);
  }
}

inline bool AilStreamMsPosition(uint32_t stream, int32_t *totalMs,
                                int32_t *currentMs) {
  auto fn = *reinterpret_cast<const AilStreamMsPositionFn *>(
      IAT_AIL_STREAM_MS_POSITION);
  if (!fn || !stream) {
    return false;
  }
  fn(stream, totalMs, currentMs);
  return true;
}

inline void AilSetStreamMsPosition(uint32_t stream, int32_t posMs) {
  auto fn = *reinterpret_cast<const AilSetStreamMsPositionFn *>(
      IAT_AIL_SET_STREAM_MS_POSITION);
  if (fn && stream) {
    fn(stream, posMs);
  }
}

inline void AilSetStreamVolume(uint32_t stream, int32_t volume) {
  auto fn = *reinterpret_cast<const AilSetStreamVolumeFn *>(
      IAT_AIL_SET_STREAM_VOLUME);
  if (fn && stream) {
    fn(stream, volume);
  }
}

const char *GetAilLastError() {
  HMODULE mss = GetModuleHandleA("Mss32.dll");
  if (!mss) {
    return "Mss32.dll not loaded";
  }
  auto fn = reinterpret_cast<AilLastErrorFn>(
      GetProcAddress(mss, "_AIL_last_error@0"));
  if (!fn) {
    return "AIL_last_error unavailable";
  }
  const char *err = fn();
  return (err && err[0]) ? err : "(empty)";
}

bool IsMilesReady() {
  return (*reinterpret_cast<const uint8_t *>(ADDR_SAMPLE_INIT) != 0) &&
         (*reinterpret_cast<const uint32_t *>(ADDR_DIG_DRIVER) != 0);
}

void ResetWlsuStreamHandles(bool clearSavedPos) {
  g_wlsuStream = 0;
  g_wlsuStaticStream = 0;
  g_wlsuPaused = false;
  g_wlsuLastProgVol = -1;
  g_wlsuLastStaticVol = -1;
  if (clearSavedPos) {
    g_wlsuSavedPosMs = 0;
    g_wlsuTotalMs = 0;
    g_wlsuHasSavedPos = false;
    g_wlsuAnchorPlayMs = 0;
  }
}

void CloseWlsuStreams(bool savePosition) {
  if (g_wlsuStream == 0 && g_wlsuStaticStream == 0) {
    if (!savePosition) {
      ResetWlsuStreamHandles(true);
    }
    return;
  }
  if (!IsMilesReady()) {
    ResetWlsuStreamHandles(!savePosition);
    return;
  }

  if (g_wlsuStream != 0) {
    if (savePosition) {
      int32_t totalMs = 0;
      int32_t curMs = -1;
      if (AilStreamMsPosition(g_wlsuStream, &totalMs, &curMs) && curMs >= 0) {
        g_wlsuSavedPosMs = curMs;
        g_wlsuTotalMs = totalMs;
        g_wlsuAnchorPlayMs = g_wlsuActivePlayMs;
        g_wlsuHasSavedPos = true;
      }
    }
    AilSetStreamVolume(g_wlsuStream, 0);
    AilPauseStream(g_wlsuStream, 1);
    AilCloseStream(g_wlsuStream);
  }
  if (g_wlsuStaticStream != 0) {
    AilSetStreamVolume(g_wlsuStaticStream, 0);
    AilPauseStream(g_wlsuStaticStream, 1);
    AilCloseStream(g_wlsuStaticStream);
  }

  ResetWlsuStreamHandles(!savePosition);
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

void UpdateWlsuStreams();

LRESULT CALLBACK SubclassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                 LPARAM lParam) {
  WNDPROC origProc = g_origWndProc;

  if (uMsg == WM_DESTROY || uMsg == WM_NCDESTROY) {
    g_shuttingDown = true;
    if (g_wlsuLockInit) {
      EnterCriticalSection(&g_wlsuLock);
      CloseWlsuStreams(false);
      LeaveCriticalSection(&g_wlsuLock);
    }
    if (origProc && GetWindowLongPtrA(hwnd, GWLP_WNDPROC) ==
                        reinterpret_cast<LONG_PTR>(SubclassWndProc)) {
      SetWindowLongPtrA(hwnd, GWLP_WNDPROC,
                        reinterpret_cast<LONG_PTR>(origProc));
    }
    g_origWndProc = nullptr;
    g_gameHwnd = nullptr;
    return CallWindowProcA(origProc, hwnd, uMsg, wParam, lParam);
  }

  if (uMsg == WM_MOUSEWHEEL) {
    const short delta = GET_WHEEL_DELTA_WPARAM(wParam);
    if (delta > 0) {
      g_pendingScrollSteps += 1;
    } else if (delta < 0) {
      g_pendingScrollSteps -= 1;
    }
  } else if (!g_shuttingDown) {
    if (uMsg == WM_ACTIVATEAPP) {
      g_windowInactive = (wParam == FALSE);
      UpdateWlsuStreams();
    } else if (uMsg == WM_ACTIVATE) {
      g_windowInactive = (LOWORD(wParam) == WA_INACTIVE);
      UpdateWlsuStreams();
    } else if (uMsg == WM_KILLFOCUS) {
      g_focusLost = true;
      UpdateWlsuStreams();
    } else if (uMsg == WM_SETFOCUS) {
      g_focusLost = false;
      UpdateWlsuStreams();
    }
  }
  return CallWindowProcA(origProc, hwnd, uMsg, wParam, lParam);
}

HWND GetGameHwnd() {
  HWND hwnd = nullptr;
  EnumWindows(
      [](HWND top, LPARAM lp) -> BOOL {
        DWORD pid = 0;
        GetWindowThreadProcessId(top, &pid);
        if (pid == GetCurrentProcessId() && IsWindowVisible(top)) {
          *reinterpret_cast<HWND *>(lp) = top;
          return FALSE;
        }
        return TRUE;
      },
      reinterpret_cast<LPARAM>(&hwnd));
  return hwnd;
}

void InstallWindowHook() {
  if (g_origWndProc) {
    return;
  }
  HWND hwnd = GetGameHwnd();
  if (hwnd) {
    g_gameHwnd = hwnd;
    g_origWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(
        hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(SubclassWndProc)));
    if (g_origWndProc) {
      WriteLog("WindowProc subclass installed OK");
    }
  }
}

void RemoveWindowHook() {
  if (g_origWndProc && g_gameHwnd && IsWindow(g_gameHwnd)) {
    if (GetWindowLongPtrA(g_gameHwnd, GWLP_WNDPROC) ==
        reinterpret_cast<LONG_PTR>(SubclassWndProc)) {
      SetWindowLongPtrA(g_gameHwnd, GWLP_WNDPROC,
                        reinterpret_cast<LONG_PTR>(g_origWndProc));
    }
    g_origWndProc = nullptr;
    g_gameHwnd = nullptr;
  }
}

void RemoveAllHooks() {
  g_shuttingDown = true;
  if (g_wlsuLockInit) {
    EnterCriticalSection(&g_wlsuLock);
    CloseWlsuStreams(false);
    LeaveCriticalSection(&g_wlsuLock);
  }
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

bool IsMP3ChannelAvailable() {
  auto isMP3Avail =
      reinterpret_cast<SampleIsMP3AvailFn>(ADDR_SAMPLE_IS_MP3_AVAILABLE);
  return isMP3Avail(reinterpret_cast<void *>(ADDR_SAMPLE_MANAGER));
}

inline uint8_t &MusicU8(void *self, size_t off) {
  return *reinterpret_cast<uint8_t *>(reinterpret_cast<uintptr_t>(self) + off);
}

inline float GetEntityPosX(const void *entity) {
  return *reinterpret_cast<const float *>(reinterpret_cast<uintptr_t>(entity) +
                                          OFF_ENTITY_POS);
}

inline float GetEntityPosY(const void *entity) {
  return *reinterpret_cast<const float *>(reinterpret_cast<uintptr_t>(entity) +
                                          OFF_ENTITY_POS + sizeof(float));
}

inline float GetEntityPosZ(const void *entity) {
  return *reinterpret_cast<const float *>(reinterpret_cast<uintptr_t>(entity) +
                                          OFF_ENTITY_POS + 2 * sizeof(float));
}

bool IsPlayerOnMansionRoof() {
  if (*reinterpret_cast<const int32_t *>(ADDR_COLLISION_IN_MEMORY) !=
      LEVEL_INDUSTRIAL) {
    g_onMansionRoof = false;
    return false;
  }

  auto findPlayerVeh =
      reinterpret_cast<FindPlayerVehicleFn>(ADDR_FIND_PLAYER_VEHICLE);
  auto findPlayerPed = reinterpret_cast<FindPlayerPedFn>(ADDR_FIND_PLAYER_PED);
  void *entity = findPlayerVeh();
  if (!entity) {
    entity = findPlayerPed();
  }
  if (!entity) {
    g_onMansionRoof = false;
    return false;
  }

  const float px = GetEntityPosX(entity);
  const float py = GetEntityPosY(entity);
  const float pz = GetEntityPosZ(entity);

  if (!g_onMansionRoof) {
    g_onMansionRoof = (px >= 1448.0f && px <= 1492.0f && py >= -202.0f &&
                       py <= -162.0f && pz >= 59.5f);
  } else {
    g_onMansionRoof = (px >= 1446.5f && px <= 1493.5f && py >= -203.5f &&
                       py <= -160.5f && pz >= 58.6f);
  }
  return g_onMansionRoof;
}

bool IsWlsuAvailable() {
  if (*reinterpret_cast<const int32_t *>(ADDR_COLLISION_IN_MEMORY) ==
      LEVEL_COMMERCIAL) {
    g_onMansionRoof = false;
    return true;
  }
  return IsPlayerOnMansionRoof();
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

inline float GetPedHealth(const void *ped) {
  return *reinterpret_cast<const float *>(reinterpret_cast<uintptr_t>(ped) +
                                          OFF_PED_HEALTH);
}

bool IsPlayerWastedOrBusted() {
  const uint8_t focus =
      *reinterpret_cast<const uint8_t *>(ADDR_PLAYER_IN_FOCUS);
  if (focus < 2) {
    const uintptr_t playerInfo =
        ADDR_WORLD_PLAYERS + static_cast<size_t>(focus) * SIZEOF_PLAYER_INFO;
    const uint8_t wbState = *reinterpret_cast<const uint8_t *>(
        playerInfo + OFF_PLAYERINFO_WB_STATE);
    if (wbState == WBSTATE_WASTED || wbState == WBSTATE_BUSTED) {
      return true;
    }
  }

  auto findPlayerPed = reinterpret_cast<FindPlayerPedFn>(ADDR_FIND_PLAYER_PED);
  void *ped = findPlayerPed();
  if (ped) {
    const int32_t pedState = GetPedState(ped);
    if (pedState == PEDSTATE_DIE || pedState == PEDSTATE_DEAD ||
        pedState == PEDSTATE_ARRESTED) {
      return true;
    }
    if (GetPedHealth(ped) <= 0.0f) {
      return true;
    }
  }
  return false;
}

inline uint8_t &VehRadioStation(void *veh) {
  return *reinterpret_cast<uint8_t *>(reinterpret_cast<uintptr_t>(veh) +
                                      OFF_VEH_RADIO_STATION);
}

uint8_t AdvanceRadioStation(uint8_t current, int32_t presses, bool mp3Avail,
                            bool wlsuAvail) {
  static constexpr uint8_t kStationsNoMP3NoWlsu[] = {
      HEAD_RADIO, DOUBLE_CLEF, JAH_RADIO,  RISE_FM,      LIPS_106, GAME_FM,
      MSX_FM,     FLASHBACK,   CHATTERBOX, POLICE_RADIO, RADIO_OFF};
  static constexpr uint8_t kStationsWithMP3NoWlsu[] = {
      HEAD_RADIO, DOUBLE_CLEF, JAH_RADIO,  RISE_FM,   LIPS_106,     GAME_FM,
      MSX_FM,     FLASHBACK,   CHATTERBOX, USERTRACK, POLICE_RADIO, RADIO_OFF};
  static constexpr uint8_t kStationsNoMP3WithWlsu[] = {
      HEAD_RADIO, DOUBLE_CLEF, JAH_RADIO,  RISE_FM,    LIPS_106,     GAME_FM,
      MSX_FM,     FLASHBACK,   CHATTERBOX, WLSU_RADIO, POLICE_RADIO, RADIO_OFF};
  static constexpr uint8_t kStationsWithMP3WithWlsu[] = {
      HEAD_RADIO, DOUBLE_CLEF,  JAH_RADIO, RISE_FM,    LIPS_106,
      GAME_FM,    MSX_FM,       FLASHBACK, CHATTERBOX, WLSU_RADIO,
      USERTRACK,  POLICE_RADIO, RADIO_OFF};

  const uint8_t *list = nullptr;
  int32_t count = 0;
  if (wlsuAvail) {
    list = mp3Avail ? kStationsWithMP3WithWlsu : kStationsNoMP3WithWlsu;
    count = static_cast<int32_t>(mp3Avail ? sizeof(kStationsWithMP3WithWlsu)
                                          : sizeof(kStationsNoMP3WithWlsu));
  } else {
    list = mp3Avail ? kStationsWithMP3NoWlsu : kStationsNoMP3NoWlsu;
    count = static_cast<int32_t>(mp3Avail ? sizeof(kStationsWithMP3NoWlsu)
                                          : sizeof(kStationsNoMP3NoWlsu));
    if (current == WLSU_RADIO) {
      current = CHATTERBOX;
    }
  }

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
  if (IsPlayerWastedOrBusted()) {
    return false;
  }

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

uint8_t __fastcall Hook_GetCarTuning(void *self, uintptr_t) {
  const uint8_t orig = g_origGetCarTuning(self);
  auto findPlayerVeh =
      reinterpret_cast<FindPlayerVehicleFn>(ADDR_FIND_PLAYER_VEHICLE);
  void *veh = findPlayerVeh();
  if (!veh) {
    return orig;
  }
  uint8_t &station = VehRadioStation(veh);
  if (station == WLSU_RADIO) {
    if (!IsWlsuAvailable()) {
      station =
          AdvanceRadioStation(WLSU_RADIO, 1, IsMP3ChannelAvailable(), false);
      g_currentStation = station;
      return station;
    }
    return WLSU_RADIO;
  }
  return orig;
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
  const int32_t totalPresses = numRetunePresses + g_wrappedRetuneBonus;
  g_wrappedRetuneBonus = 0;

  const bool wlsuAvail = IsWlsuAvailable();
  const bool mp3Avail = IsMP3ChannelAvailable();
  if (totalPresses != 0) {
    station = AdvanceRadioStation(station, totalPresses, mp3Avail, wlsuAvail);
    numRetunePresses = 0;
  } else if (station == WLSU_RADIO && !wlsuAvail) {
    station = AdvanceRadioStation(WLSU_RADIO, 1, mp3Avail, false);
  }
  g_currentStation = station;
  return station;
}

void __fastcall Hook_DisplayRadioStationName(void *self, uintptr_t) {
  if (IsPlayerWastedOrBusted()) {
    *reinterpret_cast<uint8_t *>(ADDR_DISPLAY_TIMER) = 0;
    *reinterpret_cast<const wchar_t **>(ADDR_CURRENT_STATION_PTR) = nullptr;
    return;
  }

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

  uint8_t curRadio = g_currentStation;
  if (inCar) {
    auto findPlayerVeh =
        reinterpret_cast<FindPlayerVehicleFn>(ADDR_FIND_PLAYER_VEHICLE);
    void *veh = findPlayerVeh();
    if (veh) {
      curRadio = VehRadioStation(veh);
    } else {
      curRadio = MusicU8(self, OFF_MUSIC_NEXT_TRACK);
    }
  }
  if (curRadio == STREAMED_SOUND_CITY_AMBIENT ||
      curRadio == STREAMED_SOUND_WATER_AMBIENT) {
    curRadio = RADIO_OFF;
  } else if (curRadio > STREAMED_SOUND_WATER_AMBIENT &&
             curRadio != WLSU_RADIO) {
    return;
  }

  const int32_t numRetunePresses =
      *reinterpret_cast<const int32_t *>(ADDR_NUM_RETUNE_PRESSES);
  const bool mp3Avail = IsMP3ChannelAvailable();
  const bool wlsuAvail = IsWlsuAvailable();
  const uint8_t targetStation =
      inCar
          ? AdvanceRadioStation(curRadio, numRetunePresses, mp3Avail, wlsuAvail)
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
  } else if (targetStation == WLSU_RADIO) {
    label = kWlsuLabel;
  } else {
    return;
  }

  const uint8_t nextNative = MusicU8(self, OFF_MUSIC_NEXT_TRACK);
  const uint8_t playingNative = MusicU8(self, OFF_MUSIC_PLAYING_TRACK);
  if (currentStation != label ||
      (nextNative == USERTRACK && playingNative != USERTRACK)) {
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

bool IsCutsceneActive() {
  const bool wideScreen = *reinterpret_cast<const uint8_t *>(
                              ADDR_THE_CAMERA + OFF_CAM_WIDESCREEN) != 0;
  if (wideScreen) {
    return true;
  }
  auto padGet = reinterpret_cast<PadGetFn>(ADDR_PAD_GET_PAD);
  void *pad = padGet(0);
  return pad &&
         (*reinterpret_cast<const uint8_t *>(reinterpret_cast<uintptr_t>(pad) +
                                             OFF_PAD_DISABLE_CONTROLS) != 0);
}

bool IsWlsuPausedByGameState() {
  if (*reinterpret_cast<const int32_t *>(ADDR_GAME_STATE) != GS_PLAYING_GAME) {
    return true;
  }
  const bool fgLost =
      (g_gameHwnd != nullptr) && (GetForegroundWindow() != g_gameHwnd);
  if (g_windowInactive || g_focusLost || fgLost) {
    return true;
  }
  const uint8_t userPause = *reinterpret_cast<const uint8_t *>(ADDR_USER_PAUSE);
  const uint8_t codePause = *reinterpret_cast<const uint8_t *>(ADDR_CODE_PAUSE);
  const uint8_t menuActive =
      *reinterpret_cast<const uint8_t *>(ADDR_MENU_ACTIVE);
  const uint8_t activateSaveMenu =
      *reinterpret_cast<const uint8_t *>(ADDR_ACTIVATE_SAVE_MENU);
  const int32_t menuScreen =
      *reinterpret_cast<const int32_t *>(ADDR_MENU_SCREEN);

  if (codePause != 0 || activateSaveMenu != 0) {
    return true;
  }
  if (menuActive != 0 && menuScreen == MENUPAGE_SOUND_SETTINGS) {
    return false;
  }
  return (userPause != 0) || (menuActive != 0);
}

constexpr float kWlsuTunnelKnee = 0.85f;

float ComputeWlsuTunnelDepth(float x, float y, float z) {
  // Staunton Porter Tunnel centerline from north portal opening (P0) down the
  // ramp and underground loop to the Shoreside/Portland T-junction corner (P10)
  // right beside $BarrierTunnelObj at (532.38, 99.51, -20.12).
  struct TunnelPt {
    float x;
    float y;
    float zDeck;
  };
  static constexpr TunnelPt kPts[] = {
      {289.15f, 13.18f, 8.18f},     // P0: tunl_2_cus1 north portal opening
      {377.30f, -166.21f, -21.03f}, // P1: bottom of straight ramp
      {404.47f, -201.84f, -21.03f}, // P2: tunl_2_bend #1 apex
      {443.10f, -224.40f, -21.03f}, // P3: tunl_2_bend #1 east exit
      {530.88f, -219.05f, -21.03f}, // P4: tunl_2_bend #3 west entry
      {566.46f, -191.92f, -21.03f}, // P5: tunl_2_bend #3 apex
      {589.02f, -153.29f, -21.03f}, // P6: tunl_2 south end
      {585.58f, 16.10f, -21.03f},   // P7: tunl_2_cus3 north end
      {576.78f, 59.98f, -21.03f},   // P8: tunl_2_bend #2 apex
      {551.91f, 97.21f, -21.03f},   // P9: tunl_t2 south stem
      {541.21f, 108.08f, -21.03f},  // P10: tunl_t2 T-junction center
  };
  constexpr int kNumPts = static_cast<int>(sizeof(kPts) / sizeof(kPts[0]));
  constexpr float kHalfWidth = 22.0f;
  constexpr float kRampEndS = 715.0f;
  constexpr float kCutoffS = 738.0f;

  float bestDistSq = kHalfWidth * kHalfWidth;
  float bestS = 0.0f;
  bool inCorridor = false;

  if (z <= 10.5f) {
    float cumS = 0.0f;
    for (int i = 0; i < kNumPts - 1; ++i) {
      const float x0 = kPts[i].x;
      const float y0 = kPts[i].y;
      const float z0 = kPts[i].zDeck;
      const float sx = kPts[i + 1].x - x0;
      const float sy = kPts[i + 1].y - y0;
      const float lenSq = sx * sx + sy * sy;
      const float segLen = std::sqrt(lenSq);

      float t = ((x - x0) * sx + (y - y0) * sy) / lenSq;
      if (t < 0.0f) {
        t = 0.0f;
      } else if (t > 1.0f) {
        t = 1.0f;
      }

      const float px = x0 + t * sx;
      const float py = y0 + t * sy;
      const float pzDeck = z0 + t * (kPts[i + 1].zDeck - z0);
      const float dx = x - px;
      const float dy = y - py;
      const float dSq = dx * dx + dy * dy;

      // Require elevation inside the tunnel tube so neither surface roads above
      // nor the Shoreside Z=-21 crossover tunnel underneath match P0..P1.
      if (dSq <= bestDistSq && z >= pzDeck - 2.5f && z <= pzDeck + 4.5f) {
        bestDistSq = dSq;
        bestS = cumS + t * segLen;
        inCorridor = true;
      }

      cumS += segLen;
    }
  }

  if (inCorridor) {
    if (bestS <= 0.0f) {
      return 0.0f;
    }
    if (bestS < kRampEndS) {
      return kWlsuTunnelKnee * (bestS / kRampEndS);
    }
    if (bestS < kCutoffS) {
      return kWlsuTunnelKnee +
             (1.0f - kWlsuTunnelKnee) *
                 ((bestS - kRampEndS) / (kCutoffS - kRampEndS));
    }
    return 1.0f;
  }

  // Fallback for other deep underground spaces (Subway tubes,
  // Shoreside/Portland Porter Tunnel branches past the T-junction at Z = -21).
  if (z < -2.5f) {
    float d = (-2.5f - z) / 8.0f;
    if (d < 0.0f) {
      d = 0.0f;
    } else if (d > 1.0f) {
      d = 1.0f;
    }
    return d;
  }

  return 0.0f;
}

void ComputeWlsuSignalMix(float *outProgGain, float *outStaticGain) {
  auto findPlayerVeh =
      reinterpret_cast<FindPlayerVehicleFn>(ADDR_FIND_PLAYER_VEHICLE);
  auto findPlayerPed = reinterpret_cast<FindPlayerPedFn>(ADDR_FIND_PLAYER_PED);

  void *entity = findPlayerVeh();
  if (!entity) {
    entity = findPlayerPed();
  }
  if (!entity) {
    *outProgGain = 1.0f;
    *outStaticGain = 0.0f;
    return;
  }

  const float px = GetEntityPosX(entity);
  const float py = GetEntityPosY(entity);
  const float pz = GetEntityPosZ(entity);

  if (*reinterpret_cast<const int32_t *>(ADDR_COLLISION_IN_MEMORY) !=
          LEVEL_COMMERCIAL &&
      IsPlayerOnMansionRoof()) {
    // Elevated line-of-sight fringe pickup from Salvatore's Mansion Roof
    // (~1.3 km across the river from the Liberty Campus rooftop antenna).
    float elev = (pz - 59.5f) / 12.0f;
    if (elev < 0.0f) {
      elev = 0.0f;
    } else if (elev > 1.0f) {
      elev = 1.0f;
    }
    *outProgGain = 0.19f + 0.09f * elev;
    *outStaticGain = 0.88f - 0.10f * elev;
    return;
  }

  const float dx = px - WLSU_TX_X;
  const float dy = py - WLSU_TX_Y;
  const float dist = std::sqrt(dx * dx + dy * dy);

  // 50W LPFM coverage profile centered on Liberty Campus (UNIVERS):
  // crystal clear around Campus, gentle fringe hiss by Rockford/Newport,
  // gritty/struggling signal (still audible) down at Love Media/Bedford Point.
  if (dist <= 150.0f) {
    *outProgGain = 1.0f;
    *outStaticGain = 0.0f;
  } else if (dist <= 350.0f) {
    const float t = (dist - 150.0f) / 200.0f;
    *outProgGain = 1.0f - 0.12f * t;
    *outStaticGain = 0.30f * t;
  } else if (dist <= 700.0f) {
    const float t = (dist - 350.0f) / 350.0f;
    *outProgGain = 0.88f - 0.22f * t;
    *outStaticGain = 0.30f + 0.28f * t;
  } else if (dist <= 1150.0f) {
    const float t = (dist - 700.0f) / 450.0f;
    *outProgGain = 0.66f - 0.38f * t;
    *outStaticGain = 0.58f + 0.32f * t;
  } else {
    float t = (dist - 1150.0f) / 200.0f;
    if (t > 1.0f) {
      t = 1.0f;
    }
    *outProgGain = 0.28f * (1.0f - t);
    *outStaticGain = 0.90f + 0.10f * t;
  }

  // Porter Tunnel / underground fade: gradually ramps static and breaks up the
  // broadcast from the Staunton portal mouth down the ramp and underground
  // loop, then cuts to dead air at the Shoreside/Portland T-junction corner.
  const float depth = ComputeWlsuTunnelDepth(px, py, pz);
  if (depth > 0.0f) {
    if (depth < kWlsuTunnelKnee) {
      const float u = depth / kWlsuTunnelKnee;
      *outProgGain *= (1.0f - u) * (1.0f - 0.25f * u);
      *outStaticGain += 0.85f * (1.0f - (1.0f - u) * (1.0f - u));
      if (*outStaticGain > 1.0f) {
        *outStaticGain = 1.0f;
      }
    } else {
      const float u = (depth - kWlsuTunnelKnee) / (1.0f - kWlsuTunnelKnee);
      float staticAtKnee = *outStaticGain + 0.85f;
      if (staticAtKnee > 1.0f) {
        staticAtKnee = 1.0f;
      }
      *outProgGain = 0.0f;
      *outStaticGain = staticAtKnee * (1.0f - u);
    }
  }
}

void UpdateWlsuStreams() {
  if (!g_wlsuLockInit || g_shuttingDown) {
    return;
  }
  EnterCriticalSection(&g_wlsuLock);

  if (g_shuttingDown) {
    LeaveCriticalSection(&g_wlsuLock);
    return;
  }

  if (!IsMilesReady()) {
    ResetWlsuStreamHandles(true);
    LeaveCriticalSection(&g_wlsuLock);
    return;
  }

  const DWORD now = GetTickCount();
  uint32_t dt =
      (g_wlsuLastServiceTick != 0) ? (now - g_wlsuLastServiceTick) : 0;
  if (dt > 250) {
    dt = 250;
  }
  g_wlsuLastServiceTick = now;

  const bool paused = IsWlsuPausedByGameState();
  if (!paused &&
      *reinterpret_cast<const int32_t *>(ADDR_GAME_STATE) == GS_PLAYING_GAME) {
    g_wlsuActivePlayMs += dt;
  }

  const bool wantWlsu = (*reinterpret_cast<const int32_t *>(ADDR_GAME_STATE) ==
                         GS_PLAYING_GAME) &&
                        (g_currentStation == WLSU_RADIO) && IsWlsuAvailable();

  if (!wantWlsu) {
    if (g_wlsuStream != 0 || g_wlsuStaticStream != 0) {
      CloseWlsuStreams(true);
    }
    LeaveCriticalSection(&g_wlsuLock);
    return;
  }

  EnsureAudioPathsResolved(now);
  const uint32_t digDriver =
      *reinterpret_cast<const uint32_t *>(ADDR_DIG_DRIVER);

  if (g_wlsuStream == 0 && !g_wlsuProgPath.empty() &&
      now >= g_wlsuProgRetryTick) {
    const uint32_t s = AilOpenStream(digDriver, g_wlsuProgPath.c_str());
    if (s != 0) {
      AilSetStreamLoopCount(s, 0);
      int32_t totalMs = 0;
      int32_t curMs = 0;
      AilStreamMsPosition(s, &totalMs, &curMs);
      if (totalMs > 0) {
        g_wlsuTotalMs = totalMs;
        int32_t startMs = 0;
        if (g_wlsuHasSavedPos) {
          const uint64_t elapsed = g_wlsuActivePlayMs - g_wlsuAnchorPlayMs;
          startMs = static_cast<int32_t>(
              (static_cast<uint64_t>(g_wlsuSavedPosMs) + elapsed) %
              static_cast<uint64_t>(totalMs));
        } else {
          startMs = static_cast<int32_t>((static_cast<uint64_t>(now) * 17u) %
                                         static_cast<uint64_t>(totalMs));
        }
        AilSetStreamMsPosition(s, startMs);
      }
      AilSetStreamVolume(s, 0);
      AilStartStream(s);
      AilPauseStream(s, paused ? 1 : 0);
      AilServiceStream(s, 0);
      g_wlsuStream = s;
      g_wlsuLastProgVol = 0;
      g_wlsuPaused = paused;
      WriteLog("Started WLSU.mp3 stream");
    } else {
      g_wlsuProgRetryTick = now + 1000;
      std::string err =
          std::string("WLSU.mp3 open FAILED: ") + GetAilLastError();
      WriteLog(err.c_str());
    }
  }

  if (g_wlsuStaticStream == 0 && !g_wlsuStaticPath.empty() &&
      now >= g_wlsuStaticRetryTick) {
    const uint32_t s = AilOpenStream(digDriver, g_wlsuStaticPath.c_str());
    if (s != 0) {
      AilSetStreamLoopCount(s, 0);
      AilSetStreamVolume(s, 0);
      AilStartStream(s);
      AilPauseStream(s, paused ? 1 : 0);
      AilServiceStream(s, 0);
      g_wlsuStaticStream = s;
      g_wlsuLastStaticVol = 0;
      WriteLog("Started WLSU_static.wav stream");
    } else {
      g_wlsuStaticRetryTick = now + 1000;
      std::string err =
          std::string("WLSU_static.wav open FAILED: ") + GetAilLastError();
      WriteLog(err.c_str());
    }
  }

  if (paused) {
    if (!g_wlsuPaused) {
      if (g_wlsuStream != 0) {
        AilSetStreamVolume(g_wlsuStream, 0);
        AilPauseStream(g_wlsuStream, 1);
        g_wlsuLastProgVol = 0;
      }
      if (g_wlsuStaticStream != 0) {
        AilSetStreamVolume(g_wlsuStaticStream, 0);
        AilPauseStream(g_wlsuStaticStream, 1);
        g_wlsuLastStaticVol = 0;
      }
      g_wlsuPaused = true;
    }
    LeaveCriticalSection(&g_wlsuLock);
    return;
  }

  if (g_wlsuPaused) {
    if (g_wlsuStream != 0) {
      AilPauseStream(g_wlsuStream, 0);
    }
    if (g_wlsuStaticStream != 0) {
      AilPauseStream(g_wlsuStaticStream, 0);
    }
    g_wlsuPaused = false;
  }

  // Fallback loop safety in case a stream ended naturally
  if (g_wlsuStream != 0 && AilStreamStatus(g_wlsuStream) == SMP_DONE) {
    AilSetStreamMsPosition(g_wlsuStream, 0);
    AilStartStream(g_wlsuStream);
  }
  if (g_wlsuStaticStream != 0 &&
      AilStreamStatus(g_wlsuStaticStream) == SMP_DONE) {
    AilSetStreamMsPosition(g_wlsuStaticStream, 0);
    AilStartStream(g_wlsuStaticStream);
  }

  uint8_t musicVol = *reinterpret_cast<const uint8_t *>(ADDR_SAMPLE_MUSIC_VOL);
  if (musicVol > 127) {
    musicVol = 127;
  }

  float progGain = 1.0f;
  float staticGain = 0.0f;
  ComputeWlsuSignalMix(&progGain, &staticGain);

  const bool wideScreen = (*reinterpret_cast<const uint8_t *>(
                               ADDR_THE_CAMERA + OFF_CAM_WIDESCREEN) != 0) &&
                          !IsPlayerWastedOrBusted();
  float envScale = wideScreen ? 0.0f : 1.0f;
  if (!Hook_PlayerInCar(reinterpret_cast<void *>(ADDR_MUSIC_MANAGER), 0)) {
    envScale *= (static_cast<float>(g_onFootRadioFade) / 100.0f);
  }
  if (*reinterpret_cast<const int32_t *>(ADDR_RETUNE_COUNTER) > 0) {
    envScale *= 0.25f;
  }

  int32_t progVol = static_cast<int32_t>(
      static_cast<float>(musicVol) * progGain * envScale + 0.5f);
  int32_t staticVol = static_cast<int32_t>(
      static_cast<float>(musicVol) * staticGain * envScale + 0.5f);
  if (progVol < 0)
    progVol = 0;
  if (progVol > 127)
    progVol = 127;
  if (staticVol < 0)
    staticVol = 0;
  if (staticVol > 127)
    staticVol = 127;

  if (g_wlsuStream != 0) {
    AilServiceStream(g_wlsuStream, 0);
    if (progVol != g_wlsuLastProgVol) {
      AilSetStreamVolume(g_wlsuStream, progVol);
      g_wlsuLastProgVol = progVol;
    }
  }
  if (g_wlsuStaticStream != 0) {
    AilServiceStream(g_wlsuStaticStream, 0);
    if (staticVol != g_wlsuLastStaticVol) {
      AilSetStreamVolume(g_wlsuStaticStream, staticVol);
      g_wlsuLastStaticVol = staticVol;
    }
  }

  LeaveCriticalSection(&g_wlsuLock);
}

void __fastcall Hook_ServiceAmbience(void *self, uintptr_t) {
  if (g_currentStation == RADIO_OFF) {
    g_onFootRadioFade = 100;
    g_lastFadeTick = 0;
    g_origServiceAmbience(self);
    return;
  }

  const DWORD now = GetTickCount();
  uint32_t dt = g_lastFadeTick != 0 ? (now - g_lastFadeTick) : 16;
  if (dt > 100) {
    dt = 100;
  }
  g_lastFadeTick = now;

  const bool wastedOrBusted = IsPlayerWastedOrBusted();
  const int32_t targetVol = (wastedOrBusted || IsCutsceneActive()) ? 0 : 100;
  int32_t step = static_cast<int32_t>((dt * 100) / 600);
  if (step < 1) {
    step = 1;
  }
  if (g_onFootRadioFade > targetVol) {
    g_onFootRadioFade -= step;
    if (g_onFootRadioFade < targetVol) {
      g_onFootRadioFade = targetVol;
    }
  } else if (g_onFootRadioFade < targetVol) {
    g_onFootRadioFade += step;
    if (g_onFootRadioFade > targetVol) {
      g_onFootRadioFade = targetVol;
    }
  }

  if (wastedOrBusted && g_onFootRadioFade == 0) {
    g_currentStation = RADIO_OFF;
    g_onFootRadioFade = 100;
    g_lastFadeTick = 0;
    *reinterpret_cast<uint8_t *>(ADDR_DISPLAY_TIMER) = 0;
    *reinterpret_cast<const wchar_t **>(ADDR_CURRENT_STATION_PTR) = nullptr;
    MusicU8(self, OFF_MUSIC_NEXT_TRACK) = RADIO_OFF;
    auto serviceTrack = reinterpret_cast<MusicBoolFn>(ADDR_MUSIC_SERVICE_TRACK);
    serviceTrack(self);
    auto setStreamedVol =
        reinterpret_cast<SampleSetStreamedVolFn>(ADDR_SAMPLE_SET_STREAMED_VOL);
    setStreamedVol(reinterpret_cast<void *>(ADDR_SAMPLE_MANAGER), 0, 63, 0, 0);
    g_origServiceAmbience(self);
    return;
  }

  if (g_currentStation == WLSU_RADIO) {
    MusicU8(self, OFF_MUSIC_NEXT_TRACK) = RADIO_OFF;
    auto serviceTrack = reinterpret_cast<MusicBoolFn>(ADDR_MUSIC_SERVICE_TRACK);
    serviceTrack(self);
    auto setStreamedVol =
        reinterpret_cast<SampleSetStreamedVolFn>(ADDR_SAMPLE_SET_STREAMED_VOL);
    setStreamedVol(reinterpret_cast<void *>(ADDR_SAMPLE_MANAGER), 0, 63, 0, 0);
    return;
  }

  MusicU8(self, OFF_MUSIC_NEXT_TRACK) = g_currentStation;
  auto serviceTrack = reinterpret_cast<MusicBoolFn>(ADDR_MUSIC_SERVICE_TRACK);
  auto serviceVolume = reinterpret_cast<MusicVoidFn>(ADDR_MUSIC_SERVICE_VOLUME);
  if (serviceTrack(self)) {
    serviceVolume(self);
  }

  const int32_t retuneCounter =
      *reinterpret_cast<const int32_t *>(ADDR_RETUNE_COUNTER);
  const uint8_t appliedVol = static_cast<uint8_t>(
      retuneCounter > 0 ? (g_onFootRadioFade >> 2) : g_onFootRadioFade);
  auto setStreamedVol =
      reinterpret_cast<SampleSetStreamedVolFn>(ADDR_SAMPLE_SET_STREAMED_VOL);
  setStreamedVol(reinterpret_cast<void *>(ADDR_SAMPLE_MANAGER), appliedVol, 63,
                 0, 0);
}

void __fastcall Hook_ServiceGameMode(void *self, uintptr_t) {
  InstallWindowHook();

  const bool wastedOrBusted = IsPlayerWastedOrBusted();
  if (wastedOrBusted) {
    g_wasWastedOrBusted = true;
    g_pendingScrollSteps = 0;
    *reinterpret_cast<uint8_t *>(ADDR_DISPLAY_TIMER) = 0;
    *reinterpret_cast<const wchar_t **>(ADDR_CURRENT_STATION_PTR) = nullptr;
  } else if (g_wasWastedOrBusted) {
    g_wasWastedOrBusted = false;
    g_lastPlayerVeh = nullptr;
    g_currentStation = RADIO_OFF;
    g_onFootRadioFade = 100;
    g_lastFadeTick = 0;
    g_pendingScrollSteps = 0;
    g_wrappedRetuneBonus = 0;
    *reinterpret_cast<uint8_t *>(ADDR_DISPLAY_TIMER) = 0;
    *reinterpret_cast<const wchar_t **>(ADDR_CURRENT_STATION_PTR) = nullptr;
    MusicU8(self, OFF_MUSIC_NEXT_TRACK) = RADIO_OFF;
  }

  auto findPlayerVeh =
      reinterpret_cast<FindPlayerVehicleFn>(ADDR_FIND_PLAYER_VEHICLE);
  void *veh = findPlayerVeh();
  const bool inCar = Hook_PlayerInCar(self, 0);
  const bool wlsuAvail = IsWlsuAvailable();
  const bool mp3Avail = IsMP3ChannelAvailable();

  if (inCar && veh) {
    g_onFootRadioFade = 100;
    g_lastFadeTick = 0;
    if (veh != g_lastPlayerVeh) {
      g_lastPlayerVeh = veh;

      const int16_t model = GetEntityModelIndex(veh);
      const bool radioSetByScript =
          MusicU8(self, OFF_MUSIC_RADIO_SET_BY_SCRIPT) != 0;

      if (!g_emergencyStationApplied &&
          (model == MI_POLICE || model == MI_AMBULAN) && !radioSetByScript) {
        g_emergencyStationApplied = true;
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
        if (g_currentStation == WLSU_RADIO && !wlsuAvail) {
          g_currentStation =
              AdvanceRadioStation(WLSU_RADIO, 1, mp3Avail, false);
        }
        VehRadioStation(veh) = g_currentStation;
        MusicU8(self, OFF_MUSIC_NEXT_TRACK) =
            (g_currentStation == WLSU_RADIO) ? RADIO_OFF : g_currentStation;
      } else {
        g_currentStation = VehRadioStation(veh);
      }
    } else {
      g_currentStation = VehRadioStation(veh);
    }

    if (g_currentStation == WLSU_RADIO && !wlsuAvail) {
      g_currentStation = AdvanceRadioStation(WLSU_RADIO, 1, mp3Avail, false);
      VehRadioStation(veh) = g_currentStation;
      MusicU8(self, OFF_MUSIC_NEXT_TRACK) = g_currentStation;
      *reinterpret_cast<int32_t *>(ADDR_RETUNE_COUNTER) = 30;
      *reinterpret_cast<int32_t *>(ADDR_NUM_RETUNE_PRESSES) = 0;
      *reinterpret_cast<uint8_t *>(ADDR_DISPLAY_TIMER) = 60;
      *reinterpret_cast<const wchar_t **>(ADDR_CURRENT_STATION_PTR) = nullptr;
      auto playOneShot =
          reinterpret_cast<AudioPlayOneShotFn>(ADDR_AUDIO_PLAY_ONE_SHOT);
      playOneShot(reinterpret_cast<void *>(ADDR_AUDIO_MANAGER), 0,
                  SOUND_FRONTEND_RADIO_CHANGE, 1.0f);
    }

    if (g_pendingScrollSteps != 0) {
      const int32_t stationCount =
          11 + (mp3Avail ? 1 : 0) + (wlsuAvail ? 1 : 0);

      // In GTA III, ADDR_NUM_RETUNE_PRESSES must be positive.
      // Scrolling down (-1) is equivalent to advancing (stationCount - 1)
      // forward.
      int32_t forwardSteps = g_pendingScrollSteps;
      while (forwardSteps < 0) {
        forwardSteps += stationCount;
      }
      forwardSteps %= stationCount;

      if (forwardSteps != 0) {
        int32_t &presses =
            *reinterpret_cast<int32_t *>(ADDR_NUM_RETUNE_PRESSES);
        presses = (presses + forwardSteps) % stationCount;
        *reinterpret_cast<int32_t *>(ADDR_RETUNE_COUNTER) = 30;
        auto playOneShot =
            reinterpret_cast<AudioPlayOneShotFn>(ADDR_AUDIO_PLAY_ONE_SHOT);
        playOneShot(reinterpret_cast<void *>(ADDR_AUDIO_MANAGER), 0,
                    SOUND_FRONTEND_RADIO_CHANGE, 1.0f);
      }
      g_pendingScrollSteps = 0;
    }
  } else {
    // Player is on foot!
    g_lastPlayerVeh = nullptr;

    // Discard any mouse wheel steps on foot so weapon cycling is never hijacked
    g_pendingScrollSteps = 0;

    if (!wastedOrBusted && g_currentStation == WLSU_RADIO && !wlsuAvail) {
      g_currentStation = AdvanceRadioStation(WLSU_RADIO, 1, mp3Avail, false);
      *reinterpret_cast<int32_t *>(ADDR_RETUNE_COUNTER) = 30;
      *reinterpret_cast<int32_t *>(ADDR_NUM_RETUNE_PRESSES) = 0;
      *reinterpret_cast<uint8_t *>(ADDR_DISPLAY_TIMER) = 60;
      *reinterpret_cast<const wchar_t **>(ADDR_CURRENT_STATION_PTR) = nullptr;
      auto playOneShot =
          reinterpret_cast<AudioPlayOneShotFn>(ADDR_AUDIO_PLAY_ONE_SHOT);
      playOneShot(reinterpret_cast<void *>(ADDR_AUDIO_MANAGER), 0,
                  SOUND_FRONTEND_RADIO_CHANGE, 1.0f);
    }

    static bool s_wasRKeyDown = false;
    const bool focused = !g_gameHwnd || GetForegroundWindow() == g_gameHwnd;
    const bool rKeyDown =
        focused && ((GetAsyncKeyState('R') & 0x8000) != 0 ||
                    (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0);
    const bool rJustDown = rKeyDown && !s_wasRKeyDown;
    s_wasRKeyDown = rKeyDown;

    auto padGet = reinterpret_cast<PadGetFn>(ADDR_PAD_GET_PAD);
    auto padChangeStation =
        reinterpret_cast<PadChangeStationFn>(ADDR_PAD_CHANGE_STATION);
    void *pad = padGet(0);
    const bool padRetune = pad && padChangeStation(pad);

    const bool paused =
        (*reinterpret_cast<const uint8_t *>(ADDR_USER_PAUSE) != 0) ||
        (*reinterpret_cast<const uint8_t *>(ADDR_CODE_PAUSE) != 0);
    const bool wideScreen = *reinterpret_cast<const uint8_t *>(
                                ADDR_THE_CAMERA + OFF_CAM_WIDESCREEN) != 0;
    const bool controlsDisabled =
        !pad ||
        (*reinterpret_cast<const uint8_t *>(reinterpret_cast<uintptr_t>(pad) +
                                            OFF_PAD_DISABLE_CONTROLS) != 0);

    const bool retuneTriggered = !paused && !wideScreen && !controlsDisabled &&
                                 !wastedOrBusted && (rJustDown || padRetune);

    if (retuneTriggered) {
      g_currentStation =
          AdvanceRadioStation(g_currentStation, 1, mp3Avail, wlsuAvail);

      *reinterpret_cast<int32_t *>(ADDR_RETUNE_COUNTER) = 30;
      *reinterpret_cast<int32_t *>(ADDR_NUM_RETUNE_PRESSES) = 0;
      *reinterpret_cast<uint8_t *>(ADDR_DISPLAY_TIMER) = 60;
      *reinterpret_cast<const wchar_t **>(ADDR_CURRENT_STATION_PTR) = nullptr;

      auto playOneShot =
          reinterpret_cast<AudioPlayOneShotFn>(ADDR_AUDIO_PLAY_ONE_SHOT);
      playOneShot(reinterpret_cast<void *>(ADDR_AUDIO_MANAGER), 0,
                  SOUND_FRONTEND_RADIO_CHANGE, 1.0f);
    }
  }

  // Shield >11 presses (e.g. 12 when scrolling down with 13 stations) from
  // gta3.exe's hardcoded `if (numRetunePresses > 11) numRetunePresses -= 11;`
  int32_t &presses = *reinterpret_cast<int32_t *>(ADDR_NUM_RETUNE_PRESSES);
  if (presses > 11) {
    g_wrappedRetuneBonus = presses - 1;
    presses = 1;
  } else {
    g_wrappedRetuneBonus = 0;
  }

  g_origServiceGameMode(self);

  if (g_wrappedRetuneBonus != 0) {
    if (presses != 0) {
      const int32_t stationCount =
          11 + (mp3Avail ? 1 : 0) + (wlsuAvail ? 1 : 0);
      presses = (presses + g_wrappedRetuneBonus) % stationCount;
    }
    g_wrappedRetuneBonus = 0;
  }

  static bool s_wasWlsu = false;
  const bool isWlsuNow = (g_currentStation == WLSU_RADIO);
  if (isWlsuNow != s_wasWlsu) {
    s_wasWlsu = isWlsuNow;
    char msg[96] = {};
    std::snprintf(msg, sizeof(msg),
                  isWlsuNow ? "Station state -> WLSU_RADIO (13)"
                            : "Station state -> %u (left WLSU)",
                  static_cast<unsigned>(g_currentStation));
    WriteLog(msg);
  }
}

void __fastcall Hook_AudioService(void *self, uintptr_t) {
  g_origAudioService(self);
  if (!g_shuttingDown) {
    UpdateWlsuStreams();
  }
}

void __fastcall Hook_SampleTerminate(void *self, uintptr_t) {
  if (g_wlsuLockInit) {
    EnterCriticalSection(&g_wlsuLock);
    CloseWlsuStreams(false);
    LeaveCriticalSection(&g_wlsuLock);
  }
  g_origSampleTerminate(self);
}

void __fastcall Hook_ResetMusicAfterReload(void *self, uintptr_t) {
  if (g_wlsuLockInit) {
    EnterCriticalSection(&g_wlsuLock);
    CloseWlsuStreams(false);
    LeaveCriticalSection(&g_wlsuLock);
  }
  g_lastPlayerVeh = nullptr;
  g_currentStation = RADIO_OFF;
  g_emergencyStationApplied = false;
  g_wasWastedOrBusted = false;
  g_onMansionRoof = false;
  g_onFootRadioFade = 100;
  g_lastFadeTick = 0;
  g_pendingScrollSteps = 0;
  g_wrappedRetuneBonus = 0;
  *reinterpret_cast<const wchar_t **>(ADDR_CURRENT_STATION_PTR) = nullptr;
  g_origResetMusicAfterReload(self);
}

void InstallDeferredHooks() {
  if (g_deferredInstalled) {
    return;
  }
  g_deferredInstalled = true;

  InstallWindowHook();

  bool ok = true;
  ok &=
      InstallDetour(ADDR_MUSIC_RESET_AFTER_RELOAD, &Hook_ResetMusicAfterReload,
                    &g_origResetMusicAfterReload);
  ok &= InstallDetour(ADDR_MUSIC_SERVICE_GAME_MODE, &Hook_ServiceGameMode,
                      &g_origServiceGameMode);
  ok &= InstallDetour(ADDR_MUSIC_SERVICE_AMBIENCE, &Hook_ServiceAmbience,
                      &g_origServiceAmbience);
  ok &= InstallDetour(ADDR_MUSIC_PLAYER_IN_CAR, &Hook_PlayerInCar,
                      &g_origPlayerInCar);
  ok &= InstallDetour(ADDR_MUSIC_GET_CAR_TUNING, &Hook_GetCarTuning,
                      &g_origGetCarTuning);
  ok &= InstallDetour(ADDR_MUSIC_GET_NEXT_CAR_TUNING, &Hook_GetNextCarTuning,
                      &g_origGetNextCarTuning);
  ok &= InstallDetour(ADDR_MUSIC_USES_POLICE_RADIO, &Hook_UsesPoliceRadio,
                      &g_origUsesPoliceRadio);
  ok &= InstallDetour(ADDR_MUSIC_DISPLAY_RADIO_NAME,
                      &Hook_DisplayRadioStationName,
                      &g_origDisplayRadioStationName);
  ok &= InstallDetour(ADDR_AUDIO_SERVICE, &Hook_AudioService,
                      &g_origAudioService);
  ok &= InstallDetour(ADDR_SAMPLE_TERMINATE, &Hook_SampleTerminate,
                      &g_origSampleTerminate);

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
    InitializeCriticalSection(&g_wlsuLock);
    g_wlsuLockInit = true;
    InitLogPath(hModule);
    InstallBootHook();
  } else if (reason == DLL_PROCESS_DETACH) {
    g_shuttingDown = true;
    if (lpReserved == nullptr) {
      RemoveAllHooks();
    }
    if (g_wlsuLockInit) {
      DeleteCriticalSection(&g_wlsuLock);
      g_wlsuLockInit = false;
    }
  }
  return TRUE;
}
