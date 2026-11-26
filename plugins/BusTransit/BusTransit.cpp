#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
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

// ============================================================================
// GTA III v1.0 Addresses & Offsets (verified against gta3.exe)
// ============================================================================
constexpr uintptr_t ADDR_THE_CAMERA = 0x6FACF8; // CCamera TheCamera
constexpr uintptr_t ADDR_STREAMING_INFO =
    0x6C7090; // CStreaming::ms_aInfoForModel (stride 20)
constexpr uintptr_t ADDR_SCRIPT_SPACE = 0x74B248; // CTheScripts::ScriptSpace
constexpr uintptr_t ADDR_TIME_MS = 0x885B48; // CTimer::m_snTimeInMilliseconds
constexpr uintptr_t ADDR_PED_POOL_PTR = 0x8F2C60;  // CPools::ms_pPedPool
constexpr uintptr_t ADDR_RS_MAX_WIDTH = 0x8F436C;  // RsGlobal.maximumWidth
constexpr uintptr_t ADDR_RS_MAX_HEIGHT = 0x8F4370; // RsGlobal.maximumHeight
constexpr uintptr_t ADDR_PLAYER_MONEY = 0x94139C; // CWorld::Players[0].m_nMoney
constexpr uintptr_t ADDR_PLAYER_WB_STATE =
    0x9413C8; // CWorld::Players[0].m_WBState
constexpr uintptr_t ADDR_VEH_POOL_PTR = 0x9430DC; // CPools::ms_pVehiclePool
constexpr uintptr_t ADDR_REPLAY_MODE = 0x95CD5B;  // CReplay::Mode
constexpr uintptr_t ADDR_USER_PAUSE = 0x95CD7C;   // CTimer::m_UserPause
constexpr uintptr_t ADDR_PREFS_SHOW_HUD =
    0x95CD89; // FrontEndMenuManager.m_PrefsShowHud
constexpr uintptr_t ADDR_CODE_PAUSE = 0x95CDB1; // CTimer::m_CodePause

// Functions
constexpr uintptr_t ADDR_ANIM_BLEND = 0x403710;
constexpr uintptr_t ADDR_STREAMING_REQUEST_MODEL = 0x407EA0;
constexpr uintptr_t ADDR_STREAMING_LOAD_ALL = 0x40A440;
constexpr uintptr_t ADDR_STREAMING_LOAD_SCENE = 0x40A6D0;
constexpr uintptr_t ADDR_STREAMING_SET_DELETABLE = 0x40A820;
constexpr uintptr_t ADDR_COLLISION_PROCESS_COL_MODELS = 0x410BE0;
constexpr uintptr_t ADDR_JOIN_CAR_GOTO_COORS = 0x41FA00;
constexpr uintptr_t ADDR_SCRIPTS_PROCESS = 0x439040;
constexpr uintptr_t ADDR_CLEAR_SPACE_FOR_ENTITY = 0x454060;
constexpr uintptr_t ADDR_CAMERA_FADE = 0x46B3A0;
constexpr uintptr_t ADDR_ENTITY_UPDATE_RW_FRAME = 0x474330;
constexpr uintptr_t ADDR_GET_DIST_COM_TO_BASE = 0x4755C0;
constexpr uintptr_t ADDR_PLACEABLE_SET_HEADING = 0x49FA00;
constexpr uintptr_t ADDR_FIND_PLAYER_VEHICLE = 0x4A10C0;
constexpr uintptr_t ADDR_FIND_PLAYER_PED = 0x4A1150;
constexpr uintptr_t ADDR_ENTITY_REGISTER_REF = 0x4A7480;
constexpr uintptr_t ADDR_TIMER_UPDATE = 0x4ACF70;
constexpr uintptr_t ADDR_TIMER_STOP = 0x4AD480;
constexpr uintptr_t ADDR_WORLD_ADD = 0x4AE930;
constexpr uintptr_t ADDR_FIND_GROUND_Z = 0x4B3A80;
constexpr uintptr_t ADDR_ZONES_GET_LEVEL = 0x4B6910;
constexpr uintptr_t ADDR_MATRIX_UPDATE_RW = 0x4B8EC0;
constexpr uintptr_t ADDR_CIV_PED_CTOR = 0x4BFF30;
constexpr uintptr_t ADDR_CIV_PED_NEW = 0x4C5220;
constexpr uintptr_t ADDR_PED_STOP_CLUMP_ATOMIC = 0x4C5D50;
constexpr uintptr_t ADDR_PED_WARP_INTO_CAR = 0x4D7D20;
constexpr uintptr_t ADDR_PED_SET_OBJECTIVE_ENT = 0x4D83E0;
constexpr uintptr_t ADDR_FONT_PRINT_STRING = 0x500F50;
constexpr uintptr_t ADDR_FONT_DRAW_FONTS = 0x501B50;
constexpr uintptr_t ADDR_FONT_SET_SCALE = 0x501B80;
constexpr uintptr_t ADDR_FONT_SET_COLOR = 0x501BD0;
constexpr uintptr_t ADDR_FONT_SET_JUSTIFY_OFF = 0x501C80;
constexpr uintptr_t ADDR_FONT_SET_CENTRE_ON = 0x501C90;
constexpr uintptr_t ADDR_FONT_SET_CENTRE_OFF = 0x501CB0;
constexpr uintptr_t ADDR_FONT_SET_WRAPX = 0x501CC0;
constexpr uintptr_t ADDR_FONT_SET_CENTRE_SIZE = 0x501CD0;
constexpr uintptr_t ADDR_FONT_SET_BACKGROUND_OFF = 0x501CF0;
constexpr uintptr_t ADDR_FONT_SET_RIGHT_JUSTIFY_OFF = 0x501D70;
constexpr uintptr_t ADDR_FONT_SET_PROP_ON = 0x501DA0;
constexpr uintptr_t ADDR_FONT_SET_FONT_STYLE = 0x501DB0;
constexpr uintptr_t ADDR_FONT_SET_ALPHA_FADE = 0x501DD0;
constexpr uintptr_t ADDR_FONT_SET_DROP_COLOR = 0x501DE0;
constexpr uintptr_t ADDR_FONT_SET_DROP_SHADOW_POS = 0x501E70;
constexpr uintptr_t ADDR_HUD_DRAW = 0x5052A0;
constexpr uintptr_t ADDR_AUTOMOBILE_CTOR = 0x52C6B0;
constexpr uintptr_t ADDR_AUTOMOBILE_NEW = 0x551120;
constexpr uintptr_t ADDR_AUTOMOBILE_DELETE = 0x551150;
constexpr uintptr_t ADDR_MUSIC_INITIALISE = 0x57CF70;

// CPool layout
constexpr size_t OFF_POOL_OBJECTS = 0x00;
constexpr size_t OFF_POOL_BYTEMAP = 0x04;
constexpr size_t OFF_POOL_SIZE = 0x08;
constexpr size_t VEHICLE_POOL_STRIDE = 0x5A8;
constexpr size_t PED_POOL_STRIDE = 0x5F0;
constexpr size_t STREAMING_INFO_STRIDE = 20;

// CCamera / HUD
constexpr size_t OFF_CAM_WIDESCREEN = 0x70;
constexpr uint8_t REPLAY_MODE_PLAYBACK = 1;
constexpr uint8_t WBSTATE_PLAYING = 0;
constexpr float STOP_SIGN_RADIUS = 3.0f;
constexpr int16_t FONT_STANDARD = 0; // font2 (subtitles & HUD prompts)
constexpr int16_t FONT_PAGER = 1;    // pager LCD (dark glyphs)
constexpr int16_t FONT_HEADING = 2;  // font1 (Pricedown headings)

// CEntity / CPhysical / CColModel
constexpr size_t OFF_ENTITY_VTABLE = 0x00;
constexpr size_t OFF_ENTITY_POS = 0x34;
constexpr size_t OFF_ENTITY_RW_OBJECT = 0x4C;
constexpr size_t OFF_ENTITY_FLAGS_50 = 0x50;
constexpr size_t OFF_ENTITY_FLAGS_51 = 0x51;
constexpr size_t OFF_ENTITY_MODEL_INDEX = 0x5C;
constexpr size_t OFF_PHYSICAL_MOVE_SPEED = 0x78;
constexpr size_t OFF_PHYSICAL_TURN_SPEED = 0x84;
constexpr size_t OFF_COLMODEL_NUM_LINES = 0x32;
constexpr size_t COLPOINT_SIZE = 40;

constexpr uint8_t ENTITY_TYPE_MASK = 0x07;
constexpr uint8_t ENTITY_TYPE_VEHICLE = 2;
constexpr uint8_t STATUS_PHYSICS = 3;
constexpr uint8_t STATUS_WRECKED = 5;

// CVehicle / CAutopilot offsets
constexpr size_t OFF_VEH_AREA_CODE = 0x124;                   // uint8_t
constexpr size_t OFF_VEH_AUTOPILOT_CURR_NODE = 0x12C;         // int32_t
constexpr size_t OFF_VEH_AUTOPILOT_NEXT_NODE = 0x130;         // int32_t
constexpr size_t OFF_VEH_AUTOPILOT_PREV_DIR = 0x140;          // int32_t
constexpr size_t OFF_VEH_AUTOPILOT_NEXT_DIR = 0x144;          // int32_t
constexpr size_t OFF_VEH_AUTOPILOT_START_TIME = 0x14C;        // uint32_t
constexpr size_t OFF_VEH_AUTOPILOT_CURR_LANE = 0x155;         // int8_t
constexpr size_t OFF_VEH_AUTOPILOT_NEXT_LANE = 0x156;         // int8_t
constexpr size_t OFF_VEH_AUTOPILOT_DRIVING_STYLE = 0x159;     // uint8_t
constexpr size_t OFF_VEH_AUTOPILOT_CAR_MISSION = 0x15A;       // uint8_t
constexpr size_t OFF_VEH_AUTOPILOT_TEMP_ACTION = 0x15B;       // uint8_t
constexpr size_t OFF_VEH_AUTOPILOT_TIME_TEMP_ACTION = 0x15C;  // uint32_t
constexpr size_t OFF_VEH_AUTOPILOT_MAX_TRAFFIC_SPEED = 0x160; // float
constexpr size_t OFF_VEH_AUTOPILOT_CRUISE_SPEED = 0x164;      // uint8_t
constexpr size_t OFF_VEH_AUTOPILOT_NUM_NODES = 0x194;         // int16_t
constexpr size_t OFF_VEH_DRIVER = 0x1A4;                      // CPed*
constexpr size_t OFF_VEH_PASSENGERS = 0x1A8;                  // CPed*[8]
constexpr size_t OFF_VEH_MAX_PASSENGERS = 0x1CC;              // uint8_t
constexpr size_t OFF_VEH_STEER_ANGLE = 0x1E8;                 // float
constexpr size_t OFF_VEH_GAS_PEDAL = 0x1EC;                   // float
constexpr size_t OFF_VEH_BRAKE_PEDAL = 0x1F0;                 // float
constexpr size_t OFF_VEH_CREATED_BY = 0x1F4;                  // uint8_t
constexpr size_t OFF_VEH_FLAGS_1F5 = 0x1F5;                   // uint8_t
constexpr size_t OFF_VEH_FLAGS_1F6 = 0x1F6;                   // uint8_t
constexpr size_t OFF_VEH_FLAGS_1F7 = 0x1F7;                   // uint8_t
constexpr size_t OFF_VEH_HEALTH = 0x200;                      // float
constexpr size_t OFF_VEH_RADIO_STATION = 0x229;               // uint8_t
constexpr size_t OFF_VEH_CLASS = 0x284;                       // uint32_t

