# Network

> RPC 구조, 서버 권한 정책, 기능별 복제 방식을 다룬다.
> 네트워크 관련 코드를 작성하기 전에 반드시 읽을 것.
>
> **GAS 네트워크 일반 패턴** (NetExecutionPolicy, Server-Authoritative, Client Prediction, Replication Mode)은
> `/unreal-engine` 스킬 → `references/gameplay_ability_system.md` → "Network Patterns for GAS" 참조.

---

## GA 실행 정책

```
GA 의 NetExecutionPolicy 는 GA 마다 선택한다. EWE 는 특정 정책을 강요하지 않는다.
판단 기준은 조작감이다. GA 제작자가 스스로 결정한다.
```

| 정책 | 권장 대상 | 특징 |
|------|-----------|------|
| `LocalPredicted` | 재사용 빈도가 높은 GA (기본 공격, 연타 입력) | 클라이언트가 먼저 실행하고 서버가 확인한다. 반응이 빠르다 |
| `ServerInitiated` | 재사용 빈도가 낮은 GA | 서버가 먼저 실행하고 클라이언트가 뒤따른다 |
| `ServerOnly` | 판정·상태 변경 전용 GA | 서버에서만 실행한다. 결과는 GE / Attribute 복제로 전달된다 |

### 강제 예외 — 장착 GA

```
UEWEGA_DefaultOnEquip 은 ServerOnly 를 강제한다.
  - 생성자에서 ServerOnly 로 설정
  - IsDataValid 에서 BP 가 값을 바꾸면 에러
  - OnAvatarSet 에서 ServerOnly 가 아니면 Warning 후 활성화하지 않음
```

근거: 이 GA 의 역할은 장착 GE 를 거는 것뿐이다.
클라이언트는 prediction key 없이 GE 를 적용할 수 없으므로 (`AbilitySystemComponent.cpp:813`)
클라이언트 실행은 의미가 없다. 장착 GE 와 Attribute 는 서버에서 복제된다.

### 현재 상태와 전환 계획

```
현재 모든 GA 는 엔진 기본값 LocalPredicted 로 개발한다.
ServerOnly 가 필요한 GA 를 하나씩 전환하는 것이 남은 작업이다.
→ CLAUDE.md "미결 항목" 에서 추적
```

---

## 기능별 네트워크 처리

### GA 발동

EWE 는 GA 발동용 RPC 를 따로 두지 않는다. 엔진의 `TryActivateAbility` 경로를 그대로 쓴다.

```
Client → TryActivateAbility (입력: AbilityLocalInputPressed)
  LocalPredicted  → 클라이언트 예측 실행 + ServerTryActivateAbility → 서버 확인 / 거부 시 롤백
  ServerInitiated → ServerTryActivateAbility → 서버 실행 → 클라이언트 실행
  ServerOnly      → ServerTryActivateAbility → 서버에서만 실행
Server → 유효성 검증 (Trigger, Condition, Cooldown) 은 CanActivateAbility 에서 수행
```

### 무기 장착 · 퀵슬롯

클라이언트 예측이 없다. 슬롯 선택 · 스크롤 · 배정 · 비우기 요청은 전부 서버 RPC 로만 간다.

```
Client → Server_SelectSlot / Server_ScrollSlot / Server_AssignToSlot / Server_ClearSlot [Server, Reliable]
Server → _ServerOnly 관문 안에서 검증 (Server_AssignToSlot: null · 미소유 Instance 거부)
       → Model 변경 (SelectedSlotIndex · QuickSlots)
Owner → OnRep_SelectedSlotIndex · OnRep_QuickSlots (소유자 전용)
All Clients → OnRep_EquippedWeaponInstance (별도 Confirm 브로드캐스트 없음)
```

`TryInitialEquip_ServerOnly` 는 서버 전용이다. `HasBegunPlay()` · 초기 시딩 완료 · 폰에 컨트롤러 있음 — 세 조건이 충족되면 0번 슬롯을 선택한다.
호출처는 인벤토리 초기 로드아웃(`TryApplyInitialLoadout_ServerOnly`)과 캐릭터 `PossessedBy` 두 곳이다.

### 독립 오브젝트 (Creation Function)

```
서버에서만 SpawnActor.
Owner = 서버.
bReplicates = true인 경우 클라이언트에 자동 복제.

영향 계산(데미지, GE 적용 등):
  → 반드시 HasAuthority() 검사 후 처리
  → 영향 기준은 Context.Who (생성한 플레이어) 참조
```

### Teleport

```
Client → Server_RequestTeleport(RequestedLocation) [ServerRPC]
Server → ValidateDestination() 으로 목적지 보정
       → Client_ConfirmTeleport(ValidatedLocation) [ClientRPC]
Client → 수신한 위치로 캐릭터 이동

서버 보정 없이 클라이언트가 임의 위치로 이동하는 코드 작성 금지.
```

### Weather Change

```
Client → Server_RequestWeatherChange(TargetTime, Duration) [ServerRPC]
Server → Last-Write-Wins 처리 (기존 전환 중단, 새 값으로 덮어씀)
       → Multicast_ChangeWeather(TargetTime, Duration) [NetMulticast, Reliable]
All Clients → 수신한 값으로 라이트닝 보간 시작
```

### Time Stop

