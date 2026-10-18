// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEMaxChangePolicyComponent.h"
#include "EasyWeaponryEnchanter.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "GameplayEffect.h"

void UEWEMaxChangePolicyComponent::OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
    UAbilitySystemComponent *ASC = ActiveGEContainer.Owner;
    if (!ASC || !ASC->IsOwnerActorAuthoritative())
    {
        return;
    }

    UEWEAttributeBase *AttributeSet = nullptr;
    for (const TObjectPtr<UAttributeSet> &SpawnedSet : ASC->GetSpawnedAttributes())
    {
        if (UEWEAttributeBase *CastSet = Cast<UEWEAttributeBase>(SpawnedSet.Get()))
        {
            AttributeSet = CastSet;
            break;
        }
    }

    if (!AttributeSet)
    {
        EWE_LOG(LogEWE, Warning, TEXT("MaxChangePolicy: no UEWEAttributeBase found on owner; policies not registered"));
        return;
    }

    // Called after Instant modifier execution, so on first initialization the policy is not yet
    // registered and all Max changes use the default Clamp (MaxHealth 1->100, MaxMana 0->100 are
    // both Delta 0; the following Health/Mana Override fills the values).
    for (const FEWEMaxChangePolicyEntry &Entry : Policies)
    {
        AttributeSet->SetMaxChangePolicy(Entry.MaxAttribute, Entry.Policy);
    }
}

#if WITH_EDITOR
EDataValidationResult UEWEMaxChangePolicyComponent::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    UGameplayEffect *GE = GetOwner();
    if (!GE)
    {
        return Result;
    }

    // 1. Must be an Instant GE. Duration/Infinite (no Period) never execute, so the policy is
    //    silently unregistered; Periodic re-registers every period.
    if (GE->DurationPolicy != EGameplayEffectDurationType::Instant)
    {
        Context.AddError(FText::Format(
            FText::FromString(TEXT("'{0}' must be an Instant GE to use the Max Change Policy component")),
            FText::FromString(GE->GetName())));
        Result = EDataValidationResult::Invalid;
    }

    // 2. Each entry's MaxAttribute must be a real Max attribute.
    for (const FEWEMaxChangePolicyEntry &Entry : Policies)
    {
        if (!UEWEAttributeBase::GetDependentAttribute(Entry.MaxAttribute).IsValid())
        {
            Context.AddError(FText::Format(
                FText::FromString(TEXT("'{0}' is not a Max attribute (expected MaxHealth or MaxMana)")),
                FText::FromString(Entry.MaxAttribute.GetName())));
            Result = EDataValidationResult::Invalid;
        }
    }

    // 3. All Max* modifiers must come before all dependent modifiers.
    for (const FEWEMaxChangePolicyEntry &Entry : Policies)
    {
        FGameplayAttribute Dependent = UEWEAttributeBase::GetDependentAttribute(Entry.MaxAttribute);
        if (!Dependent.IsValid())
        {
            continue;
        }

        int32 LastMaxIndex = INDEX_NONE;
        int32 FirstDependentIndex = INDEX_NONE;
        for (int32 i = 0; i < GE->Modifiers.Num(); ++i)
        {
            const FGameplayModifierInfo &Mod = GE->Modifiers[i];
            if (Mod.Attribute == Entry.MaxAttribute)
            {
                LastMaxIndex = i;
            }
            else if (Mod.Attribute == Dependent && FirstDependentIndex == INDEX_NONE)
            {
                FirstDependentIndex = i;
            }
        }

        if (LastMaxIndex != INDEX_NONE && FirstDependentIndex != INDEX_NONE && FirstDependentIndex < LastMaxIndex)
        {
            Context.AddError(FText::Format(
                FText::FromString(TEXT("In '{0}', the {1} modifier must come before the {2} modifier")),
                FText::FromString(GE->GetName()),
                FText::FromString(Entry.MaxAttribute.GetName()),
                FText::FromString(Dependent.GetName())));
            Result = EDataValidationResult::Invalid;
        }
    }

    return Result;
}
#endif
