// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EWECharacterInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UEWECharacterInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class EASYWEAPONRYENCHANTER_API IEWECharacterInterface
{
	GENERATED_BODY()

public:
	virtual void AcquireWeapon(class UEWEWeaponData* Weapon) = 0;
	virtual void EquipWeapon(class UEWEWeaponData* Weapon) = 0;
	//virtual TObjectPtr<class UAbilitySystemComponent> GetAbilitySystemComponent() = 0;
};
