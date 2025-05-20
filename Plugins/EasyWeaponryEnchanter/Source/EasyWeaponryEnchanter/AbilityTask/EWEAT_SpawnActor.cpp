// Fill out your copyright notice in the Description page of Project Settings.


#include "EWEAT_SpawnActor.h"

UEWEAT_SpawnActor* UEWEAT_SpawnActor::SpawnActor(UGameplayAbility* OwningAbility, FGameplayAbilityTargetDataHandle TargetData, TSubclassOf<AActor> Class)
{
	UEWEAT_SpawnActor* MyObj = NewAbilityTask<UEWEAT_SpawnActor>(OwningAbility);
	MyObj->CachedOwnerAbility = OwningAbility;
	MyObj->CachedTargetDataHandle = MoveTemp(TargetData);
	MyObj->CachedClassToSpawn = Class;

	return MyObj;
}

void UEWEAT_SpawnActor::Activate()
{
	Super::Activate();

	AActor* Dummy = nullptr;
	if (BeginSpawningActor(CachedOwnerAbility, CachedTargetDataHandle, CachedClassToSpawn, Dummy))
	{
		FinishSpawningActor(CachedOwnerAbility, CachedTargetDataHandle, Dummy);
	}
}
