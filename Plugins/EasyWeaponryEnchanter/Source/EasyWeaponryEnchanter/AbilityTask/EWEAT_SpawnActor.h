// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask_SpawnActor.h"
#include "EWEAT_SpawnActor.generated.h"

/**
 *
 */
UCLASS()
class EASYWEAPONRYENCHANTER_API UEWEAT_SpawnActor : public UAbilityTask_SpawnActor
{
	GENERATED_BODY()

public:
	static UEWEAT_SpawnActor* SpawnActor(UGameplayAbility* OwningAbility, FGameplayAbilityTargetDataHandle TargetData, TSubclassOf<AActor> Class);

protected:
	virtual void Activate() override;

private:
	UGameplayAbility*	CachedOwnerAbility;
	TSubclassOf<AActor> CachedClassToSpawn;

	//  location from owner
	//  rotation from owner
	//  enable physics
	//  enable collision
};
