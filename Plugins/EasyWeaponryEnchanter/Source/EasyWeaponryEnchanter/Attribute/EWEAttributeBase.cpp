// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEAttributeBase.h"
#include "EasyWeaponryEnchanter.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UEWEAttributeBase::UEWEAttributeBase()
{
    // Should be initialized with Default GE.
    InitMaxHealth(1.f);
    InitMaxMana(0.f);
    InitHealth(0.f);
    InitMana(0.f);
}

void UEWEAttributeBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UEWEAttributeBase, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UEWEAttributeBase, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UEWEAttributeBase, Mana, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UEWEAttributeBase, MaxMana, COND_None, REPNOTIFY_Always);
}

void UEWEAttributeBase::PreAttributeBaseChange(const FGameplayAttribute &Attribute, float &NewValue) const
{
    Super::PreAttributeBaseChange(Attribute, NewValue);
    ClampAttribute(Attribute, NewValue);
}

void UEWEAttributeBase::PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);

    // Clamps the aggregated current value only; base value and modifiers are preserved (non-destructive).
    ClampAttribute(Attribute, NewValue);

    // Example: range-clamp a derived stat like MoveSpeed (not defined in this class).
    //
    // else if (Attribute == GetMoveSpeedAttribute())
    // {
    //     NewValue = FMath::Clamp(NewValue, 100.f, 1000.f);
    // }
}

// Clamp layer design:
// - The current value is recorded at a single point, FGameplayAttribute::SetNumericValueChecked
//   (Pre -> SetCurrentValue -> Post). The only place to modify the value before it is recorded is
//   Pre. Re-writing it in Post (SetHealth, ApplyModToAttribute) all change the Base, which
//   permanently damages the Base and re-enters when the dependent has Duration/Infinite modifiers.
// - Pre clamping only reduces the aggregator result, preserving Base and modifiers (non-destructive).
// Layers:
//   PreAttributeBaseChange = Base range
//   PreAttributeChange     = Current range (non-destructive)
//   PostAttributeChange    = intentional correction of the dependent Base when its Max changes
void UEWEAttributeBase::PostAttributeChange(const FGameplayAttribute &Attribute, float OldValue, float NewValue)
{
    Super::PostAttributeChange(Attribute, OldValue, NewValue);

    // Correct the dependent attribute's Base when its Max attribute changes.
    // OldValue/NewValue are the Max's Current values (post-clamp).
    FGameplayAttribute Dependent = GetDependentAttribute(Attribute);
    if (!Dependent.IsValid())
    {
        // Only Max* attributes have a dependent; this avoids recursion for non-Max attributes.
        return;
    }

    UAbilitySystemComponent *ASC = GetOwningAbilitySystemComponent();
    if (!ASC || !ASC->IsOwnerActorAuthoritative())
    {
        // Authority-only: avoid client base divergence.
        return;
    }

    // Base-based correction: Current is a derived value (clamp(Base + modifiers)), so computing the
    // delta from Current would subtract the dependent's own Infinite modifier share from the Base.
    float Base = ASC->GetNumericAttributeBase(Dependent);
    float NewBase;
    switch (GetMaxChangePolicy(Attribute))
    {
        case EEWEMaxChangePolicy::KeepRatio:
            // Keep the Base/OldMax ratio. The 0-division guard lives only here: if OldMax is <= 0
            // the ratio is undefined, so fill the dependent to the new Max.
            NewBase = (OldValue > 0.f) ? Base * NewValue / OldValue : NewValue;
            break;
        case EEWEMaxChangePolicy::Clamp:
        default:
            // Only reduce the Base when it exceeds the new Max; never raise it.
            NewBase = FMath::Min(Base, NewValue);
            break;
    }

    // Always apply, even when the delta is zero: it re-runs the dependent's aggregator so
    // PreAttributeChange re-clamps Current against the new Max. Event-unit (no accumulated state).
    // ApplyModToAttribute is not a GE, so PostGameplayEffectExecute / GameplayCue are not triggered.
    ASC->ApplyModToAttribute(Dependent, EGameplayModOp::Additive, NewBase - Base);
}

