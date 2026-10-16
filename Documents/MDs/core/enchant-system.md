# Enchant System

> DataAsset 구조, GA/AT 클래스 설계, 슬롯 정책, GA 생명주기를 다룬다.
> 인챈트 관련 클래스를 작성하기 전에 반드시 읽을 것.
>
> **GA/AT 일반 패턴** (ActivateAbility 시그니처, AbilityTask 생성, ReadyForActivation 등)은
> `/unreal-engine` 스킬 → `references/gameplay_ability_system.md` 참조.

---

## 무기 GA 계층 (`UEWEGA_*`) — 현재 구현

```
UGameplayAbility
└── UEWEAbilityBase                 ← 무기 GA 공통 부모 (Ability/EWEAbilityBase.h)
    ├── UEWEGA_Creation             ← 액터 생성 (UEWEAT_SpawnActor 경유)
    ├── UEWEGA_FireProjectile       ← 투사체 발사
    └── UEWEGA_DefaultOnEquip       ← 장착 중 GE 적용
```

무기 GA 는 `UEWEWeaponData` 의 세 배열로 부여된다 (`AEWECharacter::EquipWeapon`, 전부 레벨 0).

| 배열 | InputID | 활성화 |
|------|---------|--------|
| `HoldAbilities` | `CHoldAbilityIndex` 부터 순번 | 공격 입력 Hold |
| `HitAbilities` | `CHitAbilityIndex` 부터 순번 | 공격 입력 Hit (애님 노티파이) |
| `OnEquipAbilities` | `INDEX_NONE` | 부여 즉시 자동 (아래). SourceObject = 무기 |

요소 타입은 모두 `TSubclassOf<UGameplayAbility>` 다. `OnEquipAbilities` 에는 `UEWEGA_DefaultOnEquip` 파생을 넣는다.

### UEWEGA_DefaultOnEquip

장착되는 동안 `OnEquipEffects` (`TArray<TSubclassOf<UGameplayEffect>>`) 를 캐릭터에 건다.
예: 무기별 MaxHealth 버프 Infinite GE.

| 항목 | 값 | 이유 |
|------|-----|------|
| InstancingPolicy | `InstancedPerActor` (생성자) | 적용한 Active GE 핸들을 인스턴스에 저장해야 해제 때 제거할 수 있다 |
| NetExecutionPolicy | `ServerOnly` (강제) | 클라이언트는 예측 키 없이 GE 를 적용할 수 없다. GE·Attribute 는 서버에서 복제된다 |
| CommitAbility | 호출하지 않음 | 비용·쿨타임이 없다 |

**ServerOnly 강제 — 3중 장치**

1. 생성자에서 기본값으로 설정
2. `IsDataValid` (에디터) — ServerOnly 가 아니면 저장 시 에러
3. `OnAvatarSet` (런타임) — ServerOnly 가 아니면 Warning 후 자동 활성화 생략

이것은 `network.md` "GA 실행 정책" 의 유일한 강제 예외다.

**흐름**

```
GiveAbility (EquipWeapon)
  → OnAvatarSet
      1. NetExecutionPolicy != ServerOnly → Warning, return
      2. !ActorInfo->IsNetAuthority()    → return
      3. Spec.IsActive()                 → return (재진입 방지)
      4. ASC->TryActivateAbility(Spec.Handle)
  → ActivateAbility
      OnEquipEffects 마다 MakeOutgoingGameplayEffectSpec → ASC->ApplyGameplayEffectSpecToSelf
      유효한 Active 핸들만 AppliedEffectHandles 에 저장
      EndAbility 하지 않음 — 장착 동안 활성 유지

ClearAllAbilities (다음 EquipWeapon)
  → OnRemoveAbility → EndAbility
      AppliedEffectHandles 전부 RemoveActiveGameplayEffect → Reset → Super::EndAbility
```

- `IsNetAuthority` 가드는 중복이 아니다. Spec 이 복제되므로 소유 클라이언트에서도 `OnAvatarSet` 이 불린다.
  가드가 없으면 클라이언트의 `TryActivateAbility` 가 서버로 원격 활성화 RPC 를 보낸다.
