# CLAUDE_EN.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

# EWE — Easy Weaponry Enchanter

Unreal Engine 5 C++ Plugin.
A GAS-based enchant system that assigns **Trigger → Condition → Function** combinations to weapons.
Not a "skill authoring tool" — it is an **event combinator**.

---

## 🛠️ Skill Usage Directive

**Always use the `/unreal-engine` skill when writing, modifying, or reviewing C++ code.**

This project is built on UE5 C++ + GAS. Follow this order for code tasks:

```
1. Load /unreal-engine skill (Pre-Flight Discovery, reference files)
2. Check EWE-specific rules in this CLAUDE.md
3. Read the detailed document for the task from the Document Map
```

Areas covered by the skill (not repeated in CLAUDE.md):
- UE5 C++ best practices (GC, reflection, performance)
- General GAS patterns (ASC init, AttributeSet creation, GE stacking, GA lifecycle, AbilityTask, networking)
- Enhanced Input binding
- Plugin development (Runtime/Editor module split, Build.cs)
- Epic standard naming conventions (U/A/F/E/I prefixes)

---

## 🌐 Language Setting

Follows the language setting in global config (`~/.claude/settings.json`).
This project does not override the language at the project level.

---

## 📁 Project Structure

```
/Source/EWE/
│   ├── EWE.Build.cs                    ← Main game module dependencies
│   └── Character/
│       └── EWECharacter.h              ← ASC-owning character (core)
│
/Plugins/EasyWeaponryEnchanter/
│   └── Source/EasyWeaponryEnchanter/
│       ├── Ability/                    ← GA implementations (UEWEGA_*)
│       ├── AbilityTask/                ← AT implementations (UEWEAT_*)
│       └── Weapon/                     ← Weapon classes (UEWEWeapon*)
│
/Content/
    ├── Weapons/                        ← Weapon assets
    └── Effects/                        ← Effect assets
```

---

## 📚 Document Map

**Read the relevant document before reading or modifying code.**

| Task | Reference Document |
|------|--------------------|
| Module structure, GAS components, weapon swap flow | `Documents/MDs/architecture.md` |
| DataAsset, GA/AT classes, GA lifecycle, execution flow | `Documents/MDs/enchant-system.md` |
| Trigger types, Condition evaluation, combination rules | `Documents/MDs/trigger-condition.md` |
| Adding a new Function **(read first)** | `Documents/MDs/functions/overview.md` |
| Creation / Ejection / Rotation / Attach | `Documents/MDs/functions/basic.md` |
| TimeStop / Freeze / Teleport / Possess / Weather / StatusEffect | `Documents/MDs/functions/special.md` |
| Cooldown, Recording system, Scrolls, JSON Import/Export | `Documents/MDs/cooldown-recording.md` |
| RPC, server authority, replication policies | `Documents/MDs/network.md` |
| HUD, Inventory, Enchanter UI | `Documents/MDs/ui.md` |

---

## 🔧 Development Environment

| Item | Detail |
|------|--------|
| Engine version | Unreal Engine 5.5 |
| Required plugins | `GameplayAbilities`, `EnhancedInput` |
| Build target | `Win64`, `Editor`, `Development` |

Hot Reload: `Editor → Tools → Hot Reload`. Changes to Build.cs require a full rebuild.

### Document Relationship

- `CLAUDE.md`: Korean guide (primary)
- `CLAUDE_EN.md`: English guide (update together when syncing)

---

## ⚙️ Architecture Decisions (Do Not Change)

- **ASC and AttributeSet are owned by `AEWECharacter`.** Weapons only hold GA class lists. → `architecture.md`
- **Server Confirm required. No Client Prediction.** No client feedback before confirmation. → `network.md`
- **Module split:** `EWERuntime` / `EWEEditor` (strict `WITH_EDITOR` separation)

---

## 🔒 EWE Project Rules

> For general UE5/GAS code patterns, refer to the `/unreal-engine` skill references first.
> Below are **constraints specific to this project only**. These take priority over general GAS conventions when they conflict.

### Attribute Modification — GE Only
```
Direct attribute setters are strictly forbidden in this project.
While allowed in general GAS, EWE enforces all modifications through GE.

// ❌ AttributeSet->SetHealth(100.f);
// ✅ ASC->ApplyGameplayEffectToSelf(DamageGE, ...);
```

### GE Stacking — Aggregate Only
```
This project uses Aggregate stacking exclusively.
Override / Additive forbidden. Duplicate GE applications must operate as fully independent instances.
```

### GA / AbilityTask Role Separation
```
UEWEGA_* (GA) → High-level flow orchestration only
UEWEAT_* (AT) → Actual execution steps

Direct Actor spawning in GA is forbidden.
Always go through a dedicated AbilityTask.
```

### GA Termination — PrecedingEnchantEnd Integration
```
Only EndAbility(bWasCancelled=false) is recognized as a PrecedingEnchantEnd condition trigger.
CancelAbility is NOT recognized as a proper end.
Mixing these two paths breaks the enchant chain.
```

### Function Extension — IEnchantFunction Required
```
New Functions must implement the IEnchantFunction interface.
Integration into the pipeline via ExecuteFunction() override.
→ Read functions/overview.md first.
```

### Attribute Reference Failure Handling
```
When referencing an Attribute not present in the AttributeSet:
  → Log UE_LOG(LogEWE, Warning, ...) and ignore. Never crash.
```

### Code Comments
```
All code comments must be written in English.
// ❌ 체력을 감소시킨다
// ✅ Reduces the character's health by the given amount
```

---

## 📐 Core Classes

```
UEnchantDataAsset       Defines one enchant (Trigger + Conditions + Functions + CooldownGE)
UEWEWeaponData          Defines a weapon (slot count, default Attributes, enchant slot array)
AEWECharacter           Character owning ASC / AttributeSet
UEWEGA_Base             Base class for all GAs
UEWEAT_Base             Base class for all ATs (IEnchantFunction implementor)
IEnchantFunction        Function extension interface
FEnchantContext         Execution context (caster, parent weapon reference)
```

Full class map and file locations → `architecture.md`

---

## 🏷️ Naming Conventions

> Epic standard prefixes (U/A/F/E/I) — see `/unreal-engine` skill.
> Below are **EWE project-specific prefixes**.

| Category | Prefix | Example | Filename |
|----------|--------|---------|----------|
| GameplayAbility | `UEWEGA_` | `UEWEGA_Creation` | `EWEGA_Creation.h` |
| AbilityTask | `UEWEAT_` | `UEWEAT_SpawnActor` | `EWEAT_SpawnActor.h` |
| Weapon-related | `UEWEWeapon*` | `UEWEWeaponData` | `EWEWeaponData.h` |
| General UObject | `UEWE` | `UEWEAttributeSet` | `EWEAttributeSet.h` |
| AActor-derived | `AEWE` | `AEWECharacter` | `EWECharacter.h` |
| Structs | `FEWE` / `FEnchant*` | `FEnchantContext` | - |
| Log category | `LogEWE` | - | - |

**Filename rule**: Drop the `U` / `A` prefix from the class name. e.g. `UEWEGA_Creation` → `EWEGA_Creation.h`

---

## ⚠️ Open Items

```
[ ] Editor tool direction undecided
    → Details panel customization vs standalone Graph/Tree editor window
    → Focus on runtime features until decided

[ ] Object natural destruction edge cases
    → Without DestroyConditions, no destruction other than Lifetime (current policy)
    → Update functions/basic.md if new edge cases arise
```
