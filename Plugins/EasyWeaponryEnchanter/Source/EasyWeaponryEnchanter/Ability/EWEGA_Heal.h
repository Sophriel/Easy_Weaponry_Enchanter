// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EWEAbilityBase.h"
#include "EWEGA_Heal.generated.h"

/**
 * Gameplay Ability that restores HP by applying a GameplayEffect
 */
UCLASS()
class EASYWEAPONRYENCHANTER_API UEWEGA_Heal : public UEWEAbilityBase
{
	GENERATED_BODY()

public:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle	 Handle,
		const FGameplayAbilityActorInfo*	 ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData*			 TriggerEventData) override;

	// GameplayEffect to apply for healing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heal", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> HealGE;

	// Optional override for heal amount magnitude (0 = use GE default)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heal")
	float HealAmount;
};
