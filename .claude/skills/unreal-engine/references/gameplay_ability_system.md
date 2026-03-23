# Gameplay Ability System (GAS) Reference

## Overview

The Gameplay Ability System is a framework for building abilities, attributes, and effects in gameplay-driven projects (RPGs, MOBAs, action games). Load this reference when working with GAS components, abilities, attributes, or effects.

## Core Components

### Ability System Component (ASC)

The heart of GAS. Acts as the hub for all abilities, attributes, and effects.

**Key responsibilities**:
- Manages granted abilities
- Stores and replicates attributes
- Applies and tracks gameplay effects
- Handles gameplay tags
- Manages ability activation

**Where to place ASC**:

| Scenario | ASC Placement |
|----------|--------------|
| Single-player / Listen-server | Character |
| Dedicated server (player-owned) | PlayerState |
| AI / NPC | Character or custom Actor |

> **Why PlayerState for dedicated server?** The ASC needs to live on an actor that persists through respawn. PlayerState survives respawn; Character does not.

### Key GAS Classes

| Class | Role |
|-------|------|
| `UAbilitySystemComponent` | Core component — lives on Character or PlayerState |
| `UGameplayAbility` | Base class for abilities (dash, attack, heal…) |
| `UAttributeSet` | Holds numeric attributes (Health, Stamina, Mana…) |
| `UGameplayEffect` | Modifies attributes (damage, regen, cost…) |
| `FGameplayTag` | Tag-based identification for abilities and effects |

## Setup Checklist

Before using GAS, verify in `.uproject`:
```bash
grep -i "GameplayAbilities" *.uproject
```

---

## Initial Setup

### 1. Enable Plugin & Dependencies

**.uproject**:
```json
{
  "Plugins": [
    {"Name": "GameplayAbilities", "Enabled": true}
  ]
}
```

**ProjectName.Build.cs**:
```csharp
PublicDependencyModuleNames.AddRange(new string[] {
    "Core", 
    "CoreUObject", 
    "Engine", 
    "InputCore",
    "GameplayAbilities",  // ← Core GAS module
    "GameplayTags",       // ← Tag system
    "GameplayTasks"       // ← Async task system
});
```

### 2. Create Ability System Component

**In Character class** (for single-player/listen server):

```cpp
// MyCharacter.h
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"

UCLASS()
class AMyCharacter : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AMyCharacter();

    // IAbilitySystemInterface
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
    
    UPROPERTY()
    TObjectPtr<UAttributeSet> AttributeSet;
};
```

```cpp
// MyCharacter.cpp
AMyCharacter::AMyCharacter()
{
    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
    
    AttributeSet = CreateDefaultSubobject<UMyAttributeSet>(TEXT("AttributeSet"));
}

UAbilitySystemComponent* AMyCharacter::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}
```

**In PlayerState** (for dedicated server):

```cpp
// MyPlayerState.h
#include "AbilitySystemInterface.h"

UCLASS()
class AMyPlayerState : public APlayerState, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AMyPlayerState();
    
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
    
    UPROPERTY()
    TObjectPtr<UAttributeSet> AttributeSet;
};
```

### 3. Initialize the ASC

**Critical**: Must call `InitAbilityActorInfo` to link Owner and Avatar:

```cpp
void AMyCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    
    // Server: Initialize ability actor info
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, this);
    }
}

void AMyCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    
    // Client: Initialize ability actor info
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, this);
    }
}
```

**If ASC is on PlayerState**:
```cpp
// In Character
void AMyCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    
    if (AMyPlayerState* PS = GetPlayerState<AMyPlayerState>())
    {
        // Owner = PlayerState, Avatar = Character
        PS->GetAbilitySystemComponent()->InitAbilityActorInfo(PS, this);
    }
}
```

### Initialization Order (Dedicated Server)

```
Server: PossessedBy()
    └─> InitAbilityActorInfo(PlayerState, Character)
    └─> GiveDefaultAbilities()
    └─> InitializeAttributes()

Client: OnRep_PlayerState()
    └─> InitAbilityActorInfo(PlayerState, Character)
```

## Creating Attributes

### Attribute Set Class

