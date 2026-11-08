// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "EWEQuickSlotViewModel.generated.h"

class UEWEInventoryComponent;
class UEWEWeaponInstance;
class UEWEQuickSlotItemViewModel;

/**
 * View model for the entire quick slot bar.
 *
 * Observer, not owner: the inventory component (Model) owns the slot contents and the
 * selected index. This view model subscribes to the Model's broadcasts and mirrors them
 * into per-slot item view models. Client requests (select / scroll / assign / clear) are
 * forwarded to the Model's Server_* RPCs; the server is the single writer.
 */
UCLASS(BlueprintType)
class EWE_API UEWEQuickSlotViewModel : public UMVVMViewModelBase
{
    GENERATED_BODY()

public:
    /**
     * Binds to a new inventory Model. Unsubscribes the currently bound Model, subscribes
     * the new one, and refreshes every slot. Safe to call more than once; a null New
     * simply unbinds (Verbose).
     */
    void BindModel(class UEWEInventoryComponent *Old, class UEWEInventoryComponent *New);

    // Client requests. Each forwards to the bound Model's Server_* RPC; the server runs
    // the gate and the result comes back through the Model's replication.
    void SelectSlot(int32 SlotIndex);
    void ScrollSlot(float Direction);
    void AssignToSlot(class UEWEWeaponInstance *Instance, int32 SlotIndex);
    void ClearSlot(int32 SlotIndex);

    UFUNCTION(BlueprintPure, FieldNotify) int32 GetSelectedIndex() const { return SelectedIndex; }
    UFUNCTION(BlueprintPure, FieldNotify) int32 GetSlotCount() const { return SlotItems.Num(); }

    UFUNCTION(BlueprintPure)
    UEWEQuickSlotItemViewModel *GetSlotItem(int32 SlotIndex) const;

protected:
    /** Model broadcast: the slot contents changed. Refreshes every slot. */
    void OnModelQuickSlotsChanged();

    /** Model broadcast: the selected index changed. Refreshes the selection highlight. */
    void OnModelSelectedQuickSlotChanged(int32 SlotIndex);

    /** Sizes the item array to the Model and mirrors its contents and selection. */
    void UpdateFromModel();

    UPROPERTY(BlueprintReadOnly, FieldNotify)
    TArray<TObjectPtr<UEWEQuickSlotItemViewModel>> SlotItems;

    /** INDEX_NONE until the Model reports a selection. */
    UPROPERTY(BlueprintReadOnly, FieldNotify)
    int32 SelectedIndex = INDEX_NONE;

    /** The inventory Model being observed. Weak: the pawn may be replaced. */
    TWeakObjectPtr<class UEWEInventoryComponent> BoundModel;

    FDelegateHandle QuickSlotsChangedHandle;
    FDelegateHandle SelectedQuickSlotChangedHandle;
};
