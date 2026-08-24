// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "EWEQuickSlotViewModel.generated.h"

class UEWEWeaponData;
class UEWEQuickSlotItemViewModel;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnQuickSlotSelectedEvent, int32 /*SlotIndex*/,
                                     class UEWEWeaponData * /*Weapon*/);

/** View model for the entire quick slot bar. */
UCLASS(BlueprintType)
class EWE_API UEWEQuickSlotViewModel : public UMVVMViewModelBase
{
    GENERATED_BODY()

public:
    /** Allocates the fixed slot array. Safe to call again with the same size. */
    void InitializeSlots(int32 InSlotCount);

    /** Assigns a weapon to an explicit slot. Out-of-range logs and returns.
     *  All slot-content writes must go through this (or the methods that do),
     *  so the re-equip broadcast below stays reachable. */
    void SetSlot(int32 SlotIndex, class UEWEWeaponData *Weapon);

    /** Places a weapon in the first empty slot. Returns the slot used, or INDEX_NONE. */
    int32 AddWeapon(class UEWEWeaponData *Weapon);

    /** Clears the slot. No caller yet — kept as the readable form of SetSlot(Index, nullptr). */
    void ClearSlot(int32 SlotIndex);
    void SelectSlot(int32 SlotIndex);

    /** Wraps around both ends. */
    void ScrollSlot(float Direction);

    /** True when no weapon has been assigned to any slot yet. */
    bool IsEmpty() const;

    /** Seeds the slots from an inventory weapon list. No-op if already seeded. */
    void SeedFromInventory(const TArray<class UEWEWeaponData *> &InWeapons);

    UFUNCTION(BlueprintPure, FieldNotify) int32 GetSelectedIndex() const { return SelectedIndex; }
    UFUNCTION(BlueprintPure, FieldNotify) int32 GetSlotCount() const { return SlotItems.Num(); }

    UFUNCTION(BlueprintPure)
    UEWEQuickSlotItemViewModel *GetSlotItem(int32 SlotIndex) const;

    /** Broadcast when a slot is selected. Subscribers own the equip side effect. */
    FOnQuickSlotSelectedEvent OnSlotSelected;

protected:
    UPROPERTY(BlueprintReadOnly, FieldNotify)
    TArray<TObjectPtr<UEWEQuickSlotItemViewModel>> SlotItems;

    /** INDEX_NONE until the first selection, so SelectSlot(0) at startup is not swallowed
     *  by the same-index guard. */
    UPROPERTY(BlueprintReadOnly, FieldNotify)
    int32 SelectedIndex = INDEX_NONE;
};
