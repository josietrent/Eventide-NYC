#define WIN32_LEAN_AND_MEAN
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <windows.h>

namespace {

// GTA III v1.0 globals and pools (verified against gta3.exe)
constexpr uintptr_t ADDR_THE_CAMERA = 0x6FACF8;   // CCamera TheCamera
constexpr uintptr_t ADDR_SCRIPT_SPACE = 0x74B248; // CTheScripts::ScriptSpace
constexpr uintptr_t ADDR_MODEL_INFO_PTRS =
    0x83D408; // CModelInfo::ms_modelInfoPtrs
constexpr uintptr_t ADDR_CULL_INDICES = 0x847330; // CCullZones::aIndices
constexpr uintptr_t ADDR_BIG_BLD_FOR_BLD =
    0x86C9D0; // aPointersToBigBuildingsForBuildings
constexpr uintptr_t ADDR_OBJECT_POOL_PTR = 0x880E28; // CPools::ms_pObjectPool
constexpr uintptr_t ADDR_TIME_MS = 0x885B48; // CTimer::m_snTimeInMilliseconds
constexpr uintptr_t ADDR_BIG_BLD_FOR_TREAD =
    0x8F1B8C; // aPointersToBigBuildingsForTreadables
constexpr uintptr_t ADDR_BUILDING_POOL_PTR =
    0x8F2C04;                                      // CPools::ms_pBuildingPool
constexpr uintptr_t ADDR_PED_POOL_PTR = 0x8F2C60;  // CPools::ms_pPedPool
constexpr uintptr_t ADDR_RS_MAX_WIDTH = 0x8F436C;  // RsGlobal.maximumWidth
constexpr uintptr_t ADDR_RS_MAX_HEIGHT = 0x8F4370; // RsGlobal.maximumHeight
constexpr uintptr_t ADDR_VEH_POOL_PTR = 0x9430DC;  // CPools::ms_pVehiclePool
constexpr uintptr_t ADDR_REPLAY_MODE = 0x95CD5B;   // CReplay::Mode
constexpr uintptr_t ADDR_USER_PAUSE = 0x95CD7C;    // CTimer::m_UserPause
constexpr uintptr_t ADDR_PREFS_SHOW_HUD =
    0x95CD89; // FrontEndMenuManager.m_PrefsShowHud
constexpr uintptr_t ADDR_CODE_PAUSE = 0x95CDB1; // CTimer::m_CodePause

// Pool capacity push-imm32 immediates inside CPools::Initialise (0x4A1770)
constexpr uintptr_t ADDR_OBJECT_POOL_LIMIT_IMM = 0x4A182A; // stock: 450 (0x1C2)
constexpr uintptr_t ADDR_DUMMY_POOL_LIMIT_IMM = 0x4A1847; // stock: 2802 (0xAF2)
constexpr uint32_t STOCK_OBJECT_POOL_LIMIT = 450;
constexpr uint32_t STOCK_DUMMY_POOL_LIMIT = 2802;
constexpr uint32_t TARGET_OBJECT_POOL_LIMIT = 1000;
constexpr uint32_t TARGET_DUMMY_POOL_LIMIT = 8192;

// GTA III v1.0 functions (verified against gta3.exe)
constexpr uintptr_t ADDR_BUILDING_NEW = 0x405820;
constexpr uintptr_t ADDR_SCRIPTS_PROCESS = 0x439040;
constexpr uintptr_t ADDR_LIGHT_FOR_CARS1 = 0x455760;
constexpr uintptr_t ADDR_LIGHT_FOR_CARS2 = 0x455790;
constexpr uintptr_t ADDR_FIND_TRAFFIC_LIGHT_TYPE = 0x4564A0;
constexpr uintptr_t ADDR_SHOULD_CAR_STOP_FOR_LIGHT = 0x456460;
constexpr uintptr_t ADDR_LOAD_OBJECT_INSTANCE = 0x4786B0;
constexpr uintptr_t ADDR_PHYSICAL_APPLY_COLLISION = 0x4973A0;
constexpr uintptr_t ADDR_FIND_PLAYER_VEHICLE = 0x4A10C0;
constexpr uintptr_t ADDR_FIND_PLAYER_PED = 0x4A1150;
constexpr uintptr_t ADDR_WANTED_SET_NO_DROP = 0x4ADAC0;
constexpr uintptr_t ADDR_FONT_PRINT_STRING = 0x500F50;
constexpr uintptr_t ADDR_FONT_DRAW_FONTS = 0x501B50;
constexpr uintptr_t ADDR_FONT_SET_SCALE = 0x501B80;
constexpr uintptr_t ADDR_FONT_SET_COLOR = 0x501BD0;
constexpr uintptr_t ADDR_FONT_SET_JUSTIFY_OFF = 0x501C80;
constexpr uintptr_t ADDR_FONT_SET_CENTRE_OFF = 0x501CB0;
constexpr uintptr_t ADDR_FONT_SET_BACKGROUND_OFF = 0x501CF0;
constexpr uintptr_t ADDR_FONT_SET_RIGHT_JUSTIFY_ON = 0x501D50;
constexpr uintptr_t ADDR_FONT_SET_RIGHT_JUSTIFY_OFF = 0x501D70;
constexpr uintptr_t ADDR_FONT_SET_PROP_ON = 0x501DA0;
constexpr uintptr_t ADDR_FONT_SET_FONT_STYLE = 0x501DB0;
constexpr uintptr_t ADDR_FONT_SET_RIGHT_JUSTIFY_WRAP = 0x501DC0;
constexpr uintptr_t ADDR_FONT_SET_DROP_SHADOW_POS = 0x501E70;
constexpr uintptr_t ADDR_HUD_DRAW = 0x5052A0;
constexpr uintptr_t ADDR_LOAD_CULL_ZONE_DATA = 0x524EC0;
constexpr uintptr_t ADDR_CLEAR_BLD_CULL_FLAG = 0x525680;
constexpr uintptr_t ADDR_SET_BLD_CULL_FLAG = 0x5258A0;
constexpr uintptr_t ADDR_AUTOMOBILE_PROCESS_CONTROL = 0x531470;
constexpr uintptr_t ADDR_MUSIC_INITIALISE = 0x57CF70;

// CPool layout & pool strides (verified at 0x43EAB0..0x43EB60,
// 0x4A17E0..0x4A1840)
constexpr size_t OFF_POOL_OBJECTS = 0x00;
constexpr size_t OFF_POOL_BYTEMAP = 0x04;
constexpr size_t OFF_POOL_SIZE = 0x08;
constexpr size_t BUILDING_POOL_STRIDE = 0x64;
constexpr size_t OBJECT_POOL_STRIDE = 0x19C;
constexpr size_t VEHICLE_POOL_STRIDE = 0x5A8;
constexpr size_t PED_POOL_STRIDE = 0x5F0;
constexpr int32_t BUILDING_POOL_SIZE = 5500;
constexpr int32_t TREADABLE_POOL_SIZE = 1214;
constexpr uint16_t CULL_TREE_BASE = 6000;

// CCamera / HUD constants
constexpr size_t OFF_CAM_WIDESCREEN = 0x70;
constexpr uint8_t REPLAY_MODE_PLAYBACK = 1;
constexpr int16_t FONT_HEADING = 2;

// CEntity / CPhysical / CObject offsets
constexpr size_t OFF_ENTITY_RIGHT = 0x04;        // float[3] m_matrix.right
constexpr size_t OFF_ENTITY_FWD = 0x14;          // float[3] m_matrix.up
constexpr size_t OFF_ENTITY_POS = 0x34;          // float[3] m_matrix.pos
constexpr size_t OFF_ENTITY_FLAGS_50 = 0x50;     // m_nType:3 | m_nStatus:5
constexpr size_t OFF_ENTITY_FLAGS_51 = 0x51;     // bit 2 (0x04) = bIsStatic
constexpr size_t OFF_ENTITY_FLAGS_53 = 0x53;     // bit 6 (0x40) = bCullHidden
constexpr size_t OFF_ENTITY_MODEL_INDEX = 0x5C;  // int16_t
constexpr size_t OFF_PHYSICAL_MOVE_SPEED = 0x78; // float[3]
constexpr size_t OFF_OBJECT_FLAGS_175 = 0x175; // bit 3 (0x08) = bHasBeenDamaged

constexpr uint8_t ENTITY_TYPE_MASK = 0x07;
constexpr uint8_t ENTITY_TYPE_VEHICLE = 2;
constexpr uint8_t ENTITY_TYPE_OBJECT = 4;
constexpr uint8_t ENTITY_FLAG_IS_STATIC = 0x04;
constexpr uint8_t OBJECT_FLAG_DAMAGED = 0x08;

constexpr uint8_t STATUS_SIMPLE = 2;
constexpr uint8_t STATUS_PHYSICS = 3;
constexpr uint8_t STATUS_WRECKED = 5;

// CVehicle / CAutopilot offsets
constexpr size_t OFF_VEH_AUTOPILOT_DRIVING_STYLE = 0x159;     // uint8_t
constexpr size_t OFF_VEH_AUTOPILOT_CAR_MISSION = 0x15A;       // uint8_t
constexpr size_t OFF_VEH_AUTOPILOT_TEMP_ACTION = 0x15B;       // uint8_t
constexpr size_t OFF_VEH_AUTOPILOT_TIME_TEMP_ACTION = 0x15C;  // uint32_t
constexpr size_t OFF_VEH_AUTOPILOT_MAX_TRAFFIC_SPEED = 0x160; // float
constexpr size_t OFF_VEH_AUTOPILOT_CRUISE_SPEED = 0x164;      // uint8_t
constexpr size_t OFF_VEH_DRIVER = 0x1A4;                      // CPed*
constexpr size_t OFF_VEH_CREATED_BY = 0x1F4;                  // uint8_t
constexpr size_t OFF_VEH_HEALTH = 0x200;                      // float
constexpr size_t OFF_VEH_SIREN_OR_ALARM = 0x22E;              // uint8_t
constexpr size_t OFF_VEH_CLASS = 0x284;                       // uint32_t

constexpr uint8_t DRIVINGSTYLE_STOP_FOR_CARS = 0;
constexpr uint8_t DRIVINGSTYLE_AVOID_CARS = 2;
constexpr uint8_t DRIVINGSTYLE_PLOUGH_THROUGH = 3;
constexpr uint8_t CAR_MISSION_CRUISE = 1;
constexpr uint8_t TEMPACT_WAIT = 1;
constexpr uint8_t TEMPACT_REVERSE = 2;
constexpr uint8_t MISSION_VEHICLE = 2;
constexpr uint32_t VEHICLE_AUTOMOBILE = 0;

// CPed / CPlayerPed / CWanted offsets
constexpr size_t OFF_PED_CREATED_BY = 0x160; // uint8_t
constexpr size_t OFF_PED_STATE = 0x224;      // uint32_t
constexpr size_t OFF_PED_HEALTH = 0x2C0;     // float
constexpr size_t OFF_PED_VEHICLE = 0x310;    // CVehicle*
constexpr size_t OFF_PED_IN_VEHICLE = 0x314; // uint8_t
constexpr size_t OFF_PED_TYPE = 0x32C;       // uint32_t
constexpr size_t OFF_PLAYER_WANTED = 0x53C;  // CWanted*
constexpr size_t OFF_WANTED_LEVEL = 0x18;    // int32_t

constexpr uint8_t MISSION_CHAR = 2;
constexpr uint32_t PEDSTATE_DIE = 48;
constexpr uint32_t PEDSTATE_DEAD = 49;
constexpr uint32_t PEDTYPE_COP = 6;

// CTrafficLights constants
constexpr uint8_t LIGHT_RED = 2;

// object.dat shift model IDs relative to stock GTA III
constexpr int16_t MI_VHEISTLOCDOOR = 1333;

// Streetlight & traffic signal model IDs (generic.IDE / object.dat)
constexpr int16_t MI_LAMPOST_COAST = 1342;
constexpr int16_t MI_LAMPPOST3 = 1345;
constexpr int16_t MI_STREETLAMP2 = 1352;
constexpr int16_t MI_STREETLAMP1 = 1353;
constexpr int16_t MI_LAMPPOST1 = 1393;
constexpr int16_t MI_DOUBLESTREETLGHT1 = 1394;
constexpr int16_t MI_LAMPPOST2 = 1404;
constexpr int16_t MI_TRAFFICLIGHT1 = 1405;

// Other breakable / uprootable city street furniture (generic.IDE / object.dat)
constexpr int16_t MI_FIRE_HYDRANT = 1322;
constexpr int16_t MI_BOLLARDLIGHT = 1328;
constexpr int16_t MI_PARKINGMETER = 1346;
constexpr int16_t MI_BIN1 = 1347;
constexpr int16_t MI_ROADWORKBARRIER1 = 1349;
constexpr int16_t MI_BUSSIGN1 = 1350;
constexpr int16_t MI_NOPARKINGSIGN1 = 1354;
constexpr int16_t MI_PHONESIGN = 1355;
constexpr int16_t MI_WASTEBIN = 1356;
constexpr int16_t MI_STRTBARRIER01 = 1359;
constexpr int16_t MI_TRAFFICCONE = 1360;
constexpr int16_t MI_POSTBOX1 = 1396;
constexpr int16_t MI_PARKBENCH1 = 1398;

// Pinned global variable slots in utils/globals.txt
constexpr size_t SLOT_ON_MISSION = 5002;
constexpr size_t SLOT_INTRO_PASSED = 5005;
constexpr size_t SLOT_BLEED_DONE = 5118;
constexpr size_t SLOT_POLE_TOTAL = 5146;
constexpr size_t SLOT_POLE_LAST_MODEL = 5147;
constexpr size_t SLOT_POLE_ENABLED = 5148;
constexpr size_t SLOT_COLD_OPEN_DONE = 5149;
constexpr size_t SLOT_TRAFFIC_LAW_ACTIVE = 5153;
constexpr size_t SLOT_TRAFFIC_LAW_EVENT = 5154;
constexpr size_t SLOT_TRAFFIC_LAW_FINE = 5155;
constexpr size_t SLOT_TRAFFIC_LAW_VALUE = 5156;
constexpr int32_t POLE_MODE_DISABLED = 2;

// Event IDs passed to threads/poles.txt
constexpr int32_t TLAW_EVT_HYDRANT = 1;
constexpr int32_t TLAW_EVT_POLE = 2;
constexpr int32_t TLAW_EVT_CITY_PROP = 3;
constexpr int32_t TLAW_EVT_SPEED_COP = 4;
constexpr int32_t TLAW_EVT_SPEED_CAM = 5;
constexpr int32_t TLAW_EVT_REDLIGHT_COP = 6;
constexpr int32_t TLAW_EVT_REDLIGHT_CAM = 7;
constexpr int32_t TLAW_EVT_HIT_AND_RUN = 8;

// Tuning parameters
constexpr uint8_t AI_MAX_CRUISE_SPEED =
    12; // ~27 mph (stock: 12..15, road-rage: 35..60)
constexpr float AI_MAX_TRAFFIC_SPEED = 12.0f;
constexpr uint32_t BILLIARD_ATTRIBUTION_MS = 2500;
constexpr float COP_WITNESS_RADIUS = 58.0f;
constexpr float COP_WITNESS_Z_TOL = 12.0f;
constexpr float SPEED_TO_MPH = 50.0f * 2.23693629f; // 111.8468f
constexpr float SPEED_LIMIT_STREET_MPH = 44.0f;     // 35 mph + 9 mph grace
constexpr float SPEED_LIMIT_HIGHWAY_MPH = 65.0f;    // 55 mph + 10 mph grace
constexpr uint32_t SPEEDING_SUSTAIN_MS = 1200;
constexpr uint32_t SPEED_PENALTY_COOLDOWN_MS = 7000;
constexpr uint32_t REDLIGHT_PENALTY_COOLDOWN_MS = 6000;
constexpr uint32_t PROPERTY_PENALTY_COOLDOWN_MS = 2000;
constexpr uint32_t CRASH_PENALTY_COOLDOWN_MS = 4000;
constexpr uint32_t CAMERA_STRIKE_WINDOW_MS = 90000;
constexpr size_t CAMERA_STRIKE_ESCALATION_COUNT = 3;

struct CRGBA {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
};

using MusicBoolFn = bool(__thiscall *)(void *self);
using BuildingNewFn = void *(__cdecl *)(uint32_t size);
using LoadObjectInstanceFn = void(__cdecl *)(const char *line);
using LoadCullZoneDataFn = void(__cdecl *)();
using BuildingCullFlagFn = void(__cdecl *)(uint32_t stockIdx);
using HudDrawFn = void(__cdecl *)();
using ScriptsProcessFn = void(__cdecl *)();
using ApplyCollisionFn = bool(__thiscall *)(void *self, void *entityB,
                                            void *colPoint, float *impulseA,
                                            float *impulseB);
using AutomobileProcessControlFn = void(__thiscall *)(void *self);
using FindPlayerVehicleFn = void *(__cdecl *)();
using FindPlayerPedFn = void *(__cdecl *)();
using LightStateFn = uint8_t(__cdecl *)();
using FindTrafficLightTypeFn = int32_t(__cdecl *)(void *entity);
using ShouldCarStopForLightFn = bool(__cdecl *)(void *veh);
using WantedSetLevelNoDropFn = void(__thiscall *)(void *wanted, int32_t level);
using FontVoidFn = void(__cdecl *)();
using FontScaleFn = void(__cdecl *)(float w, float h);
using FontColorFn = void(__cdecl *)(const CRGBA *color);
using FontFloatFn = void(__cdecl *)(float val);
using FontStyleFn = void(__cdecl *)(int16_t style);
using FontDropShadowFn = void(__cdecl *)(int16_t pos);
using FontPrintStringFn = void(__cdecl *)(float x, float y, const wchar_t *s);

struct HookEntry {
  uintptr_t addr = 0;
  uint8_t origBytes[16] = {};
  size_t patchLen = 0;
  uint8_t *trampoline = nullptr;
  bool active = false;
};

struct Vec3 {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};

struct CountedObject {
  void *obj = nullptr;
  int16_t modelIndex = 0;
};

struct BumpedVehicle {
  void *veh = nullptr;
  uint32_t untilMs = 0;
};

struct TrackedSignal {
  void *obj = nullptr;
  bool enteredOnRed = false;
  uint32_t lastSeenMs = 0;
};

std::string g_logPath;
std::vector<HookEntry> g_hooks;
bool g_deferredInstalled = false;

MusicBoolFn g_origInitialise = nullptr;
BuildingNewFn g_origBuildingNew = nullptr;
LoadObjectInstanceFn g_origLoadObjectInstance = nullptr;
LoadCullZoneDataFn g_origLoadCullZoneData = nullptr;
BuildingCullFlagFn g_origClearBldCullFlag = nullptr;
BuildingCullFlagFn g_origSetBldCullFlag = nullptr;
HudDrawFn g_origHudDraw = nullptr;
ScriptsProcessFn g_origScriptsProcess = nullptr;
ApplyCollisionFn g_origApplyCollision = nullptr;
AutomobileProcessControlFn g_origAutomobileProcessControl = nullptr;

// Stock ms_pBuildingPool index -> actual runtime ms_pBuildingPool index
// (compensates for Streetlamp1/Streetlamp2/lampost_coast becoming
// CDummyObjects)
int16_t g_oldToNewBuilding[BUILDING_POOL_SIZE] = {};
int32_t g_stockBuildingIdx = 0;
int32_t g_actualBuildingCount = 0;
uintptr_t g_trackedBuildingPoolPtr = 0;
bool g_inLoadObjectInstance = false;
int32_t g_lastNewBuildingIdx = -1;

std::vector<CountedObject> g_countedObjects;
std::vector<BumpedVehicle> g_bumpedVehicles;
std::vector<uint32_t> g_cameraStrikeTimes;
TrackedSignal g_trackedSignal{};

uint32_t g_speedingStartMs = 0;
uint32_t g_lastSpeedPenaltyMs = 0;
uint32_t g_lastRedLightPenaltyMs = 0;
uint32_t g_lastPropertyPenaltyMs = 0;
uint32_t g_lastCrashPenaltyMs = 0;

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
      g_logPath = full.substr(0, pos) + "\\TrafficLaw.log";
    } else {
      g_logPath = "TrafficLaw.log";
    }
  } else {
    g_logPath = "TrafficLaw.log";
  }
  HANDLE f = CreateFileA(g_logPath.c_str(), GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                         CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (f != INVALID_HANDLE_VALUE) {
    CloseHandle(f);
  }
}

