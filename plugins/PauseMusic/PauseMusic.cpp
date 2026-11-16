#define WIN32_LEAN_AND_MEAN
#include <algorithm>
#include <cctype>
#include <stdint.h>
#include <string>
#include <vector>
#include <windows.h>

namespace {

constexpr uintptr_t ADDR_CALL_AUDIO_SERVICE =
    0x57A2F0; // inside cDMAudio::Service() -> cAudioManager::Service()
constexpr uintptr_t ADDR_FN_AUDIO_SERVICE = 0x57AA60;

constexpr uintptr_t ADDR_GAME_STATE = 0x8F5838; // int32_t (7 = menu, 9 = game)
constexpr uintptr_t ADDR_MENU_ACTIVE =
    0x8F5AE9; // uint8_t FrontEndMenuManager.m_bMenuActive
constexpr uintptr_t ADDR_MENU_SCREEN =
    0x8F5F20; // int32_t FrontEndMenuManager.m_nCurrScreen
constexpr uintptr_t ADDR_USER_PAUSE = 0x95CD7C; // uint8_t CTimer::m_UserPause

constexpr uintptr_t ADDR_SAMPLE_MANAGER = 0x7341E0;
constexpr uintptr_t ADDR_SAMPLE_MUSIC_VOL =
    ADDR_SAMPLE_MANAGER + 0x01; // uint8_t 0..127
constexpr uintptr_t ADDR_SAMPLE_INIT =
    ADDR_SAMPLE_MANAGER + 0x56;                 // uint8_t m_bInitialised
constexpr uintptr_t ADDR_DIG_DRIVER = 0x8F1A24; // HDIGDRIVER

constexpr uintptr_t IAT_AIL_OPEN_STREAM =
    0x61D5C8; // uint32_t (__stdcall*)(uint32_t, const char*, int32_t)
constexpr uintptr_t IAT_AIL_STREAM_MS_POSITION =
    0x61D5CC; // void (__stdcall*)(uint32_t, int32_t*, int32_t*)
constexpr uintptr_t IAT_AIL_CLOSE_STREAM =
    0x61D5D0; // void (__stdcall*)(uint32_t)
constexpr uintptr_t IAT_AIL_PAUSE_STREAM =
    0x61D5FC; // void (__stdcall*)(uint32_t, int32_t)
constexpr uintptr_t IAT_AIL_SET_STREAM_LOOP_COUNT =
    0x61D65C; // void (__stdcall*)(uint32_t, int32_t)
constexpr uintptr_t IAT_AIL_SET_STREAM_MS_POSITION =
    0x61D668; // void (__stdcall*)(uint32_t, int32_t)
constexpr uintptr_t IAT_AIL_SET_STREAM_VOLUME =
    0x61D66C; // void (__stdcall*)(uint32_t, int32_t)
constexpr uintptr_t IAT_AIL_STREAM_STATUS =
    0x61D674; // int32_t (__stdcall*)(uint32_t)

constexpr uintptr_t ADDR_PS_GLOBAL = 0x8F437C; // RsGlobal.ps -> *(HWND*)ps

constexpr int32_t GS_FRONTEND = 7;
constexpr int32_t GS_PLAYING_GAME = 9;
constexpr int32_t MENUPAGE_SOUND_SETTINGS = 5;
constexpr int32_t SMP_DONE = 2;
constexpr UINT WM_APP_SYNC_PAUSE_MUSIC = WM_APP + 0x38;

using AudioServiceFn = void(__thiscall *)(void *self);
using AilOpenStreamFn = uint32_t(__stdcall *)(uint32_t digDriver,
                                              const char *filename,
                                              int32_t streamMem);
using AilCloseStreamFn = void(__stdcall *)(uint32_t stream);
using AilPauseStreamFn = void(__stdcall *)(uint32_t stream, int32_t onoff);
using AilStreamStatusFn = int32_t(__stdcall *)(uint32_t stream);
using AilSetStreamLoopCountFn = void(__stdcall *)(uint32_t stream,
                                                  int32_t count);
using AilStreamMsPositionFn = void(__stdcall *)(uint32_t stream,
                                                int32_t *totalMs,
                                                int32_t *currentMs);
using AilSetStreamMsPositionFn = void(__stdcall *)(uint32_t stream,
                                                   int32_t posMs);
using AilSetStreamVolumeFn = void(__stdcall *)(uint32_t stream, int32_t volume);

struct PauseMusicConfig {
  std::string path;
  int32_t volume = 80;
  int32_t fadeInMs = 400;
  int32_t fadeOutMs = 250;
  bool resumePosition = true;
  bool shuffle = true;
  bool playOnMainMenu = false;
};

CRITICAL_SECTION g_lock;
bool g_lockInit = false;
bool g_patched = false;
bool g_windowInactive = false;
bool g_focusLost = false;
bool g_windowHookAttempt = false;

int32_t g_origRel32 = 0;
AudioServiceFn g_origAudioService =
    reinterpret_cast<AudioServiceFn>(ADDR_FN_AUDIO_SERVICE);
HWND g_hwnd = nullptr;
WNDPROC g_origWndProc = nullptr;
HANDLE g_stopEvent = nullptr;
HANDLE g_watchThread = nullptr;

std::string g_asiDir;
std::string g_exeDir;
std::string g_iniPath;
FILETIME g_iniWriteTime = {};
bool g_hasIniTime = false;
uint32_t g_lastIniCheckTick = 0;

PauseMusicConfig g_cfg;
std::vector<std::string> g_tracks;
size_t g_trackIndex = 0;

uint32_t g_stream = 0;
std::string g_savedTrack;
int32_t g_savedPosMs = 0;
float g_fadeAlpha = 0.0f;
int32_t g_lastAppliedVol = -1;
uint32_t g_lastFadeTick = 0;
uint32_t g_retryAfterTick = 0;

std::string DirOfPath(const std::string &fullPath) {
  const size_t pos = fullPath.find_last_of("\\/");
  if (pos == std::string::npos) {
    return std::string();
  }
  return fullPath.substr(0, pos);
}

void InitModulePaths(HMODULE hModule) {
  char buf[MAX_PATH] = {};
  if (GetModuleFileNameA(hModule, buf, MAX_PATH) > 0) {
    g_asiDir = DirOfPath(buf);
  }
  if (GetModuleFileNameA(nullptr, buf, MAX_PATH) > 0) {
    g_exeDir = DirOfPath(buf);
  }
  if (!g_asiDir.empty()) {
    g_iniPath = g_asiDir + "\\PauseMusic.ini";
  } else if (!g_exeDir.empty()) {
    g_iniPath = g_exeDir + "\\PauseMusic.ini";
  } else {
    g_iniPath = "PauseMusic.ini";
  }
}

std::string TrimQuotesAndSpace(const std::string &raw) {
  size_t start = 0;
  size_t end = raw.size();
  while (start < end && std::isspace(static_cast<unsigned char>(raw[start]))) {
    ++start;
  }
  while (end > start &&
         std::isspace(static_cast<unsigned char>(raw[end - 1]))) {
    --end;
  }
  if (end - start >= 2 && raw[start] == '"' && raw[end - 1] == '"') {
    ++start;
    --end;
  }
  return raw.substr(start, end - start);
}

bool EqualsIgnoreCase(const std::string &a, const std::string &b) {
  if (a.size() != b.size()) {
    return false;
  }
  for (size_t i = 0; i < a.size(); ++i) {
    if (std::tolower(static_cast<unsigned char>(a[i])) !=
        std::tolower(static_cast<unsigned char>(b[i]))) {
      return false;
    }
  }
  return true;
}

bool LessIgnoreCase(const std::string &a, const std::string &b) {
  const size_t n = (std::min)(a.size(), b.size());
  for (size_t i = 0; i < n; ++i) {
    const int ca = std::tolower(static_cast<unsigned char>(a[i]));
    const int cb = std::tolower(static_cast<unsigned char>(b[i]));
    if (ca != cb) {
      return ca < cb;
    }
  }
  return a.size() < b.size();
}

bool IsSupportedAudioFile(const std::string &path) {
  const size_t dot = path.find_last_of('.');
  if (dot == std::string::npos) {
    return false;
  }
  const std::string ext = path.substr(dot);
  return EqualsIgnoreCase(ext, ".mp3") || EqualsIgnoreCase(ext, ".wav");
}

bool IsAbsolutePath(const std::string &p) {
  if (p.size() >= 2 && p[1] == ':') {
    return true;
  }
  if (p.size() >= 2 &&
      ((p[0] == '\\' && p[1] == '\\') || (p[0] == '/' && p[1] == '/'))) {
    return true;
  }
  return false;
}

std::string ResolveAudioPath(const std::string &configured) {
  if (configured.empty()) {
    return std::string();
  }
  if (IsAbsolutePath(configured)) {
    return (GetFileAttributesA(configured.c_str()) != INVALID_FILE_ATTRIBUTES)
               ? configured
               : std::string();
  }
  if (!g_asiDir.empty()) {
    std::string cand = g_asiDir + "\\" + configured;
    if (GetFileAttributesA(cand.c_str()) != INVALID_FILE_ATTRIBUTES) {
      return cand;
    }
  }
  if (!g_exeDir.empty()) {
    std::string cand = g_exeDir + "\\" + configured;
    if (GetFileAttributesA(cand.c_str()) != INVALID_FILE_ATTRIBUTES) {
      return cand;
    }
  }
  if (GetFileAttributesA(configured.c_str()) != INVALID_FILE_ATTRIBUTES) {
    return configured;
  }
  return std::string();
}

void ScanDirectoryTracks(const std::string &dir,
                         std::vector<std::string> &out) {
  std::string cleanDir = dir;
  while (!cleanDir.empty() &&
         (cleanDir.back() == '\\' || cleanDir.back() == '/')) {
    cleanDir.pop_back();
  }
  const std::string pattern = cleanDir + "\\*";
  WIN32_FIND_DATAA fd = {};
  HANDLE hFind = FindFirstFileA(pattern.c_str(), &fd);
  if (hFind == INVALID_HANDLE_VALUE) {
    return;
  }
  do {
    if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
      std::string name = fd.cFileName;
      if (IsSupportedAudioFile(name)) {
        out.push_back(cleanDir + "\\" + name);
      }
    }
  } while (FindNextFileA(hFind, &fd));
  FindClose(hFind);
}

