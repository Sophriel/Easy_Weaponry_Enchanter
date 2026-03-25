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

## Accessing FGameplayAbilityActorInfo Inside GA

`FGameplayAbilityActorInfo*` is passed as a parameter to `ActivateAbility`, `EndAbility`, and other GA overrides.
Access its data through **direct member access**, not through Getter functions that do not exist on this struct.

```cpp
void UMyAbility::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    // ❌ This function does not exist on FGameplayAbilityActorInfo
    UAbilitySystemComponent* ASC = ActorInfo->GetAbilitySystemComponent();

    // ✅ Direct member access
    UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

    // ❌ This function does not exist on FGameplayAbilityActorInfo
    AActor* Avatar = ActorInfo->GetAvatarActor();

    // ✅ Direct member access (TWeakObjectPtr)
    AActor* Avatar = ActorInfo->AvatarActor.Get();
}
```

### FGameplayAbilityActorInfo Key Members

| Member | Type | Description |
|--------|------|-------------|
| `AbilitySystemComponent` | `TWeakObjectPtr<UAbilitySystemComponent>` | The ASC reference |
| `AvatarActor` | `TWeakObjectPtr<AActor>` | The physical actor executing abilities |
| `OwnerActor` | `TWeakObjectPtr<AActor>` | The actor that owns the ASC (e.g. PlayerState) |
| `PlayerController` | `TWeakObjectPtr<APlayerController>` | The player controller |
| `SkeletalMeshComponent` | `TWeakObjectPtr<USkeletalMeshComponent>` | The skeletal mesh component |
| `AnimInstance` | `TWeakObjectPtr<UAnimInstance>` | The anim instance |

All members are `TWeakObjectPtr` — use `.Get()` for the raw pointer or `.IsValid()` to check validity before access.

> **Note**: `UGameplayAbility` itself provides wrapper functions such as
> `GetAbilitySystemComponentFromActorInfo()` and `GetAvatarActorFromActorInfo()`.
> Inside GA member functions, these wrappers can be used instead of `ActorInfo->` direct access.
> However, these wrappers belong to `UGameplayAbility`, not to `FGameplayAbilityActorInfo` —
> do not call them on the `ActorInfo` pointer.

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

AbilityTasks (AT) are units for performing asynchronous work inside a GA.
Beyond built-in ATs (`UAbilityTask_WaitDelay`, `UAbilityTask_PlayMontageAndWait`, etc.), you can create custom ATs.

### Basic Structure

```cpp
UCLASS()
class UAbilityTask_SpawnProjectile : public UAbilityTask
{
    GENERATED_BODY()

public:
    // Factory function — entry point called from GA
    UFUNCTION(BlueprintCallable, Category = "AbilityTasks",
        meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility",
                BlueprintInternalUseOnly = "true"))
    static UAbilityTask_SpawnProjectile* CreateTask(
        UGameplayAbility* OwningAbility,
        TSubclassOf<AActor> ProjectileClass,
        FVector SpawnLocation,
        FRotator SpawnRotation);

    // Delegates — notify GA of completion/failure
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

### Implementation

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
    // Resource cleanup (cancel timers, unbind delegates, etc.)
    Super::OnDestroy(bInOwnerFinished);
}
```

### Using AbilityTask from GA

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
    Task->ReadyForActivation();  // Must be called for Activate() to execute
}
```

### AbilityTask Key Rules

- Create only via `NewAbilityTask<T>()` factory (do not use `NewObject` directly)
- Task remains pending until `ReadyForActivation()` is called
- When GA ends/cancels, active ATs are automatically cleaned up (`OnDestroy` called)
- A single GA can run multiple ATs simultaneously (parallel execution)
- Calling `EndTask()` inside an AT ends only that AT; the GA continues
- To make GA end when "all ATs complete", track with a counter in the GA

---

## Advanced GA Lifecycle

### End vs Cancel Semantics

| Function | Meaning | `OnDestroy` bInOwnerFinished | Follow-up |
|------|------|------------------------------|----------|
| `EndAbility()` | Normal end (duration expired, logic complete) | `true` | Recognized as a proper end event |
| `CancelAbility()` | Forced cancel (external interrupt, stun, weapon swap, etc.) | `false` | May not be recognized as a proper end event |

```cpp
// Normal end — bReplicateEndAbility, bWasCancelled
EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

// Forced cancel from outside
AbilitySystemComponent->CancelAbilityHandle(AbilitySpecHandle);

