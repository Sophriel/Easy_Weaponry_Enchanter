# Architecture

> 모듈 구조, GAS 컴포넌트 배치, 플러그인 설정을 다룬다.
> 새 클래스를 만들거나 모듈 의존성을 건드리기 전에 반드시 읽을 것.
>
> **일반 UE 플러그인 개발 패턴** (Runtime/Editor 분리, Build.cs, WITH_EDITOR 등)은
> `/unreal-engine` 스킬 → `references/plugin_development.md` 참조.

---

## 플러그인 모듈 구조

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

### 모듈 의존성

```
메인 게임 모듈 (EWE):
  PublicDependencyModuleNames:  Core, CoreUObject, Engine, InputCore, EnhancedInput,
                                EasyWeaponryEnchanter, GameplayAbilities, GameplayTasks, GameplayTags

플러그인 런타임 (EWERuntime):
  PublicDependencyModuleNames:  GameplayAbilities, EnhancedInput, CoreUObject, Engine
  PrivateDependencyModuleNames: NetCore

플러그인 에디터 (EWEEditor):
  PublicDependencyModuleNames:  EWERuntime, UnrealEd, PropertyEditor
```

에디터 전용 로직을 EWERuntime에 두면 쿠킹 시 오류 발생. 반드시 EWEEditor로 분리.

---

## GAS 컴포넌트 배치

> **ASC 초기화 일반 패턴** (PossessedBy / OnRep_PlayerState / PlayerState 배치)은
> `/unreal-engine` 스킬 → `references/gameplay_ability_system.md` → "Initialize the ASC" 참조.

### 소유 구조 (변경 금지)

```
ACharacter (플레이어 캐릭터)
├── UAbilitySystemComponent (ASC)        ← 캐릭터 소유
└── UEWEAttributeSet                     ← 캐릭터 소유
```

EWE에서는 ASC를 **캐릭터에 직접 배치**한다 (PlayerState 아님).
무기(`AEWEWeaponActor`)는 ASC를 소유하지 않는다.
무기가 교체될 때마다 캐릭터 ASC에 GA가 등록/해제된다.

---

## AttributeSet 설계

> **AttributeSet 생성, ATTRIBUTE_ACCESSORS 매크로, Replication 일반 패턴**은
> `/unreal-engine` 스킬 → `references/gameplay_ability_system.md` → "Creating Attributes" 참조.

### UEWEAttributeSet 필수 Attribute 목록

| Attribute | 용도 | 비고 |
|-----------|------|------|
| `Health` | 현재 HP | 무기 교체 시 유지 |
| `MaxHealth` | 최대 HP | 무기 기본값으로 재설정 |
| `MoveSpeed` | 이동 속도 | 무기 기본값으로 재설정 |
| `AttackPower` | 공격력 | 무기 기본값으로 재설정 |

무기가 사용하는 모든 Attribute는 이 AttributeSet에 반드시 존재해야 한다.
존재하지 않는 Attribute를 DataAsset에서 참조하면 Warning 출력 후 무시.

### 무기 교체 시 AttributeSet 재설정

```cpp
void AEWECharacterBase::ResetAttributesForWeapon(const UWeaponDataAsset* WeaponData)
{
    // Health는 재설정 대상 아님
    float PrevHealth = AttributeSet->GetHealth();
    float NewMaxHealth = WeaponData->DefaultAttributes.MaxHealth;

    // MaxHealth 재설정
    ApplyWeaponDefaultGE(WeaponData);  // GE로 기본값 적용

    // 현재 HP가 새 MaxHP를 초과하면 클램프
    if (PrevHealth > NewMaxHealth)
    {
        // GE로 Health를 NewMaxHealth로 조정
    }

    // Position, 속도는 건드리지 않음
}
```

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
                    3. AttributeSet 재설정 (HP 클램프 포함)
                    4. 신규 무기 GA를 ASC에 등록
                    5. 클라이언트에 Confirm 브로드캐스트
```

### GA 등록/해제 코드 패턴

```cpp
// 무기 GA 해제
void AEWECharacterBase::ClearWeaponAbilities()
{
    for (FGameplayAbilitySpecHandle Handle : ActiveWeaponAbilityHandles)
    {
        ASC->CancelAbilityHandle(Handle);
        ASC->ClearAbility(Handle);
    }
    ActiveWeaponAbilityHandles.Empty();
}

// 무기 GA 등록
void AEWECharacterBase::GiveWeaponAbilities(const UWeaponDataAsset* WeaponData)
{
    for (const UEnchantDataAsset* Enchant : WeaponData->Enchants)
    {
        FGameplayAbilitySpec Spec(Enchant->AbilityClass, 1);
        FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
        ActiveWeaponAbilityHandles.Add(Handle);
    }
}
```

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

## 주요 클래스 파일 위치 (권장)

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

> ⚠️ 위 트리는 **목표 구조**다. 현재 플러그인 소스는
> `Plugins/EasyWeaponryEnchanter/Source/EasyWeaponryEnchanter/` 아래에 있고
> 일부 클래스명도 다르다 (`AEWECharacterBase` → `AEWECharacter` 등).
> 이 절과 위쪽 "플러그인 모듈 구조" 절은 실제 코드와 대조가 필요하다.

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