```
Client → Server_RequestTimeStop() [ServerRPC]
Server → 동일 틱 다수 요청 모두 수집
       → 다음 틱 시작 시 일괄 처리
       → Multicast_ApplyTimeStop(AffectedActors[]) [NetMulticast]
All Clients → 해당 액터들 Position/속도 고정
```

### Freeze

```
Client → Server_RequestFreeze(TargetActor) [ServerRPC]
Server → TargetActor 유효성 확인
       → Time Stop과 동일 틱 처리 규칙 적용
       → Multicast_ApplyFreeze(TargetActor) [NetMulticast]
All Clients → TargetActor Position/속도 고정
```

### Possess Object

```
Client → Server_RequestPossess(TargetActor) [ServerRPC]
Server → 빙의 가능 여부 확인 (Status.Possessed 태그)
       → 승인 시:
           TargetActor에 Status.Possessed 태그 부여
           Client_ConfirmPossession(TargetActor) [OwningClient만]
Client → 임시 Pawn/카메라 생성 및 입력 전환

빙의 해제:
Client → Server_RequestReleasePossession() [ServerRPC]
Server → Status.Possessed 태그 제거
       → Client_ConfirmRelease() [OwningClient만]
```

### 피격 알림 (Possess Object 중)

```
Server → Client_NotifyHitWhilePossessing() [OwningClient만, Unreliable 허용]
Client → 붉은 테두리 효과 표시 (이미 표시 중이면 무시)
```

### 쿨타임 차단

```
클라이언트: CanActivateAbility()에서 로컬 쿨타임 태그 확인 → 차단
서버: 동일하게 확인 → 차단
두 곳 모두 차단하므로 불필요한 RPC 발생을 사전에 막음
```

---

## EWE RPC 작성 규칙

### 명명 규칙

```cpp
// Server RPC: Server_ 접두사
UFUNCTION(Server, Reliable)
void Server_RequestTeleport(FVector RequestedLocation);

// Client RPC: Client_ 접두사
UFUNCTION(Client, Reliable)
void Client_ConfirmTeleport(FVector ValidatedLocation);

// Multicast: Multicast_ 접두사
UFUNCTION(NetMulticast, Reliable)
void Multicast_ApplyFreeze(AActor* TargetActor);
```

### Reliable / Unreliable 선택 기준

| 구분 | 사용 케이스 |
|------|------------|
| `Reliable` | 위치 변경, 상태 변화 등 게임플레이에 영향 |
| `Unreliable` | 파티클, 사운드, 피격 알림 등 시각/청각 효과 |

---

## 복제 설정 (Replication)

### 무기 인스턴스 · 인벤토리

인벤토리(`UEWEInventoryComponent`)와 캐릭터(`AEWECharacter`)는 무기 인스턴스(`UEWEWeaponInstance`)를 복제한다.

**복제 범위**

| 대상 | 복제 조건 |
|------|-----------|
| 인벤토리 `Items` · `QuickSlots` · `SelectedSlotIndex` | `COND_OwnerOnly` (소유자 전용) |
| 인벤토리 `QuickSlotCount` | 복제 안 함 (개발자 설정, 각 머신이 `InitializeComponent` 에서 배열 크기 지정) |
| 캐릭터 `EquippedWeaponInstance` | 모두 (모든 클라이언트) |
| Instance 서브오브젝트 | 소유자 전용, 장착 중인 것만 모두 |

**서브오브젝트 조건 전환**

복제 조건에 setter 가 없으므로, 같은 함수 안에서 `RemoveReplicatedSubObject` → `AddReplicatedSubObject` 를 연속 호출한다.
장착 시 `COND_OwnerOnly` → `COND_None`, 해제 시 `COND_None` → `COND_OwnerOnly`.

**이전 Instance 복제본 잔존**

다른 클라이언트에 이전 Instance 의 복제본이 남는다. 파괴 · 해제되지 않고 하드 에셋 참조를 유지한다.

**서브오브젝트 목록 플래그**

소유 액터(`AEWECharacter`)와 인벤토리 컴포넌트(`UEWEInventoryComponent`) 양쪽 모두 `bReplicateUsingRegisteredSubObjectList = true` 를 설정해야 한다.
컴포넌트는 C++ 생성자에서 설정한다.

**표현 경로**

`EquippedWeaponInstance` → `OnRep_EquippedWeaponInstance` → `UEWEWeaponMeshComponent`.
클라이언트 표현은 미매핑 참조 해결 후 OnRep 재발화에 의존한다.

| 경우 | 관찰 클라이언트의 Instance | OnRep 재발화 |
|------|---------------------------|--------------|
| 초기 진입 · 늦은 접속 · 재접속 | 없음 (새 채널) | 필요 |
| 세션 중 처음 장착되는 무기로 교체 | 없음 (`COND_OwnerOnly` → `COND_None` 으로 처음 복제) | 필요 |
| 세션 중 이전에 장착됐던 무기로 재교체 | 있음 (이전 복제본 잔존) | 불필요 |

어빌리티 클래스 · 메시는 Instance 가 각 머신에서 소프트 참조로 스스로 로드한다.

### 독립 오브젝트

```cpp
// Creation Function에서 생성 시 bReplicates 파라미터에 따라
FActorSpawnParameters Params;
Params.Owner = GetWorld()->GetAuthGameMode();  // Owner = 서버

AActor* Spawned = GetWorld()->SpawnActor<AActor>(
    SpawnClass, SpawnTransform, Params);

if (bReplicates)
{
    Spawned->SetReplicates(true);
}
```