void RaisePoolLimitImm(uintptr_t addr, uint32_t expectedMin, uint32_t target,
                       const char *label) {
  const uint32_t cur = *reinterpret_cast<const uint32_t *>(addr);
  if (cur >= target) {
    char msg[128] = {};
    std::snprintf(msg, sizeof(msg), "%s already >= %u (%u)", label, target,
                  cur);
    WriteLog(msg);
    return;
  }
  if (cur < expectedMin) {
    char msg[128] = {};
    std::snprintf(msg, sizeof(msg), "%s unexpected immediate %u at 0x%08X",
                  label, cur, static_cast<unsigned>(addr));
    WriteLog(msg);
    return;
  }

  DWORD oldProt = 0;
  if (VirtualProtect(reinterpret_cast<void *>(addr), sizeof(uint32_t),
                     PAGE_EXECUTE_READWRITE, &oldProt)) {
    *reinterpret_cast<uint32_t *>(addr) = target;
    VirtualProtect(reinterpret_cast<void *>(addr), sizeof(uint32_t), oldProt,
                   &oldProt);
    FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void *>(addr),
                          sizeof(uint32_t));
    char msg[128] = {};
    std::snprintf(msg, sizeof(msg), "%s raised %u -> %u", label, cur, target);
    WriteLog(msg);
  } else {
    WriteLog("VirtualProtect failed while raising pool limit");
  }
}

