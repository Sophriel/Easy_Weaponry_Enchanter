// Fill out your copyright notice in the Description page of Project Settings.

#include "EasyWeaponryEnchanter/Weapon/EWEWeaponData.h"
#include "Engine/AssetManager.h"

UEWEWeaponData::UEWEWeaponData() {}

void UEWEWeaponData::PostLoad()
{
    Super::PostLoad();

    LoadWeaponAssets();
}

void UEWEWeaponData::LoadWeaponAssets()
{
    if (!WeaponMeshAsset.IsPending())
    {
        if (WeaponMeshAsset.IsValid())
        {
            WeaponMesh = WeaponMeshAsset.Get();
        }
        return;
    }

    FStreamableManager &StreamableManager = UAssetManager::GetStreamableManager();
    StreamableManager.RequestAsyncLoad(WeaponMeshAsset.ToSoftObjectPath(),
                                       FStreamableDelegate::CreateUObject(this, &UEWEWeaponData::OnWeaponMeshLoaded));
}

void UEWEWeaponData::OnWeaponMeshLoaded()
{
    if (!WeaponMeshAsset.IsValid())
    {
        return;
    }

    WeaponMesh = WeaponMeshAsset.Get();
}
