// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EWEWeaponData.generated.h"

/**
 *
 */
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
    // Editor assign asset
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TObjectPtr<class UTexture2D> WeaponIcon;

    // Runtime caching asset
    UPROPERTY(Transient)
    TObjectPtr<class USkeletalMesh> WeaponMesh;

    UPROPERTY(EditAnywhere, Category = Weapon)
    TArray<TSubclassOf<class UGameplayAbility>> HoldAbilities;

    UPROPERTY(EditAnywhere, Category = Weapon)
    TArray<TSubclassOf<class UGameplayAbility>> HitAbilities;

protected:
    UPROPERTY(EditAnywhere, Category = Weapon, meta = (AllowPrivateAccess = "true"))
    TSoftObjectPtr<class USkeletalMesh> WeaponMeshAsset;
};
