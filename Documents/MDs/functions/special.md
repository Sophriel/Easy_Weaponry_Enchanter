# Function — Special

> TimeStop, Freeze, Teleport, PossessObject, WeatherChange, StatusEffect 의
> 파라미터, 구현 규칙, 네트워크 처리를 다룬다.

---

## Time Stop (시간 정지)

### 동작 정의

```
시전자를 제외한 세션 내 모든 액터의 Position과 속도를 고정.
Attribute 변경은 허용 (위치/속도만 고정).
모든 입력 차단 (UI 조작 허용, UI를 통한 캐릭터 변화는 차단).
```

### 파라미터

| 파라미터 | 타입 | 설명 |
|----------|------|------|
| `Duration` | `float` | 지속 시간 (0 = 재발동으로만 해제) |

### 해제 조건

```
1. 시전자가 Time Stop을 재발동 → 즉시 해제
2. Duration 만료
※ 시전자의 사망/기절 등 상태 변화는 해제에 영향 없음
```

### 네트워크 처리

```
동일 틱에 다수 요청이 서버에 도착하면 모두 처리.
효과는 다음 틱에 서버에서 일괄 적용.

구현 시 주의: Position/속도 고정은 CharacterMovementComponent의
  bIgnoreClientMovementErrorChecksAndCorrection 이나
  커스텀 Movement Replication으로 처리.
  물리 시뮬레이션 중인 액터는 Physics 일시 정지로 처리.
```

### 입력 차단 구현

```cpp
// Time Stop GE 적용 시 태그 부여
// GameplayTag: Status.TimeStop

void AEWEPlayerController::SetupInputComponent()
{
    // Time Stop 태그가 있으면 캐릭터 관련 입력 차단
    // UI 입력(마우스 커서, 탭 등)은 허용
}

// UI를 통한 캐릭터 변화 차단 예시:
// 인벤토리에서 아이템 사용 → Status.TimeStop 태그 확인 후 차단
```

---

## Freeze (빙결)

### 동작 정의

```
단일 대상 액터의 Position과 속도 고정.
Time Stop과 동일한 입력 차단 방식.
Stun과 동일한 효과 (별도 Stun 없음).
```

### 동작 예외 항목

```
예외 1: 기존에 발동 중이던 GA 및 AbilityTask 유지
        (GA에 의한 Position/속도 변경도 허용)

예외 2: OnHit Trigger 기반 인챈트는 Freeze 중에도 발동 가능
        → 피격 시 상대를 Freeze시키는 인챈트 등 허용
        → 해당 인챈트 발동이 Freeze를 해제하지는 않음

예외 3: UI 조작 자체는 허용
        → UI를 통해 캐릭터에 변화를 주는 행위는 차단
```

### Time Stop + Freeze 동시 틱 처리

```
서버 동일 틱:
  플레이어 A → Time Stop 요청
  플레이어 B → Freeze 요청

둘 다 처리. Time Stop이 먼저 적용되더라도
Freeze 요청은 다음 틱에 서버에서 처리.

Time Stop 당한 플레이어의 Freeze 시도:
  → 일반적으로 차단
  → 단, 동일 틱 패킷이면 후순위로 처리하되 발동 허용
```

### Freeze 중 상태 이상 중첩

```
Freeze + Slow 동시 적용 시:
  → 두 GE 모두 Aggregate로 독립 적용
  → Slow의 이동속도 감소는 Freeze가 해제될 때까지 효과 미발현
  → Freeze 해제 후 잔여 Slow 지속 시간만큼 Slow 효과 발현
```

---

## Teleport (공간 이동)

### 파라미터

| 파라미터 | 타입 | 설명 |
|----------|------|------|
| `TeleportMode` | `ETeleportMode` | Preview / ActorTarget / Offset |
| `OffsetVector` | `FVector` | Offset 모드 사용 시 이동 벡터 |

### 모드별 동작

```
Preview 모드:
  LMB 누르는 동안:
    → 시전자가 바라보는 방향의 착지 가능 위치 탐색 (LineTrace / NavMesh)
    → 반투명 고스트 캐릭터를 해당 위치에 표시 (클라이언트 로컬)
  LMB 뗄 때:
    → 서버에 목적지 좌표 전송
    → 서버에서 유효성 보정 후 Confirm
    → 클라이언트 위치 업데이트

ActorTarget 모드:
  → 시전자가 바라보는 액터의 위치로 즉시 이동
  → 서버에서 목적지 유효성 검증 후 처리

Offset 모드:
  → 시전자 기준 OffsetVector만큼 이동
  → 서버에서 목적지 충돌 검사 후 처리
```

### 서버 유효성 검증

```cpp
FVector UEWETeleportValidator::ValidateDestination(
    const FVector& RequestedLocation,
    const AActor* Caster)
{
    // 1. NavMesh 위 착지 가능 여부 확인
    FNavLocation NavLocation;
    bool bOnNav = NavSys->ProjectPointToNavigation(
        RequestedLocation, NavLocation, Extent);

    // 2. 캡슐 충돌 검사
    bool bClear = !World->SweepTestByChannel(...);

    if (bOnNav && bClear)
        return NavLocation.Location;

    // 유효하지 않으면 가장 가까운 유효 위치 반환
    return FindNearestValidLocation(RequestedLocation, Caster);
}
```

---

## Possess Object (오브젝트 빙의)

### 동작 정의