```cpp
// MyAttributeSet.h
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"

UCLASS()
class UMyAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UMyAttributeSet();

    // Attribute accessors - use macros for boilerplate
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Health)
    FGameplayAttributeData Health;
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, Health)

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxHealth)
    FGameplayAttributeData MaxHealth;
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, MaxHealth)

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Mana)
    FGameplayAttributeData Mana;
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, Mana)

    // Replication
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    // Rep notifies
    UFUNCTION()
    virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);
    
    UFUNCTION()
    virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);
    
    UFUNCTION()
    virtual void OnRep_Mana(const FGameplayAttributeData& OldMana);
    
    // Pre/Post attribute change
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
};
```

```cpp
// MyAttributeSet.cpp
#include "Net/UnrealNetwork.h"

UMyAttributeSet::UMyAttributeSet()
{
    // Set default values
    InitHealth(100.0f);
    InitMaxHealth(100.0f);
    InitMana(50.0f);
}

void UMyAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UMyAttributeSet, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMyAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMyAttributeSet, Mana, COND_None, REPNOTIFY_Always);
}

void UMyAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMyAttributeSet, Health, OldHealth);
}

void UMyAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
    
    // Clamp health to [0, MaxHealth]
    if (Attribute == GetHealthAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
    }
}

void UMyAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);
    
    // Handle attribute changes (e.g., death when health reaches 0)
    if (Data.EvaluatedData.Attribute == GetHealthAttribute())
    {
        SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
        
        if (GetHealth() <= 0.0f)
        {
            // Handle death
        }
    }
}
```

## Creating Gameplay Abilities

### Basic Ability Class

```cpp
// MyGameplayAbility.h
#include "Abilities/GameplayAbility.h"

UCLASS()
class UMyGameplayAbility : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UMyGameplayAbility();

protected:
    // Called when ability is activated
    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData
    ) override;
    
    // Called when ability ends
    virtual void EndAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility,
        bool bWasCancelled
    ) override;
};
```

```cpp
// MyGameplayAbility.cpp
void UMyGameplayAbility::ActivateAbility(...)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    // Ability logic here
    // ...
    
    // End ability when done (for instant abilities)
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
```

### Granting Abilities

**At runtime** (usually in BeginPlay or on possession):

```cpp
void AMyCharacter::GiveAbilities()
{
    if (!HasAuthority() || !AbilitySystemComponent)
        return;
    
    for (TSubclassOf<UGameplayAbility>& StartupAbility : DefaultAbilities)
    {
        AbilitySystemComponent->GiveAbility(
            FGameplayAbilitySpec(StartupAbility, 1, INDEX_NONE, this)
        );
    }
}
```

**In header**:
```cpp
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abilities")
TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;
```

## Activating Abilities

### By Class

```cpp
AbilitySystemComponent->TryActivateAbilityByClass(AbilityClass);
```

### By Tag

```cpp
FGameplayTagContainer TagContainer;
TagContainer.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Attack.Melee")));
AbilitySystemComponent->TryActivateAbilitiesByTag(TagContainer);
```

### From Input

```cpp
// Bind ability to input
AbilitySystemComponent->BindAbilityActivationToInputComponent(
    InputComponent,
    FGameplayAbilityInputBinds(
        "ConfirmInput",
        "CancelInput",
        "EAbilityInputID"  // Your input enum
    )
);

// Trigger via input ID
void AMyCharacter::OnAbilityInputPressed(int32 InputID)
{
    AbilitySystemComponent->AbilityLocalInputPressed(InputID);
}
```

## Gameplay Effects

### Creating a Gameplay Effect

**Blueprint/Data Asset** (most common):
1. Right-click in Content Browser
2. Blueprint Class → GameplayEffect
3. Configure:
   - Duration Policy: Instant, Duration, Infinite
   - Modifiers: Which attributes to modify
   - Magnitude: How much to modify

**C++ class** (for complex logic):
```cpp
UCLASS()
class UGE_DamageEffect : public UGameplayEffect
{
    GENERATED_BODY()

public:
    UGE_DamageEffect();
};
```

### Applying Gameplay Effects

