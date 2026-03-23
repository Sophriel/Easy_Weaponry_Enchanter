# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

# EWE — Easy Weaponry Enchanter

Unreal Engine 5 C++ Plugin.
A GAS-based enchant system that assigns **Trigger → Condition → Function** combinations to weapons.
This is not a skill authoring tool — it is an **event compositor**.

---

## 🌐 Language Settings

Follows the language setting from global settings (`~/.claude/settings.json`).
Does not override language at project level.

---

## 📁 Project Structure

```
/Source/                                        ← Main project C++ code
│   └── EWE/
│       └── Character/
│           └── EWECharacter.h                  ← Character owning the ASC (core)
│
/Plugins/EasyWeaponryEnchanter/
│   └── Source/EasyWeaponryEnchanter/
│       ├── Ability/                            ← GA implementations (UEWEGA_*)
│       ├── AbilityTask/                        ← AT implementations (UEWEAT_*)
│       └── Weapon/                             ← Weapon classes (UEWEWeapon*)
│           └── EWEWeaponData.h                 ← Weapon data structure
│
/Content/
    ├── Weapons/                                ← Weapon assets
    └── Effects/                                ← Effect assets
```

---

## 📚 Document Map

Always read the relevant document before starting any task.

| Task | Reference |
|------|-----------|
| Module structure, GAS components, plugin setup | `Documents/MDs/architecture.md` |
| DataAsset classes, GA/AT class design | `Documents/MDs/enchant-system.md` |
| Trigger / Condition implementation | `Documents/MDs/trigger-condition.md` |
| Adding a new Function (read this first, always) | `Documents/MDs/functions/overview.md` |
| Creation / Ejection / Rotation / Attach | `Documents/MDs/functions/basic.md` |
| TimeStop / Freeze / Teleport / Possess / Weather / StatusEffect | `Documents/MDs/functions/special.md` |
| Cooldown GE, Recording system, Scrolls | `Documents/MDs/cooldown-recording.md` |
| RPCs, server authority, replication policy | `Documents/MDs/network.md` |
| HUD, Inventory, Enchanter UI | `Documents/MDs/ui.md` |

---

## 🔧 Development Environment

| Item | Detail |
|------|--------|
| Engine Version | Unreal Engine 5.5 |
| Required Plugins | `GameplayAbilities`, `EnhancedInput` |
| Build Targets | `Win64`, `Editor`, `Development` |

### Build and Run

```
Editor → Play → Pi (Play in Editor)
```

### Hot Reload Workflow

```
1. Launch Editor
2. Modify C++ code
3. Editor → Tools → Hot Reload
4. Verify changes (check console logs)
```

If Build.cs is modified, Hot Reload will not apply — full rebuild required.

### Document Relationship

- `CLAUDE.md`: Korean guide (main)
- `CLAUDE_EN.md`: English guide (sync when needed)

---

## ⚙️ Core Architecture Decisions (Do Not Change)

### GAS Ownership
- **Both ASC and AttributeSet are owned by `AEWECharacter`** (`/Source/EWE/Character/EWECharacter.h`)
- Weapons (`UEWEWeaponData`) do not own an ASC — they hold a `TSubclassOf<UGameplayAbility>` list only
- On weapon equip → register the weapon's GAs to the character's ASC via `ASC->GiveAbility()`
- On weapon swap → Cancel all existing GAs, remove from ASC, then register new weapon's GAs

### Network
- **All GA activations execute only after Server Confirm** — no client-side prediction
- No client feedback before Server Confirm
- Spawned independent objects: Owner = Server; effect/damage calculations use the spawning player as reference

### Module Separation
```
EWERuntime   → Core runtime (GAs, GEs, DataAssets, Functions, etc.)
EWEEditor    → Editor-only features (strictly separated via WITH_EDITOR)
```

---

## 🔒 Hard Rules — Breaking These Will Break the System

### Attribute Modification
```
Attributes must only be modified through GameplayEffects.
Direct setter calls are strictly forbidden.

// ❌ Never do this
Health -= Damage;
AttributeSet->SetHealth(100.f);

// ✅ Correct approach
ASC->ApplyGameplayEffectToSelf(DamageGE, ...);
```

### GE Stacking
```
All GEs must use the Aggregate stacking policy.
Override and Additive are forbidden.
Duplicate GE applications must behave as fully independent instances.
```

### GA / AbilityTask Role Separation
```
GameplayAbility (UEWEGA_*)  → High-level flow definition only
AbilityTask     (UEWEAT_*)  → Actual execution steps

Never spawn actors directly inside a GA.
Actor spawning must go through a dedicated AbilityTask (e.g. UEWEAT_SpawnActor).
```

### GA Termination Distinction
```
EndAbility(bWasCancelled=false)    → Normal completion. Recognized as ended for PrecedingEnchantEnd conditions.
CancelAbility / bWasCancelled=true → Forced cancellation. NOT recognized as ended.
Never conflate the two code paths.
```

### Weapon Swap Processing Order
```
1. Receive weapon swap Confirm from server
2. CancelAbility on all active GAs + ASC->ClearAbility()
3. Reset AttributeSet to new weapon's default values
   - Retain current HP (clamp to new MaxHP if exceeded)
   - Exclude Position and velocity from reset
   - Retain all active GEs (including cooldowns) — do NOT reset
4. Register new weapon's GAs via ASC->GiveAbility()
```

