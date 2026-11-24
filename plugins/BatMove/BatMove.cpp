#define WIN32_LEAN_AND_MEAN
#include <cstdint>
#include <cstring>
#include <windows.h>

namespace {

// GTA III v1.0 addresses (verified against gta3.exe)
constexpr uintptr_t ADDR_FIND_PLAYER_PED = 0x004A1150;
constexpr uintptr_t ADDR_HOOK_SET_ATTACK_BAT =
    0x004E674E; // 6 bytes: mov ebx, eax; or dword ptr [ebx+30h], 1
constexpr uintptr_t ADDR_HOOK_ATTACK_TURN_RATE =
    0x004C74A8; // 9 bytes: cmp dword ptr [ebx+224h], 10h; jne 0x004C74BF
constexpr uintptr_t ADDR_TURN_PENALTY_APPLY = 0x004C74B1;
constexpr uintptr_t ADDR_TURN_PENALTY_SKIP = 0x004C74BF;

constexpr uint32_t WEAPONTYPE_BASEBALLBAT = 1;
constexpr int32_t ANIM_WEAPON_BAT_H = 0x2F;
constexpr int32_t ANIM_WEAPON_BAT_V = 0x30;

constexpr uint32_t ASSOC_PARTIAL = 0x10;
constexpr uint32_t ASSOC_MOVEMENT = 0x20;
constexpr uint32_t ASSOC_TRANSLATE = 0x40;

void *GetPlayerPed() {
  return reinterpret_cast<void *(__cdecl *)()>(ADDR_FIND_PLAYER_PED)();
}

bool __stdcall IsPlayerWithBat(void *pPed) {
  if (!pPed || pPed != GetPlayerPed())
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

bool IsLowerBodyBone(const char *boneName) {
  static constexpr const char *kLowerBodyBones[] = {
      "Swaist",     "Supperlegr", "Slowerlegr", "Sfootr",
      "Supperlegl", "Slowerlegl", "Sfootl",
  };
  for (const char *bone : kLowerBodyBones) {
    if (std::strcmp(boneName, bone) == 0)
      return true;
  }
  return false;
}

void __stdcall OnBatAttackAnimCreated(void *pPed, void *pAssoc) {
  if (!pAssoc || !IsPlayerWithBat(pPed))
    return;

  auto *assoc = reinterpret_cast<uint8_t *>(pAssoc);
  const int32_t animId = *reinterpret_cast<int32_t *>(assoc + 0x2C);
  if (animId != ANIM_WEAPON_BAT_H && animId != ANIM_WEAPON_BAT_V)
    return;

  // Keep bat swing as an upper-body partial anim with no root translation lock
  uint32_t &flags = *reinterpret_cast<uint32_t *>(assoc + 0x30);
  flags = (flags | ASSOC_PARTIAL) & ~(ASSOC_MOVEMENT | ASSOC_TRANSLATE);

  // WEAPON_bat_h / WEAPON_bat_v include full-body keyframes in ped.ifp; clear
  // lower-body bone sequences on this association so walk/run/sprint drives
  // legs.
  const int32_t numNodes = *reinterpret_cast<int32_t *>(assoc + 0x0C);
  auto *nodeArray = *reinterpret_cast<uint8_t **>(assoc + 0x10);
  if (!nodeArray || numNodes <= 0)
    return;

  for (int32_t i = 0; i < numNodes; ++i) {
    uint8_t *node = nodeArray + i * 0x1C;
    auto *seq = *reinterpret_cast<uint8_t **>(node + 0x14);
    if (!seq)
      continue;
    const char *boneName = reinterpret_cast<const char *>(seq + 0x08);
    if (IsLowerBodyBone(boneName)) {
      *reinterpret_cast<uintptr_t *>(node + 0x14) = 0;
    }
  }
}

// Hook at 0x004E674E (6 bytes: mov ebx, eax; or dword ptr [ebx+30h], 1)
// eax = CAnimBlendAssociation*, ebp = CPed*
__declspec(naked) void Hook_SetAttackBatAnim() {
  __asm {
    mov ebx, eax
    or dword ptr [ebx + 0x30], 1

    push eax
    push ecx
    push edx
    push ebx
    push ebp
    call OnBatAttackAnimCreated
    pop edx
    pop ecx
    pop eax
    ret
  }
}

// Hook at 0x004C74A8 (9 bytes: cmp dword ptr [ebx+224h], 10h; jne 0x004C74BF)
// ebx = CPed*, FPU stack must remain untouched
__declspec(naked) void Hook_AttackTurnRate() {
  __asm {
    cmp dword ptr [ebx + 0x224], 0x10
    jne skip_penalty

    push eax
    push ecx
    push edx
    push ebx
    call IsPlayerWithBat
    test al, al
    pop edx
    pop ecx
    pop eax
    jnz skip_penalty

    push ADDR_TURN_PENALTY_APPLY
    ret

  skip_penalty:
    push ADDR_TURN_PENALTY_SKIP
    ret
  }
}

void WriteCallNop(uintptr_t addr, void *target, size_t totalBytes) {
  DWORD oldProtect = 0;
  auto *dst = reinterpret_cast<uint8_t *>(addr);
  if (VirtualProtect(dst, totalBytes, PAGE_EXECUTE_READWRITE, &oldProtect)) {
    const int32_t rel =
        static_cast<int32_t>(reinterpret_cast<uintptr_t>(target) - (addr + 5));
    dst[0] = 0xE8;
    *reinterpret_cast<int32_t *>(dst + 1) = rel;
    for (size_t i = 5; i < totalBytes; ++i)
      dst[i] = 0x90;
    VirtualProtect(dst, totalBytes, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), dst, totalBytes);
  }
}

void WriteJumpNop(uintptr_t addr, void *target, size_t totalBytes) {
  DWORD oldProtect = 0;
  auto *dst = reinterpret_cast<uint8_t *>(addr);
  if (VirtualProtect(dst, totalBytes, PAGE_EXECUTE_READWRITE, &oldProtect)) {
    const int32_t rel =
        static_cast<int32_t>(reinterpret_cast<uintptr_t>(target) - (addr + 5));
    dst[0] = 0xE9;
    *reinterpret_cast<int32_t *>(dst + 1) = rel;
    for (size_t i = 5; i < totalBytes; ++i)
      dst[i] = 0x90;
    VirtualProtect(dst, totalBytes, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), dst, totalBytes);
  }
}

void InstallHooks() {
  WriteCallNop(ADDR_HOOK_SET_ATTACK_BAT,
               reinterpret_cast<void *>(&Hook_SetAttackBatAnim), 6);
  WriteJumpNop(ADDR_HOOK_ATTACK_TURN_RATE,
               reinterpret_cast<void *>(&Hook_AttackTurnRate), 9);
}

} // namespace

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID /*lpReserved*/) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(hModule);
    InstallHooks();
  }
  return TRUE;
}
