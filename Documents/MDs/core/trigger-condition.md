# Trigger & Condition System

> Trigger 타입 정의, Condition 판별 로직, 조합 규칙을 다룬다.

---

## Trigger 구조체

```cpp
USTRUCT(BlueprintType)
struct FEnchantTrigger
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    EEnchantTriggerType TriggerType = EEnchantTriggerType::OnInput;

    // OnInput / OnCombo 에서 사용
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TObjectPtr<UInputAction> InputAction;

    // OnCombo: 입력 조건과 AND 결합될 상태 조건
    // 예: HP 50% 이하 + RMB 입력
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TArray<FEnchantCondition> ComboStateConditions;
};
```

---

## Trigger 타입별 발동 규칙

### OnInput / OnCombo
```
기본 발동 키: LMB(좌클릭), RMB(우클릭)
추가 키 확장: 개발자가 InputMappingContext(IMC)를 직접 수정

OnCombo = OnInput + ComboStateConditions (AND)
  예: RMB를 눌렀을 때 + HP가 50% 이하인 경우
  → 입력이 먼저 들어오더라도 상태 조건이 불충족이면 발동 안 됨
```

### OnAttack
```
발동 시점: 다른 액터에게 히트 판정이 발생한 순간
           (입력 시점 아님, 피격 판정 완료 시점)
서버에서만 판정. 클라이언트에서 예측 발동 금지.
```

### OnHit
```
발동 시점: 자신이 피격 조건을 달성한 순간 (데미지 0도 포함)
다수 피격 시: 피격 횟수만큼 Trigger 발동

단, CooldownGE가 활성 중이면:
  → 클라이언트/서버 양쪽에서 차단
  → 피드백 없음

CooldownGE가 없는 인챈트: 피격 횟수 제한 없음
```

### OnCollision
```
발동 주체: 투사체(Projectile) 액터
발동 시점: 투사체가 다른 액터/오브젝트와 충돌한 순간
충돌 대상에게 OnCollisionGE 즉시 적용 가능
CollisionEnchantEvent가 있으면 추가 인챈트 이벤트 발동
```

### OnDeath
```
발동 주체: 자신(시전자)
HP가 0 이하로 떨어진 순간 서버에서 판정
사망 처리보다 Trigger 판정이 먼저 실행됨을 보장할 것
```

---

## Condition 구조체

```cpp
USTRUCT(BlueprintType)
struct FEnchantCondition
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    EEnchantConditionType ConditionType;

    // CountLimit 용
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    int32 MaxCount = 0;

    // ValueCondition 용
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FGameplayAttribute TargetAttribute;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    EConditionOperator Operator;  // LessThan, GreaterThan, Equal 등

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float Threshold = 0.f;

    // TagFilter 용
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FGameplayTagContainer RequiredTags;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FGameplayTagContainer BlockedTags;

    // PrecedingEnchantEnd 용
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    int32 PrecedingSlotIndex = -1;

    // SetEffect / Synergy 용
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FGameplayTagContainer RequiredSetTags;
};
```

---

## Condition 타입별 동작

### CountLimit
```
인챈트 발동 횟수가 MaxCount에 도달하면 이후 발동 차단.
카운터는 인챈트 인스턴스에 귀속.
무기 교체 시 카운터 초기화 여부: 인챈트 인스턴스와 함께 초기화.
```

### ValueCondition
```
캐릭터 AttributeSet의 특정 Attribute 값을 기준으로 판별.
Attribute가 AttributeSet에 없으면 Warning 출력 후 조건 false 처리.

예: HP < 50%
  TargetAttribute = Health
  Operator = LessThanPercent
  Threshold = 0.5f
```

### TagFilter
```
대상 액터의 GameplayTag를 기준으로 필터링.
RequiredTags: 대상이 모두 보유해야 발동
BlockedTags: 대상이 하나라도 보유하면 발동 차단

예시 태그 분류:
  Character.Player      플레이어 캐릭터
  Character.AI          AI 캐릭터
  Object.Destructible   파괴 가능 오브젝트
  Environment           환경 오브젝트
```

### PrecedingEnchantEnd
```
PrecedingSlotIndex 슬롯의 인챈트가 EndAbility를 완료한 시점에 발동.
CancelAbility는 트리거 조건으로 인정하지 않음.

슬롯 인덱스가 유효하지 않으면 Warning 출력 후 조건 무시.
대기 중인 인챈트는 해당 슬롯의 EndAbility 델리게이트를 구독.
```

### SetEffect / Synergy
```
RequiredSetTags를 모두 보유 중일 때만 발동.
세트 효과 및 시너지 조건으로 활용.
GameplayTagContainer 기반으로 판별.
```

---

## Condition 판별 흐름

```
인챈트 발동 시도
    │
    ▼
Conditions 배열 순회 (AND 결합)
    │
    ├─ 하나라도 false → 발동 거부
    │
    └─ 모두 true → 발동 허용
```

### Condition 판별 코드 패턴

```cpp
bool UEWEConditionEvaluator::EvaluateAll(
    const TArray<FEnchantCondition>& Conditions,
    const FEnchantContext& Context)
{
    for (const FEnchantCondition& Condition : Conditions)
    {
        if (!EvaluateSingle(Condition, Context))
        {
            return false;
        }
    }
    return true;
}
```

---

## Freeze / Time Stop 중 Trigger 발동 규칙

| 상태 | 발동 가능한 Trigger |
|------|-------------------|
| Freeze 중 | OnHit 계열만 발동 가능 |
| Freeze 중 | OnInput, OnCombo, OnAttack 차단 |
| Time Stop 당한 플레이어 | 모든 Trigger 차단 |
| Time Stop 동일 틱 패킷 | 후순위 Trigger도 처리 (Freeze 포함) |

### Silence 상태 이상 중 Trigger

```
Silence GE 활성 중: 모든 인챈트(GA) 발동 차단
단, OnHit Trigger 기반 인챈트는 Freeze와 동일하게 발동 가능
```

---

## Execution Counter (CountLimit 구현 주의사항)

```cpp
// Execution Counter는 GA 인스턴스에 귀속
// 동일 DataAsset 중복 부여 시 각각 독립 카운터를 가짐

UCLASS()
class UEWEGameplayAbility : public UGameplayAbility
{
    // ...
    int32 ExecutionCount = 0;  // 인스턴스 변수

    bool CanActivateAbility(...) const override
    {
        if (CountLimitCondition.MaxCount > 0 &&
            ExecutionCount >= CountLimitCondition.MaxCount)
        {
            return false;
        }
        return Super::CanActivateAbility(...);
    }
};
```
