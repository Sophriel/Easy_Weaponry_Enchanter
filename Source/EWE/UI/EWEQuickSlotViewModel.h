// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EWESlotContainerViewModelBase.h"
#include "EWEQuickSlotViewModel.generated.h"

class UEWEInventoryComponent;
class UEWEWeaponInstance;

/**
 * View model for the entire quick slot bar.
 *
 * Observer, not owner: the inventory component (Model) owns the slot contents and the
 * selected index. This view model subscribes to the Model's broadcasts and mirrors them
 * into per-slot item view models. Client requests (select / scroll / assign / clear) are
 * forwarded to the Model's Server_* RPCs; the server is the single writer.
 *
 * The slot item array, Model binding, and the full-mirror refresh live in the shared base
 * (UEWESlotContainerViewModelBase); this class provides the quick-slot Model accessors,
 * subscriptions, and selection handling.
 */
UCLASS(BlueprintType)
class EWE_API UEWEQuickSlotViewModel : public UEWESlotContainerViewModelBase
{
    GENERATED_BODY()

public:
    // Client requests. Each forwards to the bound Model's Server_* RPC; the server runs
    // the gate and the result comes back through the Model's replication.
    void SelectSlot(int32 SlotIndex);
    void ScrollSlot(float Direction);
    void AssignToSlot(class UEWEWeaponInstance *Instance, int32 SlotIndex);
    void ClearSlot(int32 SlotIndex);

    UFUNCTION(BlueprintPure, FieldNotify) int32 GetSelectedIndex() const { return SelectedIndex; }

#pragma region UEWESlotContainerViewModelBase
protected:
    // --- UEWESlotContainerViewModelBase override ---

    int32 GetModelSlotCount(const class UEWEInventoryComponent &Model) const override;
    const TArray<TObjectPtr<class UEWEWeaponInstance>> &
    GetModelSlots(const class UEWEInventoryComponent &Model) const override;
    void SubscribeModel(class UEWEInventoryComponent &Model) override;
    void UnsubscribeModel(class UEWEInventoryComponent &Model) override;
    void UpdateFromModel() override;

#pragma endregion

    /** Model broadcast: the slot contents changed. Refreshes every slot. */
    void OnModelQuickSlotsChanged();

    /** Model broadcast: the selected index changed. Refreshes the selection highlight. */
    void OnModelSelectedQuickSlotChanged(int32 SlotIndex);

    /** INDEX_NONE until the Model reports a selection. */
    UPROPERTY(BlueprintReadOnly, FieldNotify)
    int32 SelectedIndex = INDEX_NONE;

    FDelegateHandle QuickSlotsChangedHandle;
    FDelegateHandle SelectedQuickSlotChangedHandle;
};
