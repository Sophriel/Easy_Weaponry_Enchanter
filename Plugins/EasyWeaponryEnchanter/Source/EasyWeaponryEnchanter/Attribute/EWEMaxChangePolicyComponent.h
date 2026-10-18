// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "EWEAttributeBase.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include "EWEMaxChangePolicyComponent.generated.h"

/**
 * Registers Max-attribute change policies on the owner's UEWEAttributeBase.
 *
 * Instant GE only. The policy is AttributeSet-global and is NOT reverted when the GE is removed.
 * If no GE with this component is applied, all Max attributes use the default Clamp.
 */
UCLASS(DisplayName = "Max Change Policy (EWE)")
class EASYWEAPONRYENCHANTER_API UEWEMaxChangePolicyComponent : public UGameplayEffectComponent
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, Category = "Policy")
    TArray<FEWEMaxChangePolicyEntry> Policies;

protected:
    virtual void OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
