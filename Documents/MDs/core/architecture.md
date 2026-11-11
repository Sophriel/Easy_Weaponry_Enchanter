# Architecture

> 모듈 구조, GAS 컴포넌트 배치, 플러그인 설정을 다룬다.
> 새 클래스를 만들거나 모듈 의존성을 건드리기 전에 반드시 읽을 것.
>
> **일반 UE 플러그인 개발 패턴** (Runtime/Editor 분리, Build.cs, WITH_EDITOR 등)은
> `/unreal-engine` 스킬 → `references/plugin_development.md` 참조.

---

## 플러그인 모듈 구조

### 현재 구조

```
Plugins/EasyWeaponryEnchanter/
└── Source/EasyWeaponryEnchanter/      ← 런타임 모듈 (단일). 게임 빌드에 포함
    ├── Ability/                       ← UEWEAbilityBase, UEWEGA_*
    ├── AbilityTask/                   ← UEWEAT_*
    ├── Attribute/                     ← UEWEAttributeBase, UEWEMaxChangePolicyComponent
    ├── Interface/                     ← IEWECharacterInterface
    ├── Weapon/                        ← UEWEWeaponData, UEWEWeaponInstance, AEWEWeaponBase (GA 가 스폰하는 무기 액터(투사체 등)의 베이스)
    ├── Public/ · Private/             ← 모듈 진입점, LogEWE / EWE_LOG
    └── EasyWeaponryEnchanter.Build.cs
```

에디터 전용 모듈은 아직 없다. 에디터 검증 코드(`IsDataValid`)는 런타임 모듈 안에서
`#if WITH_EDITOR` 로 감싼다.

### 목표 구조 — Runtime / Editor 분리

> ⚠️ 목표 설계 (미구현) — 아래 `EWERuntime` / `EWEEditor` 는 계획상의 이름이다.
> 현재 런타임 모듈은 `EasyWeaponryEnchanter` 이고 Editor 모듈은 만들어지지 않았다.

```
EWE/
├── Source/
│   ├── EWERuntime/          ← 핵심 런타임. 게임 빌드에 포함
│   │   ├── Public/
│   │   └── Private/
│   └── EWEEditor/           ← 에디터 전용. 게임 빌드에 미포함
│       ├── Public/
│       └── Private/
├── Content/
└── EWE.uplugin
```

에디터 전용 로직(Details 커스터마이징, 그래프 에디터 등)이 생기면 Editor 모듈로 분리한다.
런타임 모듈에 두면 쿠킹 시 오류가 발생한다.

### 모듈 의존성

```
메인 게임 모듈 (EWE):
  PublicDependencyModuleNames:  Core, CoreUObject, Engine, InputCore, EnhancedInput,
                                EasyWeaponryEnchanter, GameplayAbilities, GameplayTasks, GameplayTags,
                                ModelViewViewModel, UMG

플러그인 런타임 (EasyWeaponryEnchanter):
  PublicDependencyModuleNames:  Core, GameplayAbilities, GameplayTasks
  PrivateDependencyModuleNames: CoreUObject, Engine, Slate, SlateCore

플러그인 에디터 (EWEEditor — 미구현):
  PublicDependencyModuleNames:  EasyWeaponryEnchanter, UnrealEd, PropertyEditor
```

---

## GAS 컴포넌트 배치

> **ASC 초기화 일반 패턴** (PossessedBy / OnRep_PlayerState / PlayerState 배치)은
> `/unreal-engine` 스킬 → `references/gameplay_ability_system.md` → "Initialize the ASC" 참조.

### 소유 구조 (변경 금지)

```
AEWECharacter (플레이어 캐릭터)
├── UAbilitySystemComponent (ASC)        ← 캐릭터 소유 (CreateDefaultSubobject)
└── UEWEAttributeBase                    ← 캐릭터 소유 (CreateDefaultSubobject)
```

EWE에서는 ASC를 **캐릭터에 직접 배치**한다 (PlayerState 아님).
무기(`UEWEWeaponData`, DataAsset)는 ASC를 소유하지 않는다.
무기가 교체될 때마다 캐릭터 ASC에 GA가 등록/해제된다.

### ASC 초기화 위치

| 시점 | 위치 | 하는 일 |
|------|------|---------|
| 컨트롤러 변경 | `AEWECharacter::NotifyControllerChanged` | `InitAbilityActorInfo(this, this)` — 무조건 호출 |
| 빙의 | `AEWECharacter::PossessedBy` | `Super::PossessedBy` 뒤에 `DefaultAttributeGE` 적용 |

