// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EWEAbilityBase.h"
#include "EWEGA_Creation.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCreationComplete, AActor*, SpawnedActor);

/**
 *
 */
UENUM(BlueprintType)
enum class ELocationType : uint8
{
	Character	 UMETA(DisplayName = "Character Location"),
	View		 UMETA(DisplayName = "Character View (Forward)"),
	Camera		 UMETA(DisplayName = "Camera Location"),
	Target		 UMETA(DisplayName = "Target Actor/Point"),
	AnchorObject UMETA(DisplayName = "Placed Anchor Object")
};

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

	FGameplayAbilityTargetDataHandle GetTargetData() const { return TargetData; };
	TSubclassOf<class AActor>		 GetActorToSpawn() const { return ActorToSpawn; };

protected:
	UFUNCTION()
	void OnActorSpawned(AActor* SpawnedActor);

	UFUNCTION()
	void OnActorSpawnFailed(AActor* SpawnedActor);

	void SetActorLocation();

protected:
	FGameplayAbilityTargetDataHandle TargetData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creation", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class AActor> ActorToSpawn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creation")
	ELocationType LocationType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creation", meta = (AllowPrivateAccess = "true", EditCondition = "LocationType == ELocationType::Character", EditConditionHides))
	FTransform TargetLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creation", meta = (AllowPrivateAccess = "true", EditCondition = "LocationType == ELocationType::Camera", EditConditionHides))
	float TargetDistance;
};
