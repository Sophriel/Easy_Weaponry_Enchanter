// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "EWEPlayerController.generated.h"

/**
 *
 */
UCLASS()
class EWE_API AEWEPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void AddWeapon(class UEWEWeaponData* Weapon);
	void SetSlot(const uint8 SlotIndex, class UEWEWeaponData* Weapon);
	void SelectSlot(const uint8 SlotIndex);
	void ScrollSlot(const float ScrollDirection);

protected:
	virtual void AcknowledgePossession(APawn* P) override;
};
