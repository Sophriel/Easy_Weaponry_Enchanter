// Fill out your copyright notice in the Description page of Project Settings.

#include "EasyWeaponryEnchanter/Weapon/EWEWeaponInstance.h"
#include "EasyWeaponryEnchanter/Weapon/EWEWeaponData.h"
#include "Abilities/GameplayAbility.h"
#include "EasyWeaponryEnchanter.h"
#include "Engine/AssetManager.h"
#include "Net/UnrealNetwork.h"

void UEWEWeaponInstance::OnRep_WeaponData()
{
    BeginLoad();
}

void UEWEWeaponInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UEWEWeaponInstance, WeaponData);
}

void UEWEWeaponInstance::InitializeWeaponData(TSoftObjectPtr<UEWEWeaponData> InWeaponData)
{
    if (!WeaponData.IsNull())
    {
        EWE_LOG(LogEWE, Warning, TEXT("Weapon instance already has weapon data; ignoring re-initialization."));
        return;
    }

    WeaponData = InWeaponData;
}

void UEWEWeaponInstance::BeginLoad()
{
    if (LoadState != EEWEWeaponLoadState::NotLoaded)
    {
        return;
    }

    if (WeaponData.IsNull())
    {
        EWE_LOG(LogEWE, Warning, TEXT("Weapon instance has no weapon data; finishing loading as failed."));
        FinishLoad(EEWEWeaponLoadState::Failed);
        return;
    }

    LoadState = EEWEWeaponLoadState::LoadingData;
    DataLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
        WeaponData.ToSoftObjectPath(),
        FStreamableDelegate::CreateUObject(this, &UEWEWeaponInstance::OnWeaponDataLoaded));
}

void UEWEWeaponInstance::OnWeaponDataLoaded()
{
    LoadedWeaponData = WeaponData.Get();

    if (!LoadedWeaponData)
    {
        EWE_LOG(LogEWE, Warning, TEXT("Weapon data load failed: %s"), *WeaponData.ToString());
        FinishLoad(EEWEWeaponLoadState::Failed);
        return;
    }

    LoadState = EEWEWeaponLoadState::LoadingAssets;
    SubAssetLoadHandle = LoadedWeaponData->RequestSubAssetLoad(
        FStreamableDelegate::CreateUObject(this, &UEWEWeaponInstance::OnSubAssetsLoaded));
}

void UEWEWeaponInstance::OnSubAssetsLoaded()
{
    bool bAnyFailed = false;

    const TSoftObjectPtr<USkeletalMesh> MeshAsset = LoadedWeaponData->GetWeaponMeshAsset();
    if (!MeshAsset.IsNull())
    {
        WeaponMesh = MeshAsset.Get();
        if (!WeaponMesh)
        {
            EWE_LOG(LogEWE, Warning, TEXT("Weapon mesh load failed: %s"), *MeshAsset.ToString());
            bAnyFailed = true;
        }
    }

    const TSoftObjectPtr<UTexture2D> IconAsset = LoadedWeaponData->GetWeaponIcon();
    if (!IconAsset.IsNull())
    {
        WeaponIcon = IconAsset.Get();
        if (!WeaponIcon)
        {
            EWE_LOG(LogEWE, Warning, TEXT("Weapon icon load failed: %s"), *IconAsset.ToString());
            bAnyFailed = true;
        }
    }

    bAnyFailed |= LoadAbilities(LoadedWeaponData->GetHoldAbilities(), HoldAbilities, TEXT("hold"));
    bAnyFailed |= LoadAbilities(LoadedWeaponData->GetHitAbilities(), HitAbilities, TEXT("hit"));
    bAnyFailed |= LoadAbilities(LoadedWeaponData->GetOnEquipAbilities(), OnEquipAbilities, TEXT("on-equip"));

    FinishLoad(bAnyFailed ? EEWEWeaponLoadState::Failed : EEWEWeaponLoadState::Ready);
}

void UEWEWeaponInstance::FinishLoad(EEWEWeaponLoadState FinalState)
{
    LoadState = FinalState;
    OnLoadFinished.Broadcast(this);
}

bool UEWEWeaponInstance::LoadAbilities(const TArray<TSoftClassPtr<UGameplayAbility>> &Source,
                                       TArray<TSubclassOf<UGameplayAbility>> &Out,
                                       const TCHAR *Category)
{
    bool bAnyFailed = false;

    for (const TSoftClassPtr<UGameplayAbility> &Ability : Source)
    {
        if (Ability.IsNull())
        {
            continue;
        }

        UClass *LoadedClass = Ability.Get();
        if (LoadedClass)
        {
            Out.Add(TSubclassOf<UGameplayAbility>(LoadedClass));
            continue;
        }

        EWE_LOG(LogEWE, Warning, TEXT("%s ability load failed: %s"), Category, *Ability.ToString());
        bAnyFailed = true;
    }

    return bAnyFailed;
}
