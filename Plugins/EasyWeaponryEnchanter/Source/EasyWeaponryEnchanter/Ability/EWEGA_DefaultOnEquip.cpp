// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEGA_DefaultOnEquip.h"
#include "EasyWeaponryEnchanter.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"

UEWEGA_DefaultOnEquip::UEWEGA_DefaultOnEquip()
{
    // Store the active handle so the equip GE can be removed on unequip.
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    // ServerOnly: the client cannot apply a GE without a prediction key, and the equip GE and
    // attributes are replicated from the server.
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UEWEGA_DefaultOnEquip::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
    Super::OnAvatarSet(ActorInfo, Spec);

    // ServerOnly is enforced by IsDataValid; guard at runtime too so a misconfigured asset
    // cannot auto-activate on the client.
    if (NetExecutionPolicy != EGameplayAbilityNetExecutionPolicy::ServerOnly)
    {
        EWE_LOG(LogEWE, Warning, TEXT("[UEWEGA_DefaultOnEquip] NetExecutionPolicy must be ServerOnly; skipping auto-activation"));
        return;
    }

    if (!ActorInfo || !ActorInfo->IsNetAuthority())
    {
        return;
    }

    // Guard against re-entry when the avatar is re-set while already active.
    if (Spec.IsActive())
    {
        return;
    }

    // IsNetAuthority() is not redundant: the spec is replicated, so OnAvatarSet also runs on the
    // owning client. Without this guard the client's TryActivateAbility would send a remote
    // activation RPC to the server.
    if (ActorInfo->AbilitySystemComponent.IsValid())
    {
        ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
    }
}

void UEWEGA_DefaultOnEquip::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    // No CommitAbility: this GA has no cost or cooldown.

    UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
    if (!ASC)
    {
        return;
    }

    // The GA path records the Ability and SourceObject (= the weapon, from the spec) in the
    // effect context automatically.
    for (const TSubclassOf<UGameplayEffect> &GEClass : OnEquipEffects)
    {
        if (!GEClass)
        {
            continue;
        }

        FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(GEClass, GetAbilityLevel());
        if (!SpecHandle.IsValid())
        {
            continue;
        }

        FActiveGameplayEffectHandle H = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
        if (H.IsValid())
        {
            AppliedEffectHandles.Add(H);
        }
    }

    // Do not EndAbility: the effects stay active for the duration of the equip.
}

void UEWEGA_DefaultOnEquip::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
    {
        for (const FActiveGameplayEffectHandle &H : AppliedEffectHandles)
        {
            if (H.IsValid())
            {
                ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(H);
            }
        }
        AppliedEffectHandles.Reset();
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

#if WITH_EDITOR
EDataValidationResult UEWEGA_DefaultOnEquip::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    // The equip GA is forced to ServerOnly (network.md "forced exception — equip GA"): the client
    // cannot apply a GE without a prediction key, and the equip GE and attributes replicate from
    // the server.
    if (NetExecutionPolicy != EGameplayAbilityNetExecutionPolicy::ServerOnly)
    {
        Context.AddError(FText::Format(
            FText::FromString(TEXT("'{0}' must be ServerOnly: the client cannot apply a GE without a prediction key")),
            FText::FromString(GetName())));
        Result = EDataValidationResult::Invalid;
    }

    return Result;
}
#endif
