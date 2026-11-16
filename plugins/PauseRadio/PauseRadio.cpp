#define WIN32_LEAN_AND_MEAN
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

namespace {

constexpr uintptr_t ADDR_CALL_MUSIC_SERVICE =
    0x57A2FA; // inside cDMAudio::Service()
constexpr uintptr_t ADDR_FN_MUSIC_SERVICE =
    0x57D440; // cMusicManager::Service()
constexpr uint32_t EXPECTED_MOV_ECX_IMM =
    0x8F3964B9; // mov ecx, 0x8F3964 at 0x57A2F5

constexpr uintptr_t ADDR_GAME_STATE = 0x8F5838; // int32_t (9 = GS_PLAYING_GAME)
constexpr uintptr_t ADDR_MUSIC_MANAGER =
    0x8F3964; // gMusicManager (+0 init, +1 disabled, +2 mode, +3 currTrack)
constexpr uintptr_t ADDR_MENU_ACTIVE =
    0x8F5AE9; // uint8_t FrontEndMenuManager.m_bMenuActive
constexpr uintptr_t ADDR_MENU_SCREEN =
    0x8F5F20; // int32_t FrontEndMenuManager.m_nCurrScreen
constexpr uintptr_t ADDR_USER_PAUSE = 0x95CD7C; // uint8_t CTimer::m_UserPause
constexpr uintptr_t ADDR_TIME_PAUSE_MS =
    0x5F7614; // uint32_t CTimer::m_snTimeInMillisecondsPauseMode

constexpr uintptr_t ADDR_SAMPLE_INIT =
    0x7341E0 + 0x56; // uint8_t cSampleManager::m_bInitialised
constexpr uintptr_t ADDR_MUSIC_HSTREAM = 0x709C50; // uint32_t HSTREAM[0]

constexpr uintptr_t IAT_AIL_STREAM_MS_POSITION =
    0x61D5CC; // void (__stdcall*)(uint32_t, int32_t*, int32_t*)
constexpr uintptr_t IAT_AIL_PAUSE_STREAM =
    0x61D5FC; // void (__stdcall*)(uint32_t, int32_t)
constexpr uintptr_t IAT_AIL_SET_STREAM_MS_POSITION =
    0x61D668; // void (__stdcall*)(uint32_t, int32_t)
constexpr uintptr_t IAT_AIL_SET_STREAM_VOLUME =
    0x61D66C; // void (__stdcall*)(uint32_t, int32_t)
constexpr uintptr_t IAT_AIL_STREAM_STATUS =
    0x61D674; // int32_t (__stdcall*)(uint32_t)

constexpr uintptr_t ADDR_PS_GLOBAL = 0x8F437C; // RsGlobal.ps -> *(HWND*)ps

constexpr int32_t GS_PLAYING_GAME = 9;
constexpr uint8_t MUSICMODE_FRONTEND = 0;
constexpr uint8_t MUSICMODE_GAME = 1;
constexpr int32_t MENUPAGE_SOUND_SETTINGS = 5;
constexpr int32_t SMP_PLAYING = 4;
constexpr int32_t SMP_PLAYINGBUTRELEASED = 16;
constexpr size_t NUM_MUSIC_TRACKS = 196; // 0x0C..0x93C (196 * 12 = 0x930)
constexpr UINT WM_APP_SYNC_PAUSE = WM_APP + 0x37;

#pragma pack(push, 1)
struct MusicTrackInfo {
  uint32_t lengthMs;
  uint32_t lastPosMs;
  uint32_t lastTimePausedMs;
};
#pragma pack(pop)
static_assert(sizeof(MusicTrackInfo) == 12, "MusicTrackInfo size mismatch");

using MusicManagerServiceFn = void(__thiscall *)(void *self);
using AilPauseStreamFn = void(__stdcall *)(uint32_t stream, int32_t onoff);
using AilStreamStatusFn = int32_t(__stdcall *)(uint32_t stream);
using AilStreamMsPositionFn = void(__stdcall *)(uint32_t stream,
                                                int32_t *totalMs,
                                                int32_t *currentMs);
using AilSetStreamMsPositionFn = void(__stdcall *)(uint32_t stream,
                                                   int32_t posMs);
using AilSetStreamVolumeFn = void(__stdcall *)(uint32_t stream, int32_t volume);
using AilStreamVolumeFn = int32_t(__stdcall *)(uint32_t stream);

CRITICAL_SECTION g_lock;
bool g_lockInit = false;
bool g_patched = false;
bool g_windowInactive = false;
bool g_focusLost = false;
bool g_windowHookAttempt = false;
bool g_loggedFirstTick = false;
bool g_prevRawPause = false;
bool g_hasPausedPos = false;
bool g_freezeActive = false;
uint32_t g_freezePrevMs = 0;
uint32_t g_pausedStream = 0;
int32_t g_pausedPosMs = 0;
int32_t g_pausedTotalMs = 0;
int32_t g_pausedVol = -1;
int32_t g_origRel32 = 0;
MusicManagerServiceFn g_origMusicService =
    reinterpret_cast<MusicManagerServiceFn>(ADDR_FN_MUSIC_SERVICE);
AilStreamVolumeFn g_ailStreamVolume = nullptr;
HWND g_hwnd = nullptr;
WNDPROC g_origWndProc = nullptr;
HANDLE g_stopEvent = nullptr;
HANDLE g_watchThread = nullptr;
char g_logPath[MAX_PATH] = {};

void WriteLog(const char *line) {
  if (!g_logPath[0]) {
    return;
  }
  HANDLE f = CreateFileA(g_logPath, FILE_APPEND_DATA,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                         OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (f == INVALID_HANDLE_VALUE) {
    return;
  }
  DWORD written = 0;
  WriteFile(f, line, static_cast<DWORD>(strlen(line)), &written, nullptr);
  WriteFile(f, "\r\n", 2, &written, nullptr);
  CloseHandle(f);
}

void InitLogPath(HMODULE hModule) {
  DWORD len = GetModuleFileNameA(hModule, g_logPath, MAX_PATH);
  if (len == 0 || len >= MAX_PATH) {
    g_logPath[0] = '\0';
    return;
  }
  char *slash = strrchr(g_logPath, '\\');
  if (slash) {
    strcpy_s(slash + 1,
             sizeof(g_logPath) - static_cast<size_t>((slash + 1) - g_logPath),
             "PauseRadio.log");
  } else {
    strcpy_s(g_logPath, "PauseRadio.log");
  }
  HANDLE f =
      CreateFileA(g_logPath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                  nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (f != INVALID_HANDLE_VALUE) {
    CloseHandle(f);
  }
}

inline uint32_t GetMusicStreamHandle() {
  if (*reinterpret_cast<const uint8_t *>(ADDR_SAMPLE_INIT) == 0) {
    return 0;
  }
  return *reinterpret_cast<const uint32_t *>(ADDR_MUSIC_HSTREAM);
}

inline void AilPauseStream(uint32_t stream, int32_t pause) {
  auto fn = *reinterpret_cast<const AilPauseStreamFn *>(IAT_AIL_PAUSE_STREAM);
  if (fn && stream) {
    fn(stream, pause);
  }
}

inline int32_t AilStreamStatus(uint32_t stream) {
  auto fn = *reinterpret_cast<const AilStreamStatusFn *>(IAT_AIL_STREAM_STATUS);
  return (fn && stream) ? fn(stream) : 0;
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

inline int32_t AilStreamVolume(uint32_t stream) {
  if (!g_ailStreamVolume) {
    HMODULE mss = GetModuleHandleA("Mss32.dll");
    if (mss) {
      g_ailStreamVolume = reinterpret_cast<AilStreamVolumeFn>(
          GetProcAddress(mss, "_AIL_stream_volume@4"));
    }
  }
  return (g_ailStreamVolume && stream) ? g_ailStreamVolume(stream) : -1;
}

void FreezeMusicPauseClock(bool paused) {
  const uint32_t now = *reinterpret_cast<const uint32_t *>(ADDR_TIME_PAUSE_MS);
  if (!paused) {
    g_freezeActive = false;
    return;
  }
  if (!g_freezeActive) {
    g_freezeActive = true;
    g_freezePrevMs = now;
    return;
  }
  if (now > g_freezePrevMs) {
    const uint32_t delta = now - g_freezePrevMs;
    auto *tracks =
        reinterpret_cast<MusicTrackInfo *>(ADDR_MUSIC_MANAGER + 0x0C);
    for (size_t i = 0; i < NUM_MUSIC_TRACKS; ++i) {
      if (tracks[i].lastTimePausedMs != 0 &&
          tracks[i].lastTimePausedMs <= now) {
        uint32_t shifted = tracks[i].lastTimePausedMs + delta;
        if (shifted > now) {
          shifted = now;
        }
        tracks[i].lastTimePausedMs = shifted;
      }
    }
  }
  g_freezePrevMs = now;
}

bool ShouldPauseRadio(bool *outUnfocused = nullptr) {
  if (*reinterpret_cast<const int32_t *>(ADDR_GAME_STATE) != GS_PLAYING_GAME) {
    return false;
  }

  const uint8_t *musicMgr =
      reinterpret_cast<const uint8_t *>(ADDR_MUSIC_MANAGER);
  if (musicMgr[0] == 0 || musicMgr[1] != 0) {
    return false;
  }
  if (musicMgr[2] != MUSICMODE_GAME && musicMgr[2] != MUSICMODE_FRONTEND) {
    return false;
  }

  const bool fgLost = (g_hwnd != nullptr) && (GetForegroundWindow() != g_hwnd);
  const bool unfocused = g_windowInactive || g_focusLost || fgLost;
  if (outUnfocused) {
    *outUnfocused = unfocused;
  }
  if (unfocused) {
    return true;
  }

  const bool paused =
      (*reinterpret_cast<const uint8_t *>(ADDR_USER_PAUSE) != 0) ||
      (*reinterpret_cast<const uint8_t *>(ADDR_MENU_ACTIVE) != 0);
  if (!paused) {
    return false;
  }

  if (*reinterpret_cast<const int32_t *>(ADDR_MENU_SCREEN) ==
      MENUPAGE_SOUND_SETTINGS) {
    return false;
  }

  return true;
}

bool SyncMusicPauseState(const char *source) {
  if (!g_lockInit) {
    return false;
  }
  EnterCriticalSection(&g_lock);

  bool unfocused = false;
  const bool wantPause = ShouldPauseRadio(&unfocused);
  const uint32_t hStream = GetMusicStreamHandle();
  const uint8_t userPause = *reinterpret_cast<const uint8_t *>(ADDR_USER_PAUSE);
  const uint8_t menuActive =
      *reinterpret_cast<const uint8_t *>(ADDR_MENU_ACTIVE);
  const uint8_t currTrack =
      *reinterpret_cast<const uint8_t *>(ADDR_MUSIC_MANAGER + 3);
  const bool rawPause = (userPause != 0) || (menuActive != 0) || unfocused;

  FreezeMusicPauseClock(wantPause);

  if (rawPause != g_prevRawPause && (!wantPause || hStream == 0)) {
    const auto *tracks =
        reinterpret_cast<const MusicTrackInfo *>(ADDR_MUSIC_MANAGER + 0x0C);
    const uint32_t savedPos =
        (currTrack < NUM_MUSIC_TRACKS) ? tracks[currTrack].lastPosMs : 0;
    char buf[256];
    _snprintf_s(buf, sizeof(buf), _TRUNCATE,
                "[%s] RAW_%s want=%d gs=%d mode=%u user=%u menu=%u scr=%d "
                "trk=%u pos=%u h0=0x%08X st0=%d t=%lu",
                source, rawPause ? "PAUSE" : "UNPAUSE", wantPause ? 1 : 0,
                *reinterpret_cast<const int32_t *>(ADDR_GAME_STATE),
                *reinterpret_cast<const uint8_t *>(ADDR_MUSIC_MANAGER + 2),
                userPause, menuActive,
                *reinterpret_cast<const int32_t *>(ADDR_MENU_SCREEN), currTrack,
                savedPos, hStream, AilStreamStatus(hStream), GetTickCount());
    WriteLog(buf);
  }
  g_prevRawPause = rawPause;

  if (wantPause) {
    if (hStream != 0 && hStream != g_pausedStream) {
      const int32_t stBefore = AilStreamStatus(hStream);
      if (stBefore == SMP_PLAYING || stBefore == SMP_PLAYINGBUTRELEASED) {
        int32_t totalMs = 0;
        int32_t curMs = -1;
        const bool gotPos = AilStreamMsPosition(hStream, &totalMs, &curMs);
        const int32_t vol = AilStreamVolume(hStream);
        AilSetStreamVolume(hStream, 0);
        AilPauseStream(hStream, 1);
        const int32_t stAfter = AilStreamStatus(hStream);

        if (stAfter != SMP_PLAYING) {
          g_pausedStream = hStream;
          g_pausedPosMs = curMs;
          g_pausedTotalMs = totalMs;
          g_pausedVol = vol;
          g_hasPausedPos = (gotPos && curMs >= 0);
        }

        char buf[320];
        _snprintf_s(buf, sizeof(buf), _TRUNCATE,
                    "[%s] PAUSED stream=0x%08X st=%d->%d pos=%d/%d vol=%d "
                    "unfoc=%d user=%u menu=%u scr=%d mode=%u trk=%u t=%lu",
                    source, hStream, stBefore, stAfter, curMs, totalMs, vol,
                    unfocused ? 1 : 0, userPause, menuActive,
                    *reinterpret_cast<const int32_t *>(ADDR_MENU_SCREEN),
                    *reinterpret_cast<const uint8_t *>(ADDR_MUSIC_MANAGER + 2),
                    currTrack, GetTickCount());
        WriteLog(buf);
      }
    }
  } else if (g_pausedStream != 0) {
    if (hStream == g_pausedStream) {
      int32_t beforeSeekMs = -1;
      int32_t totalMs = 0;
      AilStreamMsPosition(g_pausedStream, &totalMs, &beforeSeekMs);

      if (g_hasPausedPos && g_pausedPosMs >= 0) {
        int32_t targetMs = g_pausedPosMs;
        if (g_pausedTotalMs > 0 && targetMs >= g_pausedTotalMs) {
          targetMs = g_pausedTotalMs > 50 ? g_pausedTotalMs - 50 : 0;
        }
        AilSetStreamMsPosition(g_pausedStream, targetMs);
      }

      if (g_pausedVol >= 0) {
        AilSetStreamVolume(g_pausedStream, g_pausedVol);
      }
      AilPauseStream(g_pausedStream, 0);

      int32_t afterResumeMs = -1;
      AilStreamMsPosition(g_pausedStream, &totalMs, &afterResumeMs);
      const int32_t stAfter = AilStreamStatus(g_pausedStream);

      char buf[320];
      _snprintf_s(buf, sizeof(buf), _TRUNCATE,
                  "[%s] RESUMED stream=0x%08X st=%d saved=%d before=%d "
                  "after=%d vol=%d scr=%d mode=%u trk=%u t=%lu",
                  source, g_pausedStream, stAfter, g_pausedPosMs, beforeSeekMs,
                  afterResumeMs, g_pausedVol,
                  *reinterpret_cast<const int32_t *>(ADDR_MENU_SCREEN),
                  *reinterpret_cast<const uint8_t *>(ADDR_MUSIC_MANAGER + 2),
                  currTrack, GetTickCount());
      WriteLog(buf);
    } else {
      char buf[192];
      _snprintf_s(buf, sizeof(buf), _TRUNCATE,
                  "[%s] CLEARED stale paused=0x%08X curr=0x%08X t=%lu", source,
                  g_pausedStream, hStream, GetTickCount());
      WriteLog(buf);
    }
    g_pausedStream = 0;
    g_hasPausedPos = false;
    g_pausedPosMs = 0;
    g_pausedTotalMs = 0;
    g_pausedVol = -1;
  }

  LeaveCriticalSection(&g_lock);
  return wantPause;
}

DWORD WINAPI FocusWatchThread(LPVOID) {
  bool prevFgLost = false;
  while (WaitForSingleObject(g_stopEvent, 50) == WAIT_TIMEOUT) {
    if (g_hwnd && IsWindow(g_hwnd)) {
      const bool fgLost = (GetForegroundWindow() != g_hwnd);
      if (fgLost != prevFgLost) {
        prevFgLost = fgLost;
        PostMessageA(g_hwnd, WM_APP_SYNC_PAUSE, fgLost ? 1 : 0, 0);
      }
    }
  }
  return 0;
}

LRESULT CALLBACK Hook_WndProc(HWND hwnd, UINT msg, WPARAM wParam,
                              LPARAM lParam) {
  if (msg == WM_APP_SYNC_PAUSE) {
    SyncMusicPauseState("watch_msg");
    return 0;
  }
  if (msg == WM_ACTIVATEAPP) {
    g_windowInactive = (wParam == FALSE);
    SyncMusicPauseState("wndproc_app");
  } else if (msg == WM_ACTIVATE) {
    g_windowInactive = (LOWORD(wParam) == WA_INACTIVE);
    SyncMusicPauseState("wndproc_act");
  } else if (msg == WM_KILLFOCUS) {
    g_focusLost = true;
    SyncMusicPauseState("wndproc_kill");
  } else if (msg == WM_SETFOCUS) {
    g_focusLost = false;
    SyncMusicPauseState("wndproc_set");
  }
  return CallWindowProcA(g_origWndProc, hwnd, msg, wParam, lParam);
}

void EnsureWindowHooks() {
  if (g_windowHookAttempt) {
    return;
  }
  void *ps = *reinterpret_cast<void *const *>(ADDR_PS_GLOBAL);
  if (!ps) {
    return;
  }
  HWND hwnd = *reinterpret_cast<HWND const *>(ps);
  if (!hwnd || !IsWindow(hwnd)) {
    return;
  }
  g_windowHookAttempt = true;
  g_hwnd = hwnd;
  WNDPROC prev = reinterpret_cast<WNDPROC>(
      SetWindowLongA(hwnd, GWL_WNDPROC, reinterpret_cast<LONG>(&Hook_WndProc)));
  if (prev) {
    g_origWndProc = prev;
  }
  if (!g_watchThread) {
    g_stopEvent = CreateEventA(nullptr, TRUE, FALSE, nullptr);
    if (g_stopEvent) {
      g_watchThread =
          CreateThread(nullptr, 0, &FocusWatchThread, nullptr, 0, nullptr);
    }
  }
  char buf[128];
  _snprintf_s(
      buf, sizeof(buf), _TRUNCATE, "Window hooked hwnd=0x%p origWndProc=0x%p",
      static_cast<void *>(hwnd), reinterpret_cast<void *>(g_origWndProc));
  WriteLog(buf);
}

void __fastcall Hook_MusicManagerService(void *self, void * /*edx*/) {
  EnsureWindowHooks();

  if (!g_loggedFirstTick) {
    g_loggedFirstTick = true;
    char buf[160];
    _snprintf_s(buf, sizeof(buf), _TRUNCATE,
                "First Service tick: gameState=%d init=%u dis=%u mode=%u",
                *reinterpret_cast<const int32_t *>(ADDR_GAME_STATE),
                *reinterpret_cast<const uint8_t *>(ADDR_MUSIC_MANAGER),
                *reinterpret_cast<const uint8_t *>(ADDR_MUSIC_MANAGER + 1),
                *reinterpret_cast<const uint8_t *>(ADDR_MUSIC_MANAGER + 2));
    WriteLog(buf);
  }

  if (SyncMusicPauseState("tick")) {
    // Skip cMusicManager::Service() while paused so ServiceGameMode() does not
    // treat the paused HSTREAM (status != SMP_PLAYING) as finished and
    // skip/stop it.
    return;
  }

  g_origMusicService(self);
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
  const uint32_t movImm =
      *reinterpret_cast<const uint32_t *>(ADDR_CALL_MUSIC_SERVICE - 5);
  const uint8_t movHi =
      *reinterpret_cast<const uint8_t *>(ADDR_CALL_MUSIC_SERVICE - 1);
  const uint8_t op =
      *reinterpret_cast<const uint8_t *>(ADDR_CALL_MUSIC_SERVICE);
  const int32_t rel =
      *reinterpret_cast<const int32_t *>(ADDR_CALL_MUSIC_SERVICE + 1);

  if (movImm != EXPECTED_MOV_ECX_IMM || movHi != 0x00 || op != 0xE8) {
    char buf[160];
    _snprintf_s(buf, sizeof(buf), _TRUNCATE,
                "Signature mismatch at 0x57A2F5: movImm=0x%08X movHi=0x%02X "
                "op=0x%02X rel=0x%08X",
                movImm, movHi, op, static_cast<uint32_t>(rel));
    WriteLog(buf);
    return false;
  }

  g_origRel32 = rel;
  g_origMusicService = reinterpret_cast<MusicManagerServiceFn>(
      ADDR_CALL_MUSIC_SERVICE + 5 + g_origRel32);

  const int32_t hookRel32 = static_cast<int32_t>(
      reinterpret_cast<uintptr_t>(&Hook_MusicManagerService) -
      (ADDR_CALL_MUSIC_SERVICE + 5));
  g_patched = PatchCallRel32(ADDR_CALL_MUSIC_SERVICE, hookRel32);
  char buf[160];
  _snprintf_s(buf, sizeof(buf), _TRUNCATE,
              "InstallHooks patched=%d origTarget=0x%p hook=0x%p",
              g_patched ? 1 : 0, reinterpret_cast<void *>(g_origMusicService),
              reinterpret_cast<void *>(&Hook_MusicManagerService));
  WriteLog(buf);
  return g_patched;
}

void RemoveHooks() {
  if (g_stopEvent) {
    SetEvent(g_stopEvent);
  }
  if (g_watchThread) {
    WaitForSingleObject(g_watchThread, 1000);
    CloseHandle(g_watchThread);
    g_watchThread = nullptr;
  }
  if (g_stopEvent) {
    CloseHandle(g_stopEvent);
    g_stopEvent = nullptr;
  }

  if (g_pausedStream != 0) {
    if (GetMusicStreamHandle() == g_pausedStream) {
      if (g_pausedVol >= 0) {
        AilSetStreamVolume(g_pausedStream, g_pausedVol);
      }
      AilPauseStream(g_pausedStream, 0);
    }
    g_pausedStream = 0;
  }

  if (g_origWndProc && g_hwnd && IsWindow(g_hwnd)) {
    if (GetWindowLongA(g_hwnd, GWL_WNDPROC) ==
        reinterpret_cast<LONG>(&Hook_WndProc)) {
      SetWindowLongA(g_hwnd, GWL_WNDPROC,
                     reinterpret_cast<LONG>(g_origWndProc));
    }
    g_origWndProc = nullptr;
    g_hwnd = nullptr;
  }

  if (g_patched) {
    PatchCallRel32(ADDR_CALL_MUSIC_SERVICE, g_origRel32);
    g_patched = false;
  }
}

} // namespace

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(hModule);
    InitializeCriticalSection(&g_lock);
    g_lockInit = true;
    InitLogPath(hModule);
    InstallHooks();
  } else if (reason == DLL_PROCESS_DETACH) {
    if (lpReserved == nullptr) {
      RemoveHooks();
    }
    if (g_lockInit) {
      DeleteCriticalSection(&g_lock);
      g_lockInit = false;
    }
  }
  return TRUE;
}
