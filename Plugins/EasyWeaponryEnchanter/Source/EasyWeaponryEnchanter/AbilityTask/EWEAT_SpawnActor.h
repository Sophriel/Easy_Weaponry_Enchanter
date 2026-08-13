// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EasyWeaponryEnchanter/AbilityTask/EWEAT_Base.h"
#include "EWEAT_SpawnActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEWESpawnActorDelegate, AActor*, SpawnedActor);

/**
 *
 */
UCLASS()
class EASYWEAPONRYENCHANTER_API UEWEAT_SpawnActor : public UEWEAT_Base
{
	GENERATED_BODY()

public:
	static UEWEAT_SpawnActor* SpawnActor(UGameplayAbility* OwningAbility, FGameplayAbilityTargetDataHandle TargetData, TSubclassOf<AActor> Class);

	UPROPERTY(BlueprintAssignable)
	FEWESpawnActorDelegate Success;

	UPROPERTY(BlueprintAssignable)
	FEWESpawnActorDelegate DidNotSpawn;

protected:
	virtual void ExecuteFunction(const FEnchantContext& InContext) override;
	virtual bool ShouldExecuteOnServer() const override { return false; }

	bool BeginSpawningActor(UGameplayAbility* OwningAbility, TSubclassOf<AActor> Class, AActor*& SpawnedActor);

	void FinishSpawningActor(UGameplayAbility* OwningAbility, AActor* SpawnedActor);

private:
	TSubclassOf<AActor> CachedClassToSpawn;

	//  location from owner
	//  rotation from owner
	//  enable physics
	//  enable collision
};