```cpp
// Create effect context
FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
EffectContext.AddSourceObject(this);

// Create effect spec
FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
    DamageEffectClass,
    1, // Level
    EffectContext
);

if (SpecHandle.IsValid())
{
    // Apply to target
    FActiveGameplayEffectHandle ActiveHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(
        *SpecHandle.Data.Get(),
        TargetAbilitySystemComponent
    );
    
    // Or apply to self
    AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}
```

## Gameplay Tags

### Setup Gameplay Tags

**Config/DefaultGameplayTags.ini**:
```ini
[/Script/GameplayTags.GameplayTagsSettings]
+GameplayTagList=(Tag="Ability.Attack.Melee",DevComment="Melee attack ability")
+GameplayTagList=(Tag="Ability.Attack.Ranged",DevComment="Ranged attack ability")
+GameplayTagList=(Tag="Status.Stunned",DevComment="Character is stunned")
+GameplayTagList=(Tag="Status.Dead",DevComment="Character is dead")
```

### Using Tags in Abilities

```cpp
// In ability class constructor
AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Attack.Melee")));
ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Status.Attacking")));
BlockAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag(FName("Status.Stunned")));
```

### Tag Queries

```cpp
// Check if ASC has tag
bool bHasTag = AbilitySystemComponent->HasMatchingGameplayTag(
    FGameplayTag::RequestGameplayTag(FName("Status.Stunned"))
);

// Add/remove tags
FGameplayTagContainer TagsToAdd;
TagsToAdd.AddTag(FGameplayTag::RequestGameplayTag(FName("Status.Invincible")));
AbilitySystemComponent->AddLooseGameplayTags(TagsToAdd);

AbilitySystemComponent->RemoveLooseGameplayTag(
    FGameplayTag::RequestGameplayTag(FName("Status.Invincible"))
);
```

## Replication Modes

Set on ASC via `SetReplicationMode()`:

- **Full**: Replicates everything (expensive, for player characters in small games)
- **Mixed**: Replicates abilities, effects minimally (player characters in most games)
- **Minimal**: Only replicates gameplay tags (AI, NPCs, simulated proxies)

```cpp
AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
```

## Common Patterns

### Listening for Attribute Changes

```cpp
// Bind in BeginPlay
AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
    UMyAttributeSet::GetHealthAttribute()
).AddUObject(this, &AMyCharacter::OnHealthChanged);

void AMyCharacter::OnHealthChanged(const FOnAttributeChangeData& Data)
{
    // Data.NewValue, Data.OldValue
    if (Data.NewValue <= 0.f) { Die(); }
}
```

### Damage System

```cpp
// Attacker applies damage effect to target
void DealDamage(AActor* Target, float DamageAmount)
{
    if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Target))
    {
        UAbilitySystemComponent* TargetASC = ASI->GetAbilitySystemComponent();
        
        FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
        EffectContext.AddSourceObject(this);
        
        FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
            DamageEffectClass,
            1,
            EffectContext
        );
        
        if (SpecHandle.IsValid())
        {
            // Set damage magnitude
            SpecHandle.Data.Get()->SetSetByCallerMagnitude(
                FGameplayTag::RequestGameplayTag(FName("Data.Damage")),
                DamageAmount
            );
            
            AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(
                *SpecHandle.Data.Get(),
                TargetASC
            );
        }
    }
}
```

### Cooldowns

Use `UGameplayAbility::ApplyCooldown()` or set cooldown gameplay effect in ability:

```cpp
// In ability class
UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
FScalableFloat CooldownDuration = 5.0f;

UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
FGameplayTagContainer CooldownTags;
```

### Costs (Mana, Stamina)

Use `UGameplayAbility::ApplyCost()` or set cost gameplay effect:

```cpp
// In ability class
UPROPERTY(EditDefaultsOnly, Category = "Cost")
TSubclassOf<UGameplayEffect> CostGameplayEffectClass;
```

## Debugging GAS

### Console Commands

```
showdebug abilitysystem  // Show abilities, effects, tags, attributes
AbilitySystem.Debug.NextTarget  // Cycle debug target to next actor
```

### Logging

```cpp
// Enable GAS logging
LogAbilitySystem.SetVerbosity(ELogVerbosity::VeryVerbose);
```

## Best Practices