### Attribute Reference
```
If a referenced Attribute does not exist in the Character's AttributeSet:
  → Print UE_LOG(LogEWE, Warning, ...) to console
  → Silently ignore the entry — no crash
```

### Function Extension
```
All new Functions must implement the IEnchantFunction interface.
Overriding ExecuteFunction() is sufficient to integrate into the existing pipeline.
Read Documents/MDs/functions/overview.md before implementing any new Function.
```

### Unreal Reflection Macros
```
Never remove or alter UCLASS, USTRUCT, UENUM, or GENERATED_BODY.
Never remove UPROPERTY or UFUNCTION specifiers.
Use TObjectPtr for owning pointers in UE5.
```

---

## 📐 Key Class Map

```
// ── Data ─────────────────────────────────────────────────────────
UEnchantDataAsset               Defines one enchant (Trigger + Conditions + EventType + Functions + CooldownGE)
UEWEWeaponData                  Defines a weapon (slot count, Attribute defaults, enchant slot array)
  └─ EWEWeaponData.h            /Plugins/.../Weapon/EWEWeaponData.h

// ── GAS Core ─────────────────────────────────────────────────────
AEWECharacter                   Character owning ASC and AttributeSet
  └─ EWECharacter.h             /Source/EWE/Character/EWECharacter.h
UEWEGA_Base                     Base class for all GAs
  └─ EWEAbilityBase.h           /Plugins/.../Ability/EWEAbilityBase.h
UEWEAT_Base                     Base class for all ATs (implements IEnchantFunction)
  └─ /Plugins/.../AbilityTask/

// ── Interfaces ───────────────────────────────────────────────────
IEWECharacterInterface          Character-weapon contract
IEWEAttackAnimationInterface    Attack animation contract
IEnchantFunction                Function extension interface (override ExecuteFunction)

// ── Structs ──────────────────────────────────────────────────────
FEnchantTrigger                 Trigger data struct
FEnchantCondition               Condition data struct
FEnchantFunction                Function parameter wrapper struct
FEnchantContext                 Execution context (caster reference, parent weapon reference)
```

---

## 🏷️ Naming Conventions

| Category | Prefix | Example | File Name |
|----------|--------|---------|-----------|
| GameplayAbility | `UEWEGA_` | `UEWEGA_Creation` | `EWEGA_Creation.h` |
| AbilityTask | `UEWEAT_` | `UEWEAT_SpawnActor` | `EWEAT_SpawnActor.h` |
| Weapon classes | `UEWEWeapon*` | `UEWEWeaponData` | `EWEWeaponData.h` |
| General UObject | `UEWE` | `UEWEAttributeSet` | `EWEAttributeSet.h` |
| AActor-derived | `AEWE` | `AEWECharacter` | `EWECharacter.h` |
| Structs | `FEWE` | `FEnchantContext` | - |
| Log category | `LogEWE` | - | - |

> **File naming rule**: Drop the leading `U` or `A` from the class name.
> Example: `UEWEGA_Creation` → `EWEGA_Creation.h`

---

## 🚦 Trigger Type Enum

```cpp
enum class EEnchantTriggerType : uint8
{
    OnSpawn,        // When the actor/object is spawned
    OnAttack,       // When a hit is registered on another actor (not on input)
    OnHit,          // When hit condition is met (regardless of damage)
    OnKill,         // When a target is killed
    OnDeath,        // When the caster dies
    OnInput,        // On key input (Enhanced Input / IMC-based)
    OnCombo,        // State condition AND input condition
    OnCollision,    // When a projectile collides with an actor/object
};
```

Default fire keys: **LMB and RMB only**. Additional keys require developer-side IMC modification.

---

## ⚡ Enchant Execution Flow

```
Player input
    │
    ▼
Client → Server RPC
    │
    ▼
Server: Trigger evaluation
    │
    ├─ Condition not met  → Reject
    ├─ CooldownGE active  → Reject (also blocked client-side, no feedback)
    │
    └─ Approved → Activate UEWEGA_*
                    │
                    └─ UEWEAT_* execute simultaneously (if no sequencing condition)
                                │
                                └─ All ATs complete → GA EndAbility
```

---

## 🔁 Concurrent / Sequential Execution Rules

```
Default: concurrent execution
Sequential: only when a "PrecedingEnchantEnd" Condition is set
            → fires at the EndAbility moment of the preceding enchant
            → CancelAbility does NOT trigger sequencing

The same rules apply to AbilityTasks within a single GA:
  Default concurrent. GA calls EndAbility only after all ATs complete.
```

---

## 🌐 Network Handling Rules

| Feature | Behavior |
|---------|----------|
| GA activation | Execute after Server Confirm |
| Independent object spawn | Server spawns; effect calc uses spawning player as reference |
| Teleport | Server validates destination → sends corrected position to client |
| Weather Change | Server request → broadcast to all clients, Last-Write-Wins |
| Time Stop | All same-tick requests processed; applied on next tick |
| Cooldown block | Blocked on both client and server |

---

## ⚠️ Pending Items (Confirm Before Implementing)

```
[ ] Editor tooling direction undecided
    → Details panel customization only, or a dedicated Graph/Tree editor window
    → Until decided, focus on runtime features only

[ ] Object natural destruction edge cases
    → Current policy: no auto-destroy if DestroyConditions is not set (Lifetime only)
    → Update Documents/MDs/functions/basic.md if additional cases are defined
```
