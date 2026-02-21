// Fill out your copyright notice in the Description page of Project Settings.


#include "EWEAT_SpawnActor.h"
#include "AbilitySystemComponent.h"

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
	if (BeginSpawningActor(CachedOwnerAbility, CachedClassToSpawn, Dummy))
	{
		FinishSpawningActor(CachedOwnerAbility, Dummy);
	}
}

bool UEWEAT_SpawnActor::BeginSpawningActor(UGameplayAbility* OwningAbility, TSubclassOf<AActor> Class, AActor*& SpawnedActor)
{
	if (Ability && Ability->GetCurrentActorInfo()->IsNetAuthority() && ShouldBroadcastAbilityTaskDelegates())
	{
		UWorld* const World = GEngine->GetWorldFromContextObject(OwningAbility, EGetWorldErrorMode::LogAndReturnNull);
		if (World)
		{
			SpawnedActor = World->SpawnActorDeferred<AActor>(Class, FTransform::Identity, NULL, NULL, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		}
	}

	if (SpawnedActor == nullptr)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			DidNotSpawn.Broadcast(nullptr);
		}
		return false;
	}

	return true;
}

void UEWEAT_SpawnActor::FinishSpawningActor(UGameplayAbility* OwningAbility, AActor* SpawnedActor)
{
	if (SpawnedActor)
	{
		bool	   bTransformSet = false;
		FTransform SpawnTransform;
		if (FGameplayAbilityTargetData* LocationData = CachedTargetDataHandle.Get(0)) // Hardcode to use data 0. It's OK if data isn't useful/valid.
		{
			// Set location. Rotation is unaffected.
			if (LocationData->HasHitResult())
			{
				SpawnTransform.SetLocation(LocationData->GetHitResult()->Location);
				bTransformSet = true;
			}
			else if (LocationData->HasEndPoint())
			{
				SpawnTransform = LocationData->GetEndPointTransform();
				bTransformSet = true;
			}
		}
		if (!bTransformSet)
		{
			if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
			{
				SpawnTransform = ASC->GetOwner()->GetTransform();
			}
		}

		SpawnedActor->FinishSpawning(SpawnTransform);

		if (ShouldBroadcastAbilityTaskDelegates())
		{
			Success.Broadcast(SpawnedActor);
		}
	}

	EndTask();
}
