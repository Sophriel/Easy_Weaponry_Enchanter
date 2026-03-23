# Function — Basic

> Creation, Ejection, Rotation, Attach 의 파라미터와 구현 규칙을 다룬다.
> 새 Function 추가 전 `docs/functions/overview.md` 먼저 읽을 것.

---

## Creation (오브젝트 생성)

### 파라미터

| 파라미터 | 타입 | 설명 |
|----------|------|------|
| `PositionBase` | `EPositionBase` | Self / Target 기준 |
| `Offset` | `FVector` | 기준 위치로부터 오프셋 |
| `SpawnTiming` | `ESpawnTiming` | 즉시 / 딜레이 |
| `SpawnActorClass` | `TSubclassOf<AActor>` | 생성할 액터 클래스 |
| `Lifetime` | `float` | 생존 시간 (0 = 조건 소멸만, -1 = 영구) |
| `DestroyConditions[]` | `TArray<FEnchantCondition>` | 소멸 조건 목록 |
| `OnDestroyEnchant` | `FEnchantEventRef` | 소멸 시 발동할 인챈트 이벤트 |
| `FacingDirection` | `EFacingDirection` | 생성 방향 |
| `SpawnEffect` | `UParticleSystem*` | 생성 이펙트 |
| `bUsePooling` | `bool` | 오브젝트 풀링 여부 |
| `bReplicates` | `bool` | 네트워크 복제 여부 |

### 소멸 정책

```
DestroyConditions가 비어있는 경우:
  → Lifetime이 -1이면 영구 존재
  → Lifetime이 0이면 조건 없이 소멸 안 됨 (버그 주의: 사실상 영구)
  → Lifetime > 0이면 해당 시간 후 소멸

DestroyConditions가 있는 경우:
  → 조건 충족 시 즉시 소멸
  → Lifetime과 OR 관계 (먼저 충족되는 쪽으로 소멸)
```

### 소멸 시 연쇄 인챈트

```cpp
void UEWESpawnedActor::OnDestroyed()
{
    if (OnDestroyEnchantEvent.IsValid())
    {
        // 소멸 위치를 기준으로 새 인챈트 이벤트 발동
        UEWEEnchantEventManager::FireEvent(OnDestroyEnchantEvent, DestroyContext);
    }
}
```

소멸 → 파편 생성, 폭발 → 상태 이상 전파 등 체인 구현 가능.

### 생성 오브젝트 네트워크 처리

```
bReplicates = true:
  → 서버에서 SpawnActor, Replicate to clients
  → Owner = 서버
  → 영향 계산 기준 = 생성한 플레이어 (Context.Who)

bReplicates = false:
  → 서버에서만 존재하는 오브젝트 (서버 사이드 로직 전용)
```

---

## Ejection / Fire (투사체 사출)

### 파라미터

| 파라미터 | 타입 | 설명 |
|----------|------|------|
| `ProjectileClass` | `TSubclassOf<AProjectile>` | 투사체 클래스 |
| `Speed` | `float` | 발사 속도 |
| `bHoming` | `bool` | 유도 여부 |
| `OnCollisionGE` | `TSubclassOf<UGameplayEffect>` | 충돌 시 대상에게 적용할 GE |
| `CollisionEnchantEvent` | `FEnchantEventRef` | 충돌 시 발동할 인챈트 이벤트 (선택) |
| `FireCount` | `int32` | 발사 횟수 |
| `bPenetrate` | `bool` | 관통 여부 |

### 충돌 처리

```cpp
void AEWEProjectile::OnHit(
    UPrimitiveComponent* HitComp,
    AActor* OtherActor, ...)
{
    if (!HasAuthority()) return;  // 서버에서만 처리

    // OnCollisionGE 적용
    if (OnCollisionGE && OtherActor)
    {
        UAbilitySystemComponent* TargetASC =
            UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
        if (TargetASC)
        {
            FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
            Context.AddInstigator(OwnerCharacter, this);
            TargetASC->ApplyGameplayEffectToSelf(
                OnCollisionGE.GetDefaultObject(), 1.f, Context);
        }
    }

    // CollisionEnchantEvent 발동
    if (CollisionEnchantEvent.IsValid())
    {
        UEWEEnchantEventManager::FireEvent(CollisionEnchantEvent, CollisionContext);
    }

    // 관통 아닐 경우 소멸
    if (!bPenetrate)
    {
        Destroy();
    }
}
```

