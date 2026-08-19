// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "EWEQuickSlotItemViewModel.generated.h"

class UEWEWeaponData;

/** One quick slot item — shows one weapon (or empty) and selection highlight state. */
UCLASS(BlueprintType)
class EWE_API UEWEQuickSlotItemViewModel : public UMVVMViewModelBase
{
    GENERATED_BODY()

public:
    /** Assigns the weapon shown in this slot. Pass nullptr to clear it. */
    void SetWeapon(class UEWEWeaponData *InWeapon);
    void SetSelected(bool bInSelected);

    /** Fixed position of this slot in the quick slot bar. Set once at creation. */
    void SetSlotIndex(int32 InSlotIndex);

    UFUNCTION(BlueprintPure, FieldNotify) bool IsEmpty() const { return Weapon == nullptr; }
    UFUNCTION(BlueprintPure, FieldNotify) bool IsSelected() const { return bSelected; }
    UFUNCTION(BlueprintPure, FieldNotify) FText GetWeaponName() const;

    UFUNCTION(BlueprintPure, FieldNotify)
    class UEWEWeaponData *GetWeapon() const { return Weapon; }

    UFUNCTION(BlueprintPure, FieldNotify)
    int32 GetSlotIndex() const { return SlotIndex; }

protected:
    UPROPERTY(BlueprintReadOnly, FieldNotify)
    TObjectPtr<class UEWEWeaponData> Weapon;

    UPROPERTY(BlueprintReadOnly, FieldNotify)
    bool bSelected = false;

    UPROPERTY(BlueprintReadOnly, FieldNotify)
    int32 SlotIndex = 0;
};
