// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEWeaponBase.h"
#include "Components/BoxComponent.h"

AEWEWeaponBase::AEWEWeaponBase()
{
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	check(WeaponMesh);

	RootComponent = WeaponMesh;
}

// Called when the game starts or when spawned
void AEWEWeaponBase::BeginPlay()
{
	Super::BeginPlay();
}