// Cancel all by tag
FGameplayTagContainer CancelTags;
CancelTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Attack")));
AbilitySystemComponent->CancelAbilities(&CancelTags);
```

### Distinguishing End/Cancel in Callbacks

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
        // Handle forced cancel (interrupt effects, cleanup, etc.)
    }
    else
    {
        // Handle normal end (grant rewards, trigger follow-up enchants, etc.)
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
```

### Dynamic GA Grant/Revoke

When adding/removing GAs at runtime (e.g. equipment swap):

```cpp
// Grant GA (server only)
FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(
    FGameplayAbilitySpec(AbilityClass, Level, INDEX_NONE, SourceObject));

// Store handle — needed for later removal
GrantedAbilityHandles.Add(Handle);

// Remove GA (server only)
ASC->ClearAbility(Handle);
```

Typical flow for weapon swap:

```cpp
void AMyCharacter::SwapWeapon(UWeaponDataAsset* NewWeapon)
{
    if (!HasAuthority()) return;

    // 1. Force cancel currently active GAs
    ASC->CancelAbilities(&CurrentWeaponAbilityTags);

    // 2. Remove previous weapon's GAs
    for (const FGameplayAbilitySpecHandle& Handle : CurrentWeaponAbilityHandles)
    {
        ASC->ClearAbility(Handle);
    }
    CurrentWeaponAbilityHandles.Empty();

    // 3. Grant new weapon's GAs
    for (const TSubclassOf<UGameplayAbility>& AbilityClass : NewWeapon->GrantedAbilities)
    {
        FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(
            FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
        CurrentWeaponAbilityHandles.Add(Handle);
    }
}
```

### GE Stacking (Aggregate)

Stacking policy when the same GE is applied multiple times (set in Blueprint or C++ GE constructor):

| Property | Description |
|------|------|
| `StackingType` | `AggregateBySource` (independent stacks per source) / `AggregateByTarget` (single stack on target) |
| `StackLimitCount` | Maximum stack count |
| `StackDurationRefreshPolicy` | `RefreshOnSuccessfulApplication` — refresh duration on each application |
| `StackExpirationPolicy` | `RemoveSingleStackAndRefreshDuration` — remove one stack at a time / `ClearEntireStack` — remove all |

```cpp
// C++ GE stacking configuration example
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

Network execution policy for GAs. Set in constructor:

| Policy | Server | Client | Use Case |
|------|------|-----------|----------|
| `LocalPredicted` | Executes | Predicted execution → server confirm | Responsiveness-critical actions (dash, jump) |
| `LocalOnly` | - | Executes | Client-only (UI effects, camera shake) |
| `ServerOnly` | Executes | - | Server-authoritative only (AI abilities, adjudication) |
| `ServerInitiated` | Initiates | Executes after server instruction | Server controls timing, both sides need execution |

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

### Server-Authoritative Pattern (No Client Prediction)

When client prediction is not needed (turn-based, server-confirm-first games):

```cpp
// Client → request activation from server
UFUNCTION(Server, Reliable)
void ServerRequestAbilityActivation(FGameplayTag AbilityTag);

void AMyCharacter::ServerRequestAbilityActivation_Implementation(FGameplayTag AbilityTag)
{
    // Server validates the request
    if (ValidateActivation(AbilityTag))
    {
        FGameplayTagContainer TagContainer;
        TagContainer.AddTag(AbilityTag);
        ASC->TryActivateAbilitiesByTag(TagContainer);

        // Notify client of result
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

### Client Prediction Pattern

Use `LocalPredicted` when responsiveness matters. Client executes first, server confirms:

```cpp
UGA_Dash::UGA_Dash()
{
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    // Consider InstancingPolicy since rollback is needed on misprediction
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}
```

Prediction caveats:
- GE application is predictable (`ApplyGameplayEffectSpecToSelf` auto-generates a prediction key)
- Actor spawning is not predictable — spawn on server only, replicate to client
- Position changes operate separately from CharacterMovementComponent's prediction system

### Replication Mode Selection

| Scenario | Recommended Mode | Reason |
|------|----------|------|
| Player character (small games) | `Full` | Replicates everything, easy to debug |
| Player character (mid-large games) | `Mixed` | Minimal ability/effect replication, saves bandwidth |
| AI / NPC | `Minimal` | Tags only replicated, rest handled by server |
| World objects (traps, installations) | `Minimal` | Server-authoritative only |

```cpp
// Set when creating the ASC
AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
```