void ApplyPoolLimits() {
  // Adding Streetlamp1/Streetlamp2/lampost_coast to object.dat converts
  // hundreds of map props into CDummyObject entries and overflows stock
  // DummyPool (2802).
  RaisePoolLimitImm(ADDR_OBJECT_POOL_LIMIT_IMM, STOCK_OBJECT_POOL_LIMIT,
                    TARGET_OBJECT_POOL_LIMIT, "ObjectPool");
  RaisePoolLimitImm(ADDR_DUMMY_POOL_LIMIT_IMM, STOCK_DUMMY_POOL_LIMIT,
                    TARGET_DUMMY_POOL_LIMIT, "DummyPool");
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

template <typename T> inline T &FieldAt(void *base, size_t off) {
  return *reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(base) + off);
}

template <typename T> inline const T &FieldAt(const void *base, size_t off) {
  return *reinterpret_cast<const T *>(reinterpret_cast<uintptr_t>(base) + off);
}

inline int32_t *ScriptGlobalPtr(size_t slot) {
  return reinterpret_cast<int32_t *>(ADDR_SCRIPT_SPACE + slot * 4);
}

inline uint32_t TimeMs() {
  return *reinterpret_cast<const uint32_t *>(ADDR_TIME_MS);
}

// ============================================================================
// CCullZones / ms_pBuildingPool Index Remapping
// Moving Streetlamp1/Streetlamp2/lampost_coast into object.dat turns them into
// CDummyObjects instead of CBuildings, shifting subsequent ms_pBuildingPool
// slots relative to Rockstar's pre-baked data/cullzone.dat.
// ============================================================================
void ResetBuildingRemap(uintptr_t pool) {
  std::fill(std::begin(g_oldToNewBuilding), std::end(g_oldToNewBuilding),
            int16_t{-1});
  g_stockBuildingIdx = 0;
  g_actualBuildingCount = 0;
  g_trackedBuildingPoolPtr = pool;
}

void *__cdecl Hook_BuildingNew(uint32_t size) {
  void *ptr = g_origBuildingNew(size);
  if (g_inLoadObjectInstance && ptr) {
    const uintptr_t pool =
        *reinterpret_cast<const uintptr_t *>(ADDR_BUILDING_POOL_PTR);
    if (pool) {
      const uintptr_t objectsBase = FieldAt<uintptr_t>(
          reinterpret_cast<const void *>(pool), OFF_POOL_OBJECTS);
      const uintptr_t p = reinterpret_cast<uintptr_t>(ptr);
      if (objectsBase && p >= objectsBase) {
        const uintptr_t diff = p - objectsBase;
        if (diff % BUILDING_POOL_STRIDE == 0) {
          const int32_t idx = static_cast<int32_t>(diff / BUILDING_POOL_STRIDE);
          if (idx >= 0 && idx < BUILDING_POOL_SIZE) {
            g_lastNewBuildingIdx = idx;
          }
        }
      }
    }
  }
  return ptr;
}

void __cdecl Hook_LoadObjectInstance(const char *line) {
  const uintptr_t pool =
      *reinterpret_cast<const uintptr_t *>(ADDR_BUILDING_POOL_PTR);
  if (pool) {
    const uint8_t *byteMap = FieldAt<const uint8_t *>(
        reinterpret_cast<const void *>(pool), OFF_POOL_BYTEMAP);
    if (pool != g_trackedBuildingPoolPtr ||
        (g_actualBuildingCount > 0 && byteMap && (byteMap[0] & 0x80) != 0)) {
      ResetBuildingRemap(pool);
    }
  }

  int32_t modelId = -1;
  if (line) {
    char *endPtr = nullptr;
    const long parsed = std::strtol(line, &endPtr, 10);
    if (endPtr != line) {
      modelId = static_cast<int32_t>(parsed);
    }
  }

  g_inLoadObjectInstance = true;
  g_lastNewBuildingIdx = -1;
  g_origLoadObjectInstance(line);
  g_inLoadObjectInstance = false;

  if (g_lastNewBuildingIdx >= 0) {
    ++g_actualBuildingCount;
    if (modelId != MI_VHEISTLOCDOOR &&
        g_stockBuildingIdx < BUILDING_POOL_SIZE) {
      g_oldToNewBuilding[g_stockBuildingIdx++] =
          static_cast<int16_t>(g_lastNewBuildingIdx);
    }
  } else if (modelId == MI_LAMPOST_COAST || modelId == MI_STREETLAMP2 ||
             modelId == MI_STREETLAMP1) {
    void *const *modelInfoPtrs =
        reinterpret_cast<void *const *>(ADDR_MODEL_INFO_PTRS);
    if (modelInfoPtrs[modelId] != nullptr &&
        g_stockBuildingIdx < BUILDING_POOL_SIZE) {
      g_oldToNewBuilding[g_stockBuildingIdx++] = -1;
    }
  }
}

