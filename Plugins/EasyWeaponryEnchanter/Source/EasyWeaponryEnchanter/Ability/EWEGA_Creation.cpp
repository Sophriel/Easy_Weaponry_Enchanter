// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEGA_Creation.h"
#include "EasyWeaponryEnchanter/Public/EasyWeaponryEnchanter.h"
#include "EasyWeaponryEnchanter/AbilityTask/EWEAT_SpawnActor.h"

void UEWEGA_Creation::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		return;
	}

	if (ActorToSpawn == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UEWEAT_SpawnActor* SpawnTask = UEWEAT_SpawnActor::SpawnActor(this, TargetData, ActorToSpawn);
	check(SpawnTask);

	SpawnTask->Success.AddDynamic(this, &UEWEGA_Creation::OnActorSpawned);
	SpawnTask->DidNotSpawn.AddDynamic(this, &UEWEGA_Creation::OnActorSpawned);

	SpawnTask->ReadyForActivation();

	if (ActorInfo != NULL && ActorInfo->AvatarActor != NULL)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UEWEGA_Creation::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UEWEGA_Creation::OnActorSpawned(AActor* SpawnedActor)
{
	check(SpawnedActor);

	EWE_LOG(LogEWE, Log, TEXT("EWEGA_Creation: Spawned Actor"));
}

void UEWEGA_Creation::OnActorSpawnFailed(AActor* SpawnedActor)
{
	EWE_LOG(LogEWE, Error, TEXT("EWEGA_Creation: Actor Spawn Failed"));
}