constexpr uint8_t RADIO_OFF = 11;

constexpr uint8_t RANDOM_VEHICLE = 1;
constexpr uint8_t MISSION_VEHICLE = 2;
constexpr uint32_t VEHICLE_AUTOMOBILE = 0;

constexpr uint8_t DRIVINGSTYLE_STOP_FOR_CARS = 0;
constexpr uint8_t DRIVINGSTYLE_SLOW_DOWN_FOR_CARS = 1;
constexpr uint8_t DRIVINGSTYLE_AVOID_CARS = 2;
constexpr uint8_t CAR_MISSION_CRUISE = 1;
constexpr uint8_t CAR_MISSION_GOTOCOORDS = 8;
constexpr uint8_t CAR_MISSION_GOTOCOORDS_ACCURATE = 9;
constexpr uint8_t TEMPACT_NONE = 0;
constexpr uint8_t TEMPACT_WAIT = 1;
constexpr uint8_t TEMPACT_REVERSE = 2;

// CPed offsets
constexpr size_t OFF_PED_AREA_CODE = 0x124;    // uint8_t
constexpr size_t OFF_PED_FLAGS_156 = 0x156;    // uint8_t
constexpr size_t OFF_PED_FLAGS_157 = 0x157;    // uint8_t
constexpr size_t OFF_PED_FLAGS_15A = 0x15A;    // uint8_t
constexpr size_t OFF_PED_CREATED_BY = 0x160;   // uint8_t
constexpr size_t OFF_PED_OBJECTIVE = 0x164;    // uint32_t
constexpr size_t OFF_PED_VEHICLE_ANIM = 0x1D8; // CAnimBlendAssociation*
constexpr size_t OFF_PED_STATE = 0x224;        // uint32_t
constexpr size_t OFF_PED_HEALTH = 0x2C0;       // float
constexpr size_t OFF_PED_VEHICLE = 0x310;      // CVehicle*
constexpr size_t OFF_PED_IN_VEHICLE = 0x314;   // uint8_t
constexpr size_t OFF_PED_WANTED = 0x53C;       // CWanted*
constexpr size_t OFF_WANTED_LEVEL = 0x18;      // int32_t

constexpr uint8_t RANDOM_CHAR = 1;
constexpr uint8_t MISSION_CHAR = 2;
constexpr uint32_t PEDTYPE_CIVMALE = 4;
constexpr uint32_t PEDSTATE_DRIVING = 44;
constexpr uint32_t PEDSTATE_DIE = 48;
constexpr uint32_t PEDSTATE_DEAD = 49;
constexpr uint32_t OBJECTIVE_LEAVE_CAR = 13;
constexpr uint32_t OBJECTIVE_ENTER_CAR_AS_PASSENGER = 14;
constexpr uint32_t OBJECTIVE_ENTER_CAR_AS_DRIVER = 15;
constexpr uint32_t ANIM_STD_CAR_SIT = 0x6F;
constexpr uint32_t ANIM_STD_CAR_SIT_LO = 0x70;

// Model IDs (data/default.ide)
constexpr int32_t MI_MALE01 = 7;
constexpr int32_t MI_BUS = 121;
constexpr int32_t MI_COACH = 127;

// Global slots
constexpr size_t SLOT_ON_MISSION = 5002;

struct Vec3 {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};

struct CRGBA {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
};

struct RouteWaypoint {
  bool isStop;
  int stopNumber; // 1..8 when isStop, 0 otherwise
  const wchar_t *name;
  Vec3 signPos;  // bus sign / sidewalk reference
  Vec3 roadPos;  // verified CPathFind curb target
  Vec3 spawnPos; // upstream spawn coordinate (~70-100m behind stop)
  float
      headingDeg; // stop heading degrees (0=+Y N, 90=-X W, 180=-Y S, 270=+X E)
  float spawnHeadingDeg; // spawnPos heading degrees along the upstream road
};

// 8-stop counter-clockwise Portland Bus Loop (right-side passenger door faces
// curb at every stop)
constexpr RouteWaypoint kPortlandLoop[] = {
    // WP 0 -> Stop 1/8: Harwood Fire Station / Head Radio (Southbound along
    // west curb)
    {true,
     1,
     L"Harwood - Fire Station",
     {1038.25f, -118.00f, 5.20f},
     {1046.50f, -118.00f, 3.80f},
     {1050.80f, -58.00f, 4.00f},
     180.0f,
     180.0f},
    // WP 1 -> Guide: North Chinatown Ave (straight down Hepburn / Chinatown
    // Ave)
    {false,
     0,
     L"",
     {1050.80f, -320.00f, 13.80f},
     {1050.80f, -320.00f, 13.80f},
     {1050.80f, -260.00f, 13.80f},
     180.0f,
     180.0f},
    // WP 2 -> Guide: Chinatown Corner (turn West onto Red Light / Chinatown
    // Blvd)
    {false,
     0,
     L"",
     {1050.80f, -496.80f, 13.80f},
     {1050.80f, -496.80f, 13.80f},
     {1050.80f, -430.00f, 13.80f},
     180.0f,
     180.0f},
    // WP 3 -> Stop 2/8: Chinatown - Subway Border (Westbound along north curb;
    // spawns upstream on N-S ave heading South toward Chinatown corner)
    {true,
     2,
     L"Chinatown - Subway Border",
     {1024.46f, -484.75f, 15.20f},
     {1024.46f, -492.50f, 13.80f},
     {1050.80f, -415.00f, 13.80f},
     90.0f,
     180.0f},
    // WP 4 -> Stop 3/8: Red Light District - Subway (Westbound along north
    // curb)
    {true,
     3,
     L"Red Light - Subway Station",
     {946.00f, -481.25f, 15.20f},
     {946.00f, -490.50f, 13.80f},
     {1015.00f, -493.80f, 13.80f},
     90.0f,
     90.0f},
    // WP 5 -> Guide: West Blvd (Red Light -> Callahan Point)
    {false,
     0,
     L"",
     {855.80f, -698.80f, 13.80f},
     {855.80f, -698.80f, 13.80f},
     {855.80f, -630.00f, 13.80f},
     180.0f,
     180.0f},
    // WP 6 -> Guide: Callahan NW Curve
    {false,
     0,
     L"",
     {815.80f, -896.30f, 9.90f},
     {815.80f, -896.30f, 9.90f},
     {815.80f, -830.00f, 13.50f},
     180.0f,
     180.0f},
    // WP 7 -> Stop 4/8: Callahan Point - Greasy Joe's Diner (under bridge)
    {true,
     4,
     L"Callahan Pt - Diner",
     {881.80f, -1011.60f, 5.38f},
     {881.80f, -1021.00f, 3.80f},
     {820.00f, -955.00f, 5.80f},
     225.0f,
     225.0f},
    // WP 8 -> Stop 5/8: Atlantic Quays - Callahan Bridge Side (Eastbound along
    // south curb)
    {true,
     5,
     L"Atlantic Quays",
     {1106.50f, -1107.25f, 12.09f},
     {1106.50f, -1101.00f, 10.70f},
     {1025.00f, -1099.50f, 10.70f},
     270.0f,
     270.0f},
    // WP 9 -> Stop 6/8: Trenton - Portland Bus Depot (NE along depot curb)
    {true,
     6,
     L"Trenton - Bus Depot",
     {1274.50f, -954.00f, 15.20f},
     {1269.00f, -949.00f, 13.80f},
     {1205.00f, -995.00f, 13.80f},
     315.0f,
     315.0f},
    // WP 10 -> Guide: SE Industrial Turn (Depot -> Hospital)
    {false,
     0,
     L"",
     {1233.10f, -765.90f, 13.80f},
     {1233.10f, -765.90f, 13.80f},
     {1270.00f, -830.00f, 13.80f},
     45.0f,
     45.0f},
    // WP 11 -> Stop 7/8: Portland View - Hospital & Police (Northbound along
    // east curb)
    {true,
     7,
     L"Portland View - Hospital",
     {1133.25f, -606.00f, 15.20f},
     {1127.00f, -606.00f, 13.80f},
     {1125.80f, -690.00f, 13.80f},
     0.0f,
     0.0f},
    // WP 12 -> Guide: St. Marks Blvd North (Hospital -> Showroom)
    {false,
     0,
     L"",
     {1125.80f, -348.80f, 18.80f},
     {1125.80f, -348.80f, 18.80f},
     {1125.80f, -420.00f, 14.00f},
     0.0f,
     0.0f},
    // WP 13 -> Stop 8/8: Harwood - Car Showroom & Junkyard (Northbound along
    // east curb)
    {true,
     8,
     L"Harwood - Showroom",
     {1202.75f, -68.00f, 11.12f},
     {1197.00f, -68.00f, 8.80f},
     {1195.80f, -155.00f, 13.00f},
     0.0f,
     0.0f},
    // WP 14 -> Guide: North Harwood Curve (Showroom -> Fire Station)
    {false,
     0,
     L"",
     {1125.80f, -48.80f, 6.30f},
     {1125.80f, -48.80f, 6.30f},
     {1185.00f, -48.80f, 8.80f},
     90.0f,
     90.0f},
};
constexpr size_t kLoopCount = sizeof(kPortlandLoop) / sizeof(kPortlandLoop[0]);

// Function typedefs
using MusicBoolFn = bool(__thiscall *)(void *self);
using ScriptsProcessFn = void(__cdecl *)();
using HudDrawFn = void(__cdecl *)();
using ProcessColModelsFn = int(__cdecl *)(const void *matA, const void *colA,
                                          const void *matB, const void *colB,
                                          void *aSphereColPoints,
                                          void *aLineColPoints,
                                          float *aLineMaxTouchDist);
using FindPlayerPedFn = void *(__cdecl *)();
using FindPlayerVehicleFn = void *(__cdecl *)();
using StreamingReqModelFn = void(__cdecl *)(int32_t model, int32_t flags);
using StreamingLoadAllFn = void(__cdecl *)(bool priority);
using StreamingLoadSceneFn = void(__cdecl *)(const Vec3 *pos);
using StreamingSetDelFn = void(__cdecl *)(int32_t model);
using TimerVoidFn = void(__cdecl *)();
using WorldAddFn = void(__cdecl *)(void *entity);
using FindGroundZFn = float(__cdecl *)(float x, float y);
using ZonesGetLevelFn = int(__cdecl *)(const Vec3 *pos);
using ClearSpaceFn = void(__cdecl *)(const Vec3 *pos, void *entity);
using CameraFadeFn = void(__thiscall *)(void *camera, float durationSec,
                                        int16_t direction);
using JoinGotoCoorsFn = bool(__cdecl *)(void *veh, float x, float y, float z,
                                        bool arg);
using AutomobileNewFn = void *(__cdecl *)(uint32_t size);
using AutomobileDelFn = void(__cdecl *)(void *veh);
using AutomobileCtorFn = void *(__thiscall *)(void *self, int32_t modelIndex,
                                              uint8_t createdBy);
using CivPedNewFn = void *(__cdecl *)(uint32_t size);
using CivPedCtorFn = void *(__thiscall *)(void *self, uint32_t pedType,
                                          int32_t modelIndex);
using AnimBlendFn = void *(__cdecl *)(void *clump, uint32_t assocGroup,
                                      uint32_t animId, float delta);