void SetOrClearSingleBuildingCullFlag(int32_t poolIdx, bool setFlag) {
  if (poolIdx < 0 || poolIdx >= BUILDING_POOL_SIZE) {
    return;
  }
  const uintptr_t pool =
      *reinterpret_cast<const uintptr_t *>(ADDR_BUILDING_POOL_PTR);
  if (!pool) {
    return;
  }
  const uintptr_t objectsBase = FieldAt<uintptr_t>(
      reinterpret_cast<const void *>(pool), OFF_POOL_OBJECTS);
  const uint8_t *byteMap = FieldAt<const uint8_t *>(
      reinterpret_cast<const void *>(pool), OFF_POOL_BYTEMAP);
  const int32_t poolSize =
      FieldAt<int32_t>(reinterpret_cast<const void *>(pool), OFF_POOL_SIZE);
  if (!objectsBase || !byteMap || poolIdx >= poolSize) {
    return;
  }
  if ((byteMap[poolIdx] & 0x80) != 0) {
    return;
  }
  uint8_t &flags53 = FieldAt<uint8_t>(
      reinterpret_cast<void *>(objectsBase + static_cast<uintptr_t>(poolIdx) *
                                                 BUILDING_POOL_STRIDE),
      OFF_ENTITY_FLAGS_53);
  if (setFlag) {
    flags53 = (flags53 & 0xBF) | 0x40;
  } else {
    flags53 &= 0xBF;
  }
}

void ApplyBuildingCullFlag(uint16_t stockIdx, bool setFlag) {
  if (stockIdx >= CULL_TREE_BASE) {
    const uint16_t *indices =
        reinterpret_cast<const uint16_t *>(ADDR_CULL_INDICES);
    const uint32_t base = static_cast<uint32_t>(stockIdx - CULL_TREE_BASE);
    ApplyBuildingCullFlag(indices[base], setFlag);
    ApplyBuildingCullFlag(indices[base + 1], setFlag);
    ApplyBuildingCullFlag(indices[base + 2], setFlag);
    return;
  }

  if (stockIdx < BUILDING_POOL_SIZE) {
    const int16_t newIdx = g_oldToNewBuilding[stockIdx];
    if (newIdx >= 0) {
      SetOrClearSingleBuildingCullFlag(newIdx, setFlag);
    }
    const int16_t *bigBld =
        reinterpret_cast<const int16_t *>(ADDR_BIG_BLD_FOR_BLD);
    const int16_t stockBig = bigBld[stockIdx];
    if (stockBig >= 0 && stockBig < BUILDING_POOL_SIZE) {
      const int16_t newBig = g_oldToNewBuilding[stockBig];
      if (newBig >= 0) {
        SetOrClearSingleBuildingCullFlag(newBig, setFlag);
      }
    }
  }
}

void __cdecl Hook_ClearBuildingCullFlag(uint32_t stockIdx) {
  ApplyBuildingCullFlag(static_cast<uint16_t>(stockIdx), false);
}

void __cdecl Hook_SetBuildingCullFlag(uint32_t stockIdx) {
  ApplyBuildingCullFlag(static_cast<uint16_t>(stockIdx), true);
}

void __cdecl Hook_LoadCullZoneData() {
  g_origLoadCullZoneData();
  int16_t *bigTread = reinterpret_cast<int16_t *>(ADDR_BIG_BLD_FOR_TREAD);
  int32_t remappedTread = 0;
  for (int32_t i = 0; i < TREADABLE_POOL_SIZE; ++i) {
    const int16_t oldIdx = bigTread[i];
    if (oldIdx >= 0 && oldIdx < BUILDING_POOL_SIZE) {
      bigTread[i] = g_oldToNewBuilding[oldIdx];
      ++remappedTread;
    } else {
      bigTread[i] = -1;
    }
  }

  char msg[160] = {};
  std::snprintf(
      msg, sizeof(msg),
      "LoadCullZoneData: stockBlds=%d actualBlds=%d remappedBigTread=%d",
      g_stockBuildingIdx, g_actualBuildingCount, remappedTread);
  WriteLog(msg);
}

inline Vec3 GetEntityPos(const void *entity) {
  const float *pos = &FieldAt<float>(entity, OFF_ENTITY_POS);
  return {pos[0], pos[1], pos[2]};
}

inline Vec3 GetEntityFwd(const void *entity) {
  const float *fwd = &FieldAt<float>(entity, OFF_ENTITY_FWD);
  return {fwd[0], fwd[1], fwd[2]};
}

inline Vec3 GetEntityRight(const void *entity) {
  const float *right = &FieldAt<float>(entity, OFF_ENTITY_RIGHT);
  return {right[0], right[1], right[2]};
}

inline bool IsEntityType(const void *entity, uint8_t type) {
  return (FieldAt<uint8_t>(entity, OFF_ENTITY_FLAGS_50) & ENTITY_TYPE_MASK) ==
         type;
}

inline uint8_t GetEntityStatus(const void *entity) {
  return (FieldAt<uint8_t>(entity, OFF_ENTITY_FLAGS_50) >> 3) & 0x1F;
}

inline bool IsEntityStatic(const void *entity) {
  return (FieldAt<uint8_t>(entity, OFF_ENTITY_FLAGS_51) &
          ENTITY_FLAG_IS_STATIC) != 0;
}

inline bool IsObjectDamaged(const void *obj) {
  return (FieldAt<uint8_t>(obj, OFF_OBJECT_FLAGS_175) & OBJECT_FLAG_DAMAGED) !=
         0;
}

inline bool IsPedAlive(const void *ped) {
  if (!ped) {
    return false;
  }
  if (FieldAt<float>(ped, OFF_PED_HEALTH) <= 0.0f) {
    return false;
  }
  const uint32_t state = FieldAt<uint32_t>(ped, OFF_PED_STATE);
  return state != PEDSTATE_DIE && state != PEDSTATE_DEAD;
}

inline bool IsTargetPoleModel(int16_t modelIndex) {
  return modelIndex == MI_LAMPOST_COAST || modelIndex == MI_LAMPPOST1 ||
         modelIndex == MI_LAMPPOST2 || modelIndex == MI_LAMPPOST3 ||
         modelIndex == MI_STREETLAMP1 || modelIndex == MI_STREETLAMP2 ||
         modelIndex == MI_DOUBLESTREETLGHT1 || modelIndex == MI_TRAFFICLIGHT1;
}

inline bool IsOtherCityPropertyModel(int16_t modelIndex) {
  return modelIndex == MI_FIRE_HYDRANT || modelIndex == MI_BOLLARDLIGHT ||
         modelIndex == MI_PARKINGMETER || modelIndex == MI_BIN1 ||
         modelIndex == MI_ROADWORKBARRIER1 || modelIndex == MI_BUSSIGN1 ||
         modelIndex == MI_NOPARKINGSIGN1 || modelIndex == MI_PHONESIGN ||
         modelIndex == MI_WASTEBIN || modelIndex == MI_STRTBARRIER01 ||
         modelIndex == MI_TRAFFICCONE || modelIndex == MI_POSTBOX1 ||
         modelIndex == MI_PARKBENCH1;
}

inline bool IsTrackedCityPropertyModel(int16_t modelIndex) {
  return IsTargetPoleModel(modelIndex) || IsOtherCityPropertyModel(modelIndex);
}

bool IsIntactCityProperty(const void *entity, int16_t *outModel,
                          bool *outStatic, bool *outDamaged) {
  if (!entity || !IsEntityType(entity, ENTITY_TYPE_OBJECT)) {
    return false;
  }
  const int16_t mi = FieldAt<int16_t>(entity, OFF_ENTITY_MODEL_INDEX);
  if (!IsTrackedCityPropertyModel(mi)) {
    return false;
  }
  const bool isStatic = IsEntityStatic(entity);
  const bool isDamaged = IsObjectDamaged(entity);
  if (!isStatic && isDamaged) {
    return false;
  }
  if (outModel) {
    *outModel = mi;
  }
  if (outStatic) {
    *outStatic = isStatic;
  }
  if (outDamaged) {
    *outDamaged = isDamaged;
  }
  return true;
}

inline bool IsPostStoryTrafficLawActive() {
  // Testing gate: enabled as soon as 'BEACH' prints ($ColdOpenDone == 1).
  // Restore to SLOT_BLEED_DONE != 0 && SLOT_INTRO_PASSED != 0 after testing.
  return *ScriptGlobalPtr(SLOT_COLD_OPEN_DONE) != 0 &&
         *ScriptGlobalPtr(SLOT_ON_MISSION) == 0 &&
         *ScriptGlobalPtr(SLOT_TRAFFIC_LAW_ACTIVE) != 0;
}

inline int32_t GetPlayerWantedLevel(const void *playerPed) {
  if (!playerPed) {
    return 0;
  }
  const void *wanted = FieldAt<void *>(playerPed, OFF_PLAYER_WANTED);
  if (!wanted) {
    return 0;
  }
  return FieldAt<int32_t>(wanted, OFF_WANTED_LEVEL);
}

