// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEWeaponBase.h"

AEWEWeaponBase::AEWEWeaponBase()
{
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	check(WeaponMesh);

	RootComponent = WeaponMesh;
}

void AEWEWeaponBase::BeginPlay()
{
	Super::BeginPlay();
}