- GA 경로로 적용하므로 EffectContext 에 Ability 와 SourceObject(= 무기)가 자동 기록된다.
- GE 가 빠지면서 Max 가 바뀌면 `UEWEAttributeBase` 의 Max 변화 정책이 종속 Attribute 를 보정한다 (`architecture.md` "AttributeSet 설계").
- 제약: 현재 `EquipWeapon` 이 로컬 호출이므로 Standalone / 리슨 호스트에서만 동작한다.

---

## UEnchantDataAsset 구조

인챈트 1개 = DataAsset 1개 = GA 1개.

```cpp
UCLASS(BlueprintType)
class EWERUNTIME_API UEnchantDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    // 발동 조건
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enchant")
    FEnchantTrigger Trigger;

    // 다중 조건 (AND 결합)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enchant")
    TArray<FEnchantCondition> Conditions;

    // 이벤트 타입
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enchant")
    EEnchantEventType EventType = EEnchantEventType::OneShot;

    // 쿨타임 GE (없으면 쿨타임 없음)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enchant")
    TSubclassOf<UGameplayEffect> CooldownGE;

    // 실행할 Function 묶음
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enchant")
    TArray<FEnchantFunction> Functions;

    // 내부적으로 생성될 GA 클래스 (자동 지정, 수동 변경 불필요)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enchant|Internal")
    TSubclassOf<UEWEGameplayAbility> AbilityClass;
};
```

---

## UWeaponDataAsset 구조

```cpp
UCLASS(BlueprintType)
class EWERUNTIME_API UWeaponDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    // 최대 인챈트 슬롯 수 (제작자 임의 설정)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
    int32 MaxEnchantSlots = 4;

    // 슬롯 순서대로 인챈트 배열
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
    TArray<TObjectPtr<UEnchantDataAsset>> Enchants;

    // 이 무기 장착 시 적용될 Attribute 기본값
    // 키: Attribute 이름 (자유 입력), 값: 기본값
    // 존재하지 않는 Attribute 이름은 Warning 후 무시
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
    TMap<FName, float> DefaultAttributes;

    // 무기 액터 클래스
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
    TSubclassOf<AEWEWeaponActor> WeaponActorClass;
};
```

### DefaultAttributes 유효성 검증

```cpp
void UWeaponDataAsset::ValidateAttributes(const UEWEAttributeSet* AttributeSet) const
{
    for (const auto& Pair : DefaultAttributes)
    {
        if (!AttributeSet->HasAttribute(Pair.Key))
        {
            UE_LOG(LogEWE, Warning,
                TEXT("[WeaponDataAsset] Attribute '%s' not found in AttributeSet. Ignored."),
                *Pair.Key.ToString());
        }
    }
}
```

---

## UEWEGameplayAbility 설계

인챈트 1개가 런타임에 생성되는 GA 클래스.
EWE 고유 멤버만 기술한다.

```cpp
UCLASS()
class EWERUNTIME_API UEWEGameplayAbility : public UGameplayAbility
{
    GENERATED_BODY()

public:
    // 이 GA를 생성한 EnchantDataAsset 참조
    UPROPERTY()
    TObjectPtr<UEnchantDataAsset> SourceEnchant;

protected:
    // 활성화된 AbilityTask 목록
    TArray<TObjectPtr<UEWEAbilityTask_Base>> ActiveTasks;

    // 완료된 Task 수 카운터 (모두 완료 시 EndAbility 호출)
    int32 CompletedTaskCount = 0;

    void OnTaskCompleted();
};
```

### ActivateAbility 구현 패턴 (EWE 고유 흐름)

