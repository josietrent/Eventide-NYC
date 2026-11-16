#define WIN32_LEAN_AND_MEAN
#include <stdint.h>
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
    0x8F3964; // gMusicManager (+0 init, +1 disabled, +2 mode)
constexpr uintptr_t ADDR_MENU_ACTIVE =
    0x8F5AE9; // uint8_t FrontEndMenuManager.m_bMenuActive
constexpr uintptr_t ADDR_MENU_SCREEN =
    0x8F5F20; // int32_t FrontEndMenuManager.m_nCurrScreen
constexpr uintptr_t ADDR_USER_PAUSE = 0x95CD7C; // uint8_t CTimer::m_UserPause

constexpr uintptr_t ADDR_SAMPLE_INIT =
    0x7341E0 + 0x56; // uint8_t cSampleManager::m_bInitialised
constexpr uintptr_t ADDR_MUSIC_HSTREAM = 0x709C50; // uint32_t HSTREAM[0]

constexpr uintptr_t IAT_AIL_PAUSE_STREAM =
    0x61D5FC; // void (__stdcall*)(uint32_t, int32_t)
constexpr uintptr_t IAT_AIL_STREAM_STATUS =
    0x61D674; // int32_t (__stdcall*)(uint32_t)

constexpr uintptr_t ADDR_PS_GLOBAL = 0x8F437C; // RsGlobal.ps -> *(HWND*)ps

constexpr int32_t GS_PLAYING_GAME = 9;
constexpr uint8_t MUSICMODE_GAME = 1;
constexpr int32_t MENUPAGE_SOUND_SETTINGS = 5;
constexpr int32_t SMP_PLAYING = 4;

using MusicManagerServiceFn = void(__thiscall *)(void *self);
using AilPauseStreamFn = void(__stdcall *)(uint32_t stream, int32_t onoff);
using AilStreamStatusFn = int32_t(__stdcall *)(uint32_t stream);

bool g_patched = false;
bool g_windowInactive = false;
uint32_t g_pausedStream = 0;
int32_t g_origRel32 = 0;
MusicManagerServiceFn g_origMusicService =
    reinterpret_cast<MusicManagerServiceFn>(ADDR_FN_MUSIC_SERVICE);
HWND g_hwnd = nullptr;
WNDPROC g_origWndProc = nullptr;

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

bool ShouldPauseRadio() {
  if (*reinterpret_cast<const int32_t *>(ADDR_GAME_STATE) != GS_PLAYING_GAME) {
    return false;
  }

  const uint8_t *musicMgr =
      reinterpret_cast<const uint8_t *>(ADDR_MUSIC_MANAGER);
  if (musicMgr[0] == 0 || musicMgr[1] != 0 || musicMgr[2] != MUSICMODE_GAME) {
    return false;
  }

  if (g_windowInactive) {
    return true;
  }

  const bool paused =
      (*reinterpret_cast<const uint8_t *>(ADDR_USER_PAUSE) != 0) ||
      (*reinterpret_cast<const uint8_t *>(ADDR_MENU_ACTIVE) != 0);
  if (!paused) {
    return false;
  }

  // Allow Audio Setup screen (station preview & volume sliders) to play
  // normally.
  if (*reinterpret_cast<const int32_t *>(ADDR_MENU_SCREEN) ==
      MENUPAGE_SOUND_SETTINGS) {
    return false;
  }

  return true;
}

bool SyncMusicPauseState() {
  const uint32_t hStream = GetMusicStreamHandle();

  if (ShouldPauseRadio()) {
    if (hStream != 0 && hStream != g_pausedStream) {
      if (AilStreamStatus(hStream) == SMP_PLAYING) {
        AilPauseStream(hStream, 1);
        g_pausedStream = hStream;
      }
    }
    return true;
  }

  if (g_pausedStream != 0) {
    if (hStream == g_pausedStream) {
      AilPauseStream(g_pausedStream, 0);
    }
    g_pausedStream = 0;
  }
  return false;
}

LRESULT CALLBACK Hook_WndProc(HWND hwnd, UINT msg, WPARAM wParam,
                              LPARAM lParam) {
  if (msg == WM_ACTIVATEAPP) {
    g_windowInactive = (wParam == FALSE);
    SyncMusicPauseState();
  }
  return CallWindowProcA(g_origWndProc, hwnd, msg, wParam, lParam);
}

void EnsureWndProcHook() {
  if (g_origWndProc != nullptr) {
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
  WNDPROC prev = reinterpret_cast<WNDPROC>(
      SetWindowLongA(hwnd, GWL_WNDPROC, reinterpret_cast<LONG>(&Hook_WndProc)));
  if (prev) {
    g_hwnd = hwnd;
    g_origWndProc = prev;
  }
}

void __fastcall Hook_MusicManagerService(void *self, void * /*edx*/) {
  EnsureWndProcHook();

  if (SyncMusicPauseState()) {
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
  if (*reinterpret_cast<const uint32_t *>(ADDR_CALL_MUSIC_SERVICE - 5) !=
          EXPECTED_MOV_ECX_IMM ||
      *reinterpret_cast<const uint8_t *>(ADDR_CALL_MUSIC_SERVICE - 1) != 0x00) {
    return false;
  }
  if (*reinterpret_cast<const uint8_t *>(ADDR_CALL_MUSIC_SERVICE) != 0xE8) {
    return false;
  }

  g_origRel32 = *reinterpret_cast<const int32_t *>(ADDR_CALL_MUSIC_SERVICE + 1);
  g_origMusicService = reinterpret_cast<MusicManagerServiceFn>(
      ADDR_CALL_MUSIC_SERVICE + 5 + g_origRel32);

  const int32_t hookRel32 = static_cast<int32_t>(
      reinterpret_cast<uintptr_t>(&Hook_MusicManagerService) -
      (ADDR_CALL_MUSIC_SERVICE + 5));
  g_patched = PatchCallRel32(ADDR_CALL_MUSIC_SERVICE, hookRel32);
  return g_patched;
}

void RemoveHooks() {
  if (g_pausedStream != 0) {
    if (GetMusicStreamHandle() == g_pausedStream) {
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
    InstallHooks();
  } else if (reason == DLL_PROCESS_DETACH && lpReserved == nullptr) {
    RemoveHooks();
  }
  return TRUE;
}
