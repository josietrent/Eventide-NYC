#define WIN32_LEAN_AND_MEAN
#include <stdint.h>
#include <windows.h>


namespace {

// GTA III v1.0 memory addresses
// In GTA III 1.0, the animation association table for player movement groups is
// in the .data section: Group player2armed (index 2 / ANIM_STD_RUNFAST anim
// name pointer): 0x005EB264 Group playerBBBat (index 2 / ANIM_STD_RUNFAST anim
// name pointer): 0x005EB278 Pointer to "SPRINT_civi" string in .data:
// 0x005EB1B8

constexpr uintptr_t ADDR_ANIM_PLAYER2ARMED_SPRINT = 0x005EB264;
constexpr uintptr_t ADDR_ANIM_PLAYERBBBAT_SPRINT = 0x005EB278;
constexpr uintptr_t ADDR_STR_SPRINT_CIVI = 0x005EB1B8;

void PatchWeaponSprint() {
  DWORD oldProtect;
  if (VirtualProtect(reinterpret_cast<void *>(ADDR_ANIM_PLAYER2ARMED_SPRINT),
                     0x20, PAGE_EXECUTE_READWRITE, &oldProtect)) {
    // Point player2armed (Shotgun, AK-47, M16) sprint animation to SPRINT_civi
    *reinterpret_cast<uintptr_t *>(ADDR_ANIM_PLAYER2ARMED_SPRINT) =
        ADDR_STR_SPRINT_CIVI;

    // Point playerBBBat (Baseball Bat) sprint animation to SPRINT_civi
    *reinterpret_cast<uintptr_t *>(ADDR_ANIM_PLAYERBBBAT_SPRINT) =
        ADDR_STR_SPRINT_CIVI;

    VirtualProtect(reinterpret_cast<void *>(ADDR_ANIM_PLAYER2ARMED_SPRINT),
                   0x20, oldProtect, &oldProtect);
  }
}

} // namespace

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID /*lpReserved*/) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(hModule);
    PatchWeaponSprint();
  }
  return TRUE;
}
