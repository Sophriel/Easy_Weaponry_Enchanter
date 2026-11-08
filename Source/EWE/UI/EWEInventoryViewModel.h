// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "EWEInventoryViewModel.generated.h"

class UEWEInventoryComponent;
class UEWESlotItemViewModel;

/**
 * View model for the inventory grid.
 *
 * Observer, not owner: the inventory component (Model) owns the item slots. This view model
 * subscribes to the Model's per-slot change broadcast and mirrors each slot into a slot item
 * view model. Selection is local UI state owned here — it is not replicated and is unrelated
 * to the Model's quick slot selection. Assigning the selected item to a quick slot is
 * forwarded to the Model's Server_* RPC; the server is the single writer.
 */
UCLASS(BlueprintType)
class EWE_API UEWEInventoryViewModel : public UMVVMViewModelBase
{
    GENERATED_BODY()

public:
    /**
     * Binds to a new inventory Model. Unsubscribes the currently bound Model, clears the
     * selection, subscribes the new one, and pulls the current state. Safe to call more than
     * once; a null New simply unbinds (Verbose).
     */
    void BindModel(class UEWEInventoryComponent *Old, class UEWEInventoryComponent *New);

    /** Selects an inventory slot. Only a slot that holds an item can be selected; empty or
     *  out-of-range slots are ignored. */
    void SelectItemSlot(int32 SlotIndex);

    /** Clears the selection. Called by the view when the inventory is closed. */
    void ClearSelection();

    /** Assigns the selected item to a quick slot. Keeps the selection. Ignored when nothing
     *  is selected or no Model is bound. */
    void AssignSelectedToQuickSlot(int32 QuickSlotIndex);

    // TODO: When the inventory widget is implemented, broadcast GetSlotCount in
    // UpdateFromModel when the slot array is resized, so a widget can receive the slot
    // count through a binding.
    UFUNCTION(BlueprintPure, FieldNotify) int32 GetSlotCount() const { return SlotItems.Num(); }

    /**
     * The inventory slot currently selected in the UI. INDEX_NONE when nothing is selected.
     * Local UI state: not replicated, unrelated to the Model's SelectedQuickSlotIndex, and
     * never written back to the Model.
     */
    UFUNCTION(BlueprintPure, FieldNotify) int32 GetSelectedItemSlotIndex() const { return SelectedItemSlotIndex; }

    UFUNCTION(BlueprintPure)
    UEWESlotItemViewModel *GetSlotItem(int32 SlotIndex) const;

protected:
    /** Model broadcast: one item slot's content changed. Refreshes that slot and clears the
     *  selection if it was the selected slot. */
    void OnItemSlotChanged(int32 SlotIndex);

    /** Sizes the item array to the Model and mirrors every slot's content. */
    void UpdateFromModel();

    /** Applies the current selection to the slot item view models. */
    void ApplySelection();

    UPROPERTY(BlueprintReadOnly, FieldNotify)
    TArray<TObjectPtr<UEWESlotItemViewModel>> SlotItems;

    /** INDEX_NONE until a slot is selected. Local UI state — see GetSelectedItemSlotIndex. */
    UPROPERTY(BlueprintReadOnly, FieldNotify)
    int32 SelectedItemSlotIndex = INDEX_NONE;

    /** The inventory Model being observed. Weak: the pawn may be replaced. */
    TWeakObjectPtr<class UEWEInventoryComponent> BoundModel;

    FDelegateHandle ItemSlotChangedHandle;
};
