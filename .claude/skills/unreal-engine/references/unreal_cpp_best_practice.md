# Unreal C++ Best Practices

## 1. UObject & Garbage Collection

- **Always** use `UPROPERTY()` for `UObject*` member variables to ensure they are tracked by the Garbage Collector (GC).
- Use `TStrongObjectPtr<>` for non-UObject owners instead of manually calling `AddToRoot()`. `AddToRoot()` should be used sparingly because it bypasses Unreal's garbage collection.
- Understand the `IsValid()` check vs `nullptr`. `IsValid()` handles pending kill state safely.

```cpp
// ✅ GC-safe member variable
UPROPERTY()
UHealthComponent* HealthComp;

// ❌ NOT GC-safe — can be silently collected
UHealthComponent* HealthComp;
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

- **`TSoftObjectPtr` 컬렉션을 함수 파라미터로 넘길 때는 raw pointer 컬렉션으로 변환하여 전달한다.** `TSoftObjectPtr`는 저장/참조 용도이고, 함수 인터페이스에서는 이미 로드된 에셋을 다루는 것이 일반적이므로 호출 측에서 로드를 보장한 뒤 raw pointer로 넘기는 것이 바람직하다.

```cpp
// 멤버 변수: 소프트 레퍼런스로 보유
UPROPERTY()
TArray<TSoftObjectPtr<UWeaponData>> Weapons;

// ❌ 함수 파라미터에 TSoftObjectPtr 컬렉션을 그대로 전달
void SetWeapons(const TArray<TSoftObjectPtr<UWeaponData>>& InWeapons);

// ✅ raw pointer 컬렉션으로 전달 — 호출 측에서 로드를 보장
void SetWeapons(const TArray<UWeaponData*>& InWeapons);
```

이렇게 하면 함수 내부에서 로드 여부를 신경 쓸 필요 없이 유효한 포인터만 다루면 되고, 호출 측의 저장 방식(하드/소프트)에 함수가 의존하지 않게 된다.

- **Never create UCLASS or USTRUCT files autonomously.** Engine-generated files include `GENERATED_BODY()`, module API macros (`PROJECTNAME_API`), UHT-managed include order, and `.generated.h` placement — boilerplate that is difficult to replicate correctly by hand. Always instruct the user to create the class from the editor (Tools → New C++ Class), then work on the generated files.

### Header Include 최소화 — 전방선언 우선 원칙

헤더 파일에서 다른 클래스를 포인터나 레퍼런스로만 참조하는 경우, `#include` 대신 전방선언을 사용한다.
`#include`는 해당 클래스의 멤버에 실제로 접근하는 cpp 파일에서만 추가한다.

```cpp
// ❌ MyWeaponComponent.h — 불필요한 #include
#include "EWEWeaponData.h"
#include "EWEPlayerController.h"

UPROPERTY()
TObjectPtr<UEWEWeaponData> WeaponData;

void EquipWeapon(AEWEPlayerController* PC);
```

```cpp
// ✅ MyWeaponComponent.h — 전방선언으로 의존 최소화
class UEWEWeaponData;
class AEWEPlayerController;

UPROPERTY()
TObjectPtr<UEWEWeaponData> WeaponData;

void EquipWeapon(AEWEPlayerController* PC);
```

```cpp
// ✅ MyWeaponComponent.cpp — 실제 사용처에서 #include
#include "MyWeaponComponent.h"
#include "EWEWeaponData.h"        // WeaponData->GetDamage() 접근
#include "EWEPlayerController.h"  // PC->SetSlot() 호출
```

**`#include`가 헤더에 필요한 경우** (전방선언 불가):
- 해당 클래스를 **상속**할 때 (`class AMyChar : public ACharacter`)
- 해당 타입을 **값으로 보유**할 때 (`FGameplayTagContainer Tags;` — 포인터가 아닌 인라인 멤버)
- **인라인 함수**에서 해당 클래스의 멤버에 접근할 때
- **`GENERATED_BODY()`가 있는 `.generated.h`** — 항상 헤더에 include 필수

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

### Early Return으로 Cast 연쇄 평탄화

UE 코드에서 Controller 캐스팅, ASC 접근, 컴포넌트 조회가 연쇄되면 중첩 if가 3~4단으로 깊어지기 쉽다.
실패 조건을 먼저 걸러내는 early return 패턴으로 핵심 로직의 뎁스를 최소화한다.

```cpp
// ❌ 중첩 if — 뎁스가 깊어질수록 가독성 저하
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

// ✅ Early return — 실패를 먼저 걸러내고 핵심 로직은 최소 뎁스
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

반환값이 필요한 함수에서도 동일하게 적용:

```cpp
// ✅ 반환값이 있는 경우
UMyComponent* GetMyComponent(AActor* TargetActor)
{
    if (!TargetActor)
    {
        return nullptr;
    }

    return TargetActor->FindComponentByClass<UMyComponent>();
}
```