- `NotifyControllerChanged` 는 PossessedBy / UnPossessed / OnRep_Controller 세 경로의 합류점이다.
  ASC 는 컨트롤러 변경을 스스로 감지하지 않으므로 여기서 PlayerController 캐시를 갱신한다.
  `OnRep_PlayerState` 오버라이드는 두지 않는다.
- Owner·Avatar 는 항상 캐릭터 자신이다 (불변). ASC 의 Avatar 변경 분기에 도달하지 않는다.
- `Super::PossessedBy` 안에서 `NotifyControllerChanged` 가 먼저 불리므로, DefaultGE 를 걸 때는 Init 이 끝나 있다.
- 재빙의하면 DefaultGE 가 다시 적용되어 Health 가 가득 찬 상태로 리셋된다. **수용한 동작이다.**
- 이 배치는 **임시**다. ASC 를 PlayerState 로 옮길 때 장착 GA/GE 의 Avatar 변경·리스폰 처리를 다시 검토한다.

---

## AttributeSet 설계

> **AttributeSet 생성, ATTRIBUTE_ACCESSORS 매크로, Replication 일반 패턴**은
> `/unreal-engine` 스킬 → `references/gameplay_ability_system.md` → "Creating Attributes" 참조.

### UEWEAttributeBase Attribute 목록

| Attribute | 용도 | 범위 (클램프) |
|-----------|------|---------------|
| `MaxHealth` | 최대 HP | [1, ∞) |
| `Health` | 현재 HP | [0, MaxHealth 의 Current] |
| `MaxMana` | 최대 MP | [0, ∞) |
| `Mana` | 현재 MP | [0, MaxMana 의 Current] |

- 네 Attribute 모두 `COND_None` / `REPNOTIFY_Always` 로 복제된다.
- 초기값은 생성자의 `Init*` (MaxHealth 1, 나머지 0) 이고, 실제 값은 캐릭터의 `DefaultAttributeGE` 가 채운다.
- 무기가 사용하는 모든 Attribute 는 이 AttributeSet 에 반드시 존재해야 한다.
  존재하지 않는 Attribute 를 참조하면 Warning 출력 후 무시한다.

> 목표 설계 (미구현): `MoveSpeed`, `AttackPower` 등 전투 스탯은 아직 없다.

### 클램프 3계층

| 훅 | 다루는 값 | 역할 |
|----|-----------|------|
| `PreAttributeBaseChange` | Base | Base 를 범위 안으로 클램프 |
| `PreAttributeChange` | Current | Aggregator 결과만 클램프 — **비파괴** (Base·모디파이어 보존) |
| `PostAttributeChange` | Max 의 Current 변화 | 종속 Attribute 의 **Base** 를 정책대로 의도적으로 보정 |

- 범위 규칙(하한·상한)은 `UEWEAttributeBase::GetClampRange` **한 곳에만** 있다. 두 Pre 훅이 `ClampAttribute` 로 공유한다.
- `PostGameplayEffectExecute` 는 클램프하지 않는다. 사망 처리 같은 부수효과 전용이다.

**Current 클램프를 Pre 에 두는 이유**

- Current 가 기록되는 지점은 `FGameplayAttribute::SetNumericValueChecked` 하나다:
  `PreAttributeChange` → `SetCurrentValue` → `PostAttributeChange`.
- 기록 전에 값을 고칠 수 있는 곳은 Pre 뿐이다. Post 는 통보만 받으므로, 클램프하려면 다시 써야 한다.
- 다시 쓰는 수단(`SetHealth`, `ApplyModToAttribute`)은 전부 **Base** 를 바꾼다.
  종속 Attribute 에 Duration/Infinite 모디파이어가 있으면 Base 가 영구 손상되고 재진입이 생긴다.

**상한은 Max 의 Current 값**

- Health/Mana 의 상한은 `GetMaxHealth()` / `GetMaxMana()` (Current) 다. Base 클램프와 Current 클램프가 같은 값을 쓴다.
- 이유: Max 의 Base 를 쓰면 MaxHealth 장착 버프 중에도 Health Base 가 버프 전 Max 에서 막혀 회복 상한이 오르지 않는다.
- 안전 근거: Instant 모디파이어 루프에서 Max Base 가 바뀌면 다음 모디파이어 전에 Max Current 가 즉시 갱신된다.
  dirty 지연은 inhibit 전환과 네트워크 수신에서만 일어난다.
