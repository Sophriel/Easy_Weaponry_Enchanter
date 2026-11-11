// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EWEAbilityBase.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include "EWEGA_DefaultOnEquip.generated.h"

/** Auto-activates on equip and applies OnEquipEffects for the duration of the equip.
 *  Runs on the server only. */
UCLASS()
class EASYWEAPONRYENCHANTER_API UEWEGA_DefaultOnEquip : public UEWEAbilityBase
{
    GENERATED_BODY()

public:
    UEWEGA_DefaultOnEquip();

public:
    UPROPERTY(EditDefaultsOnly, Category = "OnEquip")
    TArray<TSubclassOf<UGameplayEffect>> OnEquipEffects;

protected:
    virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

private:
    // Active handles of the applied OnEquipEffects. Not UPROPERTY: FActiveGameplayEffectHandle is
    // a lightweight handle, not a UObject reference.
    TArray<FActiveGameplayEffectHandle> AppliedEffectHandles;
};
