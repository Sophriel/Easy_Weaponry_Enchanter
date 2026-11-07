// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StreamableManager.h"
#include "EWEWeaponData.generated.h"

/** Weapon definition: icon, mesh, and the ability arrays for hold, hit, and on-equip. */
UCLASS()
class EASYWEAPONRYENCHANTER_API UEWEWeaponData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UEWEWeaponData();

    FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId("EWEWeaponData", GetFName()); }

    TSoftObjectPtr<class UTexture2D> GetWeaponIcon() const { return WeaponIcon; }

    TSoftObjectPtr<class USkeletalMesh> GetWeaponMeshAsset() const { return WeaponMeshAsset; }

    const TArray<TSoftClassPtr<class UGameplayAbility>> &GetHoldAbilities() const { return HoldAbilities; }

    const TArray<TSoftClassPtr<class UGameplayAbility>> &GetHitAbilities() const { return HitAbilities; }

    const TArray<TSoftClassPtr<class UGameplayAbility>> &GetOnEquipAbilities() const { return OnEquipAbilities; }

    /**
     * Requests the mesh, icon, and ability classes in a single async load. Null paths are
     * excluded. When there is nothing to load, OnLoaded runs immediately and nullptr is
     * returned.
     */
    TSharedPtr<FStreamableHandle> RequestSubAssetLoad(FStreamableDelegate OnLoaded);

protected:
    /** Icon shown in the inventory and quick slot UI. */
    UPROPERTY(EditDefaultsOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
    TSoftObjectPtr<class UTexture2D> WeaponIcon;

    /** Skeletal mesh asset. */
    UPROPERTY(EditAnywhere, Category = Weapon, meta = (AllowPrivateAccess = "true"))
    TSoftObjectPtr<class USkeletalMesh> WeaponMeshAsset;

    /** Abilities activated while the weapon is held. */
    UPROPERTY(EditAnywhere, Category = Weapon, meta = (AllowPrivateAccess = "true"))
    TArray<TSoftClassPtr<class UGameplayAbility>> HoldAbilities;

    /** Abilities activated on hit. */
    UPROPERTY(EditAnywhere, Category = Weapon, meta = (AllowPrivateAccess = "true"))
    TArray<TSoftClassPtr<class UGameplayAbility>> HitAbilities;

    /** Granted while the weapon is equipped. Only the UEWEGA_DefaultOnEquip family auto-activates. */
    UPROPERTY(EditAnywhere, Category = Weapon, meta = (AllowPrivateAccess = "true"))
    TArray<TSoftClassPtr<class UGameplayAbility>> OnEquipAbilities;
};
