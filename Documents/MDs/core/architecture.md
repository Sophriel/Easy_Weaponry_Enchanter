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
    ├── Weapon/                        ← UEWEWeaponData, AEWEWeaponBase (액터, 현재 미사용)
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

## 무기 장착/교체 흐름

> **GA 동적 Grant/Revoke 일반 패턴**은
> `/unreal-engine` 스킬 → `references/gameplay_ability_system.md` → "Dynamic GA Grant/Revoke" 참조.

### 현재 구현 — 교체 트리거

입력에서 장착까지의 경로다. UI 계층이 담당하며 아래 "UI 계층 (MVVM)" 절이 상세를 다룬다.

```
숫자키 / 마우스 휠
  → AEWECharacter::SelectWeapon / ScrollWeapon
    → AEWEPlayerController::SelectSlot / ScrollSlot
      → UEWELocalUIManageSubsystem::SelectQuickSlot / ScrollQuickSlot
        → UEWEQuickSlotViewModel::SelectSlot          ← 선택 상태를 소유
          → OnSlotSelected 브로드캐스트
            → UEWELocalUIManageSubsystem::HandleSlotSelected
              → IEWECharacterInterface::EquipWeapon   ← 실제 장착
```

**ViewModel 은 무기를 장착하지 않는다.** 선택이 바뀌었다고 알릴 뿐이고,
장착은 그 이벤트를 구독한 서브시스템이 폰에게 시킨다.
게임플레이 효과를 UI 계층 밖에 두기 위한 분리다.

> `EquipWeapon(nullptr)` 은 **언장착(맨손)** 을 뜻한다. 빈 슬롯을 선택하면 이 경로를 탄다.
> `CurrentWeapon` 을 반드시 함께 비워야 한다 — 어빌리티는 이미 제거된 상태이므로
> `CurrentWeapon` 이 남으면 `AttackHold()` 가 없는 어빌리티를 활성화하려 든다.

### 설계 — 서버 확인 (미구현)

아래는 `network.md` 의 Server Confirm 정책에 따른 **목표 흐름**이다.
현재 코드는 로컬에서 즉시 장착하며, RPC 왕복은 아직 없다.

```
클라이언트: 무기 교체 요청 RPC → 서버
                │
                ▼
서버: 유효성 검증
                │
                ├─ 거부 → 클라이언트에 거부 응답
                │
                └─ 승인 →
                    1. 현재 발동 중인 GA 전부 CancelAbility
                    2. 기존 무기 GA ASC에서 해제
                    3. 장착 GA 해제·부여로 장착 GE 교체 (Max 변화 → 정책 보정)
                    4. 신규 무기 GA를 ASC에 등록
                    5. 클라이언트에 Confirm 브로드캐스트
```

### GA 등록/해제 — `AEWECharacter::EquipWeapon` (현재 구현)

```cpp
void AEWECharacter::EquipWeapon(UEWEWeaponData* Weapon)
{
    WeaponAbilityComponent->ClearAllAbilities();   // 이전 무기 GA 전부 제거

    if (Weapon == nullptr) { CurrentWeapon = nullptr; /* 메시 해제 */ return; }
    CurrentWeapon = Weapon;
    // ... 메시 로드·설정 ...

    // Hold / Hit: 레벨 0, InputID = CHoldAbilityIndex / CHitAbilityIndex 부터 순번
    for (auto& Ability : Weapon->HoldAbilities)
        WeaponAbilityComponent->GiveAbility(FGameplayAbilitySpec(Ability, 0, HoldAbilityIndex++));
    for (auto& Ability : Weapon->HitAbilities)
        WeaponAbilityComponent->GiveAbility(FGameplayAbilitySpec(Ability, 0, HitAbilityIndex++));

    // OnEquip: 레벨 0, InputID INDEX_NONE (입력 인덱스 충돌 방지), SourceObject = 무기
    for (auto& Ability : Weapon->OnEquipAbilities)
        WeaponAbilityComponent->GiveAbility(FGameplayAbilitySpec(Ability, 0, INDEX_NONE, Weapon));
}
```

- `OnEquipAbilities` 의 GA(`UEWEGA_DefaultOnEquip` 파생)는 부여되는 순간 `OnAvatarSet` 에서 스스로 활성화해 장착 GE 를 건다.
  상세는 `enchant-system.md` "무기 GA 계층".
- 핸들 목록을 따로 들고 있지 않는다. 무기 GA 만 이 ASC 에 있으므로 `ClearAllAbilities` 로 일괄 제거한다.