- 결과: Max 디버프 중에 회복하면 Health Base 가 디버프된 Max 에서 막힌다. 디버프가 풀려도 Clamp 정책에서는 그대로다 (Lyra 와 동일).

### Max 변화 정책 — Clamp / KeepRatio

Current 는 Max 가 바뀌어도 스스로 재계산되지 않는다. 그래서 Max 의 `PostAttributeChange` 에서 종속 Attribute 를 보정한다.

| Max | 종속 |
|-----|------|
| `MaxHealth` | `Health` |
| `MaxMana` | `Mana` |

| 정책 | 새 Base | 예 |
|------|---------|-----|
| `Clamp` (기본) | `Min(Base, NewMax)` — 넘을 때만 깎음, Max 상승 시 그대로 | 100/100 → MaxHealth 150 → 100/150 |
| `KeepRatio` | `Base × NewMax / OldMax`. OldMax ≤ 0 이면 `NewMax` (가득 채움) | 50/100 → 75/150 → 50/100 |

보정 규칙:

- **Base 기준**으로 계산한다. Current 는 `clamp(Base + 모디파이어)` 의 파생값이라, Current 로 차이를 내면 종속 Attribute 의 Infinite 모디파이어 몫까지 Base 에서 빠진다.
- **권한 측에서만** 보정한다 (`IsOwnerActorAuthoritative`). 클라이언트는 복제로 받는다.
- 보정은 `ApplyModToAttribute(Dependent, Additive, NewBase - Base)` 로 하고, **차이가 0 이어도 항상 호출**한다.
  종속 Attribute 의 Aggregator 를 다시 돌려 `PreAttributeChange` 가 새 Max 로 Current 를 다시 클램프하게 하기 위해서다.
- 0 나눗셈 가드는 KeepRatio 분기 안에만 있다. Clamp 의 MaxMana 0→N 재계산을 건너뛰지 않기 위해서다.
- Max 변화 이벤트마다 1회 계산하고 누적 상태를 두지 않는다 (KeepRatio 100→150→120 = 100→120).
- 이 보정은 "Attribute 수정 — GE 전용" 규칙의 **문서화된 예외**다 (CLAUDE.md). GE 가 아니므로 `PostGameplayEffectExecute`·GameplayCue 는 실행되지 않는다.

부작용 (현재 무해):

- Max 가 바뀔 때마다 종속 Attribute 의 값 변경 델리게이트가 같은 값으로 1회 더 발생할 수 있다 (HUD 재갱신).
- Instant GE 실행 중에 보정이 일어나면 엔진이 `CurrentModcallbackData` 를 덮어쓴다.
  그래서 바깥 Max 델리게이트가 `GEModData == nullptr` 로 방송되고, 종속 델리게이트가 Max 델리게이트보다 **먼저** 발생한다.
  현재 `GEModData` 를 읽는 곳은 없다 (`UEWEStatusViewModel` 은 값만 사용).

**정책 등록 — `UEWEMaxChangePolicyComponent`**

- GE 컴포넌트다. `Policies` 배열(`FEWEMaxChangePolicyEntry { MaxAttribute, Policy }`)을 GE 에디터에서 지정한다.
- **Instant GE 전용**이다. `OnGameplayEffectExecuted` 에서 AttributeSet 에 정책을 등록한다.
- 정책은 **AttributeSet 전역**이고 GE 가 제거돼도 원복되지 않는다. 등록이 없으면 모든 Max 는 Clamp 다.
- `IsDataValid` 가 저장 시점에 세 가지를 강제한다:
  1. GE 의 `DurationPolicy` 가 Instant 가 아니면 에러 (Duration/Infinite 는 실행이 없어 조용히 미등록되고, Periodic 은 매 주기 재등록되기 때문)
  2. `MaxAttribute` 가 Max 가 아니면 에러
  3. 같은 GE 안에서 첫 종속 모디파이어가 마지막 Max 모디파이어보다 앞이면 에러
- 실행 순서상 모디파이어가 끝난 **뒤** 등록된다. 첫 초기화 때는 정책이 아직 없어 기본 Clamp 로 처리된다
  (MaxHealth 1→100, MaxMana 0→100 모두 차이 0. 이어지는 Health/Mana 모디파이어가 값을 채운다).
