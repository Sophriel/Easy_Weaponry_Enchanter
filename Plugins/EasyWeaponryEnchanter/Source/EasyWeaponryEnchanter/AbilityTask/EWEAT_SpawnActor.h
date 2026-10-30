// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask_SpawnActor.h"
#include "EWEAT_SpawnActor.generated.h"

/** Ability task that spawns an actor at a target location. */
UCLASS()
class EASYWEAPONRYENCHANTER_API UEWEAT_SpawnActor : public UAbilityTask_SpawnActor
{
	GENERATED_BODY()

public:
	static UEWEAT_SpawnActor* SpawnActor(UGameplayAbility* OwningAbility, FGameplayAbilityTargetDataHandle TargetData, TSubclassOf<AActor> Class);

protected:
	virtual void Activate() override;

	bool BeginSpawningActor(UGameplayAbility* OwningAbility, TSubclassOf<AActor> Class, AActor*& SpawnedActor);

	void FinishSpawningActor(UGameplayAbility* OwningAbility, AActor* SpawnedActor);

private:
	UGameplayAbility*	CachedOwnerAbility;
	TSubclassOf<AActor> CachedClassToSpawn;

	// TODO: Add a member for each option below and implement its behavior for the spawned actor.
	//  location from owner
	//  rotation from owner
	//  enable physics
	//  enable collision
};