### OnCollision Trigger 연계

```
Ejection의 CollisionEnchantEvent ↔ OnCollision Trigger

투사체가 충돌 시:
  1. OnCollisionGE → 충돌 대상에게 즉시 적용
  2. CollisionEnchantEvent → 연결된 인챈트 이벤트 발동
     → 이 이벤트의 Trigger가 OnCollision이면 해당 인챈트 GA 실행
```

---

## Rotation (회전)

### 파라미터

| 파라미터 | 타입 | 설명 |
|----------|------|------|
| `RotationAxis` | `EAxis` | 기준 축 (X / Y / Z) |
| `AngularSpeed` | `float` | 각속도 (deg/s) |
| `bAttachToTarget` | `bool` | 대상 액터에 종속 여부 |

### 구현 패턴

```cpp
void UEWEAbilityTask_Rotation::ExecuteFunction(const FEnchantContext& Context)
{
    AActor* Target = GetTargetActor(Context);
    if (!Target) { NotifyCompleted(); return; }

    // Tick에서 회전 적용
    // bAttachToTarget이면 Target의 로컬 축 기준으로 회전
    // 아니면 월드 축 기준
    RotationTimerHandle = GetWorld()->GetTimerManager().SetTimer(...);
}
```

---

## Attach (액터 그룹화)

### 파라미터

| 파라미터 | 타입 | 설명 |
|----------|------|------|
| `Duration` | `float` | 그룹 유지 시간 (만료 시 자동 해제) |

### 대상 선정 방식

```
1단계: LMB 누를 때 → 시전자가 바라보는 액터 = FirstTarget
2단계: LMB 뗄 때  → 시전자가 바라보는 액터 = SecondTarget

두 대상이 모두 확정되면 그룹화 적용.
```

### 그룹화 제한

```
캐릭터(ACharacter), 폰(APawn) 타겟 지정 불가.
타겟 선정 시 반드시 클래스 타입 검사:

if (FirstTarget->IsA<ACharacter>() || FirstTarget->IsA<APawn>())
{
    // 타겟 선정 거부
}
```

### 그룹화 효과

```
1. 고정 거리 유지
   → 그룹화 시점의 두 액터 간 거리를 Distance로 기록
   → 매 틱 두 액터의 위치를 Distance가 유지되도록 보정

2. Force 공유
   → 한 액터에 가해진 외부 Force를 다른 액터에도 동일하게 적용

3. GE 공유
   → 한 액터에 GE가 적용되면 다른 액터에도 동일한 GE 전파
   → 단, 전파된 GE는 독립 인스턴스 (Aggregate)
```

### 해제 조건

```
1. Duration 만료
2. 이미 그룹화된 두 액터를 다시 대상으로 지정
3. 두 액터 중 하나가 Destroy 됨 (자동 해제)
```

### 구현 시 주의사항

```cpp
// 그룹 상태는 서버에서만 관리
// GE 공유는 HasAuthority() 검사 후 처리

void UEWEActorGroup::OnGEAppliedToMember(
    UAbilitySystemComponent* ASC,
    const FGameplayEffectSpec& Spec,
    FActiveGameplayEffectHandle Handle)
{
    if (!HasAuthority()) return;

    // 다른 멤버 액터에도 동일 GE 적용
    for (AActor* OtherMember : Members)
    {
        if (OtherMember && OtherMember != ASC->GetOwner())
        {
            UAbilitySystemComponent* OtherASC =
                UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherMember);
            if (OtherASC)
            {
                OtherASC->ApplyGameplayEffectSpecToSelf(Spec);
            }
        }
    }
}
```