void GiveWantedStarAtLeast1(void *playerPed) {
  if (!playerPed) {
    return;
  }
  void *wanted = FieldAt<void *>(playerPed, OFF_PLAYER_WANTED);
  if (!wanted) {
    return;
  }
  if (FieldAt<int32_t>(wanted, OFF_WANTED_LEVEL) < 1) {
    auto setNoDrop =
        reinterpret_cast<WantedSetLevelNoDropFn>(ADDR_WANTED_SET_NO_DROP);
    setNoDrop(wanted, 1);
  }
}

void QueueTrafficLawBanner(int32_t eventId, int32_t fine, int32_t value) {
  if (*ScriptGlobalPtr(SLOT_TRAFFIC_LAW_EVENT) == 0) {
    *ScriptGlobalPtr(SLOT_TRAFFIC_LAW_EVENT) = eventId;
    *ScriptGlobalPtr(SLOT_TRAFFIC_LAW_FINE) = fine;
    *ScriptGlobalPtr(SLOT_TRAFFIC_LAW_VALUE) = value;
  }
}

bool IsCopNearby(const Vec3 &pos, float radiusXY, float zTol) {
  const uintptr_t pool =
      *reinterpret_cast<const uintptr_t *>(ADDR_PED_POOL_PTR);
  if (!pool) {
    return false;
  }

  const uintptr_t objectsBase = FieldAt<uintptr_t>(
      reinterpret_cast<const void *>(pool), OFF_POOL_OBJECTS);
  const uint8_t *byteMap = FieldAt<const uint8_t *>(
      reinterpret_cast<const void *>(pool), OFF_POOL_BYTEMAP);
  const int32_t poolSize =
      FieldAt<int32_t>(reinterpret_cast<const void *>(pool), OFF_POOL_SIZE);
  if (!objectsBase || !byteMap || poolSize <= 0) {
    return false;
  }

  auto findPlayerPed = reinterpret_cast<FindPlayerPedFn>(ADDR_FIND_PLAYER_PED);
  void *playerPed = findPlayerPed();
  const float rSq = radiusXY * radiusXY;

  for (int32_t i = 0; i < poolSize; ++i) {
    if ((byteMap[i] & 0x80) != 0) {
      continue;
    }
    const void *ped =
        reinterpret_cast<const void *>(objectsBase + i * PED_POOL_STRIDE);
    if (ped == playerPed) {
      continue;
    }
    if (FieldAt<uint32_t>(ped, OFF_PED_TYPE) != PEDTYPE_COP ||
        !IsPedAlive(ped)) {
      continue;
    }
    const bool inVeh = FieldAt<uint8_t>(ped, OFF_PED_IN_VEHICLE) != 0;
    const void *veh = FieldAt<void *>(ped, OFF_PED_VEHICLE);
    const Vec3 copPos = (inVeh && veh) ? GetEntityPos(veh) : GetEntityPos(ped);
    const float dz = std::fabs(copPos.z - pos.z);
    if (dz > zTol) {
      continue;
    }
    const float dx = copPos.x - pos.x;
    const float dy = copPos.y - pos.y;
    if (dx * dx + dy * dy <= rSq) {
      return true;
    }
  }
  return false;
}

bool RegisterCameraStrike(uint32_t nowMs) {
  g_cameraStrikeTimes.erase(
      std::remove_if(g_cameraStrikeTimes.begin(), g_cameraStrikeTimes.end(),
                     [nowMs](uint32_t t) {
                       return nowMs < t ||
                              (nowMs - t) > CAMERA_STRIKE_WINDOW_MS;
                     }),
      g_cameraStrikeTimes.end());
  g_cameraStrikeTimes.push_back(nowMs);
  if (g_cameraStrikeTimes.size() >= CAMERA_STRIKE_ESCALATION_COUNT) {
    g_cameraStrikeTimes.clear();
    return true;
  }
  return false;
}

void RecordBumpedVehicle(void *veh, uint32_t nowMs) {
  if (!veh) {
    return;
  }
  const uint32_t untilMs = nowMs + BILLIARD_ATTRIBUTION_MS;
  for (auto &entry : g_bumpedVehicles) {
    if (entry.veh == veh) {
      entry.untilMs = untilMs;
      return;
    }
  }
  g_bumpedVehicles.push_back({veh, untilMs});
}

bool WasRecentlyBumpedByPlayer(const void *veh, uint32_t nowMs) {
  if (!veh) {
    return false;
  }
  for (const auto &entry : g_bumpedVehicles) {
    if (entry.veh == veh && static_cast<int32_t>(entry.untilMs - nowMs) > 0) {
      return true;
    }
  }
  return false;
}

bool IsObjectAlreadyCounted(const void *obj) {
  for (const auto &entry : g_countedObjects) {
    if (entry.obj == obj) {
      return true;
    }
  }
  return false;
}

void PruneTrackedState(uint32_t nowMs) {
  g_bumpedVehicles.erase(
      std::remove_if(g_bumpedVehicles.begin(), g_bumpedVehicles.end(),
                     [nowMs](const BumpedVehicle &e) {
                       return static_cast<int32_t>(e.untilMs - nowMs) <= 0;
                     }),
      g_bumpedVehicles.end());

  if (g_countedObjects.empty()) {
    return;
  }

  const uintptr_t pool =
      *reinterpret_cast<const uintptr_t *>(ADDR_OBJECT_POOL_PTR);
  if (pool == 0) {
    g_countedObjects.clear();
    return;
  }

  const uintptr_t objectsBase = FieldAt<uintptr_t>(
      reinterpret_cast<const void *>(pool), OFF_POOL_OBJECTS);
  const uint8_t *byteMap = FieldAt<const uint8_t *>(
      reinterpret_cast<const void *>(pool), OFF_POOL_BYTEMAP);
  const int32_t poolSize =
      FieldAt<int32_t>(reinterpret_cast<const void *>(pool), OFF_POOL_SIZE);

  if (!objectsBase || !byteMap || poolSize <= 0) {
    g_countedObjects.clear();
    return;
  }

  const uintptr_t poolEnd =
      objectsBase + static_cast<uintptr_t>(poolSize) * OBJECT_POOL_STRIDE;

  g_countedObjects.erase(
      std::remove_if(
          g_countedObjects.begin(), g_countedObjects.end(),
          [&](const CountedObject &entry) {
            const uintptr_t p = reinterpret_cast<uintptr_t>(entry.obj);
            if (p < objectsBase || p >= poolEnd) {
              return true;
            }
            const uintptr_t offset = p - objectsBase;
            if (offset % OBJECT_POOL_STRIDE != 0) {
              return true;
            }
            const int32_t idx =
                static_cast<int32_t>(offset / OBJECT_POOL_STRIDE);
            if (idx < 0 || idx >= poolSize || (byteMap[idx] & 0x80) != 0) {
              return true;
            }
            if (!IsEntityType(entry.obj, ENTITY_TYPE_OBJECT)) {
              return true;
            }
            if (FieldAt<int16_t>(entry.obj, OFF_ENTITY_MODEL_INDEX) !=
                entry.modelIndex) {
              return true;
            }
            // If the object re-streamed upright and intact, allow future hit
            if (IsEntityStatic(entry.obj) && !IsObjectDamaged(entry.obj)) {
              return true;
            }
            return false;
          }),
      g_countedObjects.end());
}

// ============================================================================
// Always-on AI Driver Taming (active regardless of story completion)
// ============================================================================
bool ShouldTameVehicle(void *veh, void *playerPed) {
  if (!veh) {
    return false;
  }
  if (FieldAt<uint8_t>(veh, OFF_VEH_CREATED_BY) == MISSION_VEHICLE) {
    return false;
  }
  const uint8_t status = GetEntityStatus(veh);
  if (status != STATUS_SIMPLE && status != STATUS_PHYSICS) {
    return false;
  }
  if (FieldAt<float>(veh, OFF_VEH_HEALTH) <= 0.0f) {
    return false;
  }
  if (FieldAt<uint8_t>(veh, OFF_VEH_SIREN_OR_ALARM) != 0) {
    return false;
  }
  if (FieldAt<uint8_t>(veh, OFF_VEH_AUTOPILOT_CAR_MISSION) !=
      CAR_MISSION_CRUISE) {
    return false;
  }
  void *driver = FieldAt<void *>(veh, OFF_VEH_DRIVER);
  if (!driver || driver == playerPed || !IsPedAlive(driver)) {
    return false;
  }
  if (FieldAt<uint8_t>(driver, OFF_PED_CREATED_BY) == MISSION_CHAR) {
    return false;
  }
  return true;
}