using EntityRegRefFn = void(__thiscall *)(void *entity, void **ref);
using GetDistToBaseFn = float(__thiscall *)(void *entity);
using SetHeadingFn = void(__thiscall *)(void *entity, float rad);
using ThisVoidFn = void(__thiscall *)(void *self);
using PedWarpIntoCarFn = void(__thiscall *)(void *ped, void *veh);
using PedSetObjectiveEntFn = void(__thiscall *)(void *ped, uint32_t obj,
                                                void *ent);
using EntityTeleportFn = void(__thiscall *)(void *entity, Vec3 pos);

using FontVoidFn = void(__cdecl *)();
using FontFloatFn = void(__cdecl *)(float v);
using FontScaleFn = void(__cdecl *)(float w, float h);
using FontColorFn = void(__cdecl *)(const CRGBA *color);
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

enum class TransitState {
  Idle = 0,
  ApproachingStop,
  DwellingAtStop,
  SkipFadeOut,
};

// Config & state
std::string g_logPath;
std::string g_iniPath;
std::vector<HookEntry> g_hooks;
bool g_deferredInstalled = false;

bool g_enabled = true;
int32_t g_fare = 5;
uint32_t g_dwellMs = 7000;
uint8_t g_cruiseSpeed = 15;
bool g_useCoachAlt = false;
bool g_allowAmbient = true;
int g_boardKey = 'G';
int g_skipKey = 'N';

MusicBoolFn g_origInitialise = nullptr;
ScriptsProcessFn g_origScriptsProcess = nullptr;
HudDrawFn g_origHudDraw = nullptr;
ProcessColModelsFn g_origProcessColModels = nullptr;

// Active bus state
void *g_bus = nullptr;
void *g_driver = nullptr;
bool g_spawnedByUs = false;
TransitState g_state = TransitState::Idle;
size_t g_wpIdx = 0;
uint32_t g_stateStartMs = 0;
uint32_t g_dwellStartMs = 0;
uint32_t g_lastProgressMs = 0;
Vec3 g_lastProgressPos{};
uint32_t g_reverseUntilMs = 0;
uint32_t g_stallStep = 0;
uint32_t g_lastDiagLogMs = 0;
uint32_t g_boardAttemptStartMs = 0;
uint32_t g_lastSpawnMs = 0;
uint32_t g_dispatchIndex = 0;
bool g_stopRequested = false;
bool g_wasPrevBoardDown = false;
bool g_wasPrevSkipDown = false;

// Transient HUD notice (e.g. "Need $5 for Bus Fare")
wchar_t g_noticeText[96] = {};
uint32_t g_noticeUntilMs = 0;

// Cached HUD strings computed in game loop, rendered in Hook_HudDraw
bool g_showHudPrompt = false;
wchar_t g_hudLine1[128] = {};
wchar_t g_hudLine2[128] = {};
CRGBA g_hudColor1{147, 196, 211, 255};
CRGBA g_hudColor2{235, 235, 235, 255};

// ============================================================================
// Utilities & Detour Installer
// ============================================================================
template <typename T> inline T &FieldAt(void *base, size_t off) {
  return *reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(base) + off);
}

template <typename T> inline const T &FieldAt(const void *base, size_t off) {
  return *reinterpret_cast<const T *>(reinterpret_cast<uintptr_t>(base) + off);
}

inline uint32_t TimeMs() {
  return *reinterpret_cast<const uint32_t *>(ADDR_TIME_MS);
}

inline int32_t *ScriptGlobalPtr(size_t slot) {
  return reinterpret_cast<int32_t *>(ADDR_SCRIPT_SPACE + slot * 4);
}

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
  char exeBuf[MAX_PATH] = {};
  if (GetModuleFileNameA(nullptr, exeBuf, MAX_PATH) > 0) {
    std::string full(exeBuf);
    const size_t pos = full.find_last_of("\\/");
    g_logPath = (pos != std::string::npos)
                    ? full.substr(0, pos) + "\\BusTransit.log"
                    : "BusTransit.log";
  } else {
    g_logPath = "BusTransit.log";
  }
  HANDLE f = CreateFileA(g_logPath.c_str(), GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                         CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (f != INVALID_HANDLE_VALUE) {
    CloseHandle(f);
  }

  char modBuf[MAX_PATH] = {};
  if (GetModuleFileNameA(hModule, modBuf, MAX_PATH) > 0) {
    std::string full(modBuf);
    const size_t pos = full.find_last_of("\\/");
    if (pos != std::string::npos) {
      g_iniPath = full.substr(0, pos) + "\\BusTransit.ini";
    }
  }
}

