// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEGA_FireProjectile.h"
#include "EasyWeaponryEnchanter/Public/EasyWeaponryEnchanter.h"

#include "EWEGA_Creation.h"
#include "EasyWeaponryEnchanter/AbilityTask/EWEAT_SpawnActor.h"

UEWEGA_FireProjectile::UEWEGA_FireProjectile()
	: ProjectileSpeed(1.f)
{
}

void UEWEGA_FireProjectile::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		return;
	}

	switch (FireType)
	{
		case EFireType::SpawnFromClass:
			SpawnActorFromClass(ActorInfo);
			break;

		case EFireType::SpawnFromCreation:
			SpawnActorFromCreation(ActorInfo);
			break;

		default:
			break;
	}

	if (ActorInfo != NULL && ActorInfo->AvatarActor != NULL)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UEWEGA_FireProjectile::SpawnActorFromClass(const FGameplayAbilityActorInfo* ActorInfo)
{
	FGameplayAbilityTargetDataHandle TargetData;
	UEWEAT_SpawnActor*				 SpawnTask = UEWEAT_SpawnActor::SpawnActor(this, TargetData, TargetActor);
	check(SpawnTask);

	SpawnTask->Success.AddDynamic(this, &UEWEGA_FireProjectile::FireProjectile);
	SpawnTask->ReadyForActivation();
}

void UEWEGA_FireProjectile::SpawnActorFromCreation(const FGameplayAbilityActorInfo* ActorInfo)
{
	check(CreationSkillClass);

	const UEWEGA_Creation* CreationSkill = CreationSkillClass.GetDefaultObject();
	check(CreationSkill);

	UEWEAT_SpawnActor* SpawnTask = UEWEAT_SpawnActor::SpawnActor(this, CreationSkill->GetTargetData(), CreationSkill->GetActorToSpawn());
	check(SpawnTask);

	SpawnTask->Success.AddDynamic(this, &UEWEGA_FireProjectile::FireProjectile);
	SpawnTask->ReadyForActivation();
}

void UEWEGA_FireProjectile::FireProjectile(AActor* ProjectileActor)
{
	check(ProjectileActor);

	UPrimitiveComponent* RootComp = Cast<UPrimitiveComponent>(ProjectileActor->GetRootComponent());
	if (!RootComp || !RootComp->IsSimulatingPhysics())
		return;

	FVector Forward = ProjectileActor->GetActorForwardVector();
	FVector Impulse = Forward * ProjectileSpeed;

	RootComp->AddImpulse(Impulse, NAME_None, true);
	EWE_LOG(LogEWE, Log, TEXT("EWEGA_FireProjectile: Actor Fired"));
}