void ApplyAIDriverTaming(void *veh, uint32_t nowMs) {
  uint8_t &style = FieldAt<uint8_t>(veh, OFF_VEH_AUTOPILOT_DRIVING_STYLE);
  if (style == DRIVINGSTYLE_AVOID_CARS ||
      style == DRIVINGSTYLE_PLOUGH_THROUGH) {
    style = DRIVINGSTYLE_STOP_FOR_CARS;
  }

  uint8_t &cruise = FieldAt<uint8_t>(veh, OFF_VEH_AUTOPILOT_CRUISE_SPEED);
  if (cruise > AI_MAX_CRUISE_SPEED) {
    cruise = AI_MAX_CRUISE_SPEED;
  }

  float &maxSpeed = FieldAt<float>(veh, OFF_VEH_AUTOPILOT_MAX_TRAFFIC_SPEED);
  if (maxSpeed > AI_MAX_TRAFFIC_SPEED) {
    maxSpeed = AI_MAX_TRAFFIC_SPEED;
  }

  // Stop drivers from snapping into reverse while waiting at red traffic lights
  uint8_t &tempAct = FieldAt<uint8_t>(veh, OFF_VEH_AUTOPILOT_TEMP_ACTION);
  if (tempAct == TEMPACT_REVERSE) {
    auto shouldStop = reinterpret_cast<ShouldCarStopForLightFn>(
        ADDR_SHOULD_CAR_STOP_FOR_LIGHT);
    if (shouldStop(veh)) {
      tempAct = TEMPACT_WAIT;
      FieldAt<uint32_t>(veh, OFF_VEH_AUTOPILOT_TIME_TEMP_ACTION) = nowMs + 250;
    }
  }
}

void __fastcall Hook_AutomobileProcessControl(void *self, uintptr_t) {
  auto findPlayerPed = reinterpret_cast<FindPlayerPedFn>(ADDR_FIND_PLAYER_PED);
  void *playerPed = findPlayerPed();
  const bool tame = ShouldTameVehicle(self, playerPed);
  const uint32_t nowMs = tame ? TimeMs() : 0;

  if (tame) {
    ApplyAIDriverTaming(self, nowMs);
  }

  g_origAutomobileProcessControl(self);

  if (tame && ShouldTameVehicle(self, playerPed)) {
    ApplyAIDriverTaming(self, nowMs);
  }
}

// ============================================================================
// Post-Story "Be Good" Traffic Law Checks + Bottom-Right Speedometer
// ============================================================================
inline bool IsHighwayOrTunnel(float x, float y, float z) {
  // Porter Tunnel underground roadways
  if (z <= 4.0f) {
    return true;
  }
  // Callahan Bridge + approaches (Portland <-> Staunton)
  if (x >= 380.0f && x <= 880.0f && y >= -980.0f && y <= -890.0f &&
      z >= 13.0f) {
    return true;
  }
  // Shoreside Lift Bridge (Staunton <-> Shoreside Vale)
  if (x >= -260.0f && x <= -110.0f && y >= -335.0f && y <= -245.0f &&
      z >= 10.0f) {
    return true;
  }
  return false;
}

void DrawSpeedometer() {
  if (*reinterpret_cast<const uint8_t *>(ADDR_REPLAY_MODE) ==
          REPLAY_MODE_PLAYBACK ||
      *reinterpret_cast<const uint8_t *>(ADDR_PREFS_SHOW_HUD) == 0 ||
      *reinterpret_cast<const uint8_t *>(ADDR_USER_PAUSE) != 0 ||
      *reinterpret_cast<const uint8_t *>(ADDR_CODE_PAUSE) != 0 ||
      *reinterpret_cast<const uint8_t *>(ADDR_THE_CAMERA +
                                         OFF_CAM_WIDESCREEN) != 0) {
    return;
  }

  auto findPlayerPed = reinterpret_cast<FindPlayerPedFn>(ADDR_FIND_PLAYER_PED);
  auto findPlayerVeh =
      reinterpret_cast<FindPlayerVehicleFn>(ADDR_FIND_PLAYER_VEHICLE);
  void *playerPed = findPlayerPed();
  void *playerVeh = findPlayerVeh();
  if (!playerPed || !playerVeh ||
      FieldAt<void *>(playerVeh, OFF_VEH_DRIVER) != playerPed) {
    return;
  }

  const int32_t screenW = *reinterpret_cast<const int32_t *>(ADDR_RS_MAX_WIDTH);
  const int32_t screenH =
      *reinterpret_cast<const int32_t *>(ADDR_RS_MAX_HEIGHT);
  if (screenW <= 0 || screenH <= 0) {
    return;
  }

  const float *vel = &FieldAt<float>(playerVeh, OFF_PHYSICAL_MOVE_SPEED);
  const float speed2D = std::sqrt(vel[0] * vel[0] + vel[1] * vel[1]);
  const float mph = speed2D * SPEED_TO_MPH;
  const int32_t mphRounded = static_cast<int32_t>(mph + 0.5f);

  wchar_t label[32] = {};
  std::swprintf(label, sizeof(label) / sizeof(label[0]), L"%d MPH", mphRounded);

  CRGBA fg{147, 196, 211, 255};
  if (IsPostStoryTrafficLawActive() &&
      FieldAt<uint32_t>(playerVeh, OFF_VEH_CLASS) == VEHICLE_AUTOMOBILE &&
      FieldAt<uint8_t>(playerVeh, OFF_VEH_SIREN_OR_ALARM) == 0) {
    const Vec3 vehPos = GetEntityPos(playerVeh);
    const float limitMph = IsHighwayOrTunnel(vehPos.x, vehPos.y, vehPos.z)
                               ? SPEED_LIMIT_HIGHWAY_MPH
                               : SPEED_LIMIT_STREET_MPH;
    if (mph > limitMph) {
      fg = CRGBA{228, 74, 74, 255};
    } else if (mph >= limitMph - 5.0f) {
      fg = CRGBA{235, 198, 72, 255};
    }
  }

  const float scaleX = static_cast<float>(screenW) / 640.0f;
  const float scaleY = static_cast<float>(screenH) / 448.0f;
  const float posX = static_cast<float>(screenW) - 24.0f * scaleX;
  const float posY = static_cast<float>(screenH) - 64.0f * scaleY;

  auto setJustifyOff = reinterpret_cast<FontVoidFn>(ADDR_FONT_SET_JUSTIFY_OFF);
  auto setCentreOff = reinterpret_cast<FontVoidFn>(ADDR_FONT_SET_CENTRE_OFF);
  auto setBgOff = reinterpret_cast<FontVoidFn>(ADDR_FONT_SET_BACKGROUND_OFF);
  auto setRightOn =
      reinterpret_cast<FontVoidFn>(ADDR_FONT_SET_RIGHT_JUSTIFY_ON);
  auto setRightOff =
      reinterpret_cast<FontVoidFn>(ADDR_FONT_SET_RIGHT_JUSTIFY_OFF);
  auto setRightWrap =
      reinterpret_cast<FontFloatFn>(ADDR_FONT_SET_RIGHT_JUSTIFY_WRAP);
  auto setScale = reinterpret_cast<FontScaleFn>(ADDR_FONT_SET_SCALE);
  auto setPropOn = reinterpret_cast<FontVoidFn>(ADDR_FONT_SET_PROP_ON);
  auto setFontStyle = reinterpret_cast<FontStyleFn>(ADDR_FONT_SET_FONT_STYLE);
  auto setDropShadow =
      reinterpret_cast<FontDropShadowFn>(ADDR_FONT_SET_DROP_SHADOW_POS);
  auto setColor = reinterpret_cast<FontColorFn>(ADDR_FONT_SET_COLOR);
  auto printString =
      reinterpret_cast<FontPrintStringFn>(ADDR_FONT_PRINT_STRING);
  auto drawFonts = reinterpret_cast<FontVoidFn>(ADDR_FONT_DRAW_FONTS);

  setJustifyOff();
  setCentreOff();
  setBgOff();
  setRightOn();
  setRightWrap(0.0f);
  setScale(0.72f * scaleX, 1.25f * scaleY);
  setPropOn();
  setFontStyle(FONT_HEADING);
  setDropShadow(0);

  const CRGBA shadow{0, 0, 0, 255};
  setColor(&shadow);
  printString(posX + 2.0f, posY + 2.0f, label);

  setColor(&fg);
  printString(posX, posY, label);

  setRightOff();
  drawFonts();
}

void __cdecl Hook_HudDraw() {
  g_origHudDraw();
  DrawSpeedometer();
}

void ResetContinuousTrafficState() {
  g_speedingStartMs = 0;
  g_trackedSignal = {};
}