1. **Authority checks**: GAS only works on server. Check `HasAuthority()` before granting/activating
2. **Init ASC early**: Call `InitAbilityActorInfo` in `PossessedBy` and `OnRep_PlayerState`
3. **Use tags liberally**: Tags are lightweight and powerful for blocking/allowing abilities
4. **Attribute clamping**: Clamp in `PreAttributeChange` and `PostGameplayEffectExecute`
5. **Replication mode**: Use Minimal for AI/NPCs, Mixed for players
6. **Owner vs Avatar**: Understand difference (Owner has ASC, Avatar is controlled actor)

## Example Projects

- **Lyra** (Epic Games) - Full GAS implementation in action game
- **ActionRPG** (Epic Games) - Simplified GAS for RPG
- **GASDocumentation** (tranek on GitHub) - Comprehensive community documentation
- **Valley of the Ancient** (Epic Games) - Advanced GAS usage

## Common Issues

**Ability not activating**:
- Check authority (server-only)
- Verify `CommitAbility()` succeeds (costs, cooldowns, tags)
- Ensure ASC is initialized

**Attributes not replicating**:
- Add `DOREPLIFETIME_CONDITION_NOTIFY` in `GetLifetimeReplicatedProps`
- Implement OnRep functions
- Set ASC replication mode

**Tags not working**:
- Initialize tags in GameplayTags.ini
- Grant tags via effects or ASC methods
- Check tag queries use correct hierarchical names

---

## Custom AbilityTask

AbilityTask(AT)는 GA 내부에서 비동기 작업을 수행하는 단위이다.
엔진 내장 AT(`UAbilityTask_WaitDelay`, `UAbilityTask_PlayMontageAndWait` 등) 외에 커스텀 AT를 만들 수 있다.

### 기본 구조

```cpp
UCLASS()
class UAbilityTask_SpawnProjectile : public UAbilityTask
{
    GENERATED_BODY()

public:
    // 팩토리 함수 — GA에서 호출하는 진입점
    UFUNCTION(BlueprintCallable, Category = "AbilityTasks",
        meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility",
                BlueprintInternalUseOnly = "true"))
    static UAbilityTask_SpawnProjectile* CreateTask(
        UGameplayAbility* OwningAbility,
        TSubclassOf<AActor> ProjectileClass,
        FVector SpawnLocation,
        FRotator SpawnRotation);

    // 델리게이트 — 완료/실패를 GA에 알림
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectileSpawned, AActor*, SpawnedActor);

    UPROPERTY(BlueprintAssignable)
    FOnProjectileSpawned OnSpawned;

    UPROPERTY(BlueprintAssignable)
    FOnProjectileSpawned OnFailed;

protected:
    virtual void Activate() override;
    virtual void OnDestroy(bool bInOwnerFinished) override;

private:
    UPROPERTY()
    TSubclassOf<AActor> ProjectileClass;

    FVector Location;
    FRotator Rotation;
};
```

### 구현

```cpp
UAbilityTask_SpawnProjectile* UAbilityTask_SpawnProjectile::CreateTask(
    UGameplayAbility* OwningAbility,
    TSubclassOf<AActor> InClass,
    FVector InLocation,
    FRotator InRotation)
{
    UAbilityTask_SpawnProjectile* Task = NewAbilityTask<UAbilityTask_SpawnProjectile>(OwningAbility);
    Task->ProjectileClass = InClass;
    Task->Location = InLocation;
    Task->Rotation = InRotation;
    return Task;
}

void UAbilityTask_SpawnProjectile::Activate()
{
    if (!Ability || !AbilitySystemComponent.IsValid())
    {
        OnFailed.Broadcast(nullptr);
        EndTask();
        return;
    }

    UWorld* World = GetWorld();
    if (World && ProjectileClass)
    {
        FActorSpawnParameters Params;
        Params.Owner = GetAvatarActor();
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        AActor* Spawned = World->SpawnActor<AActor>(ProjectileClass, Location, Rotation, Params);
        OnSpawned.Broadcast(Spawned);
    }
    else
    {
        OnFailed.Broadcast(nullptr);
    }

    EndTask();
}

void UAbilityTask_SpawnProjectile::OnDestroy(bool bInOwnerFinished)
{
    // 리소스 정리 (타이머 해제, 델리게이트 해제 등)
    Super::OnDestroy(bInOwnerFinished);
}
```