- 정책 저장소(`MaxChangePolicies`)는 `UPROPERTY(Transient)`, 비복제다. AttributeSet 이 캐릭터 서브오브젝트라 재빙의로는 유지되고,
  리스폰(새 액터) 시에는 `PossessedBy` 의 DefaultGE 재적용이 다시 등록한다.
- 현재 `BP_DefaultAttributeGE` (`Content/Weapons/GameplayEffects/`) 에 붙어 있다: MaxHealth=Clamp, MaxMana=Clamp.

> 범위 밖 (별도 작업 후보): GE 별 정책 — Max 를 바꾼 원인 GE 의 정책을 적용하는 방식. 도입하면 Instant 전용 제한과 전역 등록은 폐기 대상이다.

### GE 작성 규칙 — 모디파이어 순서

- 엔진은 Instant GE 의 모디파이어를 **배열 순서대로** Base 에 적용한다.
- **한 GE 안에서 Max* 모디파이어를 먼저 둔다.** Health 가 MaxHealth 보다 앞이면, Health Base 를 기록하는 시점의 MaxHealth 가 아직 1 이라 Base Health 가 1 로 영구 손상된다.
- 정책 컴포넌트가 붙은 GE 는 `IsDataValid` 가 이 순서를 강제한다. 그 밖의 GE 는 이 관례에 의존한다.
- magnitude 는 루프 전에 일괄 계산된다. Max* 에 의존하는 % 모디파이어는 **이전** Max 로 평가되므로 고정량을 쓰거나 GE 를 분리한다.
- 예시: `BP_DefaultAttributeGE` — 전부 Scalable Float + ModOp Override, 순서 MaxHealth → Health → MaxMana → Mana.

> CLAUDE.md 의 "GE 스택 — Aggregate 전용 (Override / Additive 금지)" 은 **스태킹** 규칙이다.
> 모디파이어의 ModOp `Override` 와는 무관하다.

---

## 무기 3단 구조

무기는 세 계층으로 나뉜다.

| 계층 | 클래스 | 역할 |
|------|--------|------|
| 정의 | `UEWEWeaponData` | 불변 정의 (소프트 참조). 런타임에 쓰지 않는다 |
| 상태 | `UEWEWeaponInstance` | 한 자루의 상태. 자체 로드 (`BeginLoad` · `FinishLoad` · `IsLoadFinished` · `OnLoadFinished`) |
| 표현 | `UEWEWeaponMeshComponent` | 캐릭터 손의 메시. Instance 를 받아 표시 |

로드 실패는 로그만 남기고 진행한다. 메시가 없으면 손이 비워지고, GA 클래스가 없으면 부여되지 않는다.

---

## 무기 장착/교체 흐름

> **GA 동적 Grant/Revoke 일반 패턴**은
> `/unreal-engine` 스킬 → `references/gameplay_ability_system.md` → "Dynamic GA Grant/Revoke" 참조.

### 현재 구현 — 교체 트리거

입력에서 장착까지의 경로다.

```
숫자키 / 마우스 휠
  → AEWECharacter::SelectWeapon / ScrollWeapon
    → UEWEInventoryComponent::Server_SelectQuickSlot / Server_ScrollQuickSlot
      → SelectQuickSlot_ServerOnly / ScrollQuickSlot_ServerOnly
        → ApplyEquipTransition_ServerOnly
```

**장착은 서버의 인벤토리 Model 관문이 결정한다.** ViewModel 은 Model 을 관찰만 한다.

`ApplyEquipTransition_ServerOnly` (인벤토리) 는 3단계를 따른다.
① Instance 서브오브젝트 조건 전환 → ② `SetEquippedWeaponInstance_ServerOnly` (손 표현 포함) → ③ `EquipWeapon_ServerOnly` (GA 부여).
② · ③ 은 **캐릭터** 함수이며, 인벤토리는 `IEWECharacterInterface` 로 호출한다.

> `EquippedWeaponInstance` 가 null 이면 **맨손(언장착)** 이다. 빈 슬롯을 선택하면 이 상태가 된다.
> `EquipWeapon_ServerOnly(nullptr)` 는 기존 무기 GA 를 제거하고 (`ClearAllAbilities`) 신규 부여를 하지 않는다.

### 서버 권한 흐름