void UpdateTrafficLaw(uint32_t nowMs) {
  if (!IsPostStoryTrafficLawActive()) {
    ResetContinuousTrafficState();
    g_cameraStrikeTimes.clear();
    return;
  }

  auto findPlayerPed = reinterpret_cast<FindPlayerPedFn>(ADDR_FIND_PLAYER_PED);
  auto findPlayerVeh =
      reinterpret_cast<FindPlayerVehicleFn>(ADDR_FIND_PLAYER_VEHICLE);
  void *playerPed = findPlayerPed();
  void *playerVeh = findPlayerVeh();

  if (!playerPed || !playerVeh ||
      FieldAt<void *>(playerVeh, OFF_VEH_DRIVER) != playerPed ||
      FieldAt<uint32_t>(playerVeh, OFF_VEH_CLASS) != VEHICLE_AUTOMOBILE ||
      FieldAt<uint8_t>(playerVeh, OFF_VEH_SIREN_OR_ALARM) != 0) {
    ResetContinuousTrafficState();
    return;
  }

  // Don't stack speeding/red-light tickets mid-police-chase
  if (GetPlayerWantedLevel(playerPed) > 0) {
    ResetContinuousTrafficState();
    return;
  }

  const Vec3 vehPos = GetEntityPos(playerVeh);
  const float *vel = &FieldAt<float>(playerVeh, OFF_PHYSICAL_MOVE_SPEED);
  const float speed2D = std::sqrt(vel[0] * vel[0] + vel[1] * vel[1]);
  const float mph = speed2D * SPEED_TO_MPH;

  // 1. Speeding check
  const float limitMph = IsHighwayOrTunnel(vehPos.x, vehPos.y, vehPos.z)
                             ? SPEED_LIMIT_HIGHWAY_MPH
                             : SPEED_LIMIT_STREET_MPH;
  if (mph > limitMph) {
    if (g_speedingStartMs == 0) {
      g_speedingStartMs = nowMs;
    } else if (nowMs - g_speedingStartMs >= SPEEDING_SUSTAIN_MS &&
               nowMs - g_lastSpeedPenaltyMs >= SPEED_PENALTY_COOLDOWN_MS) {
      g_lastSpeedPenaltyMs = nowMs;
      g_speedingStartMs = 0;
      const int32_t mphRounded =
          std::max<int32_t>(1, static_cast<int32_t>(mph + 0.5f));
      const bool copNear = IsCopNearby(vehPos, 65.0f, COP_WITNESS_Z_TOL);
      if (copNear || RegisterCameraStrike(nowMs)) {
        QueueTrafficLawBanner(TLAW_EVT_SPEED_COP, 150, mphRounded);
        GiveWantedStarAtLeast1(playerPed);
      } else {
        QueueTrafficLawBanner(TLAW_EVT_SPEED_CAM, 100, mphRounded);
      }
    }
  } else if (mph < limitMph - 4.0f) {
    g_speedingStartMs = 0;
  }

  // 2. Red-light intersection check
  if (g_trackedSignal.obj && (nowMs - g_trackedSignal.lastSeenMs) > 2500) {
    g_trackedSignal = {};
  }

  if (speed2D < 0.045f) {
    // Allow staying stopped at a light to preserve its enteredOnRed state
    if (g_trackedSignal.obj) {
      g_trackedSignal.lastSeenMs = nowMs;
    }
    return;
  }

  const Vec3 rawFwd = GetEntityFwd(playerVeh);
  const float fwdLen = std::sqrt(rawFwd.x * rawFwd.x + rawFwd.y * rawFwd.y);
  if (fwdLen < 0.001f) {
    return;
  }
  const float vfx = rawFwd.x / fwdLen;
  const float vfy = rawFwd.y / fwdLen;

  // Must be driving forwards (not reversing)
  if ((vel[0] * vfx + vel[1] * vfy) <= 0.03f) {
    return;
  }

  const uintptr_t pool =
      *reinterpret_cast<const uintptr_t *>(ADDR_OBJECT_POOL_PTR);
  if (!pool) {
    return;
  }
  const uintptr_t objectsBase = FieldAt<uintptr_t>(
      reinterpret_cast<const void *>(pool), OFF_POOL_OBJECTS);
  const uint8_t *byteMap = FieldAt<const uint8_t *>(
      reinterpret_cast<const void *>(pool), OFF_POOL_BYTEMAP);
  const int32_t poolSize =
      FieldAt<int32_t>(reinterpret_cast<const void *>(pool), OFF_POOL_SIZE);
  if (!objectsBase || !byteMap || poolSize <= 0) {
    return;
  }

  auto findLightType =
      reinterpret_cast<FindTrafficLightTypeFn>(ADDR_FIND_TRAFFIC_LIGHT_TYPE);
  auto lightCars1 = reinterpret_cast<LightStateFn>(ADDR_LIGHT_FOR_CARS1);
  auto lightCars2 = reinterpret_cast<LightStateFn>(ADDR_LIGHT_FOR_CARS2);

  void *bestLight = nullptr;
  float bestAbsS = 1e9f;
  float bestS = 0.0f;

  for (int32_t i = 0; i < poolSize; ++i) {
    if ((byteMap[i] & 0x80) != 0) {
      continue;
    }
    void *obj = reinterpret_cast<void *>(objectsBase + i * OBJECT_POOL_STRIDE);
    if (!IsEntityType(obj, ENTITY_TYPE_OBJECT) || !IsEntityStatic(obj)) {
      continue;
    }
    if (FieldAt<int16_t>(obj, OFF_ENTITY_MODEL_INDEX) != MI_TRAFFICLIGHT1) {
      continue;
    }

    const Vec3 lPos = GetEntityPos(obj);
    if (std::fabs(vehPos.z - lPos.z) > 6.0f) {
      continue;
    }
    const float dx = vehPos.x - lPos.x;
    const float dy = vehPos.y - lPos.y;
    if (dx * dx + dy * dy > 32.0f * 32.0f) {
      continue;
    }

    const Vec3 lFwd = GetEntityFwd(obj);
    const Vec3 lRight = GetEntityRight(obj);
    const float parallelDot = std::fabs(vfx * lFwd.x + vfy * lFwd.y);
    if (parallelDot < 0.75f) {
      continue;
    }
    const float lateralDist = std::fabs(dx * lRight.x + dy * lRight.y);
    if (lateralDist > 18.0f) {
      continue;
    }

    const float s = dx * vfx + dy * vfy;
    if (s < -18.0f || s > 16.0f) {
      continue;
    }

    const float absS = std::fabs(s);
    if (obj == g_trackedSignal.obj) {
      bestLight = obj;
      bestS = s;
      break;
    }
    if (absS < bestAbsS) {
      bestAbsS = absS;
      bestLight = obj;
      bestS = s;
    }
  }

  if (!bestLight) {
    return;
  }

  const int32_t lightType = findLightType(bestLight);
  const uint8_t lightState = (lightType == 1) ? lightCars1() : lightCars2();

  if (bestS >= -18.0f && bestS <= -1.5f) {
    if (g_trackedSignal.obj != bestLight) {
      g_trackedSignal.obj = bestLight;
      g_trackedSignal.enteredOnRed = (lightState == LIGHT_RED);
    } else if (lightState != LIGHT_RED) {
      // Green or yellow during approach means legal entry
      g_trackedSignal.enteredOnRed = false;
    }
    g_trackedSignal.lastSeenMs = nowMs;
  } else if (bestS >= 3.0f) {
    if (g_trackedSignal.obj == bestLight && g_trackedSignal.enteredOnRed &&
        lightState == LIGHT_RED && speed2D >= 0.055f &&
        nowMs - g_lastRedLightPenaltyMs >= REDLIGHT_PENALTY_COOLDOWN_MS) {
      g_lastRedLightPenaltyMs = nowMs;
      const bool copNear = IsCopNearby(vehPos, 65.0f, COP_WITNESS_Z_TOL);
      if (copNear || RegisterCameraStrike(nowMs)) {
        QueueTrafficLawBanner(TLAW_EVT_REDLIGHT_COP, 150, 0);
        GiveWantedStarAtLeast1(playerPed);
      } else {
        QueueTrafficLawBanner(TLAW_EVT_REDLIGHT_CAM, 100, 0);
      }
    }
    if (g_trackedSignal.obj == bestLight) {
      g_trackedSignal = {};
    }
  }
}