**해제 체인 — 장착 GE 가 빠지는 경로**

```
EquipWeapon(다음 무기 또는 nullptr)
  → ClearAllAbilities
    → (엔진) OnRemoveAbility → 활성 인스턴스 EndAbility
      → UEWEGA_DefaultOnEquip::EndAbility
        → 저장해 둔 Active GE 핸들 RemoveActiveGameplayEffect
          → MaxHealth 등 Current 변화
            → UEWEAttributeBase::PostAttributeChange → 정책대로 Health Base 보정
```

**제약:** 장착 GA 는 ServerOnly + `IsNetAuthority` 가드라 **Standalone / 리슨 서버 호스트**에서만 활성화된다.
`EquipWeapon` 은 아직 로컬 호출이므로, 원격 클라이언트에서는 장착 GE 가 걸리지 않는다.
서버 RPC 경로(위 "설계 — 서버 확인") 완료 후 해소된다.

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
        └─ 명령 중계 ─► AEWECharacter │
                                      ▼
                              인벤토리 · ASC (Model)
```

**의존 방향은 View → ViewModel → Model 이다.**
ViewModel 은 View 를 참조하지 않는다. 위젯이 사라져도 ViewModel 은 살아 있다.

| 계층 | 클래스 | 책임 |
|------|--------|------|
| Model | `UEWEInventoryComponent`, `UAbilitySystemComponent` | 진짜 데이터 |
| ViewModel | `UEWEStatusViewModel`, `UEWETargetInfoViewModel`, `UEWEQuickSlotViewModel`, `UEWEQuickSlotItemViewModel` | 표시 상태 + 변경 알림 |
| View | `UEWEStatus`, `UEWETargetInfo`, `UEWEQuickSlot` + 각 WBP | 바인딩만 |
| 소유·중재 | `UEWELocalUIManageSubsystem` | VM 생성·주입, 게임플레이 명령 중계 |

### ViewModel 은 서브시스템이 소유한다

```cpp
// UEWELocalUIManageSubsystem::Initialize
QuickSlotViewModel = NewObject<UEWEQuickSlotViewModel>(this);
QuickSlotViewModel->OnSlotSelected.AddUObject(this, &ThisClass::HandleSlotSelected);
```

- **생성과 구독은 `Initialize()` 에서 1회.** `Bind*()` 안에 두지 않는다 —
  `Bind*()` 는 여러 경로에서 호출되고 조기 반환 분기가 많아, 구독이 아예 일어나지 않거나
  두 번 일어나는 창이 생긴다.
- VM 은 **위젯보다 오래 산다.** 위젯을 재생성해도 슬롯 배정이 보존된다.
- 서브시스템은 VM 을 밖으로 넘기지 않는다. → 아래 "포워딩 함수를 유지하는 이유"

### create-then-bind 패턴

세 위젯이 같은 형태를 따른다. 어느 쪽이 먼저 준비되든 결과가 같다.

```
Initialize()             →  VM 생성 + 구독
Create<X>Widget()        →  VM 주입 → Bind<X>()      ← 위젯이 준비된 경로
AcknowledgePossession()  →  Bind<X>()                ← 폰이 준비된 경로
```

`Bind*()` 는 **몇 번 불려도 안전**해야 한다. 이미 채워졌으면 덮어쓰지 않는다.

### 초기화 순서 무관성 (변경 금지)

인벤토리 로드 · 폰 빙의 · 위젯 부착은 **서로 순서가 보장되지 않는 세 개의 축**이다.
어느 축도 다른 축의 순서를 가정하지 않는다.

```
각 축은 준비되면 알린다        인벤토리 → OnWeaponsSynced
받는 쪽이 조건을 판정한다      서브시스템 → GetSlotCount() / IsEmpty() / GetSelectedIndex()
```

**"타이밍이 아니라 조건으로 게이팅한다."** 이 원칙을 깨는 수정은 하지 말 것 —
이 저장소에서 같은 부류의 버그가 네 번 재발했다.

### 퀵슬롯 ViewModel 불변 — 모든 슬롯 쓰기는 `SetSlot()` 을 거친다

```cpp
SeedFromInventory()  ─┐
AddWeapon()          ─┼─►  SetSlot()  ─►  SlotItem->SetWeapon()
ClearSlot()          ─┘         │
                                └─ SlotIndex == SelectedIndex 이면 OnSlotSelected 재발화
