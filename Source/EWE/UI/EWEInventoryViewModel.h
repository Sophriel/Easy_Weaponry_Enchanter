// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EWESlotContainerViewModelBase.h"
#include "EWEInventoryViewModel.generated.h"

class UEWEInventoryComponent;
class UEWEWeaponInstance;

/**
 * View model for the inventory grid.
 *
 * Observer, not owner: the inventory component (Model) owns the item slots. This view model
 * subscribes to the Model's per-slot change broadcast and mirrors each slot into a slot item
 * view model. Selection is local UI state owned here — it is not replicated and is unrelated
 * to the Model's quick slot selection. Assigning the selected item to a quick slot is
 * forwarded to the Model's Server_* RPC; the server is the single writer.
 *
 * The slot item array, Model binding, and the full-mirror refresh live in the shared base
 * (UEWESlotContainerViewModelBase); this class provides the inventory Model accessors, the
 * per-slot subscription, and local selection handling.
 */
UCLASS(BlueprintType)
class EWE_API UEWEInventoryViewModel : public UEWESlotContainerViewModelBase
{
    GENERATED_BODY()

public:
    /** Selects an inventory slot. Only a slot that holds an item can be selected; empty or
     *  out-of-range slots are ignored. */
    void SelectItemSlot(int32 SlotIndex);

    /** Clears the selection. Called by the view when the inventory is closed. */
    void ClearSelection();

    /** Assigns the selected item to a quick slot. Keeps the selection. Ignored when nothing
     *  is selected or no Model is bound. */
    void AssignSelectedToQuickSlot(int32 QuickSlotIndex);

    /**
     * The inventory slot currently selected in the UI. INDEX_NONE when nothing is selected.
     * Local UI state: not replicated, unrelated to the Model's SelectedQuickSlotIndex, and
     * never written back to the Model.
     */
    UFUNCTION(BlueprintPure, FieldNotify) int32 GetSelectedItemSlotIndex() const { return SelectedItemSlotIndex; }

protected:
    /** Model broadcast: one item slot's content changed. Refreshes that slot and clears the
     *  selection if it was the selected slot. */
    void OnItemSlotChanged(int32 SlotIndex);

    // --- UEWESlotContainerViewModelBase hooks ---

    int32 GetModelSlotCount(const class UEWEInventoryComponent &Model) const override;
    const TArray<TObjectPtr<class UEWEWeaponInstance>> &GetModelSlots(const class UEWEInventoryComponent &Model) const override;
    void SubscribeModel(class UEWEInventoryComponent &Model) override;
    void UnsubscribeModel(class UEWEInventoryComponent &Model) override;
    void OnModelBound() override;
    void UpdateFromModel() override;

    /** Applies the current selection to the slot item view models. */
    void ApplySelection();

    /** INDEX_NONE until a slot is selected. Local UI state — see GetSelectedItemSlotIndex. */
    UPROPERTY(BlueprintReadOnly, FieldNotify)
    int32 SelectedItemSlotIndex = INDEX_NONE;

    FDelegateHandle ItemSlotChangedHandle;
};
