// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EWEWeaponBase.generated.h"

/**
 * Base for weapons spawned by gameplay abilities (projectiles, etc.). The mesh is the root
 * so a physics-driven weapon follows its simulated position.
 */
UCLASS()
class EASYWEAPONRYENCHANTER_API AEWEWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AEWEWeaponBase(const FObjectInitializer& ObjectInitializer);

	FORCEINLINE class USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USkeletalMeshComponent> WeaponMesh;
};
