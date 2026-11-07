// Fill out your copyright notice in the Description page of Project Settings.

#include "EasyWeaponryEnchanter/Weapon/EWEWeaponData.h"
#include "Engine/AssetManager.h"

UEWEWeaponData::UEWEWeaponData() {}

TSharedPtr<FStreamableHandle> UEWEWeaponData::RequestSubAssetLoad(FStreamableDelegate OnLoaded)
{
    TArray<FSoftObjectPath> Paths;

    if (!WeaponMeshAsset.IsNull())
    {
        Paths.Add(WeaponMeshAsset.ToSoftObjectPath());
    }

    if (!WeaponIcon.IsNull())
    {
        Paths.Add(WeaponIcon.ToSoftObjectPath());
    }

    // Collect the ability class paths. A local lambda keeps this helper out of the unity
    // build's global namespace, where a same-named helper in another TU would collide.
    auto AddAbilityPaths = [](const TArray<TSoftClassPtr<UGameplayAbility>> &Abilities, TArray<FSoftObjectPath> &OutPaths)
    {
        for (const TSoftClassPtr<UGameplayAbility> &Ability : Abilities)
        {
            if (!Ability.IsNull())
            {
                OutPaths.Add(Ability.ToSoftObjectPath());
            }
        }
    };

    AddAbilityPaths(HoldAbilities, Paths);
    AddAbilityPaths(HitAbilities, Paths);
    AddAbilityPaths(OnEquipAbilities, Paths);

    if (Paths.IsEmpty())
    {
        // Nothing to stream. Finish loading immediately so the caller does not hang.
        OnLoaded.ExecuteIfBound();
        return nullptr;
    }

    FStreamableManager &StreamableManager = UAssetManager::GetStreamableManager();
    return StreamableManager.RequestAsyncLoad(Paths, OnLoaded);
}
