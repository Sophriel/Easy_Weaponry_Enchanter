// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EWECharacterInterface.generated.h"

const int32 CHoldAbilityIndex = 1000;
const int32 CHitAbilityIndex = 2000;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UEWECharacterInterface : public UInterface
{
	GENERATED_BODY()
};

class EASYWEAPONRYENCHANTER_API IEWECharacterInterface
{
	GENERATED_BODY()

public:
	/**
	 * Server-only equip gate. Grants the instance's loaded ability classes (SourceObject =
	 * instance) or, for nullptr, removes the current weapon GAs and clears any pending
	 * deferred-grant subscription. Called by the inventory's ApplyEquipTransition_ServerOnly.
	 */
	virtual void EquipWeapon_ServerOnly(class UEWEWeaponInstance* Instance) = 0;

	/**
	 * Server-only: sets the equipped instance and refreshes the hand visual. The inventory
	 * Model is the single writer.
	 */
	virtual void SetEquippedWeaponInstance_ServerOnly(class UEWEWeaponInstance* Instance) = 0;

	virtual class UEWEWeaponInstance* GetEquippedWeaponInstance() const = 0;
};
