---
name: unreal-engine
description: >
  Comprehensive Unreal Engine C++ and Blueprint development assistant with deep project structure understanding.
  Use when helping with Unreal Engine projects, including: C++ gameplay programming, Blueprint development,
  input system configuration (Enhanced Input), Gameplay Ability System (GAS), project structure navigation,
  asset discovery and referencing, plugin development and integration (Runtime/Editor module split, Build.cs configuration),
  data-driven design (DataAsset, DataTable), API lookups for underdocumented features, and debugging.
  Triggers on any Unreal Engine development question, especially when working within a .uproject directory.
  Also use when the user is creating a plugin, designing data-driven gameplay systems, working with AbilityTasks,
  or building multiplayer ability systems with server authority.
---

# Unreal Engine Development Assistant

## When to Use

Use this skill when:
- Developing C++ code for Unreal Engine 5.x projects
- Writing Actors, Components, or UObject-derived classes
- Optimizing performance-critical code in Unreal Engine
- Debugging memory leaks or garbage collection issues
- Implementing Blueprint-exposed functionality
- Following Epic Games' coding standards and conventions
- Working with Unreal's reflection system (UCLASS, USTRUCT, UFUNCTION)
- Managing asset loading and soft references

Do not use this skill when:
- Working with Blueprint-only projects (no C++ code)
- Developing for Unreal Engine versions prior to 5.x
- Working on non-Unreal game engines
- The task is unrelated to Unreal Engine development

---

## Core Philosophy: Zero Assumptions

**CRITICAL**: Never make assumptions about the user's project. Every Unreal project is unique in structure, assets, and configuration. Always verify before suggesting code or assets.

### UCLASS / USTRUCT File Creation Policy

**NEVER autonomously create new UCLASS or USTRUCT header/source files.**

Unreal Engine generates these files with engine-managed boilerplate that cannot be reliably replicated by hand:
- `GENERATED_BODY()` macro and UHT-managed include order
- Module API export macros (`PROJECTNAME_API`)
- Correct `.generated.h` include placement
- Blueprint asset registration hooks

**Correct workflow**:
1. User creates the class from Unreal Editor (**Tools → New C++ Class** or Content Browser)
2. Engine generates the `.h` / `.cpp` pair with proper boilerplate
3. Claude works on the generated files — adding members, functions, logic

If a new class is needed, **tell the user to create it from the editor** and describe exactly which parent class to choose. Do not generate the file yourself.

### Helper Function Usage Policy

**Before calling any helper function, verify it is declared in the existing codebase.**

Do not assume helper functions exist (e.g. `GetUIManager()`, `GetInventory()`). Always check first using Pre-Flight Step 4. If the function is not found, use the direct Unreal API instead of inventing the call.

```cpp
// ✅ Confirmed declared in codebase — use it
UIManager = GetUIManager();

// ✅ Not found in codebase — use direct API instead
UIManager = GetLocalPlayer()->GetSubsystem<UMyLocalUISubsystem>();

// ❌ Assumed to exist without verification — causes linker error if missing
UIManager = GetUIManager();
```

---

## Pre-Flight Discovery Protocol

When a user asks for Unreal Engine help, ALWAYS execute this discovery sequence FIRST:

### 1. Locate the .uproject File

```bash
find . -maxdepth 2 -name "*.uproject" -type f
```

**If found**, extract:
- Engine version from `"EngineAssociation"` field
- Enabled plugins from `"Plugins"` array
- Module dependencies from `"Modules"` array

### 2. Map the Project Structure

**Standard Unreal project layout**:
```
ProjectRoot/
├── ProjectName.uproject
├── Source/
│   └── ProjectName/
│       ├── Public/            ← Header files (.h)
│       ├── Private/           ← Implementation files (.cpp)
│       └── ProjectName.Build.cs
├── Content/
│   ├── Blueprints/
│   ├── Input/                 ← Input Actions & Mapping Contexts
│   ├── Characters/
│   └── UI/
├── Config/
│   ├── DefaultEngine.ini
│   ├── DefaultInput.ini
│   └── DefaultGame.ini
└── Plugins/
```

### 3. Discover Sources and Content Assets

```bash
# C++ source files
view Source/*/Public
view Source/*/Private
find Source -name "*.h" -o -name "*.cpp" | head -20

# Engine version & plugin check
grep "EngineAssociation" *.uproject
grep -i "GameplayAbilities" *.uproject

# Content assets
find Content -type f -name "*.uasset" | head -50
find Content -type f -name "*IA_*" -o -name "*InputAction*"
find Content -type f -name "*IMC_*" -o -name "*InputMappingContext*"
find Content -type f -name "BP_*.uasset" | head -20

# Config
ls -la Config/
```