교체 요청이 서버 관문을 거치는 현재 흐름이다.

```
클라이언트: Server_SelectQuickSlot / Server_ScrollQuickSlot / Server_AssignToQuickSlot / Server_ClearQuickSlot RPC → 서버
                │
                ▼
서버: 관문 검증
  Server_SelectQuickSlot = 인덱스 검증
  Server_ScrollQuickSlot = 방향 0 · 빈 슬롯이면 무시, 선택 없음이면 0번 선택, 그 외 다음 인덱스 계산 → SelectQuickSlot_ServerOnly 경유
  Server_AssignToQuickSlot = 인덱스 · null · 미소유 Instance 거부
  Server_ClearQuickSlot = 인덱스 검증, 이미 비어 있으면 무시
                │
                ▼
ApplyEquipTransition_ServerOnly
  ① Instance 서브오브젝트 조건 전환 (장착 COND_None, 해제 COND_OwnerOnly)
  ② SetEquippedWeaponInstance_ServerOnly (손 표현 포함)
  ③ EquipWeapon_ServerOnly — 그 안에서 ClearAllAbilities → GrantWeaponAbilities
                │
                ▼
결과 전달:
  소유자 → OnRep_SelectedQuickSlotIndex · OnRep_QuickSlots
  모든 클라이언트 → OnRep_EquippedWeaponInstance
  (별도 Confirm 브로드캐스트 없음)
```

**미구현:**
- 거부 시 클라이언트 응답 — 현재는 서버 Warning 로그만
- 활성 GA 가 있을 때의 교체 차단 (`ui.md` "슬롯 교체 제한")

### GA 등록/해제 — `AEWECharacter::EquipWeapon_ServerOnly` (현재 구현)

```cpp
void AEWECharacter::EquipWeapon_ServerOnly(UEWEWeaponInstance *Instance)
{
    WeaponAbilityComponent->ClearAllAbilities();   // 이전 무기 GA 전부 제거
    ClearPendingEquipSubscription();               // 이전 pending equip 구독 해제

    if (!Instance) return;                          // 맨손: 신규 부여 없음

    if (Instance->IsLoadFinished())
        GrantWeaponAbilities(Instance);
    else {                                          // 로드 중: 완료 시 부여
        PendingEquipInstance = Instance;
        PendingEquipHandle = Instance->OnLoadFinished.AddUObject(this, &ThisClass::OnPendingEquipLoadFinished);
    }
}

void AEWECharacter::GrantWeaponAbilities(UEWEWeaponInstance *Instance)
{
    // 세 카테고리 모두: 레벨 0, SourceObject = Instance
    // InputID = CHoldAbilityIndex + i / CHitAbilityIndex + i / OnEquip INDEX_NONE
    for (int32 i = 0; i < HoldAbilities.Num(); ++i)
        WeaponAbilityComponent->GiveAbility(FGameplayAbilitySpec(HoldAbilities[i], 0, CHoldAbilityIndex + i, Instance));
    for (int32 i = 0; i < HitAbilities.Num(); ++i)
        WeaponAbilityComponent->GiveAbility(FGameplayAbilitySpec(HitAbilities[i], 0, CHitAbilityIndex + i, Instance));
    for (int32 i = 0; i < OnEquipAbilities.Num(); ++i)
        WeaponAbilityComponent->GiveAbility(FGameplayAbilitySpec(OnEquipAbilities[i], 0, INDEX_NONE, Instance));
}
```

- `OnEquipAbilities` 의 GA(`UEWEGA_DefaultOnEquip` 파생)는 부여되는 순간 `OnAvatarSet` 에서 스스로 활성화해 장착 GE 를 건다.
  상세는 `enchant-system.md` "무기 GA 계층".
- 핸들 목록을 따로 들고 있지 않는다. 무기 GA 만 이 ASC 에 있으므로 `ClearAllAbilities` 로 일괄 제거한다.
- **로드 중 장착:** 대상이 로드 중이면 완료 시 부여 — `PendingEquipInstance` + `OnLoadFinished` 구독. `OnPendingEquipLoadFinished` 에서 여전히 장착된 Instance 인지 확인 후 부여한다.

**해제 체인 — 장착 GE 가 빠지는 경로**