bool __fastcall Hook_ApplyCollision(void *self, uintptr_t, void *entityB,
                                    void *colPoint, float *impulseA,
                                    float *impulseB) {
  if (!self || !entityB) {
    return g_origApplyCollision(self, entityB, colPoint, impulseA, impulseB);
  }

  void *prop = nullptr;
  void *hitter = nullptr;
  int16_t propModel = 0;
  bool wasStatic = false;
  bool wasDamaged = false;

  void *otherVehInCrash = nullptr;
  Vec3 preRelVel{};
  float prePlayerSpeed = 0.0f;

  auto findPlayerVeh =
      reinterpret_cast<FindPlayerVehicleFn>(ADDR_FIND_PLAYER_VEHICLE);
  auto findPlayerPed = reinterpret_cast<FindPlayerPedFn>(ADDR_FIND_PLAYER_PED);
  void *playerVeh = findPlayerVeh();
  void *playerPed = findPlayerPed();
  const uint32_t nowMs = TimeMs();

  if (IsIntactCityProperty(entityB, &propModel, &wasStatic, &wasDamaged)) {
    prop = entityB;
    hitter = self;
  } else if (IsIntactCityProperty(self, &propModel, &wasStatic, &wasDamaged)) {
    prop = self;
    hitter = entityB;
  } else if (IsEntityType(self, ENTITY_TYPE_VEHICLE) &&
             IsEntityType(entityB, ENTITY_TYPE_VEHICLE) && playerVeh) {
    if (self == playerVeh) {
      RecordBumpedVehicle(entityB, nowMs);
      otherVehInCrash = entityB;
    } else if (entityB == playerVeh) {
      RecordBumpedVehicle(self, nowMs);
      otherVehInCrash = self;
    }

    if (otherVehInCrash && playerPed &&
        FieldAt<void *>(playerVeh, OFF_VEH_DRIVER) == playerPed &&
        FieldAt<uint8_t>(playerVeh, OFF_VEH_SIREN_OR_ALARM) == 0 &&
        FieldAt<uint32_t>(playerVeh, OFF_VEH_CLASS) == VEHICLE_AUTOMOBILE &&
        GetEntityStatus(otherVehInCrash) != STATUS_WRECKED &&
        FieldAt<void *>(otherVehInCrash, OFF_VEH_DRIVER) != nullptr &&
        FieldAt<void *>(otherVehInCrash, OFF_VEH_DRIVER) != playerPed) {
      const float *pVel = &FieldAt<float>(playerVeh, OFF_PHYSICAL_MOVE_SPEED);
      const float *oVel =
          &FieldAt<float>(otherVehInCrash, OFF_PHYSICAL_MOVE_SPEED);
      prePlayerSpeed = std::sqrt(pVel[0] * pVel[0] + pVel[1] * pVel[1]);
      preRelVel = {pVel[0] - oVel[0], pVel[1] - oVel[1], 0.0f};
    } else {
      otherVehInCrash = nullptr;
    }
  }

  const bool result =
      g_origApplyCollision(self, entityB, colPoint, impulseA, impulseB);

  // Check city property uproot / damage (streetlights, hydrants, meters, signs)
  if (prop) {
    const bool newlyBroken = (wasStatic && !IsEntityStatic(prop)) ||
                             (!wasDamaged && IsObjectDamaged(prop));

    if (newlyBroken && playerPed) {
      const bool byPlayer = (playerVeh && hitter == playerVeh) ||
                            (hitter == playerPed) ||
                            (IsEntityType(hitter, ENTITY_TYPE_VEHICLE) &&
                             WasRecentlyBumpedByPlayer(hitter, nowMs));

      if (byPlayer && !IsObjectAlreadyCounted(prop)) {
        g_countedObjects.push_back({prop, propModel});

        const Vec3 propPos = GetEntityPos(prop);
        const bool copWitnessed =
            IsPostStoryTrafficLawActive() &&
            IsCopNearby(propPos, COP_WITNESS_RADIUS, COP_WITNESS_Z_TOL);

        if (copWitnessed) {
          if (nowMs - g_lastPropertyPenaltyMs >= PROPERTY_PENALTY_COOLDOWN_MS) {
            g_lastPropertyPenaltyMs = nowMs;
            int32_t evt = TLAW_EVT_CITY_PROP;
            int32_t fine = 125;
            if (propModel == MI_FIRE_HYDRANT) {
              evt = TLAW_EVT_HYDRANT;
              fine = 250;
            } else if (IsTargetPoleModel(propModel)) {
              evt = TLAW_EVT_POLE;
              fine = 150;
            }
            QueueTrafficLawBanner(evt, fine, 0);
            GiveWantedStarAtLeast1(playerPed);
          }
        } else if (IsTargetPoleModel(propModel) &&
                   *ScriptGlobalPtr(SLOT_POLE_ENABLED) != POLE_MODE_DISABLED) {
          *ScriptGlobalPtr(SLOT_POLE_TOTAL) += 1;
          *ScriptGlobalPtr(SLOT_POLE_LAST_MODEL) =
              static_cast<int32_t>(propModel);
        }
      }
    }
  }

  // Check post-story at-fault high-speed vehicle collision
  if (otherVehInCrash && IsPostStoryTrafficLawActive() &&
      GetPlayerWantedLevel(playerPed) == 0 &&
      nowMs - g_lastCrashPenaltyMs >= CRASH_PENALTY_COOLDOWN_MS) {
    const float relSpeed =
        std::sqrt(preRelVel.x * preRelVel.x + preRelVel.y * preRelVel.y);
    if (prePlayerSpeed >= 0.14f && relSpeed >= 0.24f) {
      const Vec3 pPos = GetEntityPos(playerVeh);
      const Vec3 oPos = GetEntityPos(otherVehInCrash);
      const Vec3 pFwd = GetEntityFwd(playerVeh);
      const float forwardHit =
          (oPos.x - pPos.x) * pFwd.x + (oPos.y - pPos.y) * pFwd.y;
      if (forwardHit > 0.0f) {
        const bool copNear = IsCopNearby(pPos, 60.0f, COP_WITNESS_Z_TOL);
        if (copNear || relSpeed >= 0.34f) {
          g_lastCrashPenaltyMs = nowMs;
          if (copNear || RegisterCameraStrike(nowMs)) {
            QueueTrafficLawBanner(TLAW_EVT_HIT_AND_RUN, 200, 0);
            GiveWantedStarAtLeast1(playerPed);
          } else {
            QueueTrafficLawBanner(TLAW_EVT_HIT_AND_RUN, 150, 0);
          }
        }
      }
    }
  }

  return result;
}

void __cdecl Hook_ScriptsProcess() {
  g_origScriptsProcess();
  const uint32_t nowMs = TimeMs();
  PruneTrackedState(nowMs);
  UpdateTrafficLaw(nowMs);
}

void InstallDeferredHooks() {
  if (g_deferredInstalled) {
    return;
  }
  g_deferredInstalled = true;
  ResetBuildingRemap(0);

  const bool okBldNew =
      InstallDetour(ADDR_BUILDING_NEW, &Hook_BuildingNew, &g_origBuildingNew);
  const bool okLoadInst =
      InstallDetour(ADDR_LOAD_OBJECT_INSTANCE, &Hook_LoadObjectInstance,
                    &g_origLoadObjectInstance);
  const bool okLoadCull =
      InstallDetour(ADDR_LOAD_CULL_ZONE_DATA, &Hook_LoadCullZoneData,
                    &g_origLoadCullZoneData);
  const bool okClearCull =
      InstallDetour(ADDR_CLEAR_BLD_CULL_FLAG, &Hook_ClearBuildingCullFlag,
                    &g_origClearBldCullFlag);
  const bool okSetCull = InstallDetour(
      ADDR_SET_BLD_CULL_FLAG, &Hook_SetBuildingCullFlag, &g_origSetBldCullFlag);
  const bool okHudDraw =
      InstallDetour(ADDR_HUD_DRAW, &Hook_HudDraw, &g_origHudDraw);
  const bool okCol = InstallDetour(ADDR_PHYSICAL_APPLY_COLLISION,
                                   &Hook_ApplyCollision, &g_origApplyCollision);
  const bool okScr = InstallDetour(ADDR_SCRIPTS_PROCESS, &Hook_ScriptsProcess,
                                   &g_origScriptsProcess);
  const bool okCar = InstallDetour(ADDR_AUTOMOBILE_PROCESS_CONTROL,
                                   &Hook_AutomobileProcessControl,
                                   &g_origAutomobileProcessControl);

  WriteLog(okBldNew ? "BuildingNew hook (0x405820) OK"
                    : "BuildingNew hook (0x405820) FAILED");
  WriteLog(okLoadInst ? "LoadObjectInstance hook (0x4786B0) OK"
                      : "LoadObjectInstance hook (0x4786B0) FAILED");
  WriteLog(okLoadCull ? "LoadCullZoneData hook (0x524EC0) OK"
                      : "LoadCullZoneData hook (0x524EC0) FAILED");
  WriteLog(okClearCull ? "ClearBuildingCullFlag hook (0x525680) OK"
                       : "ClearBuildingCullFlag hook (0x525680) FAILED");
  WriteLog(okSetCull ? "SetBuildingCullFlag hook (0x5258A0) OK"
                     : "SetBuildingCullFlag hook (0x5258A0) FAILED");
  WriteLog(okHudDraw ? "HudDraw hook (0x5052A0) OK"
                     : "HudDraw hook (0x5052A0) FAILED");
  WriteLog(okCol ? "ApplyCollision hook (0x4973A0) OK"
                 : "ApplyCollision hook (0x4973A0) FAILED");
  WriteLog(okScr ? "ScriptsProcess hook (0x439040) OK"
                 : "ScriptsProcess hook (0x439040) FAILED");
  WriteLog(okCar ? "AutomobileProcessControl hook (0x531470) OK"
                 : "AutomobileProcessControl hook (0x531470) FAILED");
}

bool __fastcall Hook_Initialise(void *self, uintptr_t) {
  const bool ok = g_origInitialise(self);
  InstallDeferredHooks();
  return ok;
}

bool InstallBootHook() {
  ApplyPoolLimits();
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
