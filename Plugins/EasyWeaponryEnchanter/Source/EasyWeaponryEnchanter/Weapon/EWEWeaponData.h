// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EWEWeaponData.generated.h"

/** Weapon definition: icon, mesh, and the ability arrays for hold, hit, and on-equip. */
UCLASS()
class EASYWEAPONRYENCHANTER_API UEWEWeaponData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UEWEWeaponData();

    FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId("EWEWeaponData", GetFName()); }

    virtual void PostLoad() override;
    void LoadWeaponAssets();

protected:
    void OnWeaponMeshLoaded();

public:
    /** Icon shown in the inventory and quick slot UI. */
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TObjectPtr<class UTexture2D> WeaponIcon;

    UPROPERTY(Transient)
    TObjectPtr<class USkeletalMesh> WeaponMesh;

    UPROPERTY(EditAnywhere, Category = Weapon)
    TArray<TSubclassOf<class UGameplayAbility>> HoldAbilities;

    UPROPERTY(EditAnywhere, Category = Weapon)
    TArray<TSubclassOf<class UGameplayAbility>> HitAbilities;

    /** Granted while the weapon is equipped. Only the UEWEGA_DefaultOnEquip family auto-activates. */
    UPROPERTY(EditAnywhere, Category = Weapon)
    TArray<TSubclassOf<class UGameplayAbility>> OnEquipAbilities;

protected:
    UPROPERTY(EditAnywhere, Category = Weapon, meta = (AllowPrivateAccess = "true"))
    TSoftObjectPtr<class USkeletalMesh> WeaponMeshAsset;
};
