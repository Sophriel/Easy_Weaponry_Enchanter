// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EasyWeaponryEnchanter/Function/EnchantContext.h"
#include "EWEAbilityBase.h"
#include "EWEGA_Creation.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCreationComplete, AActor*, SpawnedActor);

UCLASS()
class EASYWEAPONRYENCHANTER_API UEWEGA_Creation : public UEWEAbilityBase
{
	GENERATED_BODY()

public:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle	 Handle,
		const FGameplayAbilityActorInfo*	 ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData*			 TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle	 Handle,
		const FGameplayAbilityActorInfo*	 ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool								 bReplicateEndAbility,
		bool								 bWasCancelled) override;

	FEnchantFunction BuildCreationFunction() const;

protected:
	UFUNCTION()
	void OnActorSpawned(AActor* SpawnedActor);

	UFUNCTION()
	void OnActorSpawnFailed(AActor* SpawnedActor);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creation", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class AActor> ActorToSpawn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creation")
	ELocationType LocationType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creation", meta = (AllowPrivateAccess = "true", EditCondition = "LocationType == ELocationType::Character", EditConditionHides))
	FTransform TargetLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creation", meta = (AllowPrivateAccess = "true", EditCondition = "LocationType == ELocationType::Camera", EditConditionHides))
	float TargetDistance;
};
