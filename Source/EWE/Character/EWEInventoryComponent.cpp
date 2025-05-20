// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEInventoryComponent.h"
#include "EWECharacter.h"
#include "EasyWeaponryEnchanter/Weapon/EWEWeaponBase.h"

// Sets default values for this component's properties
UEWEInventoryComponent::UEWEInventoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

// Called when the game starts
void UEWEInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	AEWECharacter* OwnerCharacter = Cast<AEWECharacter>(GetOwner());
	check(OwnerCharacter);

	if (StartWeapon != nullptr)
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.Owner = OwnerCharacter;

		TObjectPtr<AEWEWeaponBase> Weapon = GetWorld()->SpawnActor<AEWEWeaponBase>(StartWeapon, SpawnInfo);
		Weapon->DisableComponentsSimulatePhysics();
		Weapon->GetWeaponMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Weapon->AttachToComponent(OwnerCharacter->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, TEXT("WeaponSocket"));

		AcquireWeapon(Weapon);
	}

	if (Weapons.IsEmpty() == true)
	{
		return;
	}

	OwnerCharacter->EquipWeapon(Weapons.Top());
}

void UEWEInventoryComponent::AcquireWeapon(AEWEWeaponBase* NewWeapon)
{
	check(NewWeapon);
	Weapons.Add(NewWeapon);
}