```

선택 중인 슬롯의 **내용**이 바뀌면 재장착이 필요한데, `SelectSlot()` 은 인덱스가 그대로라
아무것도 하지 않는다. 그 구멍을 `SetSlot()` 이 막는다.

`SlotItem->SetWeapon()` 을 직접 부르는 새 경로를 만들면 이 보호가 조용히 사라진다.

### 슬롯 개수는 위젯이 소유한다

`UEWEQuickSlot::MaxSlotSize`(BP 편집 가능)가 부착 시 VM 을 사이징한다.
위젯이 붙기 전까지 VM 은 **unsized**(0슬롯)이며, 이는 에러가 아니라 기대 상태다 —
VM 의 슬롯 조작 메서드는 unsized 를 `Verbose` no-op 으로 처리한다.

> **수용한 트레이드오프:** 위젯이 안 뜨면 슬롯·시딩·장착이 전부 사라진다.
> 게임플레이 효과가 프레젠테이션 에셋의 성공에 결합된다.
> BP 워크플로(디자이너가 슬롯 수를 조정)를 지키기 위해 받아들였다.
> 대안이던 "위젯 CDO 에서 조기 사이징"은 클래스 로드 타이밍이라는 변수를 새로 들이므로 거절했다.

### 포워딩 함수를 유지하는 이유 (결정)

서브시스템의 `AddWeaponToQuickSlot` / `SetQuickSlot` / `SelectQuickSlot` / `ScrollQuickSlot`
넷을 삭제하고 `GetQuickSlotViewModel()` 만 노출하자는 안이 있었다. **채택하지 않는다.**

- **널 가드가 1곳에서 N곳으로 흩어진다.** 지금은 포워더 4개에 모여 있다.
- **소유권 캡슐화가 깨진다.** VM 은 `Deinitialize()` 에서 `nullptr` 이 되는데,
  생 포인터를 나눠주면 호출자가 캐싱할 수 있다.

> 진짜 개선 여지는 포워더가 아니라 `Character → PC → Subsystem → VM` 3-hop 체인이다.
> 입력 라우팅 설계 전반의 문제라 별도 검토 사항으로 남긴다.

### View 측 규칙

- **BP 프로퍼티 바인딩은 금지되어 있다.** `Config/DefaultEditor.ini` 의
  `PropertyBindingRule=Prevent`. 반드시 ViewModel 바인딩을 쓴다.
- 자식 위젯은 부모가 호출해주는 게 아니라 **자기 Entry VM 을 관찰**한다.
  `BP_ItemSlot` 이 `UEWEQuickSlotItemViewModel::IsSelected` 에 바인딩하는 식.
- ViewModel 주입 이름은 위젯 안에 격리한다. 서브시스템은 BP 측 이름을 몰라야 한다.

> `IsSelected` 바인딩은 슬롯 변경 1회당 **두 번** 발화한다 (이전 슬롯 false, 새 슬롯 true).
> 하이라이트는 이게 맞지만, 사운드를 넣을 때는 `bSelected == true` 가드가 필요하다.

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
    ├── EWEWeaponData.h                 ← 무기 DataAsset
    └── EWEWeaponBase.h                 ← 액터 (현재 미사용)

Source/EWE/Character/
├── EWECharacter.h                      ← ASC / AttributeSet 소유, EquipWeapon
├── EWEPlayerController.h
└── EWEInventoryComponent.h
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
├── EWELocalUIManageSubsystem.h/.cpp   ← VM 소유 · 위젯 생성 · 명령 중계
├── EWEUIAsset.h/.cpp                  ← 위젯 클래스 참조 (비동기 로드)
│
├── EWEStatus.h/.cpp                   ← View
├── EWEStatusViewModel.h/.cpp          ← ASC 구독, HP/MP 노출
│
├── EWETargetInfo.h/.cpp               ← View
├── EWETargetInfoViewModel.h/.cpp      ← 타겟 ASC 구독
│
├── EWEQuickSlot.h/.cpp                ← View (슬롯 위젯 생성 · Entry VM 주입)
├── EWEQuickSlotViewModel.h/.cpp       ← 슬롯 배열 · 선택 인덱스 · OnSlotSelected
├── EWEQuickSlotItemViewModel.h/.cpp   ← 슬롯 1칸 (Weapon / bSelected / SlotIndex)
│
└── EWEInventory.h/.cpp                ← 인벤토리 위젯 (MVVM 미전환)

Content/UI/HUD/
├── BP_Status / BP_TargetInfo / BP_QuickSlot / BP_ItemSlot
```

`UEWEInventory` 는 아직 MVVM 이 아니다. 서브시스템을 직접 조회하는 경로가 남아 있다.
