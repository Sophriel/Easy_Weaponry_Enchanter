// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEWeaponBase.h"
#include "Components/SkeletalMeshComponent.h"

AEWEWeaponBase::AEWEWeaponBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	// Weapons can move on their own (projectiles): the physics runs on the server, so the
	// movement must be replicated for clients to see the projectile fly.
	SetReplicateMovement(true);

	// The mesh is the root: for physics-driven weapons (projectiles) the actor's position
	// must follow the simulated mesh.
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	check(WeaponMesh);
	RootComponent = WeaponMesh;
}
