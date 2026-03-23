# Architecture

> 모듈 구조, GAS 컴포넌트 배치, 플러그인 설정을 다룬다.
> 새 클래스를 만들거나 모듈 의존성을 건드리기 전에 반드시 읽을 것.

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
EWERuntime:
  PublicDependencyModuleNames:  GameplayAbilities, EnhancedInput, CoreUObject, Engine
  PrivateDependencyModuleNames: NetCore

EWEEditor:
  PublicDependencyModuleNames:  EWERuntime, UnrealEd, PropertyEditor
```

### 에디터 코드 분리 규칙

```cpp
// 런타임 클래스 내 에디터 전용 코드는 반드시 이 매크로로 감쌀 것
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& e) override;
#endif
```

에디터 전용 로직을 EWERuntime에 두면 쿠킹 시 오류 발생. 반드시 EWEEditor로 분리.

---

## GAS 컴포넌트 배치

### 소유 구조 (변경 금지)

```
ACharacter (플레이어 캐릭터)
├── UAbilitySystemComponent (ASC)        ← 캐릭터 소유
└── UEWEAttributeSet                     ← 캐릭터 소유
```

무기(`AEWEWeaponActor`)는 ASC를 소유하지 않는다.
무기가 교체될 때마다 캐릭터 ASC에 GA가 등록/해제된다.

### ASC 초기화 위치

```cpp
// ACharacter::BeginPlay 또는 PossessedBy 에서 초기화
void AEWECharacterBase::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, this);
    }
}

// 클라이언트는 OnRep_PlayerState 또는 AcknowledgePossession 에서
void AEWECharacterBase::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    AbilitySystemComponent->InitAbilityActorInfo(this, this);
}
```

---

## AttributeSet 설계

### UEWEAttributeSet 필수 Attribute 목록

| Attribute | 용도 | 비고 |
|-----------|------|------|
| `Health` | 현재 HP | 무기 교체 시 유지 |
| `MaxHealth` | 최대 HP | 무기 기본값으로 재설정 |
| `MoveSpeed` | 이동 속도 | 무기 기본값으로 재설정 |
| `AttackPower` | 공격력 | 무기 기본값으로 재설정 |

무기가 사용하는 모든 Attribute는 이 AttributeSet에 반드시 존재해야 한다.
존재하지 않는 Attribute를 DataAsset에서 참조하면 Warning 출력 후 무시.

### Attribute 접근 패턴

```cpp
// 읽기
float CurrentHP = AttributeSet->GetHealth();

// 쓰기는 반드시 GE를 통해 (직접 세터 사용 금지)
// AttributeSet->SetHealth(100.f);  ← 금지
```

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
