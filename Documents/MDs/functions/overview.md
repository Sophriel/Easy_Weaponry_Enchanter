# Function System — Overview

> 새 Function을 추가하기 전에 반드시 이 문서를 먼저 읽을 것.
> 공통 컨텍스트 구조, IEnchantFunction 인터페이스, 파이프라인 통합 방법을 다룬다.

---

## Function이란

인챈트가 "무엇을 어떻게 실행하는가"를 정의하는 단위.
GA 내부에서 **AbilityTask**로 구현된다.

```
UEnchantDataAsset.Functions[]
    └─ FEnchantFunction (파라미터 래퍼)
           └─ UEWEAbilityTask_Base (AbilityTask, IEnchantFunction 구현체)
```

하나의 GA는 여러 Function을 가질 수 있으며, **기본적으로 동시 실행**된다.
GA는 **모든 Task가 완료된 시점**에 EndAbility를 호출한다.

---

## 공통 컨텍스트 구조 (FEnchantContext)

모든 Function이 공유하는 실행 컨텍스트.

```cpp
USTRUCT(BlueprintType)
struct FEnchantContext
{
    GENERATED_BODY()

    // 기준 액터 (시전자)
    UPROPERTY()
    TWeakObjectPtr<AActor> Who;

    // 실행 타이밍
    UPROPERTY()
    EFunctionTiming When;

    // 위치 기준
    UPROPERTY()
    EPositionBase Where;  // Self / Target / World

    // 대상
    UPROPERTY()
    FGameplayAbilityTargetDataHandle Target;

    // 실행 방식
    UPROPERTY()
    EFunctionMethod How;

    // 부모 무기 참조
    UPROPERTY()
    TWeakObjectPtr<AEWEWeaponActor> ParentWeapon;
};
```

---

## IEnchantFunction 인터페이스

**새 Function을 추가할 때 반드시 이 인터페이스를 구현할 것.**

```cpp
UINTERFACE(MinimalAPI, BlueprintType)
class UEnchantFunction : public UInterface
{
    GENERATED_BODY()
};

class EWERUNTIME_API IEnchantFunction
{
    GENERATED_BODY()

public:
    // 핵심 실행 메서드 — 반드시 오버라이드
    virtual void ExecuteFunction(const FEnchantContext& Context) = 0;

    // 완료 콜백 — Task 완료 시 반드시 호출
    virtual void NotifyCompleted() = 0;

    // 강제 중단 처리 (CancelAbility 시 호출됨)
    virtual void InterruptFunction() {}

    // 서버 전용 실행 여부 (기본 true)
    virtual bool ShouldExecuteOnServer() const { return true; }
};
```

---

## 새 Function 추가 절차

### 1. FEnchantFunction 파라미터 구조체에 타입 추가

```cpp
UENUM(BlueprintType)
enum class EEnchantFunctionType : uint8
{
    Creation,
    Ejection,
    Rotation,
    Attach,
    TimeStop,
    Freeze,
    Teleport,
    PossessObject,
    WeatherChange,
    StatusEffect,
    MyNewFunction,   // ← 새 타입 추가
};
```

### 2. AbilityTask 클래스 생성

```cpp
UCLASS()
class EWERUNTIME_API UEWEAbilityTask_MyNewFunction
    : public UEWEAbilityTask_Base
    , public IEnchantFunction
{
    GENERATED_BODY()

public:
    // 파라미터 (FEnchantFunction에서 전달받음)
    UPROPERTY()
    float MyParam = 0.f;

    // IEnchantFunction 구현
    virtual void ExecuteFunction(const FEnchantContext& Context) override;
    virtual void NotifyCompleted() override;
    virtual void InterruptFunction() override;

protected:
    virtual void Activate() override
    {
        ExecuteFunction(Context);
    }
};
```

### 3. FEnchantFunction 래퍼에 파라미터 추가

```cpp
USTRUCT(BlueprintType)
struct FEnchantFunction
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    EEnchantFunctionType FunctionType;

    // ... 기존 파라미터들 ...

    // MyNewFunction 파라미터
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        meta = (EditCondition = "FunctionType == EEnchantFunctionType::MyNewFunction"))
    float MyParam = 0.f;
};
```

### 4. Function Factory에 등록

```cpp
UEWEAbilityTask_Base* UEWEFunctionFactory::CreateTask(
    UGameplayAbility* OwningAbility,
    const FEnchantFunction& FuncData,
    const FEnchantContext& Context)
{
    switch (FuncData.FunctionType)
    {
        // ... 기존 케이스들 ...

        case EEnchantFunctionType::MyNewFunction:
        {
            auto* Task = UEWEAbilityTask_MyNewFunction::New<UEWEAbilityTask_MyNewFunction>(
                OwningAbility, NAME_None);
            Task->MyParam = FuncData.MyParam;
            Task->Context = Context;
            return Task;
        }
    }
    return nullptr;
}
```

### 5. Blueprint 노출 확인

```cpp
// 블루프린트에서 접근이 필요한 경우 UPROPERTY / UFUNCTION 스펙 확인
UFUNCTION(BlueprintCallable, Category = "EWE|Function")
void ExecuteFunction(const FEnchantContext& Context);
```

---

## Function 동시 실행 / 완료 처리

```
GA.ActivateAbility()
    │
    ├─ Task A → ReadyForActivation()   ┐
    ├─ Task B → ReadyForActivation()   ├─ 동시 실행
    └─ Task C → ReadyForActivation()   ┘
                    │
                    ├─ Task A 완료 → OnCompleted 호출
                    ├─ Task B 완료 → OnCompleted 호출
                    └─ Task C 완료 → OnCompleted 호출
                                        │
                                        └─ 모두 완료 → GA.EndAbility()
```

**개별 Task 완료 시 GA를 종료하면 안 된다. 반드시 모든 Task 완료 후 EndAbility.**

---

## 네트워크 주의사항

```
모든 Function의 ExecuteFunction()은 서버에서만 실행됨이 기본.
클라이언트에서 시각 효과(파티클, 사운드)가 필요하면:
  → Multicast RPC 또는 RepNotify를 사용해 클라이언트에 전달
  → ExecuteFunction() 내에서 직접 클라이언트 효과 재생 금지

예외: Blind, 붉은 테두리 같은 순수 로컬 UI 효과는 클라이언트 전용
```

---

## 각 Function 상세 문서

- **Creation, Ejection, Rotation, Attach** → `docs/functions/basic.md`
- **TimeStop, Freeze, Teleport, PossessObject, WeatherChange, StatusEffect** → `docs/functions/special.md`
