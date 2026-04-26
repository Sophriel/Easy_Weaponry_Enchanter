# Unreal C++ Best Practices

## 1. UObject & Garbage Collection

- **Always** use `UPROPERTY()` for both `UObject*` and `TObjectPtr<>` member variables to ensure they are tracked by the Garbage Collector (GC). `TObjectPtr` does not provide automatic GC tracking on its own — the `UPROPERTY()` macro is what exposes the member to the reflection system and GC.
- Understand the `IsValid()` check vs `nullptr`. `IsValid()` handles pending kill state safely.

```cpp
// ✅ GC-safe — raw pointer with UPROPERTY
UPROPERTY()
UHealthComponent* HealthComp;

// ✅ GC-safe — TObjectPtr with UPROPERTY
UPROPERTY()
TObjectPtr<UHealthComponent> HealthComp;

// ❌ NOT GC-safe — raw pointer without UPROPERTY (silently collected)
UHealthComponent* HealthComp;

// ❌ NOT GC-safe — TObjectPtr without UPROPERTY
// TObjectPtr alone does NOT register with the GC
TObjectPtr<UHealthComponent> HealthComp;
```

- **Prefer `TObjectPtr<>` for UCLASS/USTRUCT member variables. Use raw pointers for function parameters and return values.** `TObjectPtr` provides access tracking and editor-time integrity checks for members (the UE5-recommended direction). Function signatures should use raw pointers — `TObjectPtr` is not supported in reflection-exposed signatures (`UFUNCTION` parameters, return values, delegate parameters). Local variables can use either; raw pointers are typical, but `TObjectPtr` is acceptable.

```cpp
class AMyCharacter : public ACharacter
{
    // ✅ Member variable — use TObjectPtr
    UPROPERTY()
    TObjectPtr<UInventoryComponent> Inventory;

    // ✅ Function parameter — use raw pointer
    void EquipWeapon(UWeaponData* WeaponData);

    // ✅ Function return value — use raw pointer
    UWeaponData* GetEquippedWeapon() const;
};

void AMyCharacter::EquipWeapon(UWeaponData* WeaponData)
{
    // ✅ Local variable — raw pointer is typical
    UInventoryComponent* InvComp = Inventory.Get();
}
```

- **Use `TStrongObjectPtr<>` or `FGCObject` for cross-thread access and non-UObject owners.** `UPROPERTY()` only protects members of UCLASS/USTRUCT types on the game thread. Outside these contexts, both raw pointers and `TObjectPtr` can dangle when GC runs. Avoid `AddToRoot()` for this purpose — it bypasses Unreal's garbage collection entirely and should be used sparingly.

  Two scenarios that require special handling:

  **Non-UObject native C++ classes holding UObject references** — `UPROPERTY()` has no effect because the class has no reflection. Use `TStrongObjectPtr<>` for simple ownership, or implement `FGCObject` with `AddReferencedObjects()` for more control.

  **Cross-thread access (FRunnable, async tasks, etc.)** — `TObjectPtr` access trackers assume the game thread and may assert in debug builds. Raw pointers can dangle if GC runs on the game thread while a worker thread holds the pointer. Use `TStrongObjectPtr<>` to maintain a strong reference safely across threads.

```cpp
// ❌ Native class holding raw pointer — GC will collect WeaponData
class FWeaponInventory  // not a UCLASS
{
    UWeaponData* WeaponData;  // dangles after GC
};

// ✅ TStrongObjectPtr keeps WeaponData alive
class FWeaponInventory
{
    TStrongObjectPtr<UWeaponData> WeaponData;
};

// ❌ FRunnable holding raw pointer — race with GC
class FAnalysisTask : public FRunnable
{
    UWeaponData* WeaponData;  // unsafe across threads
};

// ✅ TStrongObjectPtr is thread-safe for keeping the reference alive
class FAnalysisTask : public FRunnable
{
    TStrongObjectPtr<UWeaponData> WeaponData;
};
```

---

## 2. Unreal Reflection System

- Use `UCLASS()`, `USTRUCT()`, `UENUM()`, `UFUNCTION()` to expose types to the reflection system and Blueprints.
- Minimize `BlueprintReadWrite` when possible; prefer `BlueprintReadOnly` for state that shouldn't be overwritten by UI/Level Blueprint logic.
- **Never use `uint32` as a parameter or return type in `UFUNCTION`**. Blueprint does not support unsigned integer types. Use `int32` instead.
- **Never use `TObjectPtr<>` in `UFUNCTION` parameters, return values, or `DECLARE_DYNAMIC_MULTICAST_DELEGATE` parameter lists**. The reflection system does not support `TObjectPtr` in these contexts. Use raw pointers (`UObject*`) instead. `TObjectPtr` is only valid as a class member variable.