```
바라보는 액터에 임시 Pawn 컴포넌트 + 카메라를 생성하여
해당 액터를 플레이어가 조작하도록 전환.
```

### 빙의 가능 조건

```
대상 액터가 이미 빙의된 Pawn이면 불가.
(GameplayTag: Status.Possessed 를 확인)
```

### 빙의 중 허용 입력

```
이동 (WASD)
점프
카메라 회전 (마우스)

나머지 모든 입력 차단 (LMB, RMB, UI 등)
```

### 원본 캐릭터 피격 알림

```
빙의 중 원본 캐릭터가 피격 시:
  → 클라이언트 화면에 붉은 테두리 효과 표시
  → 효과는 1회 적용, 빙의 해제 전까지 유지 (중복 적용 없음)

구현: PlayerController에 ClientRPC로 알림 전송
```

### 빙의 해제 시퀀스

```cpp
void AEWEPlayerController::ReleasePossession()
{
    if (!PossessedObject) return;

    // 1. 임시 Pawn/카메라 제거
    PossessedObject->RemoveTemporaryPawnComponents();

    // 2. Status.Possessed 태그 제거
    OriginalCharacter->GetASC()->RemoveLooseGameplayTag(
        FGameplayTag::RequestGameplayTag("Status.Possessed"));

    // 3. 즉시 원본 캐릭터로 Possess 전환
    Possess(OriginalCharacter);

    // 4. 붉은 테두리 효과 제거
    RemoveHitBorderEffect();

    PossessedObject = nullptr;
}
```

### 대상 액터 파괴 시 처리

```
빙의 중 대상 액터가 Destroy되면:
  → 즉시 ReleasePossession() 호출
  → 기본 패널티 없음
  → OnDestroyEnchant 이벤트가 설정되어 있으면 발동 (UI 이펙트 등)
```

---

## Weather Change (날씨 변경)

### 파라미터

| 파라미터 | 타입 | 설명 |
|----------|------|------|
| `TargetTimeOfDay` | `float` | 목표 시간대 (0.0 ~ 24.0) |
| `TransitionDuration` | `float` | 전환 소요 시간 (초) |

### 서버 처리 및 동기화

```
서버에서 요청 수신
  → Last-Write-Wins: 동일 틱 또는 이후 요청이 이전 요청을 덮어씀
  → Multicast RPC로 전체 클라이언트에 브로드캐스트

클라이언트:
  → TargetTimeOfDay와 TransitionDuration을 수신
  → DirectionalLight, SkyAtmosphere 등을 TransitionDuration 동안 보간
```

### 구현 패턴

```cpp
UFUNCTION(NetMulticast, Reliable)
void AEWEWeatherManager::Multicast_ChangeWeather(
    float TargetTime, float TransitionDuration)
{
    // 기존 전환 중단
    GetWorldTimerManager().ClearTimer(TransitionHandle);

    StartTimeOfDay = CurrentTimeOfDay;
    TargetTimeOfDay = TargetTime;
    TransitionElapsed = 0.f;
    TotalTransitionDuration = TransitionDuration;

    // Tick에서 보간 처리
}
```

---

## Status Effect (상태 이상)

### 지원 목록

| 상태 이상 | 효과 | GameplayTag |
|-----------|------|-------------|
| Slow | 이동속도 감소 | `Status.Slow` |
| Burn | 지속 데미지 | `Status.Burn` |
| Silence | 인챈트(GA) 발동 차단 | `Status.Silence` |
| Root | 이동 차단 (입력은 허용) | `Status.Root` |
| Blind | 시야 차단 (화면 효과) | `Status.Blind` |

Stun = Freeze와 동일. 별도 제공 안 함.

### 공통 파라미터

| 파라미터 | 타입 | 설명 |
|----------|------|------|
| `StatusEffectType` | `EStatusEffectType` | 위 5종 중 선택 |
| `Duration` | `float` | 지속 시간 |
| `Magnitude` | `float` | 효과 강도 |
| `StatusEffectGE` | `TSubclassOf<UGameplayEffect>` | 적용할 GE 클래스 |

### 중첩 정책 (변경 금지)

```
모든 상태 이상: GE Aggregate 방식
→ 동일 상태 이상 중복 적용 시 완전 독립 인스턴스
→ 복리 효과 가능 (예: Slow x2 = 속도 감소율 중첩)

각 인스턴스의 Duration은 독립적으로 카운트.
```

### Silence 처리

```cpp
// GA 발동 전 Silence 태그 확인
bool UEWEGameplayAbility::CanActivateAbility(...) const
{
    // OnHit Trigger 기반 인챈트는 Silence 중에도 발동 가능
    if (SourceEnchant->Trigger.TriggerType == EEnchantTriggerType::OnHit)
    {
        return Super::CanActivateAbility(...);
    }

    // 그 외 인챈트는 Silence 태그가 있으면 차단
    if (ActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(
        FGameplayTag::RequestGameplayTag("Status.Silence")))
    {
        return false;
    }

    return Super::CanActivateAbility(...);
}
```

### Blind 처리

```
Blind는 순수 클라이언트 시각 효과.
GE 적용 시 클라이언트에서 화면 PostProcess 효과 활성화.
GameplayTag(Status.Blind) RepNotify 또는 GE OnActive 델리게이트로 감지.

서버에서는 Attribute 변화 없음 (게임플레이 영향 없음).
```