void LoadConfig() {
  if (g_iniPath.empty() ||
      GetFileAttributesA(g_iniPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
    return;
  }
  g_enabled =
      GetPrivateProfileIntA("General", "Enabled", 1, g_iniPath.c_str()) != 0;
  g_fare = std::max(0, static_cast<int>(GetPrivateProfileIntA(
                           "General", "Fare", 5, g_iniPath.c_str())));
  const int dwell =
      GetPrivateProfileIntA("General", "DwellMs", 7000, g_iniPath.c_str());
  g_dwellMs = static_cast<uint32_t>(std::max(2000, std::min(dwell, 20000)));
  const int speed =
      GetPrivateProfileIntA("General", "CruiseSpeed", 15, g_iniPath.c_str());
  g_cruiseSpeed = static_cast<uint8_t>(std::max(8, std::min(speed, 25)));
  g_useCoachAlt = GetPrivateProfileIntA("General", "UseCoachAlt", 0,
                                        g_iniPath.c_str()) != 0;
  g_allowAmbient = GetPrivateProfileIntA("General", "AllowAmbientBuses", 1,
                                         g_iniPath.c_str()) != 0;

  auto readVk = [](const char *key, int fallback) -> int {
    char buf[32] = {};
    GetPrivateProfileStringA("Controls", key, "", buf, sizeof(buf),
                             g_iniPath.c_str());
    if (buf[0] == '\0') {
      return fallback;
    }
    char *end = nullptr;
    const long v = std::strtol(buf, &end, 0);
    if (end != buf && v > 0 && v < 255) {
      return static_cast<int>(v);
    }
    return fallback;
  };
  g_boardKey = readVk("BoardKey", 'G');
  g_skipKey = readVk("SkipKey", 'N');
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
      if (mod == 0 && (sib & 7) == 5)
        len += 4;
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

  if (op == 0x0F) {
    const uint8_t op2 = p[i++];
    if (op2 >= 0x80 && op2 <= 0x8F)
      return i + 4;
    if (op2 == 0xB6 || op2 == 0xB7 || op2 == 0xBE || op2 == 0xBF ||
        op2 == 0xAF || op2 == 0x40 || (op2 >= 0x41 && op2 <= 0x4F)) {
      return i + modrmLen(&p[i]);
    }
    return 0;
  }

  if ((op >= 0x50 && op <= 0x5F) || op == 0x90 || op == 0xC3)
    return i;
  if (op == 0x6A || op == 0xEB || (op >= 0x70 && op <= 0x7F))
    return i + 1;
  if (op == 0x68 || op == 0xE8 || op == 0xE9)
    return i + imm32Size;
  if (op >= 0xB8 && op <= 0xBF)
    return i + imm32Size;
  if (op >= 0xB0 && op <= 0xB7)
    return i + 1;
  if (op >= 0xA0 && op <= 0xA3)
    return i + (addrSize16 ? 2 : 4);
  if (op == 0x04 || op == 0x0C || op == 0x14 || op == 0x1C || op == 0x24 ||
      op == 0x2C || op == 0x34 || op == 0x3C || op == 0xA8)
    return i + 1;
  if (op == 0x05 || op == 0x0D || op == 0x15 || op == 0x1D || op == 0x25 ||
      op == 0x2D || op == 0x35 || op == 0x3D || op == 0xA9)
    return i + imm32Size;
  if ((op <= 0x3B && (op & 7) <= 3) || op == 0x63 || op == 0x84 || op == 0x85 ||
      (op >= 0x88 && op <= 0x8D) || op == 0x8F || (op >= 0xD8 && op <= 0xDF)) {
    return i + modrmLen(&p[i]);
  }
  if (op == 0x80 || op == 0x82 || op == 0x83 || op == 0xC0 || op == 0xC1 ||
      op == 0xC6) {
    return i + modrmLen(&p[i]) + 1;
  }
  if (op == 0x81 || op == 0xC7)
    return i + modrmLen(&p[i]) + imm32Size;
  if (op == 0xD0 || op == 0xD1 || op == 0xD2 || op == 0xD3)
    return i + modrmLen(&p[i]);
  if (op == 0xF6) {
    const uint8_t reg = (p[i] >> 3) & 7;
    return i + modrmLen(&p[i]) + (reg <= 1 ? 1 : 0);
  }
  if (op == 0xF7) {
    const uint8_t reg = (p[i] >> 3) & 7;
    return i + modrmLen(&p[i]) + (reg <= 1 ? imm32Size : 0);
  }
  if (op == 0xFE || op == 0xFF)
    return i + modrmLen(&p[i]);
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
    if (insnLen == 0 || copyLen + insnLen > 16)
      return false;
    const uint8_t first = src[copyLen];
    if (first == 0xEB || (first >= 0x70 && first <= 0x7F))
      return false;
    copyLen += insnLen;
  }

  uint8_t *tramp = static_cast<uint8_t *>(VirtualAlloc(
      nullptr, 32, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
  if (!tramp)
    return false;

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
  for (size_t k = 5; k < copyLen; ++k)
    dst[k] = 0x90;
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

// ============================================================================
// Entity & Math Helpers
// ============================================================================
inline Vec3 GetEntityPos(const void *entity) {
  const float *p = &FieldAt<float>(entity, OFF_ENTITY_POS);
  return {p[0], p[1], p[2]};
}

inline float Dist2D(const Vec3 &a, const Vec3 &b) {
  const float dx = a.x - b.x;
  const float dy = a.y - b.y;
  return std::sqrt(dx * dx + dy * dy);
}

inline bool IsEntityType(const void *entity, uint8_t type) {
  return (FieldAt<uint8_t>(entity, OFF_ENTITY_FLAGS_50) & ENTITY_TYPE_MASK) ==
         type;
}

inline uint8_t GetEntityStatus(const void *entity) {
  return (FieldAt<uint8_t>(entity, OFF_ENTITY_FLAGS_50) >> 3) & 0x1F;
}

inline bool IsPedAlive(const void *ped) {
  if (!ped)
    return false;
  if (FieldAt<float>(ped, OFF_PED_HEALTH) <= 0.0f)
    return false;
  const uint32_t state = FieldAt<uint32_t>(ped, OFF_PED_STATE);
  return state != PEDSTATE_DIE && state != PEDSTATE_DEAD;
}

inline bool IsVehicleInPool(const void *veh) {
  if (!veh)
    return false;
  const uintptr_t pool =
      *reinterpret_cast<const uintptr_t *>(ADDR_VEH_POOL_PTR);
  if (!pool)
    return false;
  const uintptr_t objectsBase = FieldAt<uintptr_t>(
      reinterpret_cast<const void *>(pool), OFF_POOL_OBJECTS);
  const uint8_t *byteMap = FieldAt<const uint8_t *>(
      reinterpret_cast<const void *>(pool), OFF_POOL_BYTEMAP);
  const int32_t poolSize =
      FieldAt<int32_t>(reinterpret_cast<const void *>(pool), OFF_POOL_SIZE);
  if (!objectsBase || !byteMap || poolSize <= 0)
    return false;

  const uintptr_t ptr = reinterpret_cast<uintptr_t>(veh);
  const uintptr_t end =
      objectsBase + static_cast<uintptr_t>(poolSize) * VEHICLE_POOL_STRIDE;
  if (ptr < objectsBase || ptr >= end)
    return false;
  const uintptr_t diff = ptr - objectsBase;
  if (diff % VEHICLE_POOL_STRIDE != 0)
    return false;
  const int32_t idx = static_cast<int32_t>(diff / VEHICLE_POOL_STRIDE);
  if (idx < 0 || idx >= poolSize || (byteMap[idx] & 0x80) != 0)
    return false;
  return IsEntityType(veh, ENTITY_TYPE_VEHICLE);
}

inline bool IsPedInPool(const void *ped) {
  if (!ped)
    return false;
  const uintptr_t pool =
      *reinterpret_cast<const uintptr_t *>(ADDR_PED_POOL_PTR);
  if (!pool)
    return false;
  const uintptr_t objectsBase = FieldAt<uintptr_t>(
      reinterpret_cast<const void *>(pool), OFF_POOL_OBJECTS);
  const uint8_t *byteMap = FieldAt<const uint8_t *>(
      reinterpret_cast<const void *>(pool), OFF_POOL_BYTEMAP);
  const int32_t poolSize =
      FieldAt<int32_t>(reinterpret_cast<const void *>(pool), OFF_POOL_SIZE);
  if (!objectsBase || !byteMap || poolSize <= 0)
    return false;

  const uintptr_t ptr = reinterpret_cast<uintptr_t>(ped);
  const uintptr_t end =
      objectsBase + static_cast<uintptr_t>(poolSize) * PED_POOL_STRIDE;
  if (ptr < objectsBase || ptr >= end)
    return false;
  const uintptr_t diff = ptr - objectsBase;
  if (diff % PED_POOL_STRIDE != 0)
    return false;
  const int32_t idx = static_cast<int32_t>(diff / PED_POOL_STRIDE);
  return idx >= 0 && idx < poolSize && (byteMap[idx] & 0x80) == 0;
}

inline bool IsTransitVehicleModel(int16_t mi) {
  return mi == MI_BUS || mi == MI_COACH;
}

inline bool IsPortlandArea(const Vec3 &pos) {
  return pos.x >= 720.0f && pos.x <= 1550.0f && pos.y >= -1300.0f &&
         pos.y <= 50.0f;
}

inline int32_t GetPlayerWantedLevel(const void *playerPed) {
  if (!playerPed)
    return 0;
  const void *wanted = FieldAt<void *>(playerPed, OFF_PED_WANTED);
  return wanted ? FieldAt<int32_t>(wanted, OFF_WANTED_LEVEL) : 0;
}

inline bool HasFreePassengerSeat(const void *veh) {
  const uint8_t maxPass = FieldAt<uint8_t>(veh, OFF_VEH_MAX_PASSENGERS);
  const uint8_t slots = std::min<uint8_t>(maxPass, 8);
  for (uint8_t i = 0; i < slots; ++i) {
    void *const *passengers = &FieldAt<void *>(veh, OFF_VEH_PASSENGERS);
    if (passengers[i] == nullptr) {
      return true;
    }
  }
  return false;
}

inline float GetVehicleSpeed2D(const void *veh) {
  const float *v = &FieldAt<float>(veh, OFF_PHYSICAL_MOVE_SPEED);
  return std::sqrt(v[0] * v[0] + v[1] * v[1]);
}

void LogBusDiag(const char *tag, const void *veh, size_t wpIdx) {
  if (!veh)
    return;
  const Vec3 p = GetEntityPos(veh);
  const float spd = GetVehicleSpeed2D(veh);
  const float distT =
      (wpIdx < kLoopCount) ? Dist2D(p, kPortlandLoop[wpIdx].roadPos) : -1.0f;
  char msg[256] = {};
  std::snprintf(
      msg, sizeof(msg),
      "[%s] wp=%u pos=(%.1f,%.1f,%.1f) dT=%.1f spd=%.3f hp=%.0f "
      "mis=%u sty=%u tmp=%u gas=%.2f brk=%.2f str=%.2f st=%u f1F5=0x%02X "
      "nd=(%d,%d n=%d) dir=(%d,%d) drv=%p",
      tag, static_cast<unsigned>(wpIdx), p.x, p.y, p.z, distT, spd,
      FieldAt<float>(veh, OFF_VEH_HEALTH),
      FieldAt<uint8_t>(veh, OFF_VEH_AUTOPILOT_CAR_MISSION),
      FieldAt<uint8_t>(veh, OFF_VEH_AUTOPILOT_DRIVING_STYLE),
      FieldAt<uint8_t>(veh, OFF_VEH_AUTOPILOT_TEMP_ACTION),
      FieldAt<float>(veh, OFF_VEH_GAS_PEDAL),
      FieldAt<float>(veh, OFF_VEH_BRAKE_PEDAL),
      FieldAt<float>(veh, OFF_VEH_STEER_ANGLE), GetEntityStatus(veh),
      FieldAt<uint8_t>(veh, OFF_VEH_FLAGS_1F5),
      FieldAt<int32_t>(veh, OFF_VEH_AUTOPILOT_CURR_NODE),
      FieldAt<int32_t>(veh, OFF_VEH_AUTOPILOT_NEXT_NODE),
      static_cast<int>(FieldAt<int16_t>(veh, OFF_VEH_AUTOPILOT_NUM_NODES)),
      FieldAt<int32_t>(veh, OFF_VEH_AUTOPILOT_PREV_DIR),
      FieldAt<int32_t>(veh, OFF_VEH_AUTOPILOT_NEXT_DIR),
      FieldAt<void *>(veh, OFF_VEH_DRIVER));
  WriteLog(msg);
}

bool IsModelLoaded(int32_t model) {
  const uint8_t *info = reinterpret_cast<const uint8_t *>(
      ADDR_STREAMING_INFO +
      static_cast<uintptr_t>(model) * STREAMING_INFO_STRIDE);
  return info[0] == 1;
}

bool EnsureModelLoaded(int32_t model) {
  if (IsModelLoaded(model))
    return true;
  auto reqModel =
      reinterpret_cast<StreamingReqModelFn>(ADDR_STREAMING_REQUEST_MODEL);
  auto timerStop = reinterpret_cast<TimerVoidFn>(ADDR_TIMER_STOP);
  auto loadAll = reinterpret_cast<StreamingLoadAllFn>(ADDR_STREAMING_LOAD_ALL);
  auto timerUpd = reinterpret_cast<TimerVoidFn>(ADDR_TIMER_UPDATE);

  reqModel(model, 0x16);
  if (!IsModelLoaded(model)) {
    timerStop();
    loadAll(false);
    timerUpd();
  }
  return IsModelLoaded(model);
}

// ============================================================================
// Route Helpers
// ============================================================================
size_t NextWpIndex(size_t idx) { return (idx + 1) % kLoopCount; }

size_t NextStopWpIndex(size_t fromIdx) {
  for (size_t step = 1; step <= kLoopCount; ++step) {
    const size_t candidate = (fromIdx + step) % kLoopCount;
    if (kPortlandLoop[candidate].isStop) {
      return candidate;
    }
  }
  return 0;
}

int FindNearestStopWp(const Vec3 &pos, float maxRadius,
                      float *outDist = nullptr) {
  int best = -1;
  float bestD = maxRadius;
  for (size_t i = 0; i < kLoopCount; ++i) {
    if (!kPortlandLoop[i].isStop)
      continue;
    const float d = Dist2D(pos, kPortlandLoop[i].signPos);
    if (d <= bestD && std::fabs(pos.z - kPortlandLoop[i].signPos.z) < 4.5f) {
      bestD = d;
      best = static_cast<int>(i);
    }
  }
  if (outDist)
    *outDist = bestD;
  return best;
}

int FindNearestLoopWp(const Vec3 &pos) {
  int best = 0;
  float bestD = 1e9f;
  for (size_t i = 0; i < kLoopCount; ++i) {
    const float d = Dist2D(pos, kPortlandLoop[i].roadPos);
    if (d < bestD) {
      bestD = d;
      best = static_cast<int>(i);
    }
  }
  return best;
}

// ============================================================================
// Vehicle Control & Spawning
// ============================================================================
void DriveToWaypoint(void *veh, size_t wpIdx,
                     uint8_t drivingStyle = DRIVINGSTYLE_SLOW_DOWN_FOR_CARS) {
  if (!veh || wpIdx >= kLoopCount)
    return;
  const Vec3 &dst = kPortlandLoop[wpIdx].roadPos;
  auto joinGoto = reinterpret_cast<JoinGotoCoorsFn>(ADDR_JOIN_CAR_GOTO_COORS);
  const bool accurate = joinGoto(veh, dst.x, dst.y, dst.z, false);

  FieldAt<uint8_t>(veh, OFF_VEH_AUTOPILOT_DRIVING_STYLE) = drivingStyle;
  FieldAt<uint8_t>(veh, OFF_VEH_AUTOPILOT_CRUISE_SPEED) = g_cruiseSpeed;
  FieldAt<float>(veh, OFF_VEH_AUTOPILOT_MAX_TRAFFIC_SPEED) =
      static_cast<float>(g_cruiseSpeed);
  FieldAt<uint8_t>(veh, OFF_VEH_AUTOPILOT_CAR_MISSION) =
      accurate ? CAR_MISSION_GOTOCOORDS_ACCURATE : CAR_MISSION_GOTOCOORDS;
  FieldAt<uint8_t>(veh, OFF_VEH_AUTOPILOT_TEMP_ACTION) = TEMPACT_NONE;
  FieldAt<uint32_t>(veh, OFF_VEH_AUTOPILOT_TIME_TEMP_ACTION) = 0;
  FieldAt<uint32_t>(veh, OFF_VEH_AUTOPILOT_START_TIME) = TimeMs();

  uint8_t &flags50 = FieldAt<uint8_t>(veh, OFF_ENTITY_FLAGS_50);
  flags50 = (flags50 & 0x07) | (STATUS_PHYSICS << 3);
  uint8_t &flags1F5 = FieldAt<uint8_t>(veh, OFF_VEH_FLAGS_1F5);
  flags1F5 = (flags1F5 & 0xCF) | 0x10; // clear handbrake (0x20), set 0x10
}

void HoldBusStopped(void *veh, uint32_t nowMs, uint32_t holdDurationMs) {
  if (!veh)
    return;
  FieldAt<uint8_t>(veh, OFF_VEH_AUTOPILOT_TEMP_ACTION) = TEMPACT_WAIT;
  FieldAt<uint32_t>(veh, OFF_VEH_AUTOPILOT_TIME_TEMP_ACTION) =
      nowMs + holdDurationMs;
  FieldAt<float>(veh, OFF_VEH_GAS_PEDAL) = 0.0f;
  FieldAt<float>(veh, OFF_VEH_BRAKE_PEDAL) = 1.0f;
}

void ReleaseManagedBus() {
  if (g_bus && IsVehicleInPool(g_bus)) {
    const int16_t mi = FieldAt<int16_t>(g_bus, OFF_ENTITY_MODEL_INDEX);
    if (IsTransitVehicleModel(mi)) {
      if (g_spawnedByUs) {
        FieldAt<uint8_t>(g_bus, OFF_VEH_CREATED_BY) = RANDOM_VEHICLE;
      }
      void *drv = FieldAt<void *>(g_bus, OFF_VEH_DRIVER);
      if (drv && IsPedInPool(drv) && IsPedAlive(drv)) {
        if (g_spawnedByUs) {
          FieldAt<uint8_t>(drv, OFF_PED_CREATED_BY) = RANDOM_CHAR;
        }
        FieldAt<uint8_t>(g_bus, OFF_VEH_AUTOPILOT_CAR_MISSION) =
            CAR_MISSION_CRUISE;
        FieldAt<uint8_t>(g_bus, OFF_VEH_AUTOPILOT_TEMP_ACTION) = TEMPACT_NONE;
        FieldAt<uint8_t>(g_bus, OFF_VEH_AUTOPILOT_CRUISE_SPEED) = 12;
      }
    }
  }
  if (g_spawnedByUs) {
    auto setDeletable =
        reinterpret_cast<StreamingSetDelFn>(ADDR_STREAMING_SET_DELETABLE);
    setDeletable(MI_BUS);
    setDeletable(MI_COACH);
    setDeletable(MI_MALE01);
  }
  g_bus = nullptr;
  g_driver = nullptr;
  g_spawnedByUs = false;
  g_state = TransitState::Idle;
  g_stopRequested = false;
  g_reverseUntilMs = 0;
  g_stallStep = 0;
  g_lastDiagLogMs = 0;
  g_boardAttemptStartMs = 0;
}

bool SpawnTransitBusForStop(size_t targetWpIdx, const Vec3 &playerPos,
                            uint32_t nowMs) {
  if (targetWpIdx >= kLoopCount)
    return false;
  const RouteWaypoint &wp = kPortlandLoop[targetWpIdx];

  // Prefer wp.spawnPos; if player stands too close (< 22m), step back one
  // waypoint
  Vec3 spawn = wp.spawnPos;
  float spawnHdgDeg = wp.spawnHeadingDeg;
  if (Dist2D(spawn, playerPos) < 22.0f) {
    const size_t prevIdx = (targetWpIdx + kLoopCount - 1) % kLoopCount;
    spawn = kPortlandLoop[prevIdx].spawnPos;
    spawnHdgDeg = kPortlandLoop[prevIdx].spawnHeadingDeg;
  }

  constexpr float DEG_TO_RAD = 3.14159265f / 180.0f;
  const float spawnRad = spawnHdgDeg * DEG_TO_RAD;
  // Offset +2.3m right of the road centerline so the bus starts in its own lane
  spawn.x += std::cos(spawnRad) * 2.3f;
  spawn.y += std::sin(spawnRad) * 2.3f;

  // Ensure no existing vehicle blocks the exact spawn location
  const uintptr_t pool =
      *reinterpret_cast<const uintptr_t *>(ADDR_VEH_POOL_PTR);
  if (pool) {
    const uintptr_t objectsBase = FieldAt<uintptr_t>(
        reinterpret_cast<const void *>(pool), OFF_POOL_OBJECTS);
    const uint8_t *byteMap = FieldAt<const uint8_t *>(
        reinterpret_cast<const void *>(pool), OFF_POOL_BYTEMAP);
    const int32_t poolSize =
        FieldAt<int32_t>(reinterpret_cast<const void *>(pool), OFF_POOL_SIZE);
    for (int32_t i = 0; i < poolSize; ++i) {
      if ((byteMap[i] & 0x80) != 0)
        continue;
      const void *other =
          reinterpret_cast<const void *>(objectsBase + i * VEHICLE_POOL_STRIDE);
      if (Dist2D(GetEntityPos(other), spawn) < 12.0f) {
        return false;
      }
    }
  }

  const int32_t busModel =
      (g_useCoachAlt && (g_dispatchIndex % 2 == 1)) ? MI_COACH : MI_BUS;
  if (!EnsureModelLoaded(busModel) || !EnsureModelLoaded(MI_MALE01)) {
    return false;
  }

  auto vehNew = reinterpret_cast<AutomobileNewFn>(ADDR_AUTOMOBILE_NEW);
  auto vehDelete = reinterpret_cast<AutomobileDelFn>(ADDR_AUTOMOBILE_DELETE);
  auto vehCtor = reinterpret_cast<AutomobileCtorFn>(ADDR_AUTOMOBILE_CTOR);
  auto pedNew = reinterpret_cast<CivPedNewFn>(ADDR_CIV_PED_NEW);
  auto pedCtor = reinterpret_cast<CivPedCtorFn>(ADDR_CIV_PED_CTOR);
  auto findGroundZ = reinterpret_cast<FindGroundZFn>(ADDR_FIND_GROUND_Z);
  auto getDistBase =
      reinterpret_cast<GetDistToBaseFn>(ADDR_GET_DIST_COM_TO_BASE);
  auto setHeading = reinterpret_cast<SetHeadingFn>(ADDR_PLACEABLE_SET_HEADING);
  auto updateRwMat = reinterpret_cast<ThisVoidFn>(ADDR_MATRIX_UPDATE_RW);
  auto updateRwFrame =
      reinterpret_cast<ThisVoidFn>(ADDR_ENTITY_UPDATE_RW_FRAME);
  auto clearSpace = reinterpret_cast<ClearSpaceFn>(ADDR_CLEAR_SPACE_FOR_ENTITY);
  auto worldAdd = reinterpret_cast<WorldAddFn>(ADDR_WORLD_ADD);
  auto registerRef = reinterpret_cast<EntityRegRefFn>(ADDR_ENTITY_REGISTER_REF);
  auto blendAnim = reinterpret_cast<AnimBlendFn>(ADDR_ANIM_BLEND);
  auto stopClumpAtomic =
      reinterpret_cast<ThisVoidFn>(ADDR_PED_STOP_CLUMP_ATOMIC);
  auto getZoneLevel = reinterpret_cast<ZonesGetLevelFn>(ADDR_ZONES_GET_LEVEL);

  // Allocate both pool slots before constructing or adding to world so failure
  // never leaves a driverless vehicle behind.
  void *veh = vehNew(VEHICLE_POOL_STRIDE);
  if (!veh)
    return false;
  void *driver = pedNew(PED_POOL_STRIDE);
  if (!driver) {
    vehDelete(veh);
    return false;
  }

  vehCtor(veh, busModel, MISSION_VEHICLE);
  FieldAt<uint8_t>(veh, OFF_VEH_RADIO_STATION) = RADIO_OFF;

  const float gz = findGroundZ(spawn.x, spawn.y);
  const float baseZ = (gz > -90.0f ? gz : spawn.z) + getDistBase(veh);
  const Vec3 worldSpawn{spawn.x, spawn.y, baseZ};

  FieldAt<float>(veh, OFF_ENTITY_POS + 0) = worldSpawn.x;
  FieldAt<float>(veh, OFF_ENTITY_POS + 4) = worldSpawn.y;
  FieldAt<float>(veh, OFF_ENTITY_POS + 8) = worldSpawn.z;

  clearSpace(&worldSpawn, veh);

  uint8_t &flags50 = FieldAt<uint8_t>(veh, OFF_ENTITY_FLAGS_50);
  flags50 = (flags50 & 0x07) | (STATUS_PHYSICS << 3);
  FieldAt<uint8_t>(veh, OFF_VEH_FLAGS_1F5) =
      (FieldAt<uint8_t>(veh, OFF_VEH_FLAGS_1F5) & 0xCF) |
      0x10; // engine on, handbrake off
  FieldAt<uint8_t>(veh, OFF_VEH_FLAGS_1F7) =
      (FieldAt<uint8_t>(veh, OFF_VEH_FLAGS_1F7) & 0xFB) | 0x04;
  FieldAt<float>(veh, OFF_VEH_STEER_ANGLE) = 0.0f;
  FieldAt<float>(veh, OFF_VEH_GAS_PEDAL) = 0.0f;
  FieldAt<float>(veh, OFF_VEH_BRAKE_PEDAL) = 0.0f;

  setHeading(veh, spawnRad);
  updateRwMat(reinterpret_cast<uint8_t *>(veh) + 4);
  updateRwFrame(veh);

  const uint8_t areaCode = static_cast<uint8_t>(getZoneLevel(&worldSpawn));
  FieldAt<uint8_t>(veh, OFF_VEH_AREA_CODE) = areaCode;
  worldAdd(veh);

  // Attach the driver directly using the exact CREATE_CHAR_INSIDE_CAR
  // (0x0129 / 0x43E660) sequence so pDriver is always bound immediately.
  pedCtor(driver, PEDTYPE_CIVMALE, MI_MALE01);
  FieldAt<uint8_t>(driver, OFF_PED_CREATED_BY) = MISSION_CHAR;
  FieldAt<uint8_t>(driver, OFF_PED_FLAGS_156) &= 0xFD;
  FieldAt<uint8_t>(driver, OFF_PED_FLAGS_15A) &= 0xFD;
  FieldAt<float>(driver, OFF_ENTITY_POS + 0) = worldSpawn.x;
  FieldAt<float>(driver, OFF_ENTITY_POS + 4) = worldSpawn.y;
  FieldAt<float>(driver, OFF_ENTITY_POS + 8) = worldSpawn.z;
  FieldAt<uint32_t>(driver, OFF_PED_STATE) = PEDSTATE_DRIVING;

  FieldAt<void *>(veh, OFF_VEH_DRIVER) = driver;
  registerRef(driver, &FieldAt<void *>(veh, OFF_VEH_DRIVER));
  FieldAt<void *>(driver, OFF_PED_VEHICLE) = veh;
  registerRef(veh, &FieldAt<void *>(driver, OFF_PED_VEHICLE));
  FieldAt<uint8_t>(driver, OFF_PED_IN_VEHICLE) = 1;
  FieldAt<uint8_t>(driver, OFF_ENTITY_FLAGS_51) &= 0xFE;

  void *clump = FieldAt<void *>(driver, OFF_ENTITY_RW_OBJECT);
  if (clump) {
    const uint32_t animId = (FieldAt<uint8_t>(veh, OFF_VEH_FLAGS_1F6) & 0x08)
                                ? ANIM_STD_CAR_SIT_LO
                                : ANIM_STD_CAR_SIT;
    FieldAt<void *>(driver, OFF_PED_VEHICLE_ANIM) =
        blendAnim(clump, 0, animId, 100.0f);
  }
  stopClumpAtomic(driver);
  FieldAt<uint8_t>(driver, OFF_PED_AREA_CODE) = areaCode;
  worldAdd(driver);

  g_bus = veh;
  g_driver = driver;
  g_spawnedByUs = true;
  g_wpIdx = targetWpIdx;
  g_state = TransitState::ApproachingStop;
  g_stateStartMs = nowMs;
  g_lastProgressMs = nowMs;
  g_lastProgressPos = worldSpawn;
  g_reverseUntilMs = 0;
  g_stallStep = 0;
  g_lastDiagLogMs = nowMs;
  g_stopRequested = false;
  g_lastSpawnMs = nowMs;
  ++g_dispatchIndex;

  DriveToWaypoint(g_bus, g_wpIdx);
  LogBusDiag("Spawn", g_bus, g_wpIdx);
  return true;
}

void *FindNearbyBoardableBus(const Vec3 &playerPos, float maxDist) {
  const uintptr_t pool =
      *reinterpret_cast<const uintptr_t *>(ADDR_VEH_POOL_PTR);
  if (!pool)
    return nullptr;
  const uintptr_t objectsBase = FieldAt<uintptr_t>(
      reinterpret_cast<const void *>(pool), OFF_POOL_OBJECTS);
  const uint8_t *byteMap = FieldAt<const uint8_t *>(
      reinterpret_cast<const void *>(pool), OFF_POOL_BYTEMAP);
  const int32_t poolSize =
      FieldAt<int32_t>(reinterpret_cast<const void *>(pool), OFF_POOL_SIZE);
  if (!objectsBase || !byteMap || poolSize <= 0)
    return nullptr;

  void *best = nullptr;
  float bestDist = maxDist;

  for (int32_t i = 0; i < poolSize; ++i) {
    if ((byteMap[i] & 0x80) != 0)
      continue;
    void *veh = reinterpret_cast<void *>(objectsBase + i * VEHICLE_POOL_STRIDE);
    if (!IsEntityType(veh, ENTITY_TYPE_VEHICLE))
      continue;
    if (FieldAt<uint32_t>(veh, OFF_VEH_CLASS) != VEHICLE_AUTOMOBILE)
      continue;
    const int16_t mi = FieldAt<int16_t>(veh, OFF_ENTITY_MODEL_INDEX);
    if (!IsTransitVehicleModel(mi))
      continue;
    if (!g_allowAmbient && veh != g_bus)
      continue;
    if (GetEntityStatus(veh) == STATUS_WRECKED ||
        FieldAt<float>(veh, OFF_VEH_HEALTH) <= 0.0f) {
      continue;
    }
    void *drv = FieldAt<void *>(veh, OFF_VEH_DRIVER);
    if (!drv || !IsPedAlive(drv))
      continue;
    if (veh != g_bus &&
        FieldAt<uint8_t>(veh, OFF_VEH_CREATED_BY) == MISSION_VEHICLE) {
      continue;
    }
    if (!HasFreePassengerSeat(veh))
      continue;

    const Vec3 vPos = GetEntityPos(veh);
    if (std::fabs(vPos.z - playerPos.z) > 4.5f)
      continue;
    const float d = Dist2D(vPos, playerPos);
    if (d < bestDist) {
      bestDist = d;
      best = veh;
    }
  }
  return best;
}

void *FindApproachingAmbientBus(const Vec3 &stopPos, float maxDist) {
  const uintptr_t pool =
      *reinterpret_cast<const uintptr_t *>(ADDR_VEH_POOL_PTR);
  if (!pool)
    return nullptr;
  const uintptr_t objectsBase = FieldAt<uintptr_t>(
      reinterpret_cast<const void *>(pool), OFF_POOL_OBJECTS);
  const uint8_t *byteMap = FieldAt<const uint8_t *>(
      reinterpret_cast<const void *>(pool), OFF_POOL_BYTEMAP);
  const int32_t poolSize =
      FieldAt<int32_t>(reinterpret_cast<const void *>(pool), OFF_POOL_SIZE);
  if (!objectsBase || !byteMap || poolSize <= 0)
    return nullptr;

  void *best = nullptr;
  float bestD = maxDist;
  for (int32_t i = 0; i < poolSize; ++i) {
    if ((byteMap[i] & 0x80) != 0)
      continue;
    void *veh = reinterpret_cast<void *>(objectsBase + i * VEHICLE_POOL_STRIDE);
    if (!IsEntityType(veh, ENTITY_TYPE_VEHICLE))
      continue;
    const int16_t mi = FieldAt<int16_t>(veh, OFF_ENTITY_MODEL_INDEX);
    if (!IsTransitVehicleModel(mi))
      continue;
    if (FieldAt<uint8_t>(veh, OFF_VEH_CREATED_BY) == MISSION_VEHICLE)
      continue;
    if (GetEntityStatus(veh) == STATUS_WRECKED ||
        FieldAt<float>(veh, OFF_VEH_HEALTH) <= 0.0f) {
      continue;
    }
    void *drv = FieldAt<void *>(veh, OFF_VEH_DRIVER);
    if (!drv || !IsPedAlive(drv))
      continue;
    if (!HasFreePassengerSeat(veh))
      continue;
    const Vec3 vPos = GetEntityPos(veh);
    const float d = Dist2D(vPos, stopPos);
    if (d < bestD && std::fabs(vPos.z - stopPos.z) < 8.0f) {
      bestD = d;
      best = veh;
    }
  }
  return best;
}

bool ValidateManagedBus(void *playerPed) {
  if (!g_bus)
    return false;
  if (!IsVehicleInPool(g_bus))
    return false;
  const int16_t mi = FieldAt<int16_t>(g_bus, OFF_ENTITY_MODEL_INDEX);
  if (!IsTransitVehicleModel(mi))
    return false;
  if (GetEntityStatus(g_bus) == STATUS_WRECKED ||
      FieldAt<float>(g_bus, OFF_VEH_HEALTH) <= 0.0f) {
    return false;
  }
  void *drv = FieldAt<void *>(g_bus, OFF_VEH_DRIVER);
  if (!drv || drv == playerPed || !IsPedInPool(drv) || !IsPedAlive(drv)) {
    return false;
  }
  g_driver = drv;
  return true;
}

void PerformTripSkipToStop(void *playerPed, size_t dstStopWpIdx,
                           uint32_t nowMs) {
  if (!g_bus || !ValidateManagedBus(playerPed) || dstStopWpIdx >= kLoopCount)
    return;

  const RouteWaypoint &dst = kPortlandLoop[dstStopWpIdx];
  auto loadScene =
      reinterpret_cast<StreamingLoadSceneFn>(ADDR_STREAMING_LOAD_SCENE);
  auto timerStop = reinterpret_cast<TimerVoidFn>(ADDR_TIMER_STOP);
  auto timerUpd = reinterpret_cast<TimerVoidFn>(ADDR_TIMER_UPDATE);
  auto findGroundZ = reinterpret_cast<FindGroundZFn>(ADDR_FIND_GROUND_Z);
  auto getDistBase =
      reinterpret_cast<GetDistToBaseFn>(ADDR_GET_DIST_COM_TO_BASE);
  auto clearSpace = reinterpret_cast<ClearSpaceFn>(ADDR_CLEAR_SPACE_FOR_ENTITY);
  auto setHeading = reinterpret_cast<SetHeadingFn>(ADDR_PLACEABLE_SET_HEADING);
  auto camFade = reinterpret_cast<CameraFadeFn>(ADDR_CAMERA_FADE);

  timerStop();
  loadScene(&dst.roadPos);
  timerUpd();

  const float gz = findGroundZ(dst.roadPos.x, dst.roadPos.y);
  const float baseZ = (gz > -90.0f ? gz : dst.roadPos.z) + getDistBase(g_bus);
  const Vec3 targetPos{dst.roadPos.x, dst.roadPos.y, baseZ};

  clearSpace(&targetPos, g_bus);

  float *moveSpd = &FieldAt<float>(g_bus, OFF_PHYSICAL_MOVE_SPEED);
  moveSpd[0] = moveSpd[1] = moveSpd[2] = 0.0f;
  float *turnSpd = &FieldAt<float>(g_bus, OFF_PHYSICAL_TURN_SPEED);
  turnSpd[0] = turnSpd[1] = turnSpd[2] = 0.0f;

  // Call CEntity::Teleport (vtable[11] = +0x2C)
  void **vtable = FieldAt<void **>(g_bus, OFF_ENTITY_VTABLE);
  auto teleport = reinterpret_cast<EntityTeleportFn>(vtable[11]);
  teleport(g_bus, targetPos);

  constexpr float DEG_TO_RAD = 3.14159265f / 180.0f;
  setHeading(g_bus, dst.headingDeg * DEG_TO_RAD);

  g_wpIdx = dstStopWpIdx;
  g_state = TransitState::DwellingAtStop;
  g_stateStartMs = nowMs;
  g_dwellStartMs = nowMs;
  g_lastProgressMs = nowMs;
  g_lastProgressPos = targetPos;
  g_stallStep = 0;
  HoldBusStopped(g_bus, nowMs, g_dwellMs);

  camFade(reinterpret_cast<void *>(ADDR_THE_CAMERA), 0.35f, 1);

  if (g_stopRequested) {
    g_stopRequested = false;
    auto setObj =
        reinterpret_cast<PedSetObjectiveEntFn>(ADDR_PED_SET_OBJECTIVE_ENT);
    FieldAt<uint8_t>(playerPed, OFF_PED_FLAGS_157) &= 0x7F;
    setObj(playerPed, OBJECTIVE_LEAVE_CAR, g_bus);
  }
}

// ============================================================================
// Per-Frame Logic
// ============================================================================
void UpdateBusTransit(uint32_t nowMs) {
  g_showHudPrompt = false;
  if (!g_enabled)
    return;

  if (*reinterpret_cast<const uint8_t *>(ADDR_REPLAY_MODE) ==
          REPLAY_MODE_PLAYBACK ||
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

  const bool boardDown = (GetAsyncKeyState(g_boardKey) & 0x8000) != 0;
  const bool boardPressed = boardDown && !g_wasPrevBoardDown;
  g_wasPrevBoardDown = boardDown;

  const bool skipDown = (GetAsyncKeyState(g_skipKey) & 0x8000) != 0;
  const bool skipPressed = skipDown && !g_wasPrevSkipDown;
  g_wasPrevSkipDown = skipDown;

  if (*reinterpret_cast<const uint8_t *>(ADDR_PLAYER_WB_STATE) !=
          WBSTATE_PLAYING ||
      !playerPed || !IsPedAlive(playerPed)) {
    ReleaseManagedBus();
    return;
  }

  const bool inVehicle = FieldAt<uint8_t>(playerPed, OFF_PED_IN_VEHICLE) != 0;
  const Vec3 playerPos = GetEntityPos(playerPed);

  if (g_bus && !ValidateManagedBus(playerPed)) {
    WriteLog("[Validate] Managed bus invalid/wrecked, releasing");
    ReleaseManagedBus();
  }

  const bool ridingManagedBus =
      inVehicle && playerVeh != nullptr && playerVeh == g_bus &&
      FieldAt<void *>(playerVeh, OFF_VEH_DRIVER) != playerPed;

  // --------------------------------------------------------------------------
  // 1. Handle Trip-Skip Fade-Out Completion While Riding
  // --------------------------------------------------------------------------
  if (g_state == TransitState::SkipFadeOut) {
    if (!ridingManagedBus) {
      g_state = TransitState::ApproachingStop;
    } else if (nowMs - g_stateStartMs >= 360) {
      const size_t nextStop = NextStopWpIndex(g_wpIdx);
      PerformTripSkipToStop(playerPed, nextStop, nowMs);
    }
    if (ridingManagedBus) {
      g_showHudPrompt = true;
      std::swprintf(g_hudLine1, 128, L"Skipping to next stop...");
      g_hudLine2[0] = L'\0';
      g_hudColor1 = CRGBA{147, 196, 211, 255};
      return;
    }
  }

  // --------------------------------------------------------------------------
  // 2. While Riding Inside Bus as Passenger
  // --------------------------------------------------------------------------
  if (ridingManagedBus) {
    const float busSpeed = GetVehicleSpeed2D(g_bus);

    if (boardPressed) {
      const uint32_t curObj = FieldAt<uint32_t>(playerPed, OFF_PED_OBJECTIVE);
      if (curObj != OBJECTIVE_LEAVE_CAR) {
        if (g_state == TransitState::DwellingAtStop || busSpeed < 0.04f) {
          HoldBusStopped(g_bus, nowMs, 3500);
          auto setObj = reinterpret_cast<PedSetObjectiveEntFn>(
              ADDR_PED_SET_OBJECTIVE_ENT);
          FieldAt<uint8_t>(playerPed, OFF_PED_FLAGS_157) &= 0x7F;
          setObj(playerPed, OBJECTIVE_LEAVE_CAR, g_bus);
          g_stopRequested = false;
        } else {
          g_stopRequested = !g_stopRequested;
        }
      }
    } else if (skipPressed && g_state != TransitState::SkipFadeOut) {
      g_state = TransitState::SkipFadeOut;
      g_stateStartMs = nowMs;
      auto camFade = reinterpret_cast<CameraFadeFn>(ADDR_CAMERA_FADE);
      camFade(reinterpret_cast<void *>(ADDR_THE_CAMERA), 0.35f, 0);
      return;
    }

    if (FieldAt<uint32_t>(playerPed, OFF_PED_OBJECTIVE) ==
        OBJECTIVE_LEAVE_CAR) {
      HoldBusStopped(g_bus, nowMs, 1500);
    }
  }

  // --------------------------------------------------------------------------
  // 3. Dispatch / Adopt Bus if Player Is On Foot Near a Portland Stop
  // --------------------------------------------------------------------------
  float distToStop = 0.0f;
  const int nearStopIdx =
      (!inVehicle && IsPortlandArea(playerPos))
          ? FindNearestStopWp(playerPos, STOP_SIGN_RADIUS, &distToStop)
          : -1;

  if (!inVehicle && !g_bus && nearStopIdx >= 0 &&
      *ScriptGlobalPtr(SLOT_ON_MISSION) == 0 &&
      GetPlayerWantedLevel(playerPed) < 2) {
    void *ambient =
        FindApproachingAmbientBus(kPortlandLoop[nearStopIdx].roadPos, 85.0f);
    if (ambient) {
      g_bus = ambient;
      g_driver = FieldAt<void *>(ambient, OFF_VEH_DRIVER);
      g_spawnedByUs = false;
      g_wpIdx = static_cast<size_t>(nearStopIdx);
      g_state = TransitState::ApproachingStop;
      g_stateStartMs = nowMs;
      g_lastProgressMs = nowMs;
      g_lastProgressPos = GetEntityPos(ambient);
      g_stallStep = 0;
      g_lastDiagLogMs = nowMs;
      DriveToWaypoint(g_bus, g_wpIdx);
      LogBusDiag("Adopt", g_bus, g_wpIdx);
    } else if (nowMs - g_lastSpawnMs >= 4000) {
      SpawnTransitBusForStop(static_cast<size_t>(nearStopIdx), playerPos,
                             nowMs);
    }
  }

  // Release managed bus if player walked far away while not riding/boarding
  const bool isBoarding =
      !inVehicle && FieldAt<uint32_t>(playerPed, OFF_PED_OBJECTIVE) ==
                        OBJECTIVE_ENTER_CAR_AS_PASSENGER;
  if (g_bus && !ridingManagedBus && !isBoarding) {
    const float distBus = Dist2D(GetEntityPos(g_bus), playerPos);
    if (distBus > 195.0f || inVehicle) {
      ReleaseManagedBus();
    }
  }

  // --------------------------------------------------------------------------
  // 4. Advance Bus Route State Machine (Approaching <-> Dwelling)
  // --------------------------------------------------------------------------
  if (g_bus) {
    const Vec3 busPos = GetEntityPos(g_bus);
    const float busSpeed = GetVehicleSpeed2D(g_bus);
    const float busHealth = FieldAt<float>(g_bus, OFF_VEH_HEALTH);
    const float distPlayerToBus = Dist2D(busPos, playerPos);
    const RouteWaypoint &wp = kPortlandLoop[g_wpIdx];
    const float distTarget = Dist2D(busPos, wp.roadPos);

    if (nowMs - g_stateStartMs <= 12000 && nowMs - g_lastDiagLogMs >= 2000) {
      g_lastDiagLogMs = nowMs;
      LogBusDiag("Tick", g_bus, g_wpIdx);
    }

    if (isBoarding) {
      HoldBusStopped(g_bus, nowMs, 1500);
      if (g_boardAttemptStartMs != 0 && nowMs - g_boardAttemptStartMs >= 4500 &&
          distPlayerToBus < 12.0f && HasFreePassengerSeat(g_bus)) {
        auto warpIntoCar =
            reinterpret_cast<PedWarpIntoCarFn>(ADDR_PED_WARP_INTO_CAR);
        warpIntoCar(playerPed, g_bus);
        g_boardAttemptStartMs = 0;
      }
    } else {
      g_boardAttemptStartMs = 0;
    }

    if (g_state == TransitState::ApproachingStop) {
      const float arrivalRadius = wp.isStop ? 11.5f : 15.0f;
      if (distTarget <= arrivalRadius &&
          std::fabs(busPos.z - wp.roadPos.z) < 5.0f) {
        g_stallStep = 0;
        if (wp.isStop) {
          g_state = TransitState::DwellingAtStop;
          g_stateStartMs = nowMs;
          g_dwellStartMs = nowMs;
          HoldBusStopped(g_bus, nowMs, g_dwellMs);
          LogBusDiag("ArriveStop", g_bus, g_wpIdx);
        } else {
          g_wpIdx = NextWpIndex(g_wpIdx);
          g_stateStartMs = nowMs;
          g_lastProgressMs = nowMs;
          g_lastProgressPos = busPos;
          DriveToWaypoint(g_bus, g_wpIdx);
          LogBusDiag("NextWP", g_bus, g_wpIdx);
        }
      } else {
        if (g_reverseUntilMs != 0) {
          if (static_cast<int32_t>(nowMs - g_reverseUntilMs) >= 0) {
            g_reverseUntilMs = 0;
            DriveToWaypoint(g_bus, g_wpIdx, DRIVINGSTYLE_AVOID_CARS);
            g_lastProgressMs = nowMs;
            g_lastProgressPos = busPos;
          }
        } else {
          if (Dist2D(busPos, g_lastProgressPos) >= 1.5f) {
            g_lastProgressPos = busPos;
            g_lastProgressMs = nowMs;
            g_stallStep = 0;
          } else if (!ridingManagedBus && busHealth < 750.0f &&
                     nowMs - g_lastProgressMs >= 2000) {
            LogBusDiag("DamagedRelease", g_bus, g_wpIdx);
            ReleaseManagedBus();
          } else if (nowMs - g_lastProgressMs >= 4500) {
            if (g_stallStep == 0) {
              LogBusDiag("StallAvoid", g_bus, g_wpIdx);
              g_stallStep = 1;
              DriveToWaypoint(g_bus, g_wpIdx, DRIVINGSTYLE_AVOID_CARS);
              g_lastProgressMs = nowMs;
            } else if (!ridingManagedBus) {
              LogBusDiag("StallRelease", g_bus, g_wpIdx);
              ReleaseManagedBus();
            } else if (g_stallStep == 1) {
              LogBusDiag("StallRev", g_bus, g_wpIdx);
              g_stallStep = 2;
              FieldAt<uint8_t>(g_bus, OFF_VEH_AUTOPILOT_TEMP_ACTION) =
                  TEMPACT_REVERSE;
              FieldAt<uint32_t>(g_bus, OFF_VEH_AUTOPILOT_TIME_TEMP_ACTION) =
                  nowMs + 500;
              g_reverseUntilMs = nowMs + 600;
            }
          }
        }
      }
    } else if (g_state == TransitState::DwellingAtStop) {
      const bool playerLeaving =
          FieldAt<uint32_t>(playerPed, OFF_PED_OBJECTIVE) ==
          OBJECTIVE_LEAVE_CAR;
      if (g_stopRequested && ridingManagedBus && busSpeed < 0.045f &&
          !playerLeaving) {
        g_stopRequested = false;
        auto setObj =
            reinterpret_cast<PedSetObjectiveEntFn>(ADDR_PED_SET_OBJECTIVE_ENT);
        FieldAt<uint8_t>(playerPed, OFF_PED_FLAGS_157) &= 0x7F;
        setObj(playerPed, OBJECTIVE_LEAVE_CAR, g_bus);
      }

      if (isBoarding || playerLeaving) {
        HoldBusStopped(g_bus, nowMs, 1800);
        g_dwellStartMs = nowMs - (g_dwellMs / 2);
      } else if (nowMs - g_dwellStartMs >= g_dwellMs) {
        g_wpIdx = NextWpIndex(g_wpIdx);
        g_state = TransitState::ApproachingStop;
        g_stateStartMs = nowMs;
        g_lastProgressMs = nowMs;
        g_lastProgressPos = busPos;
        g_stallStep = 0;
        DriveToWaypoint(g_bus, g_wpIdx);
        LogBusDiag("Depart", g_bus, g_wpIdx);
      } else {
        HoldBusStopped(g_bus, nowMs, 500);
      }
    }
  }

  // --------------------------------------------------------------------------
  // 5. Handle On-Foot Boarding Input Near Any Stopped/Slow Bus or Coach
  // --------------------------------------------------------------------------
  void *boardTarget = nullptr;
  if (!inVehicle && !isBoarding) {
    boardTarget = FindNearbyBoardableBus(playerPos, 9.5f);
    if (boardTarget && GetVehicleSpeed2D(boardTarget) > 0.09f) {
      if (boardTarget != g_bus || g_state != TransitState::DwellingAtStop) {
        boardTarget = nullptr;
      }
    }
  }

  if (boardTarget && boardPressed) {
    int32_t &money = *reinterpret_cast<int32_t *>(ADDR_PLAYER_MONEY);
    if (money < g_fare) {
      std::swprintf(g_noticeText, 96, L"Need $%d for Bus Fare", g_fare);
      g_noticeUntilMs = nowMs + 2500;
    } else {
      money -= g_fare;

      if (boardTarget != g_bus) {
        if (g_bus)
          ReleaseManagedBus();
        g_bus = boardTarget;
        g_driver = FieldAt<void *>(boardTarget, OFF_VEH_DRIVER);
        g_spawnedByUs = false;
        const int nearestWp = FindNearestLoopWp(GetEntityPos(boardTarget));
        g_wpIdx = NextWpIndex(static_cast<size_t>(nearestWp));
        g_state = TransitState::DwellingAtStop;
        g_stateStartMs = nowMs;
        g_dwellStartMs = nowMs;
        g_lastProgressMs = nowMs;
        g_lastProgressPos = GetEntityPos(boardTarget);
        g_stallStep = 0;
        g_stopRequested = false;
      }

      HoldBusStopped(g_bus, nowMs, 4000);
      auto setObj =
          reinterpret_cast<PedSetObjectiveEntFn>(ADDR_PED_SET_OBJECTIVE_ENT);
      FieldAt<uint8_t>(playerPed, OFF_PED_FLAGS_157) &= 0x7F;
      setObj(playerPed, OBJECTIVE_ENTER_CAR_AS_PASSENGER, g_bus);
      g_boardAttemptStartMs = nowMs;
    }
  }

  // --------------------------------------------------------------------------
  // 6. Build HUD Prompt Strings
  // --------------------------------------------------------------------------
  const char boardChar = static_cast<char>(g_boardKey);
  const char skipChar = static_cast<char>(g_skipKey);

  if (g_noticeUntilMs != 0 &&
      static_cast<int32_t>(g_noticeUntilMs - nowMs) > 0) {
    g_showHudPrompt = true;
    std::swprintf(g_hudLine1, 128, L"%ls", g_noticeText);
    g_hudLine2[0] = L'\0';
    g_hudColor1 = CRGBA{235, 198, 72, 255};
    return;
  }

  if (ridingManagedBus) {
    g_showHudPrompt = true;
    const size_t nextStopIdx =
        kPortlandLoop[g_wpIdx].isStop ? g_wpIdx : NextStopWpIndex(g_wpIdx);
    const RouteWaypoint &stopWp = kPortlandLoop[nextStopIdx];
    const float d = Dist2D(GetEntityPos(g_bus), stopWp.roadPos);

    if (g_state == TransitState::DwellingAtStop) {
      std::swprintf(g_hudLine1, 128, L"AT STOP (%d/8): %ls", stopWp.stopNumber,
                    stopWp.name);
      g_hudColor1 = CRGBA{120, 224, 142, 255};
      std::swprintf(g_hudLine2, 128, L"[%c] Step Off   [%c] Skip to Next Stop",
                    boardChar, skipChar);
    } else if (g_stopRequested) {
      std::swprintf(g_hudLine1, 128, L"STOP REQUESTED: %ls (%.0fm)",
                    stopWp.name, d);
      g_hudColor1 = CRGBA{235, 198, 72, 255};
      std::swprintf(g_hudLine2, 128, L"[%c] Cancel Request   [%c] Skip to Stop",
                    boardChar, skipChar);
    } else {
      std::swprintf(g_hudLine1, 128, L"Portland Loop -> %ls (%.0fm)",
                    stopWp.name, d);
      g_hudColor1 = CRGBA{147, 196, 211, 255};
      std::swprintf(g_hudLine2, 128, L"[%c] Request Next Stop   [%c] Skip Stop",
                    boardChar, skipChar);
    }
  } else if (isBoarding) {
    g_showHudPrompt = true;
    std::swprintf(g_hudLine1, 128, L"Boarding Bus ($%d Fare)...", g_fare);
    g_hudLine2[0] = L'\0';
    g_hudColor1 = CRGBA{147, 196, 211, 255};
  } else if (boardTarget) {
    g_showHudPrompt = true;
    std::swprintf(g_hudLine1, 128, L"Press [%c] to Board Bus ($%d Fare)",
                  boardChar, g_fare);
    g_hudLine2[0] = L'\0';
    g_hudColor1 = CRGBA{120, 224, 142, 255};
  } else if (nearStopIdx >= 0 && distToStop <= STOP_SIGN_RADIUS) {
    g_showHudPrompt = true;
    const RouteWaypoint &sw = kPortlandLoop[nearStopIdx];
    std::swprintf(g_hudLine1, 128, L"Bus Stop (%d/8): %ls", sw.stopNumber,
                  sw.name);
    g_hudColor1 = CRGBA{147, 196, 211, 255};
    if (g_bus) {
      const float dBus = Dist2D(GetEntityPos(g_bus), sw.roadPos);
      if (g_state == TransitState::DwellingAtStop && dBus < 14.0f) {
        std::swprintf(g_hudLine2, 128,
                      L"Bus stopped - walk to door & press [%c]", boardChar);
      } else {
        std::swprintf(g_hudLine2, 128, L"Portland Loop Bus approaching (%.0fm)",
                      dBus);
      }
    } else {
      std::swprintf(g_hudLine2, 128, L"Waiting for next Portland Loop Bus...");
    }
  }
}

void DrawBusHud() {
  if (!g_showHudPrompt || g_hudLine1[0] == L'\0')
    return;

  if (*reinterpret_cast<const uint8_t *>(ADDR_PLAYER_WB_STATE) !=
          WBSTATE_PLAYING ||
      *reinterpret_cast<const uint8_t *>(ADDR_REPLAY_MODE) ==
          REPLAY_MODE_PLAYBACK ||
      *reinterpret_cast<const uint8_t *>(ADDR_PREFS_SHOW_HUD) == 0 ||
      *reinterpret_cast<const uint8_t *>(ADDR_USER_PAUSE) != 0 ||
      *reinterpret_cast<const uint8_t *>(ADDR_CODE_PAUSE) != 0 ||
      *reinterpret_cast<const uint8_t *>(ADDR_THE_CAMERA +
                                         OFF_CAM_WIDESCREEN) != 0) {
    return;
  }

  const int32_t screenW = *reinterpret_cast<const int32_t *>(ADDR_RS_MAX_WIDTH);
  const int32_t screenH =
      *reinterpret_cast<const int32_t *>(ADDR_RS_MAX_HEIGHT);
  if (screenW <= 0 || screenH <= 0)
    return;

  const float scaleX = static_cast<float>(screenW) / 640.0f;
  const float scaleY = static_cast<float>(screenH) / 448.0f;
  const float centerX = static_cast<float>(screenW) * 0.5f;
  const float wrapWidth = static_cast<float>(screenW) * 0.92f;
  const float baseY = static_cast<float>(screenH) - 72.0f * scaleY;

  auto setJustifyOff = reinterpret_cast<FontVoidFn>(ADDR_FONT_SET_JUSTIFY_OFF);
  auto setCentreOn = reinterpret_cast<FontVoidFn>(ADDR_FONT_SET_CENTRE_ON);
  auto setCentreOff = reinterpret_cast<FontVoidFn>(ADDR_FONT_SET_CENTRE_OFF);
  auto setWrapx = reinterpret_cast<FontFloatFn>(ADDR_FONT_SET_WRAPX);
  auto setCentreSize = reinterpret_cast<FontFloatFn>(ADDR_FONT_SET_CENTRE_SIZE);
  auto setBgOff = reinterpret_cast<FontVoidFn>(ADDR_FONT_SET_BACKGROUND_OFF);
  auto setRightOff =
      reinterpret_cast<FontVoidFn>(ADDR_FONT_SET_RIGHT_JUSTIFY_OFF);
  auto setScale = reinterpret_cast<FontScaleFn>(ADDR_FONT_SET_SCALE);
  auto setPropOn = reinterpret_cast<FontVoidFn>(ADDR_FONT_SET_PROP_ON);
  auto setFontStyle = reinterpret_cast<FontStyleFn>(ADDR_FONT_SET_FONT_STYLE);
  auto setAlphaFade = reinterpret_cast<FontFloatFn>(ADDR_FONT_SET_ALPHA_FADE);
  auto setDropColor = reinterpret_cast<FontColorFn>(ADDR_FONT_SET_DROP_COLOR);
  auto setDropShadow =
      reinterpret_cast<FontDropShadowFn>(ADDR_FONT_SET_DROP_SHADOW_POS);
  auto setColor = reinterpret_cast<FontColorFn>(ADDR_FONT_SET_COLOR);
  auto printString =
      reinterpret_cast<FontPrintStringFn>(ADDR_FONT_PRINT_STRING);
  auto drawFonts = reinterpret_cast<FontVoidFn>(ADDR_FONT_DRAW_FONTS);

  const CRGBA shadow{0, 0, 0, 235};

  setJustifyOff();
  setRightOff();
  setBgOff();
  setCentreOn();
  setWrapx(wrapWidth);
  setCentreSize(wrapWidth);
  setPropOn();
  setAlphaFade(255.0f);
  setFontStyle(FONT_STANDARD);
  setDropShadow(1);
  setDropColor(&shadow);

  setScale(0.52f * scaleX, 0.92f * scaleY);
  setColor(&g_hudColor1);
  printString(centerX, baseY, g_hudLine1);

  if (g_hudLine2[0] != L'\0') {
    setScale(0.44f * scaleX, 0.80f * scaleY);
    const float line2Y = baseY + 21.0f * scaleY;
    setColor(&g_hudColor2);
    printString(centerX, line2Y, g_hudLine2);
  }

  setDropShadow(0);
  setCentreOff();
  drawFonts();
}

int __cdecl Hook_ProcessColModels(const void *matA, const void *colA,
                                  const void *matB, const void *colB,
                                  void *aSphereColPoints, void *aLineColPoints,
                                  float *aLineMaxTouchDist) {
  // Multiple vanilla callers (0x4541FE, 0x4CC2D8, 0x530880, etc.) pass NULL
  // for line outputs even when colA is a vehicle with 4 suspension lines,
  // causing CCollision::TestLineTriangle (0x40F669) to dereference NULL.
  if ((!aLineColPoints || !aLineMaxTouchDist) && colA) {
    const int16_t numLines = FieldAt<int16_t>(colA, OFF_COLMODEL_NUM_LINES);
    if (numLines > 0) {
      constexpr size_t kScratchLines = 64;
      const size_t count = static_cast<size_t>(numLines);
      if (count <= kScratchLines) {
        alignas(16) uint8_t scratchColPoints[kScratchLines * COLPOINT_SIZE];
        float scratchTouchDist[kScratchLines];
        if (!aLineColPoints) {
          std::memset(scratchColPoints, 0, count * COLPOINT_SIZE);
          aLineColPoints = scratchColPoints;
        }
        if (!aLineMaxTouchDist) {
          for (size_t i = 0; i < count; ++i) {
            scratchTouchDist[i] = 1.0f;
          }
          aLineMaxTouchDist = scratchTouchDist;
        }
        return g_origProcessColModels(matA, colA, matB, colB, aSphereColPoints,
                                      aLineColPoints, aLineMaxTouchDist);
      }
      std::vector<uint8_t> heapColPoints;
      std::vector<float> heapTouchDist;
      if (!aLineColPoints) {
        heapColPoints.assign(count * COLPOINT_SIZE, 0);
        aLineColPoints = heapColPoints.data();
      }
      if (!aLineMaxTouchDist) {
        heapTouchDist.assign(count, 1.0f);
        aLineMaxTouchDist = heapTouchDist.data();
      }
      return g_origProcessColModels(matA, colA, matB, colB, aSphereColPoints,
                                    aLineColPoints, aLineMaxTouchDist);
    }
  }
  return g_origProcessColModels(matA, colA, matB, colB, aSphereColPoints,
                                aLineColPoints, aLineMaxTouchDist);
}

void __cdecl Hook_ScriptsProcess() {
  g_origScriptsProcess();
  UpdateBusTransit(TimeMs());
}

void __cdecl Hook_HudDraw() {
  g_origHudDraw();
  DrawBusHud();
}

void InstallDeferredHooks() {
  if (g_deferredInstalled)
    return;
  g_deferredInstalled = true;

  const bool okCol =
      InstallDetour(ADDR_COLLISION_PROCESS_COL_MODELS, &Hook_ProcessColModels,
                    &g_origProcessColModels);
  const bool okScr = InstallDetour(ADDR_SCRIPTS_PROCESS, &Hook_ScriptsProcess,
                                   &g_origScriptsProcess);
  const bool okHud =
      InstallDetour(ADDR_HUD_DRAW, &Hook_HudDraw, &g_origHudDraw);

  WriteLog(okCol ? "Collision::ProcessColModels hook (0x410BE0) OK"
                 : "Collision::ProcessColModels hook (0x410BE0) FAILED");
  WriteLog(okScr ? "ScriptsProcess hook (0x439040) OK"
                 : "ScriptsProcess hook (0x439040) FAILED");
  WriteLog(okHud ? "HudDraw hook (0x5052A0) OK"
                 : "HudDraw hook (0x5052A0) FAILED");
}

bool __fastcall Hook_Initialise(void *self, uintptr_t) {
  const bool ok = g_origInitialise(self);
  InstallDeferredHooks();
  return ok;
}

void InstallBootHook() {
  const bool ok =
      InstallDetour(ADDR_MUSIC_INITIALISE, &Hook_Initialise, &g_origInitialise);
  WriteLog(ok ? "Boot hook (cMusicManager::Initialise) OK"
              : "Boot hook (cMusicManager::Initialise) FAILED");
}

} // namespace

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(hModule);
    InitPaths(hModule);
    LoadConfig();
    if (g_enabled) {
      InstallBootHook();
    } else {
      WriteLog("BusTransit disabled via BusTransit.ini");
    }
  } else if (reason == DLL_PROCESS_DETACH) {
    if (lpReserved == nullptr) {
      RemoveAllHooks();
    }
  }
  return TRUE;
}
