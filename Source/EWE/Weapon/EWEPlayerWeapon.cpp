// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEPlayerWeapon.h"
#include "Components/SceneComponent.h"
#include "EWE/Character/EWEWeaponMeshComponent.h"
#include "EWE/EWELog.h"
#include "Net/UnrealNetwork.h"

AEWEPlayerWeapon::AEWEPlayerWeapon(const FObjectInitializer& ObjectInitializer)
	: AEWEWeaponBase(ObjectInitializer.SetDefaultSubobjectClass<UEWEWeaponMeshComponent>(TEXT("WeaponMesh")))
{
	// v11 quick-slot pre-spawn policy — no longer spawned, kept for the future drop /
	// pickup feature. An empty scene root keeps the actor's origin independent of the mesh
	// (the base roots at the mesh for projectiles); a hand-attached weapon must not drag
	// the actor.
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	check(Root);
	RootComponent = Root;
	WeaponMesh->SetupAttachment(Root);

	// v11 quick-slot pre-spawn policy — no longer spawned, kept for drop / pickup.
	WeaponMesh->SetVisibility(false);

	// v11 quick-slot pre-spawn policy — no longer spawned, kept for drop / pickup.
	bOnlyRelevantToOwner = true;

	// v11 quick-slot pre-spawn policy — no longer spawned, kept for drop / pickup.
	SetReplicateMovement(false);
}

void AEWEPlayerWeapon::SetEquippedRelevancy_ServerOnly(bool bEquipped)
{
	if (!HasAuthority())
	{
		EWE_LOG(LogEWE, Warning, TEXT("SetEquippedRelevancy: no authority"));
		return;
	}

	// Equipped = visible to all clients; unequipped = owner-only.
	bOnlyRelevantToOwner = !bEquipped;
}

void AEWEPlayerWeapon::SetInstance(UEWEWeaponInstance* InInstance)
{
	Instance = InInstance;

	// Not exercised while this actor is unspawned (see class comment).
	UEWEWeaponMeshComponent* MeshComponent = GetWeaponMeshComponent();
	if (!MeshComponent)
	{
		return;
	}

	MeshComponent->SetInstance(InInstance);
}

void AEWEPlayerWeapon::OnRep_Instance()
{
	// The replicated value is already in Instance — just apply it to the mesh component.
	SetInstance(Instance);
}

void AEWEPlayerWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEWEPlayerWeapon, Instance);
}

UEWEWeaponMeshComponent* AEWEPlayerWeapon::GetWeaponMeshComponent() const
{
	UEWEWeaponMeshComponent* MeshComponent = Cast<UEWEWeaponMeshComponent>(WeaponMesh);
	if (!MeshComponent)
	{
		EWE_LOG(LogEWE, Warning, TEXT("AEWEPlayerWeapon::WeaponMesh is not a UEWEWeaponMeshComponent"));
	}
	return MeshComponent;
}