```cpp
// ❌ uint32 is not supported by Blueprint
UFUNCTION(BlueprintCallable)
UWeaponData* GetWeapon(uint32 TargetWeapon);

// ✅ Use int32 instead
UFUNCTION(BlueprintCallable)
UWeaponData* GetWeapon(int32 TargetWeapon);

// ❌ TObjectPtr not allowed in UFUNCTION return value or Delegate params
UFUNCTION(BlueprintCallable)
TArray<TObjectPtr<UWeaponData>> GetAllWeapons() const;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponsSynced,
    const TArray<TObjectPtr<UWeaponData>>&, Weapons, int32, NewCount);

// ✅ Use raw pointer instead
UFUNCTION(BlueprintCallable)
TArray<UWeaponData*> GetAllWeapons() const;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponsSynced,
    const TArray<UWeaponData*>&, Weapons, int32, NewCount);
```

- **When passing `TSoftObjectPtr` collections as function parameters, convert to raw pointer collections.** `TSoftObjectPtr` is for storage/reference purposes. Function interfaces typically deal with already-loaded assets, so the caller should guarantee loading and pass raw pointers.

```cpp
// Member variable: stored as soft reference
UPROPERTY()
TArray<TSoftObjectPtr<UWeaponData>> Weapons;

// ❌ Passing TSoftObjectPtr collection directly as function parameter
void SetWeapons(const TArray<TSoftObjectPtr<UWeaponData>>& InWeapons);

// ✅ Pass as raw pointer collection — caller guarantees loading
void SetWeapons(const TArray<UWeaponData*>& InWeapons);
```

This way the function only deals with valid pointers without worrying about load state, and the function does not depend on the caller's storage strategy (hard/soft).

- **Never create UCLASS or USTRUCT files autonomously.** Engine-generated files include `GENERATED_BODY()`, module API macros (`PROJECTNAME_API`), UHT-managed include order, and `.generated.h` placement — boilerplate that is difficult to replicate correctly by hand. Always instruct the user to create the class from the editor (Tools → New C++ Class), then work on the generated files.

### Minimize Header Includes — Prefer Forward Declarations

When a header file only references another class by pointer or reference, use a forward declaration instead of `#include`.
Add `#include` only in the cpp file where you actually access that class's members.

```cpp
// ❌ MyWeaponComponent.h — unnecessary #include
#include "EWEWeaponData.h"
#include "EWEPlayerController.h"

UPROPERTY()
TObjectPtr<UEWEWeaponData> WeaponData;

void EquipWeapon(AEWEPlayerController* PC);
```

```cpp
// ✅ MyWeaponComponent.h — minimize dependencies with forward declarations
class UEWEWeaponData;
class AEWEPlayerController;

UPROPERTY()
TObjectPtr<UEWEWeaponData> WeaponData;

void EquipWeapon(AEWEPlayerController* PC);
```

```cpp
// ✅ MyWeaponComponent.cpp — #include at actual usage site
#include "MyWeaponComponent.h"
#include "EWEWeaponData.h"        // Access WeaponData->GetDamage()
#include "EWEPlayerController.h"  // Call PC->SetSlot()
```

**Cases where `#include` is required in headers** (forward declaration not possible):
- **Inheriting** from the class (`class AMyChar : public ACharacter`)
- **Holding the type by value** (`FGameplayTagContainer Tags;` — inline member, not a pointer)
- Accessing the class's members in **inline functions**
- **`.generated.h` with `GENERATED_BODY()`** — must always be included in headers

### #include Paths — Use Full Path Relative to Module Root

When writing `#include`, do not use the filename alone — always specify the **full relative path from the module's `Public/` or `Private/` directory**. UE's build system only registers the module root in the include path, so omitting subdirectory structure causes a compile error as the file cannot be found.

```cpp
// ❌ Filename only — compile error (file not found)
#include "EWEAttributeBase.h"

// ✅ Full path relative to module root
#include "EasyWeaponryEnchanter/Attribute/EWEAttributeBase.h"
```

To verify the path, search for the header location in the `Source/` directory:

```bash
find Source -name "EWEAttributeBase.h"
# → Source/EasyWeaponryEnchanter/Public/EasyWeaponryEnchanter/Attribute/EWEAttributeBase.h
#    Path after module root (Public/): EasyWeaponryEnchanter/Attribute/EWEAttributeBase.h
```

### UHT and Forward Declarations in Delegate Macros

`DECLARE_DYNAMIC_MULTICAST_DELEGATE_*` macros work correctly with forward declarations **when the parameter type is a `UObject`-derived pointer**. UHT only needs the type name and pointer notation — a full `#include` is not required in this case.

