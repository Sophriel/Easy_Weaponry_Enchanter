// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EWEAbilityBase.h"
#include "EWEGA_Creation.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCreationComplete, AActor*, SpawnedActor);

/** Where a spawned actor is placed. */
UENUM(BlueprintType)
enum class ELocationType : uint8
{
	/** Character transform plus the TargetLocation offset. */
	Character	 UMETA(DisplayName = "Character Location"),
	/** 200 units in front of the character, along its forward vector. */
	View		 UMETA(DisplayName = "Character View (Forward)"),
	/** Along the camera direction at TargetDistance from the character. */
	Camera		 UMETA(DisplayName = "Camera Location"),
	/** The TargetLocation transform itself. */
	Target		 UMETA(DisplayName = "Target Actor/Point"),
	/** Not implemented yet. */
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

	/** Character: offset added to the character's transform.
	 *  Target: the spawn transform itself, set at runtime by Blueprint or the ability
	 *  (hidden in the editor for this mode). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creation", meta = (AllowPrivateAccess = "true", EditCondition = "LocationType == ELocationType::Character", EditConditionHides))
	FTransform TargetLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creation", meta = (AllowPrivateAccess = "true", EditCondition = "LocationType == ELocationType::Camera", EditConditionHides))
	float TargetDistance;
};
