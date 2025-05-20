// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EWEAbilityBase.h"
#include "EWEGA_FireProjectile.generated.h"

UENUM(BlueprintType)
enum class EFireType : uint8
{
	SpawnFromClass	  UMETA(DisplayName = "Spawn From Class and Fire"),
	SpawnFromCreation UMETA(DisplayName = "Use Creation Ability and Fire")
};

/**
 *
 */
UCLASS()
class EASYWEAPONRYENCHANTER_API UEWEGA_FireProjectile : public UEWEAbilityBase
{
	GENERATED_BODY()

public:
	UEWEGA_FireProjectile();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle	 Handle,
		const FGameplayAbilityActorInfo*	 ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData*			 TriggerEventData) override;

protected:
	void SpawnActorFromClass(const FGameplayAbilityActorInfo* OwnerInfo);
	void SpawnActorFromCreation(const FGameplayAbilityActorInfo* OwnerInfo);

	UFUNCTION()
	void FireProjectile(AActor* ProjectileActor);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
	EFireType FireType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire|Spawn", meta = (EditCondition = "FireType == EFireType::SpawnFromClass", EditConditionHides))
	TSubclassOf<class AActor> TargetActor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire|Creation", meta = (EditCondition = "FireType == EFireType::SpawnFromCreation", EditConditionHides))
	TSubclassOf<class UEWEGA_Creation> CreationSkillClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
	float ProjectileSpeed;

#pragma region Editor

protected:
	UPROPERTY(Transient)
	bool bUseSpawn;

	UPROPERTY(Transient)
	bool bUseCreation;

#pragma endregion
};