```
EquipWeapon_ServerOnly(다음 Instance 또는 nullptr)
  → ClearAllAbilities
    → (엔진) OnRemoveAbility → 활성 인스턴스 EndAbility
      → UEWEGA_DefaultOnEquip::EndAbility
        → 저장해 둔 Active GE 핸들 RemoveActiveGameplayEffect
          → MaxHealth 등 Current 변화
            → UEWEAttributeBase::PostAttributeChange → 정책대로 Health Base 보정
```

### 인벤토리 Model — 퀵슬롯 쓰기는 서버 관문 경유

`QuickSlots` · `SelectedQuickSlotIndex` 를 바꾸는 **런타임** 경로는 모두 아래 관문을 거친다.
클라이언트는 `Server_*` RPC 로 요청한다.

- `AssignToQuickSlot_ServerOnly` — 퀵슬롯에 Instance 배정 (이전 퀵슬롯에서 이동)
- `ClearQuickSlot_ServerOnly` — 퀵슬롯 비우기
- `SelectQuickSlot_ServerOnly` — 퀵슬롯 선택
- `ScrollQuickSlot_ServerOnly` — 퀵슬롯 스크롤

**예외:** 초기 시딩 `CreateInitialWeaponInstances_ServerOnly` 는 1회성 초기화로 `QuickSlots` 를 직접 채운다.

**불변식:** "장착된 Instance = 선택 슬롯의 내용" 을 관문이 지킨다.
선택 중인 슬롯의 내용이 바뀌면 관문이 `ApplyEquipTransition_ServerOnly` 를 호출하므로,
배열을 직접 쓰면 재장착 · 해제가 조용히 빠진다.

`RemoveWeapon_ServerOnly` 는 슬롯을 비울 때 `ClearQuickSlot_ServerOnly` 를 거친다.

```cpp
// ❌ QuickSlots[i] = Instance;
// ✅ AssignToQuickSlot_ServerOnly(Instance, i);
```

### 인벤토리 `Items` 구조

`Items` 는 고정 크기 슬롯 배열이다. `InitializeComponent` 에서 `InventorySlotCount`(개발자 설정, 기본 32)만큼 `nullptr` 로 초기화한다.

- **재정렬 없음.** 무기를 제거하면 해당 슬롯만 `nullptr` 가 되고, 뒤의 아이템이 앞으로 오지 않는다.
- **가득 참.** `AcquireWeapon_ServerOnly` 는 첫 빈 슬롯을 찾아 채운다. 빈 슬롯이 없으면 `Client_NotifyAcquireRejected`(Client, Reliable) 로 소유 클라이언트에 1회 통지하고 획득을 거부한다. 소유 연결이 없으면(빙의 전 초기 시딩, 봇) 통지는 전달되지 않는다.
- **슬롯별 알림.** `OnItemSlotChanged(int32)` 는 서버 쓰기 경로(`AcquireWeapon_ServerOnly` · `RemoveWeapon_ServerOnly`)와 클라이언트 `OnRep_Items` 양쪽에서 브로드캐스트된다. `OnRep_Items` 는 인덱스별 비교해 변경된 인덱스만 알린다. 뷰모델은 이 이벤트를 구독해 해당 슬롯만 갱신한다.

### 손의 표현

캐릭터 `WeaponMesh` (`UEWEWeaponMeshComponent`, `WeaponSocket` 부착, `NoCollision`).

- 서버: `SetEquippedWeaponInstance_ServerOnly` 에서 `WeaponMesh->SetInstance(...)`
- 클라이언트: `OnRep_EquippedWeaponInstance` 에서 `WeaponMesh->SetInstance(...)`

**메시 컴포넌트 동작:**
- 같은 유효 Instance 는 no-op
- null 은 항상 비움
- 로드 중에는 이전 메시 유지
- 로드 완료 후에도 `OnLoadFinished` 구독 유지 (런타임 재로드 시 재구성)
- 메시가 null 이면 비움
- 소켓 정렬 없음

**`AEWEPlayerWeapon`:** 현재 스폰되지 않는다. 장착 무기 드랍 · 획득 기능용.
정책 코드(숨김 · 관련성 · 이동 복제 off · Scene 루트)는 그 기능 설계 시 재검토.

---

## UI 계층 (MVVM)

HUD 는 **MVVM(Model-View-ViewModel)** 으로 구성한다.
`ModelViewViewModel` 플러그인을 사용하며, `EWE.uproject` / `EWE.Build.cs` 에 의존성이 있다.

### 계층과 의존 방향