```cpp
// ✅ Forward declaration is sufficient for UObject* pointer params
class UEWEWeaponData;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponAcquiredEvent, UEWEWeaponData*, Weapon);
```

`#include` is required when:
- The parameter is a **value type** (not a pointer), e.g. `FMyStruct`
- The code **accesses members** of the type in the same translation unit

---

## 3. Performance: Tick, Casting, Structs

- **Tick**: Disable Ticking (`bCanEverTick = false`) by default. Only enable it if absolutely necessary. Prefer timers (`GetWorldTimerManager()`) or event-driven logic.
- **Casting**: Avoid `Cast<T>()` in hot loops. Cache references in `BeginPlay` or `PostInitializeComponents`.
- **Structs vs Classes**: Use `F` structs for data-heavy, non-UObject types to reduce overhead.

---

## 4. Naming Conventions

Follow Epic Games' coding standard. Always identify the existing project convention first (Step 4 of Pre-Flight), then conform to it.

| Prefix | Type | Example |
|--------|------|---------|
| `T` | Templates | `TArray`, `TMap` |
| `U` | UObject-derived | `UCharacterMovementComponent` |
| `A` | AActor-derived | `AMyGameMode` |
| `S` | Slate Widgets | `SMyWidget` |
| `F` | Structs | `FVector`, `FHitResult` |
| `E` | Enums | `EWeaponState` |
| `I` | Interfaces | `IInteractable` |
| `b` | Booleans | `bIsDead`, `bCanEverTick` |

---

## 5. Common Patterns

### Robust Component Lookup

Avoid `GetComponentByClass` in `Tick`. Do it in `PostInitializeComponents` or `BeginPlay`.

```cpp
void AMyCharacter::PostInitializeComponents() {
    Super::PostInitializeComponents();
    HealthComp = FindComponentByClass<UHealthComponent>();
    check(HealthComp); // Fail hard in dev if missing
}
```

### Interface Implementation

Use interfaces to decouple systems (e.g., Interaction system).

```cpp
if (TargetActor->Implements<UInteractable>()) {
    IInteractable::Execute_OnInteract(TargetActor, this);
}
```

### Async Loading (Soft References)

Avoid hard references (`TSubclassOf<AActor>`) for large assets, which force load orders. Use `TSoftClassPtr` or `TSoftObjectPtr`.

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite)
TSoftClassPtr<AWeapon> WeaponClassToLoad;

void AMyCharacter::Equip() {
    if (WeaponClassToLoad.IsPending()) {
        WeaponClassToLoad.LoadSynchronous(); // Or use StreamableManager for async
    }
}
```

### ULocalPlayerSubsystem Access

Before calling a helper function to access a subsystem, verify it is declared in the codebase. If not found, use the direct API via `GetLocalPlayer()`.

```cpp
// ✅ Helper function confirmed in codebase — prefer it for readability
UIManager = GetUIManager();

// ✅ No helper found — access directly via LocalPlayer
UIManager = GetLocalPlayer()->GetSubsystem<UMyLocalUISubsystem>();

// ❌ Calling an unverified helper — linker error if not implemented
UIManager = GetUIManager(); // assumed, not confirmed
```

> `ULocalPlayerSubsystem` is tied to a specific local player instance.
> Always retrieve it through `GetLocalPlayer()`, not through global or world subsystem accessors.

### Flatten Cast Chains with Early Return

In UE code, chained Controller casts, ASC access, and component lookups easily lead to 3-4 levels of nested ifs.
Use the early return pattern to filter out failure conditions first, minimizing the depth of core logic.

```cpp
// ❌ Nested ifs — readability degrades with increasing depth
APlayerController* PC = GetOwningPlayer();
if (PC)
{
    AMyPlayerController* MyPC = Cast<AMyPlayerController>(PC);
    if (MyPC)
    {
        UAbilitySystemComponent* ASC = MyPC->GetAbilitySystemComponent();
        if (ASC)
        {
            ASC->TryActivateAbilitiesByTag(TagContainer);
        }
    }
}

// ✅ Early return — filter failures first, core logic at minimal depth
AMyPlayerController* MyPC = Cast<AMyPlayerController>(GetOwningPlayer());
if (!MyPC)
{
    return;
}

UAbilitySystemComponent* ASC = MyPC->GetAbilitySystemComponent();
if (!ASC)
{
    return;
}

ASC->TryActivateAbilitiesByTag(TagContainer);
```

Apply the same pattern for functions with return values:

```cpp
// ✅ When a return value is needed
UMyComponent* GetMyComponent(AActor* TargetActor)
{
    if (!TargetActor)
    {
        return nullptr;
    }

    return TargetActor->FindComponentByClass<UMyComponent>();
}
```