### 4. Understand Existing Code

**Before suggesting ANY code**:
- Read existing character/controller classes to understand patterns
- Check what components are already added
- Identify naming conventions (e.g., `IA_` prefix for Input Actions)
- Look for existing helper classes or base classes

```bash
find Source -name "*Character.h" -o -name "*Character.cpp"
```

---

## C++ Code Quality

> Always verify these principles before finalizing any generated code.
> → Full reference: `references/unreal_cpp_best_practice.md`

- **GC Safety**: All `UObject*` members must be wrapped in `UPROPERTY()`. Use `TStrongObjectPtr<>` for non-UObject owners; avoid `AddToRoot()` as it bypasses GC.
- **Reflection**: Use `BlueprintReadOnly` by default. Only use `BlueprintReadWrite` when BP truly needs write access.
- **Tick**: Disabled (`bCanEverTick = false`) by default. Prefer timers or events.
- **Casting**: Never `Cast<T>()` in hot loops. Cache in `BeginPlay` or `PostInitializeComponents`.
- **Soft References**: Use `TSoftClassPtr` / `TSoftObjectPtr` instead of hard `TSubclassOf` for large assets.

---

## Naming Conventions

Follow Epic Games' coding standard. Identify the project's existing convention first (Step 4).

| Prefix | Type | Example |
|--------|------|---------|
| `T` | Templates | `TArray`, `TMap` |
| `U` | UObject-derived | `UHealthComponent` |
| `A` | AActor-derived | `AMyGameMode` |
| `S` | Slate Widgets | `SMyWidget` |
| `F` | Structs | `FVector`, `FHitResult` |
| `E` | Enums | `EWeaponState` |
| `I` | Interfaces | `IInteractable` |
| `b` | Booleans | `bIsDead` |

---

## Input System (Enhanced Input)

> → Full reference: `references/enhanced_input.md`

**NEVER assume input action names.** Always run discovery (Step 3) first.

Key points:
- Bind in `SetupPlayerInputComponent` using `UEnhancedInputComponent`
- Add `UInputMappingContext` via `UEnhancedInputLocalPlayerSubsystem` in `BeginPlay`
- Declare `UInputAction*` properties as `BlueprintReadOnly`
- `.uasset` files are binary — use `find` to discover, not to read

---

## Gameplay Ability System (GAS)

> → Full reference: `references/gameplay_ability_system.md`

Check `.uproject` for `"GameplayAbilities"` plugin before proceeding.

Key points:
- Add `GameplayAbilities`, `GameplayTags`, `GameplayTasks` to `Build.cs`
- Place ASC on **Character** (single-player) or **PlayerState** (dedicated server)
- Grant abilities on server only (`HasAuthority()`)
- Use `FGameplayTag` for ability identification over string names
- Custom AbilityTask creation: see reference → "Custom AbilityTask" section
- GA lifecycle (End vs Cancel semantics, dynamic grant/revoke): see reference → "Advanced GA Lifecycle"
- Server-authoritative activation (no client prediction): see reference → "Network Patterns"

---

## Plugin Guidance

> → Full reference: `references/plugin_guidance.md`

- Always verify plugin is enabled in `.uproject` before writing plugin-dependent code
- For unknown or experimental plugins, search documentation and be transparent about uncertainty
- Check plugin stability status (`Stable` / `Beta` / `Experimental`) before use in production

---

## Plugin Development

> → Full reference: `references/plugin_development.md`

For creating new UE plugins (not just using existing ones). Key points:
- Runtime/Editor 모듈 분리 (`.uplugin` Modules 배열)
- `WITH_EDITOR` 매크로로 에디터 전용 코드 격리
- Build.cs에서 모듈 간 의존성 설정 (Public vs Private)
- 외부 프로젝트에서 플러그인 모듈을 의존으로 추가하는 방법

---

## Data-Driven Design

> → Full reference: `references/data_driven_design.md`

For DataAsset / DataTable based gameplay data architecture. Key points:
- `UDataAsset` vs `UPrimaryDataAsset` 선택 기준
- DataAsset으로 게임플레이 파라미터 정의 (무기, 스킬, 아이템 등)
- DataTable + CSV/JSON import 워크플로
- AssetManager를 통한 DataAsset 런타임 로딩

---

## Debugging

> → Full reference: `references/debugging.md`