```
                 ┌─ 소유 ─────────────┐
UEWELocalUIManageSubsystem            ▼
  (LocalPlayerSubsystem)      UEWE*ViewModel  ◄── 참조 ──  UEWE* / WBP_* (View)
        │                             │
        ├─ Model 바인딩               │
        └─ UI 명령 중계               │
                                      ▼
                              인벤토리 · ASC (Model)
```

**의존 방향은 View → ViewModel → Model 이다.**
ViewModel 은 View 를 참조하지 않는다. 위젯이 사라져도 ViewModel 은 살아 있다.

| 계층 | 클래스 | 책임 |
|------|--------|------|
| Model | `UEWEInventoryComponent` (서버 권한, 소유자 전용 복제), `UAbilitySystemComponent` | 진짜 데이터 |
| ViewModel | `UEWEStatusViewModel`, `UEWETargetInfoViewModel`, `UEWEQuickSlotViewModel`, `UEWEInventoryViewModel`, `UEWESlotItemViewModel` (퀵슬롯 · 인벤토리 공유), `UEWESlotContainerViewModelBase` (퀵슬롯 · 인벤토리 공통 부모) | 표시 상태 + 변경 알림 |
| View | `UEWEStatus`, `UEWETargetInfo`, `UEWEQuickSlot`, `UEWEInventory`, `UEWESlotItem` + 각 WBP, `UEWEUserWidgetBase` (공통 부모) | 바인딩만 (`BindViewModel` 로 자기 뷰에 바인딩) |
| 소유·중재 | `UEWELocalUIManageSubsystem` | VM 생성 · 주입 (VM 객체만 `SetViewModel` 으로 넘김), Model 바인딩 (PC 릴레이, 퀵슬롯 · 인벤토리 VM), UI 명령 중계 (인벤토리 토글 · 타겟 정보) |

### 핵심 원칙

- **ViewModel 은 서브시스템이 소유하고, 위젯보다 오래 산다.** VM 은 위젯 생성 함수(`Create<X>Widget()`)에서
  함께 만든다. 위젯을 재생성해도 VM 의 Model 바인딩이 유지된다.
- **주입은 VM 객체만 넘긴다.** 각 위젯이 자기 `UMVVMView` 에 자기 이름으로 바인딩한다 (이름을 넣을 수 없으면 클래스 방식).
  서브시스템은 BP 측 ViewModel 이름을 모른다.
- **표시 상태는 VM 필드로 표현한다.** 서브시스템은 위젯을 직접 조작하지 않는다
  (예: TargetInfo 의 보임 여부는 VM 의 `HasTarget` 바인딩).

### 초기화 순서 무관성 (변경 금지)

인벤토리 Model 복제 도착 · 폰 빙의 · 위젯 부착은 **서로 순서가 보장되지 않는 세 개의 축**이다.
어느 축도 다른 축의 순서를 가정하지 않는다. 각 축은 준비되면 알리고, 받는 쪽이 조건을 판정한다.

**"타이밍이 아니라 조건으로 게이팅한다."** 이 원칙을 깨는 수정은 하지 말 것.

> VM 생성 · 주입 절차, Model 바인딩 경로, create-then-bind, 축별 알림 · 판정 지점, View 측 바인딩 규칙
> → `features/ui.md` "HUD MVVM 구조"

---

## 주요 클래스 파일 위치

### 현재 구조

```
Plugins/EasyWeaponryEnchanter/Source/EasyWeaponryEnchanter/
├── Ability/
│   ├── EWEAbilityBase.h                ← 무기 GA 공통 부모 (UEWEAbilityBase)
│   ├── EWEGA_Creation.h
│   ├── EWEGA_FireProjectile.h
│   └── EWEGA_DefaultOnEquip.h          ← 장착 GE 적용 GA
├── AbilityTask/
│   └── EWEAT_SpawnActor.h
├── Attribute/
│   ├── EWEAttributeBase.h              ← AttributeSet + 클램프 + Max 변화 정책
│   └── EWEMaxChangePolicyComponent.h   ← 정책 등록 GE 컴포넌트
├── Interface/
│   └── EWECharacterInterface.h
├── Public/
│   └── EasyWeaponryEnchanter.h         ← 모듈 + LogEWE
└── Weapon/
    ├── EWEWeaponData.h                 ← 무기 DataAsset (불변 정의, 소프트 참조)
    ├── EWEWeaponInstance.h             ← 한 자루의 상태 (자체 로드 BeginLoad · FinishLoad · IsLoadFinished · OnLoadFinished)
    └── EWEWeaponBase.h                 ← GA 가 스폰하는 무기 액터(투사체 등)의 베이스

Source/EWE/Character/
├── EWECharacter.h                      ← ASC / AttributeSet 소유, EquipWeapon_ServerOnly · EquippedWeaponInstance · WeaponMesh
├── EWEPlayerController.h
├── EWEInventoryComponent.h             ← Model (Items · QuickSlots · SelectedQuickSlotIndex, 서버 관문 · Server_* RPC)
└── EWEWeaponMeshComponent.h            ← 손의 표현 (WeaponSocket 부착, NoCollision)

Source/EWE/Weapon/
└── EWEPlayerWeapon.h                   ← 현재 스폰되지 않음 (장착 무기 드랍 · 획득용 보존)
```

