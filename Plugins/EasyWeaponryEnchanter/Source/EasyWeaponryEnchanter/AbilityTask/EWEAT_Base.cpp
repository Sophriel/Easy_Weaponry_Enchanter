// Fill out your copyright notice in the Description page of Project Settings.

#include "EasyWeaponryEnchanter/AbilityTask/EWEAT_Base.h"

#include "Abilities/GameplayAbility.h"
#include "EasyWeaponryEnchanter/Weapon/EWEWeaponBase.h"

void UEWEAT_Base::Activate()
{
	Super::Activate();

	if (!Ability || !AbilitySystemComponent.IsValid())
	{
		NotifyCompleted();
		return;
	}

	RefreshEnchantContextFromAbility();

	const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
	if (!ActorInfo)
	{
		NotifyCompleted();
		return;
	}

	if (ShouldExecuteOnServer() && !ActorInfo->IsNetAuthority())
	{
		NotifyCompleted();
		return;
	}

	ExecuteFunction(EnchantContext);
}

void UEWEAT_Base::OnDestroy(bool bInOwnerFinished)
{
	if (!bInOwnerFinished && !bHasCompleted)
	{
		InterruptFunction();
	}

	Super::OnDestroy(bInOwnerFinished);
}

void UEWEAT_Base::NotifyCompleted()
{
	if (bHasCompleted)
	{
		return;
	}

	bHasCompleted = true;
	EndTask();
}

void UEWEAT_Base::InterruptFunction()
{
	bHasCompleted = true;
}

void UEWEAT_Base::InitializeEnchantContext(FGameplayAbilityTargetDataHandle InTargetData)
{
	EnchantContext.TargetData = MoveTemp(InTargetData);
	RefreshEnchantContextFromAbility();
}

void UEWEAT_Base::RefreshEnchantContextFromAbility()
{
	if (!Ability)
	{
		return;
	}

	if (!EnchantContext.SourceActor.IsValid())
	{
		EnchantContext.SourceActor = Ability->GetOwningActorFromActorInfo();
	}

	if (!EnchantContext.AvatarActor.IsValid())
	{
		EnchantContext.AvatarActor = Ability->GetAvatarActorFromActorInfo();
	}

	if (!EnchantContext.SourceActor.IsValid())
	{
		EnchantContext.SourceActor = EnchantContext.AvatarActor;
	}

	if (!EnchantContext.ParentWeapon.IsValid())
	{
		if (AEWEWeaponBase* AvatarWeapon = Cast<AEWEWeaponBase>(EnchantContext.AvatarActor.Get()))
		{
			EnchantContext.ParentWeapon = AvatarWeapon;
		}
		else if (AEWEWeaponBase* SourceWeapon = Cast<AEWEWeaponBase>(EnchantContext.SourceActor.Get()))
		{
			EnchantContext.ParentWeapon = SourceWeapon;
		}
	}
}
