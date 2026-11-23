#define WIN32_LEAN_AND_MEAN
#include <cstdint>
#include <windows.h>

namespace {

// GTA III v1.0 addresses (verified against gta3.exe)
constexpr uintptr_t ADDR_HOOK_PLAYER_CONTROL_WALK = 0x004F051D;
constexpr uintptr_t ADDR_HOOK_SET_ATTACK_ANIM = 0x004E6410;
constexpr uintptr_t ADDR_WALK_SKIP_ATTACK = 0x004F06B0;
constexpr uintptr_t ADDR_WALK_CONTINUE = 0x004F052A;

constexpr uint32_t WEAPONTYPE_BASEBALLBAT = 1;

bool g_patched = false;

// Checks whether ped is player holding a baseball bat
bool __stdcall IsPlayerWithBat(void *pPed) {
  if (!pPed)
    return false;
  auto *pBytes = reinterpret_cast<uint8_t *>(pPed);
  // In CPed, offset 0x498 is m_currentWeapon (int8)
  const int8_t currentWepSlot = *reinterpret_cast<int8_t *>(pBytes + 0x498);
  if (currentWepSlot < 0 || currentWepSlot >= 13)
    return false;
  // CWeapon m_weapons[13] is at 0x35C (sizeof(CWeapon) = 0x18)
  const uint32_t wepType =
      *reinterpret_cast<uint32_t *>(pBytes + 0x35C + currentWepSlot * 0x18);
  return wepType == WEAPONTYPE_BASEBALLBAT;
}

// Hook at 0x004F051D (7 bytes: cmp dword ptr [esi+224h], 10h; jz 0x4F06B0)
__declspec(naked) void Hook_PlayerControlWalk_AttackCheck() {
  __asm {
    cmp dword ptr [esi + 0x224], 0x10
    jnz continue_walk

    pushfd
    push eax
    push ecx
    push edx
    push esi
    call IsPlayerWithBat
    test al, al
    pop edx
    pop ecx
    pop eax
    popfd
    jnz continue_walk // Allow movement if holding baseball bat

    push ADDR_WALK_SKIP_ATTACK
    ret

  continue_walk:
    push ADDR_WALK_CONTINUE
    ret
  }
}

// Hook at 0x004E6410 (4 bytes: or dword ptr [esi+30h], 1)
// esi holds pAnimBlendAssoc, ebp holds CPed*
__declspec(naked) void Hook_SetAttackAnimFlags() {
  __asm {
    or dword ptr [esi + 0x30], 1 // original ASSOC_RUNNING (0x01)
    pushfd
    push eax
    push ecx
    push edx
    push ebp // CPed*
    call IsPlayerWithBat
    test al, al
    pop edx
    pop ecx
    pop eax
    popfd
    jz done
    or dword ptr [esi + 0x30], 0x10 // ASSOC_PARTIAL: blend as partial on upper body
  done:
    ret
  }
}

void InstallHooks() {
  DWORD oldProtect = 0;

  // 1. Hook CPlayerPed::PlayerControlWalk attack lockout (0x004F051D, 13 bytes
  // original: 83 BE 24 02 00 00 10 0F 84 86 01 00 00)
  auto *targetWalk = reinterpret_cast<uint8_t *>(ADDR_HOOK_PLAYER_CONTROL_WALK);
  if (VirtualProtect(targetWalk, 13, PAGE_EXECUTE_READWRITE, &oldProtect)) {
    const int32_t rel = static_cast<int32_t>(
        reinterpret_cast<uintptr_t>(&Hook_PlayerControlWalk_AttackCheck) -
        (ADDR_HOOK_PLAYER_CONTROL_WALK + 5));
    targetWalk[0] = 0xE9;
    *reinterpret_cast<int32_t *>(targetWalk + 1) = rel;
    for (int i = 5; i < 13; ++i)
      targetWalk[i] = 0x90; // nop remainder
    VirtualProtect(targetWalk, 13, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), targetWalk, 13);
  }

  // 2. Hook CPed::SetAttack anim flags (0x004E6410, call our helper via 5-byte
  // call)
  auto *targetAnim = reinterpret_cast<uint8_t *>(ADDR_HOOK_SET_ATTACK_ANIM);
  if (VirtualProtect(targetAnim, 5, PAGE_EXECUTE_READWRITE, &oldProtect)) {
    const int32_t rel = static_cast<int32_t>(
        reinterpret_cast<uintptr_t>(&Hook_SetAttackAnimFlags) -
        (ADDR_HOOK_SET_ATTACK_ANIM + 5));
    targetAnim[0] = 0xE8;
    *reinterpret_cast<int32_t *>(targetAnim + 1) = rel;
    VirtualProtect(targetAnim, 5, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), targetAnim, 5);
  }

  g_patched = true;
}

} // namespace

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID /*lpReserved*/) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(hModule);
    InstallHooks();
  }
  return TRUE;
}