- Use `UE_LOG` with custom log categories (`DEFINE_LOG_CATEGORY`) for contextual filtering
- Use `GEngine->AddOnScreenDebugMessage()` for quick in-editor feedback
- Use **Visual Logger** for AI and spatial debugging (`UE_VLOG_*` macros)
- Prefer `check()` / `ensure()` over silent failures

---

## API Knowledge Gaps

**When uncertain about API usage**:

1. Search Epic's documentation: `web_search: "Unreal Engine [ClassName] API [EngineVersion]"`
2. Search community resources: `web_search: "Unreal Engine [feature] example code C++"`
3. Check Epic Developer Community forums
4. Reference example projects: Lyra, Valley of the Ancient, ActionRPG

---

## Version-Specific Considerations

Always check `.uproject` for `"EngineAssociation"`:

| Version | Notes |
|---------|-------|
| `5.5+` | Mutable/Customization systems, new input features |
| `5.0–5.4` | Stable Enhanced Input, Experimental GAS improvements |
| `4.27` | Legacy input system; requires manual Enhanced Input setup |

When suggesting code, verify feature availability:
```
web_search: "Unreal Engine [feature] [version] availability"
```

---

## Common Pitfalls

> → Full reference: `references/common_pitfalls.md`

- **Never assume** asset names or paths — always discover with `find`
- Missing `UPROPERTY()` → silent GC collection
- `Cast<T>()` in Tick → severe performance cost
- Hard `TSubclassOf` references → forced asset loading at startup
- `BlueprintReadWrite` on internal state → fragile BP/C++ boundary

---

## Workflow Decision Tree

```
User asks for Unreal help
    │
    ├─> Find .uproject ─> Extract version & plugins
    │
    ├─> Map project structure ─> View Source/ and Content/
    │
    ├─> Identify question type:
    │   ├─> Input system?       ─> references/enhanced_input.md
    │   ├─> GAS-related?        ─> references/gameplay_ability_system.md
    │   ├─> Plugin development? ─> references/plugin_development.md
    │   ├─> Plugin usage?       ─> references/plugin_guidance.md
    │   ├─> DataAsset/DataTable? ─> references/data_driven_design.md
    │   ├─> Debugging?          ─> references/debugging.md
    │   ├─> C++ quality?        ─> references/unreal_cpp_best_practice.md
    │   └─> General C++?        ─> Read existing classes for patterns
    │
    ├─> Provide solution with:
    │   ├─> Verified asset references
    │   ├─> Version-appropriate code
    │   └─> Project-specific patterns
    │
    └─> If uncertain about API/plugin ─> Search documentation
```

---

## Checklists

### Pre-Code Checklist
Before providing ANY code suggestion:

- [ ] Found and read .uproject file
- [ ] Identified engine version
- [ ] Mapped Source/ directory structure
- [ ] Discovered Content/ assets (especially for input/blueprints)
- [ ] Read existing class files for patterns
- [ ] Verified asset paths and names
- [ ] Checked plugin availability
- [ ] Searched documentation for uncertain APIs
- [ ] Used project-specific naming conventions

### Pre-PR / Code Review Checklist
Before finalizing any C++ code:

- [ ] Does this Actor need to Tick? Can it be replaced with a Timer or event?
- [ ] Are all `UObject*` members wrapped in `UPROPERTY`?
- [ ] Are hard references (`TSubclassOf`) causing load chains? Can they be `TSoftClassPtr`?
- [ ] Are `Cast<T>()` calls cached in `BeginPlay`, not repeated in `Tick`?
- [ ] Are Blueprint-exposed properties using the appropriate access specifier (`ReadOnly` vs `ReadWrite`)?
- [ ] Did you clean up delegates in `EndPlay`?

---

## Reference Files

Load the appropriate reference file when deeper detail is needed:

| File | Load When |
|------|-----------|
| `references/unreal_cpp_best_practice.md` | GC, reflection, performance, naming, common patterns |
| `references/enhanced_input.md` | Enhanced Input binding, Input Actions, Mapping Contexts |
| `references/gameplay_ability_system.md` | GAS setup, abilities, attributes, effects, AbilityTasks, network patterns |
| `references/plugin_guidance.md` | Unknown or experimental plugins (사용 측) |
| `references/plugin_development.md` | Plugin 생성, 모듈 분리, Build.cs, WITH_EDITOR (개발 측) |
| `references/data_driven_design.md` | DataAsset, DataTable, AssetManager, 데이터 주도 설계 |
| `references/debugging.md` | UE_LOG, Visual Logger, crash/assert patterns |
| `references/common_pitfalls.md` | Troubleshooting build errors, GC bugs, performance issues |