### 목표 구조

> ⚠️ 목표 설계 (미구현) — 현재 코드에 없음. 아래 이름은 계획상의 이름이다.

```
EWERuntime/Public/
├── Ability/
│   ├── EWEGameplayAbility.h
│   └── EWEAbilityTask_Base.h
├── Attribute/
│   └── EWEAttributeSet.h
├── Character/
│   └── EWECharacterBase.h
├── Data/
│   ├── EnchantDataAsset.h
│   └── WeaponDataAsset.h
├── Function/
│   ├── IEnchantFunction.h
│   └── EWEFunctionLibrary.h
├── Trigger/
│   └── EWETriggerTypes.h
├── Condition/
│   └── EWEConditionTypes.h
├── Recording/
│   └── EWERecordingComponent.h
└── Weapon/
    └── EWEWeaponActor.h
```

> 목표 트리의 이름과 실제 이름의 대응: `EWEGameplayAbility` → `EWEAbilityBase`, `EWEAttributeSet` → `EWEAttributeBase`,
> `EWECharacterBase` → `EWECharacter` (게임 모듈), `WeaponDataAsset` → `EWEWeaponData`.
> 나머지(`EWEAbilityTask_Base`, `EnchantDataAsset`, `IEnchantFunction`, Trigger/Condition/Recording, `EWEWeaponActor`)는 미구현이다.

### 게임 모듈 UI — 실제 구조

```
Source/EWE/UI/
├── EWELocalUIManageSubsystem.h/.cpp     ← VM 소유 · 위젯 생성 · 명령 중계
├── EWEUIAsset.h/.cpp                    ← 위젯 클래스 참조 (비동기 로드)
├── EWEUserWidgetBase.h/.cpp             ← 공통 위젯 부모 (BindViewModel 헬퍼)
├── EWESlotContainerViewModelBase.h/.cpp ← 퀵슬롯 · 인벤토리 VM 공통 부모 (슬롯 아이템 배열 · BindModel 흐름 · 내용 미러링 · GetSlotCount 브로드캐스트)
│
├── EWEStatus.h/.cpp                     ← View
├── EWEStatusViewModel.h/.cpp            ← ASC 구독, HP/MP 노출
│
├── EWETargetInfo.h/.cpp                 ← View
├── EWETargetInfoViewModel.h/.cpp        ← 타겟 ASC 구독
│
├── EWEQuickSlot.h/.cpp                  ← View (슬롯 위젯 생성 · 아이템 VM 전달)
├── EWEQuickSlotViewModel.h/.cpp         ← 인벤토리 Model 관찰 (BindModel)
├── EWESlotItem.h/.cpp                   ← View (슬롯 1칸, 자기 뷰에 바인딩). 퀵슬롯 · 인벤토리 공유
├── EWESlotItemViewModel.h/.cpp          ← Instance 1개 표시 (SetInstance, 로드 완료 구독). 퀵슬롯 · 인벤토리 공유
├── EWEInventoryViewModel.h/.cpp         ← 인벤토리 Model 관찰 (BindModel), 선택 상태 소유
│
└── EWEInventory.h/.cpp                  ← 인벤토리 위젯 (VM 주입, 슬롯 위젯 생성 미구현)

Content/UI/HUD/
├── BP_Status / BP_TargetInfo / BP_QuickSlot / BP_ItemSlot
```

`UEWEInventory` 는 VM 을 주입받는다 (`SetViewModel`). 슬롯 위젯 생성은 미구현이다.
