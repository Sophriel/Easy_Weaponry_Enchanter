// Fill out your copyright notice in the Description page of Project Settings.

#include "EasyWeaponryEnchanter/Weapon/EWEWeaponData.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"

UEWEWeaponData::UEWEWeaponData()
{
}

void UEWEWeaponData::PostLoad()
{
	Super::PostLoad();

	if (WeaponMesh.IsPending() == false)
		return;

	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();

	StreamableManager.RequestAsyncLoad(
		WeaponMesh.ToSoftObjectPath(),
		FStreamableDelegate::CreateUObject(this, &UEWEWeaponData::OnWeaponMeshLoaded));
}

void UEWEWeaponData::OnWeaponMeshLoaded()
{
}