void ShuffleTracks(std::vector<std::string> &tracks) {
  if (tracks.size() <= 1) {
    return;
  }
  uint32_t state = static_cast<uint32_t>(GetTickCount()) ^
                   static_cast<uint32_t>(GetCurrentProcessId() * 2654435761u);
  for (size_t i = tracks.size() - 1; i > 0; --i) {
    state = state * 1664525u + 1013904223u;
    const size_t j = static_cast<size_t>(state) % (i + 1);
    std::swap(tracks[i], tracks[j]);
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

void CloseActiveStream(bool savePosition) {
  if (g_stream == 0) {
    return;
  }
  if (savePosition && g_cfg.resumePosition && g_trackIndex < g_tracks.size()) {
    int32_t totalMs = 0;
    int32_t curMs = -1;
    if (AilStreamMsPosition(g_stream, &totalMs, &curMs) && curMs >= 0) {
      g_savedTrack = g_tracks[g_trackIndex];
      g_savedPosMs = curMs;
    }
  } else if (!savePosition) {
    g_savedTrack.clear();
    g_savedPosMs = 0;
  }
  AilSetStreamVolume(g_stream, 0);
  AilPauseStream(g_stream, 1);
  AilCloseStream(g_stream);
  g_stream = 0;
  g_fadeAlpha = 0.0f;
  g_lastAppliedVol = -1;
}

std::string SelectActiveIniFile() {
  char pathBuf[1024] = {};
  if (!g_iniPath.empty() &&
      GetFileAttributesA(g_iniPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
    GetPrivateProfileStringA("PauseMusic", "Path", "", pathBuf,
                             static_cast<DWORD>(sizeof(pathBuf)),
                             g_iniPath.c_str());
    if (!TrimQuotesAndSpace(pathBuf).empty()) {
      return g_iniPath;
    }
  }
  if (!g_exeDir.empty()) {
    std::string rootIni = g_exeDir + "\\PauseMusic.ini";
    if (rootIni != g_iniPath &&
        GetFileAttributesA(rootIni.c_str()) != INVALID_FILE_ATTRIBUTES) {
      return rootIni;
    }
  }
  return g_iniPath;
}

void ReloadConfig(bool force) {
  const uint32_t now = GetTickCount();
  if (!force && now - g_lastIniCheckTick < 500) {
    return;
  }
  g_lastIniCheckTick = now;

  const std::string activeIni = SelectActiveIniFile();
  WIN32_FILE_ATTRIBUTE_DATA attr = {};
  if (!GetFileAttributesExA(activeIni.c_str(), GetFileExInfoStandard, &attr)) {
    if (force || g_hasIniTime) {
      g_hasIniTime = false;
      CloseActiveStream(false);
      g_tracks.clear();
      g_trackIndex = 0;
    }
    return;
  }

  if (!force && g_hasIniTime &&
      CompareFileTime(&attr.ftLastWriteTime, &g_iniWriteTime) == 0) {
    return;
  }

  g_iniWriteTime = attr.ftLastWriteTime;
  g_hasIniTime = true;

  PauseMusicConfig nextCfg;
  char pathBuf[1024] = {};
  GetPrivateProfileStringA("PauseMusic", "Path", "", pathBuf,
                           static_cast<DWORD>(sizeof(pathBuf)),
                           activeIni.c_str());
  nextCfg.path = TrimQuotesAndSpace(pathBuf);

  int32_t vol = static_cast<int32_t>(
      GetPrivateProfileIntA("PauseMusic", "Volume", 80, activeIni.c_str()));
  if (vol < 0)
    vol = 0;
  if (vol > 100)
    vol = 100;
  nextCfg.volume = vol;

  int32_t fadeIn = static_cast<int32_t>(
      GetPrivateProfileIntA("PauseMusic", "FadeInMs", 400, activeIni.c_str()));
  if (fadeIn < 0)
    fadeIn = 0;
  if (fadeIn > 10000)
    fadeIn = 10000;
  nextCfg.fadeInMs = fadeIn;

  int32_t fadeOut = static_cast<int32_t>(
      GetPrivateProfileIntA("PauseMusic", "FadeOutMs", 250, activeIni.c_str()));
  if (fadeOut < 0)
    fadeOut = 0;
  if (fadeOut > 10000)
    fadeOut = 10000;
  nextCfg.fadeOutMs = fadeOut;

  nextCfg.resumePosition = GetPrivateProfileIntA("PauseMusic", "ResumePosition",
                                                 1, activeIni.c_str()) != 0;
  nextCfg.shuffle =
      GetPrivateProfileIntA("PauseMusic", "Shuffle", 1, activeIni.c_str()) != 0;
  nextCfg.playOnMainMenu = GetPrivateProfileIntA("PauseMusic", "PlayOnMainMenu",
                                                 0, activeIni.c_str()) != 0;

  const bool pathOrOrderChanged = !EqualsIgnoreCase(nextCfg.path, g_cfg.path) ||
                                  (nextCfg.shuffle != g_cfg.shuffle);

  g_cfg = nextCfg;
  if (force || pathOrOrderChanged) {
    CloseActiveStream(false);
    g_tracks.clear();
    g_trackIndex = 0;

    const std::string resolved = ResolveAudioPath(g_cfg.path);
    if (!resolved.empty()) {
      const DWORD fileAttr = GetFileAttributesA(resolved.c_str());
      if (fileAttr != INVALID_FILE_ATTRIBUTES) {
        if ((fileAttr & FILE_ATTRIBUTE_DIRECTORY) != 0) {
          ScanDirectoryTracks(resolved, g_tracks);
          std::sort(g_tracks.begin(), g_tracks.end(), LessIgnoreCase);
          if (g_cfg.shuffle) {
            ShuffleTracks(g_tracks);
          }
        } else if (IsSupportedAudioFile(resolved)) {
          g_tracks.push_back(resolved);
        }
      }
    }
  }
}

bool ShouldPlayPauseMusic(bool *outImmediateCut = nullptr) {
  if (outImmediateCut) {
    *outImmediateCut = false;
  }

  if (g_tracks.empty()) {
    return false;
  }
  if (*reinterpret_cast<const uint8_t *>(ADDR_SAMPLE_INIT) == 0 ||
      *reinterpret_cast<const uint32_t *>(ADDR_DIG_DRIVER) == 0) {
    if (outImmediateCut) {
      *outImmediateCut = true;
    }
    return false;
  }

  const bool fgLost = (g_hwnd != nullptr) && (GetForegroundWindow() != g_hwnd);
  if (g_windowInactive || g_focusLost || fgLost) {
    if (outImmediateCut) {
      *outImmediateCut = true;
    }
    return false;
  }

  const int32_t gameState = *reinterpret_cast<const int32_t *>(ADDR_GAME_STATE);
  const uint8_t menuActive =
      *reinterpret_cast<const uint8_t *>(ADDR_MENU_ACTIVE);
  const uint8_t userPause = *reinterpret_cast<const uint8_t *>(ADDR_USER_PAUSE);
  const int32_t menuScreen =
      *reinterpret_cast<const int32_t *>(ADDR_MENU_SCREEN);

  if (menuScreen == MENUPAGE_SOUND_SETTINGS && menuActive != 0) {
    if (outImmediateCut) {
      *outImmediateCut = true;
    }
    return false;
  }

  if (gameState == GS_PLAYING_GAME) {
    return (menuActive != 0) || (userPause != 0);
  }
  if (gameState == GS_FRONTEND && g_cfg.playOnMainMenu) {
    return menuActive != 0;
  }

  if (outImmediateCut) {
    *outImmediateCut = true;
  }
  return false;
}

int32_t ComputeTargetMilesVolume() {
  uint8_t gameMusicVol =
      *reinterpret_cast<const uint8_t *>(ADDR_SAMPLE_MUSIC_VOL);
  if (gameMusicVol > 127) {
    gameMusicVol = 127;
  }
  int32_t vol = (static_cast<int32_t>(gameMusicVol) * g_cfg.volume) / 100;
  if (vol < 0)
    vol = 0;
  if (vol > 127)
    vol = 127;
  return vol;
}

bool OpenTrackStream(uint32_t digDriver, int32_t initialVolume) {
  if (g_tracks.empty() || g_trackIndex >= g_tracks.size()) {
    return false;
  }
  const std::string &track = g_tracks[g_trackIndex];
  const uint32_t stream = AilOpenStream(digDriver, track.c_str());
  if (stream == 0) {
    return false;
  }

  AilSetStreamLoopCount(stream, (g_tracks.size() == 1) ? 0 : 1);

  if (g_cfg.resumePosition && g_savedPosMs > 0 && g_savedTrack == track) {
    int32_t totalMs = 0;
    int32_t curMs = 0;
    AilStreamMsPosition(stream, &totalMs, &curMs);
    if (totalMs <= 0 || g_savedPosMs < totalMs) {
      AilSetStreamMsPosition(stream, g_savedPosMs);
    }
  }

  AilSetStreamVolume(stream, initialVolume);
  AilPauseStream(stream, 0);

  g_stream = stream;
  g_lastAppliedVol = initialVolume;
  return true;
}

void UpdatePauseMusic() {
  if (!g_lockInit) {
    return;
  }
  EnterCriticalSection(&g_lock);

  ReloadConfig(false);

  const uint32_t now = GetTickCount();
  uint32_t dtMs = (g_lastFadeTick != 0) ? (now - g_lastFadeTick) : 0;
  if (dtMs > 250) {
    dtMs = 250;
  }
  g_lastFadeTick = now;

  bool immediateCut = false;
  const bool wantPlay = ShouldPlayPauseMusic(&immediateCut);
  const uint32_t digDriver =
      *reinterpret_cast<const uint32_t *>(ADDR_DIG_DRIVER);
  const int32_t targetVol = ComputeTargetMilesVolume();

  if (wantPlay) {
    if (g_stream != 0 && g_tracks.size() > 1 &&
        AilStreamStatus(g_stream) == SMP_DONE) {
      CloseActiveStream(false);
      g_trackIndex = (g_trackIndex + 1) % g_tracks.size();
      if (g_trackIndex == 0 && g_cfg.shuffle) {
        ShuffleTracks(g_tracks);
      }
    }

    if (g_stream == 0 && now >= g_retryAfterTick) {
      g_fadeAlpha = (g_cfg.fadeInMs <= 0) ? 1.0f : 0.0f;
      const int32_t initVol = static_cast<int32_t>(
          static_cast<float>(targetVol) * g_fadeAlpha + 0.5f);
      if (!OpenTrackStream(digDriver, initVol)) {
        if (g_tracks.size() > 1) {
          g_trackIndex = (g_trackIndex + 1) % g_tracks.size();
        }
        g_retryAfterTick = now + 1000;
      }
    }

    if (g_stream != 0) {
      if (g_cfg.fadeInMs <= 0) {
        g_fadeAlpha = 1.0f;
      } else if (g_fadeAlpha < 1.0f) {
        g_fadeAlpha +=
            static_cast<float>(dtMs) / static_cast<float>(g_cfg.fadeInMs);
        if (g_fadeAlpha > 1.0f) {
          g_fadeAlpha = 1.0f;
        }
      }

      int32_t desiredVol = static_cast<int32_t>(
          static_cast<float>(targetVol) * g_fadeAlpha + 0.5f);
      if (desiredVol < 0)
        desiredVol = 0;
      if (desiredVol > 127)
        desiredVol = 127;

      if (desiredVol != g_lastAppliedVol) {
        AilSetStreamVolume(g_stream, desiredVol);
        g_lastAppliedVol = desiredVol;
      }
    }
  } else if (g_stream != 0) {
    if (immediateCut || g_cfg.fadeOutMs <= 0) {
      CloseActiveStream(true);
    } else {
      g_fadeAlpha -=
          static_cast<float>(dtMs) / static_cast<float>(g_cfg.fadeOutMs);
      if (g_fadeAlpha <= 0.001f) {
        CloseActiveStream(true);
      } else {
        int32_t desiredVol = static_cast<int32_t>(
            static_cast<float>(targetVol) * g_fadeAlpha + 0.5f);
        if (desiredVol < 0)
          desiredVol = 0;
        if (desiredVol != g_lastAppliedVol) {
          AilSetStreamVolume(g_stream, desiredVol);
          g_lastAppliedVol = desiredVol;
        }
      }
    }
  }

  LeaveCriticalSection(&g_lock);
}

DWORD WINAPI FocusWatchThread(LPVOID) {
  bool prevFgLost = false;
  while (WaitForSingleObject(g_stopEvent, 50) == WAIT_TIMEOUT) {
    if (g_hwnd && IsWindow(g_hwnd)) {
      const bool fgLost = (GetForegroundWindow() != g_hwnd);
      if (fgLost != prevFgLost) {
        prevFgLost = fgLost;
        PostMessageA(g_hwnd, WM_APP_SYNC_PAUSE_MUSIC, fgLost ? 1 : 0, 0);
      }
    }
  }
  return 0;
}

LRESULT CALLBACK Hook_WndProc(HWND hwnd, UINT msg, WPARAM wParam,
                              LPARAM lParam) {
  if (msg == WM_APP_SYNC_PAUSE_MUSIC) {
    UpdatePauseMusic();
    return 0;
  }
  if (msg == WM_ACTIVATEAPP) {
    g_windowInactive = (wParam == FALSE);
    UpdatePauseMusic();
  } else if (msg == WM_ACTIVATE) {
    g_windowInactive = (LOWORD(wParam) == WA_INACTIVE);
    UpdatePauseMusic();
  } else if (msg == WM_KILLFOCUS) {
    g_focusLost = true;
    UpdatePauseMusic();
  } else if (msg == WM_SETFOCUS) {
    g_focusLost = false;
    UpdatePauseMusic();
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
}

void __fastcall Hook_AudioService(void *self, void * /*edx*/) {
  g_origAudioService(self);
  EnsureWindowHooks();
  UpdatePauseMusic();
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
  const uint8_t op =
      *reinterpret_cast<const uint8_t *>(ADDR_CALL_AUDIO_SERVICE);
  if (op != 0xE8) {
    return false;
  }

  g_origRel32 = *reinterpret_cast<const int32_t *>(ADDR_CALL_AUDIO_SERVICE + 1);
  g_origAudioService = reinterpret_cast<AudioServiceFn>(
      ADDR_CALL_AUDIO_SERVICE + 5 + g_origRel32);

  const int32_t hookRel32 =
      static_cast<int32_t>(reinterpret_cast<uintptr_t>(&Hook_AudioService) -
                           (ADDR_CALL_AUDIO_SERVICE + 5));
  g_patched = PatchCallRel32(ADDR_CALL_AUDIO_SERVICE, hookRel32);
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

  if (g_lockInit) {
    EnterCriticalSection(&g_lock);
    CloseActiveStream(false);
    LeaveCriticalSection(&g_lock);
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
    PatchCallRel32(ADDR_CALL_AUDIO_SERVICE, g_origRel32);
    g_patched = false;
  }
}

} // namespace

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(hModule);
    InitializeCriticalSection(&g_lock);
    g_lockInit = true;
    InitModulePaths(hModule);
    ReloadConfig(true);
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
