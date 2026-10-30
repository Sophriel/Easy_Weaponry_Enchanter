// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEGA_Heal.h"
#include "EasyWeaponryEnchanter.h"
#include "EasyWeaponryEnchanter/Attribute/EWEAttributeBase.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"

void UEWEGA_Heal::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    // Check server authority
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        return;
    }

    // Validate HealGE
    if (!HealGE)
    {
        EWE_LOG(LogEWE, Error, TEXT("[UEWEGA_Heal] HealGE is not set"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    // Get AbilitySystemComponent from actor info
    UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
    if (!ASC)
    {
        EWE_LOG(LogEWE, Error, TEXT("[UEWEGA_Heal] Failed to get ASC"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    // Get avatar actor (character) for context
    AActor* AvatarActor = ActorInfo->AvatarActor.Get();
    if (!AvatarActor)
    {
        EWE_LOG(LogEWE, Error, TEXT("[UEWEGA_Heal] Avatar actor is null"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    // Create effect context
    FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
    EffectContext.AddInstigator(AvatarActor, AvatarActor);

    // Create GameplayEffectSpec using MakeOutgoingSpec
    FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(HealGE, 1.0f, EffectContext);
    FGameplayEffectSpec* Spec = SpecHandle.Data.Get();

    if (Spec)
    {
        Spec->CaptureAttributeDataFromTarget(ASC);
        ASC->ApplyGameplayEffectSpecToSelf(*Spec);
    }

    // End ability
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