```cpp
void UEWEGameplayAbility::ActivateAbility(...)
{
    Super::ActivateAbility(...);

    if (!SourceEnchant) { EndAbility(...); return; }

    // 쿨타임 GE 적용
    if (SourceEnchant->CooldownGE)
    {
        ApplyGameplayEffectToOwner(..., SourceEnchant->CooldownGE, ...);
    }

    // 모든 Function(AbilityTask)을 동시에 실행
    // "PrecedingEnchantEnd" 조건이 있는 경우 해당 조건이 충족될 때까지 대기
    for (const FEnchantFunction& FuncData : SourceEnchant->Functions)
    {
        UEWEAbilityTask_Base* Task = CreateFunctionTask(FuncData);
        if (Task)
        {
            Task->OnCompleted.AddUObject(this, &UEWEGameplayAbility::OnTaskCompleted);
            Task->ReadyForActivation();
            ActiveTasks.Add(Task);
        }
    }

    if (ActiveTasks.IsEmpty())
    {
        EndAbility(...);
    }
}
```

---

## UEWEAbilityTask_Base 설계

Function 1개 = AbilityTask 1개.
EWE 고유 멤버만 기술한다.

```cpp
UCLASS(Abstract)
class EWERUNTIME_API UEWEAbilityTask_Base : public UAbilityTask
{
    GENERATED_BODY()

public:
    // Task 정상 완료 시 호출
    FSimpleDelegate OnCompleted;

    // FEnchantContext: 시전자, 부모 무기, 실행 컨텍스트
    UPROPERTY()
    FEnchantContext Context;

    // IEnchantFunction 인터페이스 구현
    virtual void ExecuteFunction() PURE_VIRTUAL(UEWEAbilityTask_Base::ExecuteFunction, );
};
```

---

## GA 생명주기 규칙 (변경 금지)

> **GA End vs Cancel 일반 의미**는
> `/unreal-engine` 스킬 → `references/gameplay_ability_system.md` → "Advanced GA Lifecycle" 참조.
> 아래는 **EWE 인챈트 체인에서의 추가 규칙**이다.

```
EndAbility(bWasCancelled = false)
  → 정상 종료
  → "PrecedingEnchantEnd" Condition의 트리거로 인정됨
  → 다음 인챈트 발동 가능

EndAbility(bWasCancelled = true) = CancelAbility
  → 강제 취소
  → "PrecedingEnchantEnd" Condition 트리거 불가
  → 다음 인챈트 대기 유지

GA가 완전히 종료되는 조건:
  → 모든 AbilityTask가 OnCompleted를 호출한 시점
  → 개별 Task 완료로 GA가 종료되지 않음
```

### 실수하기 쉬운 패턴

```cpp
// ❌ 잘못된 패턴: Task 하나 완료 시 바로 EndAbility
void UEWEGameplayAbility::OnTaskCompleted()
{
    EndAbility(...);  // 다른 Task들이 아직 실행 중일 수 있음
}

// ✅ 올바른 패턴: 모든 Task 완료 후 EndAbility
void UEWEGameplayAbility::OnTaskCompleted()
{
    CompletedTaskCount++;
    if (CompletedTaskCount >= ActiveTasks.Num())
    {
        EndAbility(..., false);  // bWasCancelled = false
    }
}
```

---

## EventType 별 동작

| EventType | 동작 | AbilityTask 완료 조건 |
|-----------|------|----------------------|
| `OneShot` | 1회 실행 후 종료 | Task 즉시 완료 |
| `Continuous` | 일정 주기 반복 | Duration 만료 시 완료 |
| `Sustained` | 입력 유지 중 지속 | 입력 해제 시 완료 |
| `Charged` | 입력 유지 후 발동 | 발동 완료 시 |

---

## 인챈트 슬롯 정책

- 슬롯 수는 `UWeaponDataAsset::MaxEnchantSlots`로 제한
- 실행 순서는 `Enchants` 배열 인덱스 순서로 고정
- 동일 DataAsset을 같은 무기에 중복 등록 가능 → 완전 독립 GA 인스턴스 생성
- 인챈트 슬롯 편집은 해당 무기가 **미장착** 상태일 때만 허용
- 퀵슬롯 교체는 **모든 GA가 End/Cancel 처리된 상태**일 때만 허용