### GA에서 AbilityTask 사용

```cpp
void UGA_FireWeapon::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    UAbilityTask_SpawnProjectile* Task = UAbilityTask_SpawnProjectile::CreateTask(
        this, ProjectileClass,
        GetAvatarActor()->GetActorLocation(),
        GetAvatarActor()->GetActorRotation());

    Task->OnSpawned.AddDynamic(this, &UGA_FireWeapon::OnProjectileSpawned);
    Task->OnFailed.AddDynamic(this, &UGA_FireWeapon::OnProjectileFailed);
    Task->ReadyForActivation();  // 반드시 호출해야 Activate() 실행됨
}
```

### AbilityTask 핵심 규칙

- `NewAbilityTask<T>()` 팩토리로만 생성 (`NewObject` 직접 사용 금지)
- `ReadyForActivation()` 호출 전까지 Task는 대기 상태
- GA가 End/Cancel되면 활성 AT도 자동 정리 (`OnDestroy` 호출)
- 하나의 GA에서 여러 AT를 동시에 실행할 수 있음 (병렬 실행)
- AT 내부에서 `EndTask()`를 호출하면 해당 AT만 종료, GA는 계속 진행
- GA 종료 시점을 "모든 AT 완료"로 만들려면 GA에서 카운터로 추적 필요

---

## Advanced GA Lifecycle

### End vs Cancel 시맨틱

| 함수 | 의미 | `OnDestroy` bInOwnerFinished | 후속 로직 |
|------|------|------------------------------|----------|
| `EndAbility()` | 정상 종료 (지속시간 만료, 로직 완료) | `true` | 종료 이벤트로 인정 |
| `CancelAbility()` | 강제 취소 (외부 인터럽트, 스턴, 무기 교체 등) | `false` | 종료 이벤트로 미인정될 수 있음 |

```cpp
// 정상 종료 — bReplicateEndAbility, bWasCancelled
EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

// 외부에서 강제 취소
AbilitySystemComponent->CancelAbilityHandle(AbilitySpecHandle);

// 태그 기반 일괄 취소
FGameplayTagContainer CancelTags;
CancelTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Attack")));
AbilitySystemComponent->CancelAbilities(&CancelTags);
```

### End/Cancel 콜백 구분

```cpp
void UMyAbility::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (bWasCancelled)
    {
        // 강제 취소 시 처리 (이펙트 중단, 정리 등)
    }
    else
    {
        // 정상 종료 시 처리 (보상 지급, 후속 인챈트 트리거 등)
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
```

### GA 동적 등록/해제

장비 교체 등으로 GA를 런타임에 추가/제거할 때:

```cpp
// GA 부여 (서버에서만)
FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(
    FGameplayAbilitySpec(AbilityClass, Level, INDEX_NONE, SourceObject));

// Handle 저장 — 나중에 제거할 때 필요
GrantedAbilityHandles.Add(Handle);

// GA 제거 (서버에서만)
ASC->ClearAbility(Handle);
```

무기 교체 시 전형적 흐름:

```cpp
void AMyCharacter::SwapWeapon(UWeaponDataAsset* NewWeapon)
{
    if (!HasAuthority()) return;

    // 1. 현재 활성 GA 강제 Cancel
    ASC->CancelAbilities(&CurrentWeaponAbilityTags);

    // 2. 이전 무기의 GA들 제거
    for (const FGameplayAbilitySpecHandle& Handle : CurrentWeaponAbilityHandles)
    {
        ASC->ClearAbility(Handle);
    }
    CurrentWeaponAbilityHandles.Empty();

    // 3. 새 무기의 GA들 부여
    for (const TSubclassOf<UGameplayAbility>& AbilityClass : NewWeapon->GrantedAbilities)
    {
        FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(
            FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
        CurrentWeaponAbilityHandles.Add(Handle);
    }
}
```

### GE Stacking (Aggregate)

동일 GE가 중복 적용될 때의 스태킹 정책 (블루프린트 또는 C++ GE 생성자에서 설정):

