# Network

> RPC 구조, 서버 권한 정책, 기능별 복제 방식을 다룬다.
> 네트워크 관련 코드를 작성하기 전에 반드시 읽을 것.
>
> **GAS 네트워크 일반 패턴** (NetExecutionPolicy, Server-Authoritative, Client Prediction, Replication Mode)은
> `/unreal-engine` 스킬 → `references/gameplay_ability_system.md` → "Network Patterns for GAS" 참조.

---

## 기본 원칙 (변경 금지)

```
모든 GA 발동은 Server Confirm 이후 실행.
클라이언트 예측(Prediction) 없음.
서버 Confirm 전 클라이언트 피드백 없음.
```

이 원칙 때문에 레이턴시 환경에서 체감 반응이 늦을 수 있으나,
동기화 문제를 사전 차단하는 것을 우선으로 설계됨.

---

## 기능별 네트워크 처리

### GA 발동

```
Client → Server_RequestActivateAbility() [ServerRPC]
Server → 유효성 검증 (Trigger, Condition, Cooldown)
       → 승인 시: GA 활성화 + Multicast로 시각 효과 전파
       → 거부 시: 무응답 (클라이언트 피드백 없음)
```

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
void Server_RequestActivateAbility(FGameplayAbilitySpecHandle Handle);

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
| `Reliable` | GA 발동, 위치 변경, 상태 변화 등 게임플레이에 영향 |
| `Unreliable` | 파티클, 사운드, 피격 알림 등 시각/청각 효과 |

---

## 복제 설정 (Replication)

### AEWEWeaponActor

```cpp
AEWEWeaponActor::AEWEWeaponActor()
{
    bReplicates = true;
    SetReplicatingMovement(true);
}

void AEWEWeaponActor::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AEWEWeaponActor, CurrentEnchants);
    DOREPLIFETIME(AEWEWeaponActor, EquippedByCharacter);
}
```

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
