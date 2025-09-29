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

protected:
	virtual void PostLoad() override;
	void		 OnWeaponMeshLoaded();

public:
	UPROPERTY(EditAnywhere, Category = Weapon)
	TSoftObjectPtr<class USkeletalMesh> WeaponMesh;

	UPROPERTY(EditAnywhere, Category = Weapon)
	TArray<TSubclassOf<class UGameplayAbility>> Abilities;
};