| 속성 | 설명 |
|------|------|
| `StackingType` | `AggregateBySource` (소스별 독립 스택) / `AggregateByTarget` (대상 기준 단일 스택) |
| `StackLimitCount` | 최대 스택 수 |
| `StackDurationRefreshPolicy` | `RefreshOnSuccessfulApplication` — 적용 시마다 지속시간 갱신 |
| `StackExpirationPolicy` | `RemoveSingleStackAndRefreshDuration` — 스택 1개씩 소멸 / `ClearEntireStack` — 전체 소멸 |

```cpp
// C++에서 GE 스태킹 설정 예시
UGE_BurnEffect::UGE_BurnEffect()
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FScalableFloat(5.f);

    StackingType = EGameplayEffectStackingType::AggregateByTarget;
    StackLimitCount = 5;
    StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
    StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::RemoveSingleStackAndRefreshDuration;
}
```

---

## Network Patterns for GAS

### NetExecutionPolicy

GA의 네트워크 실행 정책. 생성자에서 설정:

| 정책 | 서버 | 클라이언트 | 사용 시점 |
|------|------|-----------|----------|
| `LocalPredicted` | 실행 | 예측 실행 → 서버 확인 | 반응성이 중요한 액션 (대시, 점프) |
| `LocalOnly` | - | 실행 | 클라이언트 전용 (UI 이펙트, 카메라 셰이크) |
| `ServerOnly` | 실행 | - | 서버 권한 전용 (AI 어빌리티, 판정) |
| `ServerInitiated` | 실행 시작 | 서버 지시 후 실행 | 서버가 타이밍 제어하되 양쪽 실행 필요 |

```cpp
UCLASS()
class UGA_ServerAuthoritative : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_ServerAuthoritative()
    {
        NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    }
};
```

### Server-Authoritative 패턴 (클라이언트 예측 없음)

클라이언트 예측이 필요 없는 경우 (턴제, 서버 확인 우선 게임):

```cpp
// 클라이언트 → 서버에 발동 요청
UFUNCTION(Server, Reliable)
void ServerRequestAbilityActivation(FGameplayTag AbilityTag);

void AMyCharacter::ServerRequestAbilityActivation_Implementation(FGameplayTag AbilityTag)
{
    // 서버에서 유효성 판정
    if (ValidateActivation(AbilityTag))
    {
        FGameplayTagContainer TagContainer;
        TagContainer.AddTag(AbilityTag);
        ASC->TryActivateAbilitiesByTag(TagContainer);

        // 클라이언트에 결과 통보
        ClientConfirmActivation(AbilityTag);
    }
    else
    {
        ClientRejectActivation(AbilityTag);
    }
}

UFUNCTION(Client, Reliable)
void ClientConfirmActivation(FGameplayTag AbilityTag);

UFUNCTION(Client, Reliable)
void ClientRejectActivation(FGameplayTag AbilityTag);
```

### Client Prediction 패턴

반응성이 중요한 경우 `LocalPredicted`를 사용. 클라이언트가 먼저 실행하고 서버가 확인:

```cpp
UGA_Dash::UGA_Dash()
{
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    // 예측 실패 시 롤백이 필요하므로 InstancingPolicy도 고려
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}
```

예측 시 주의사항:
- GE 적용은 예측 가능 (`ApplyGameplayEffectSpecToSelf`가 예측 키를 자동 생성)
- 액터 스폰은 예측 불가 — 서버에서만 스폰 후 클라이언트에 복제
- 위치 변경은 CharacterMovementComponent의 예측 시스템과 별도로 동작

### Replication Mode 선택

| 상황 | 추천 모드 | 이유 |
|------|----------|------|
| 플레이어 캐릭터 (소규모 게임) | `Full` | 모든 정보 복제, 디버깅 용이 |
| 플레이어 캐릭터 (중대형 게임) | `Mixed` | 어빌리티/이펙트 최소 복제로 대역폭 절약 |
| AI / NPC | `Minimal` | 태그만 복제, 나머지는 서버 처리 |
| 월드 오브젝트 (트랩, 설치물) | `Minimal` | 서버 권한으로만 동작 |

```cpp
// ASC 생성 시 설정
AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
```