bool UEWEAttributeBase::GetClampRange(const FGameplayAttribute &Attribute, float &OutMin, float &OutMax) const
{
    if (Attribute == GetMaxHealthAttribute())
    {
        // MaxHealth: [1, max float].
        OutMin = 1.f;
        OutMax = TNumericLimits<float>::Max();
        return true;
    }
    if (Attribute == GetMaxManaAttribute())
    {
        // MaxMana: [0, max float].
        OutMin = 0.f;
        OutMax = TNumericLimits<float>::Max();
        return true;
    }
    if (Attribute == GetHealthAttribute())
    {
        // Health: [0, MaxHealth Current]. The upper bound is the Max's Current value, not its
        // Base, so a MaxHealth equip buff raises the recovery cap immediately.
        OutMin = 0.f;
        OutMax = GetMaxHealth();
        return true;
    }
    if (Attribute == GetManaAttribute())
    {
        // Mana: [0, MaxMana Current].
        OutMin = 0.f;
        OutMax = GetMaxMana();
        return true;
    }
    return false;
}

void UEWEAttributeBase::ClampAttribute(const FGameplayAttribute &Attribute, float &NewValue) const
{
    float Min = 0.f;
    float Max = 0.f;
    if (GetClampRange(Attribute, Min, Max))
    {
        NewValue = FMath::Clamp(NewValue, Min, Max);
    }
}

void UEWEAttributeBase::SetMaxChangePolicy(const FGameplayAttribute &MaxAttribute, EEWEMaxChangePolicy Policy)
{
    if (!GetDependentAttribute(MaxAttribute).IsValid())
    {
        EWE_LOG(LogEWE, Warning, TEXT("SetMaxChangePolicy: %s is not a Max attribute; ignored"), *MaxAttribute.GetName());
        return;
    }

    for (FEWEMaxChangePolicyEntry &Entry : MaxChangePolicies)
    {
        if (Entry.MaxAttribute == MaxAttribute)
        {
            Entry.Policy = Policy;
            return;
        }
    }

    FEWEMaxChangePolicyEntry NewEntry;
    NewEntry.MaxAttribute = MaxAttribute;
    NewEntry.Policy = Policy;
    MaxChangePolicies.Add(NewEntry);
}

EEWEMaxChangePolicy UEWEAttributeBase::GetMaxChangePolicy(const FGameplayAttribute &MaxAttribute) const
{
    for (const FEWEMaxChangePolicyEntry &Entry : MaxChangePolicies)
    {
        if (Entry.MaxAttribute == MaxAttribute)
        {
            return Entry.Policy;
        }
    }
    return EEWEMaxChangePolicy::Clamp;
}

FGameplayAttribute UEWEAttributeBase::GetDependentAttribute(const FGameplayAttribute &MaxAttribute)
{
    if (MaxAttribute == GetMaxHealthAttribute())
    {
        return GetHealthAttribute();
    }
    if (MaxAttribute == GetMaxManaAttribute())
    {
        return GetManaAttribute();
    }
    return FGameplayAttribute();
}

void UEWEAttributeBase::PostGameplayEffectExecute(const FGameplayEffectModCallbackData &Data)
{
    Super::PostGameplayEffectExecute(Data);

    // Side effects such as death handling go here. Clamping is handled by the Pre* hooks.

    // Example: Damage meta-attribute drain (Damage attribute not defined yet).
    //
    // else if (Data.EvaluatedData.Attribute == GetDamageAttribute())
    // {
    //     float DamageDealt = GetDamage();
    //     SetHealth(FMath::Clamp(GetHealth() - DamageDealt, 0.f, GetMaxHealth()));
    //     SetDamage(0.f);
    // }
}

void UEWEAttributeBase::OnRep_Health(const FGameplayAttributeData &OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UEWEAttributeBase, Health, OldValue);
}

void UEWEAttributeBase::OnRep_MaxHealth(const FGameplayAttributeData &OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UEWEAttributeBase, MaxHealth, OldValue);
}

void UEWEAttributeBase::OnRep_Mana(const FGameplayAttributeData &OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UEWEAttributeBase, Mana, OldValue);
}

void UEWEAttributeBase::OnRep_MaxMana(const FGameplayAttributeData &OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UEWEAttributeBase, MaxMana, OldValue);
}