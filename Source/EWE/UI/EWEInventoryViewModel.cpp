// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEInventoryViewModel.h"
#include "EWE/UI/EWESlotItemViewModel.h"
#include "EWE/Character/EWEInventoryComponent.h"
#include "EasyWeaponryEnchanter/Weapon/EWEWeaponInstance.h"
#include "EWE/EWELog.h"

#pragma region UEWESlotContainerViewModelBase

// --- UEWESlotContainerViewModelBase override ---

int32 UEWEInventoryViewModel::GetModelSlotCount(const UEWEInventoryComponent &Model) const
{
    return Model.GetInventorySlotCount();
}

const TArray<TObjectPtr<UEWEWeaponInstance>> &UEWEInventoryViewModel::GetModelSlots(const UEWEInventoryComponent &Model) const
{
    return Model.GetItemSlots();
}

void UEWEInventoryViewModel::SubscribeModel(UEWEInventoryComponent &Model)
{
    ItemSlotChangedHandle = Model.OnItemSlotChanged.AddUObject(this, &UEWEInventoryViewModel::OnItemSlotChanged);
}

void UEWEInventoryViewModel::UnsubscribeModel(UEWEInventoryComponent &Model)
{
    Model.OnItemSlotChanged.Remove(ItemSlotChangedHandle);
    ItemSlotChangedHandle.Reset();
}

void UEWEInventoryViewModel::OnModelBound()
{
    // A new Model is a fresh inventory — the previous selection no longer applies.
    ClearSelection();
}

void UEWEInventoryViewModel::UpdateFromModel()
{
    Super::UpdateFromModel();

    ApplySelection();
}

#pragma endregion

void UEWEInventoryViewModel::SelectItemSlot(int32 SlotIndex)
{
    UEWEInventoryComponent *Model = BoundModel.Get();
    if (!Model)
    {
        EWE_LOG(LogEWE, Verbose, TEXT("SelectItemSlot: no model bound"));
        return;
    }

    // Only a slot that holds an item can be selected. An empty or out-of-range slot is an
    // expected state, not an error — pass quietly.
    const TArray<TObjectPtr<UEWEWeaponInstance>> &Slots = Model->GetItemSlots();
    if (!Slots.IsValidIndex(SlotIndex) || !Slots[SlotIndex])
    {
        EWE_LOG(LogEWE, Verbose, TEXT("SelectItemSlot: slot %d is empty or out of range"), SlotIndex);
        return;
    }

    if (UE_MVVM_SET_PROPERTY_VALUE(SelectedItemSlotIndex, SlotIndex))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetSelectedItemSlotIndex);
    }
    ApplySelection();
}

void UEWEInventoryViewModel::ClearSelection()
{
    if (SelectedItemSlotIndex == INDEX_NONE)
    {
        return;   // Nothing selected — nothing to clear.
    }

    if (UE_MVVM_SET_PROPERTY_VALUE(SelectedItemSlotIndex, INDEX_NONE))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetSelectedItemSlotIndex);
    }
    ApplySelection();
}

void UEWEInventoryViewModel::AssignSelectedToQuickSlot(int32 QuickSlotIndex)
{
    UEWEInventoryComponent *Model = BoundModel.Get();
    if (!Model)
    {
        EWE_LOG(LogEWE, Verbose, TEXT("AssignSelectedToQuickSlot: no model bound"));
        return;
    }

    // Nothing selected — nothing to assign.
    if (SelectedItemSlotIndex == INDEX_NONE)
    {
        EWE_LOG(LogEWE, Verbose, TEXT("AssignSelectedToQuickSlot: no item selected"));
        return;
    }

    const TArray<TObjectPtr<UEWEWeaponInstance>> &Slots = Model->GetItemSlots();
    if (!Slots.IsValidIndex(SelectedItemSlotIndex))
    {
        EWE_LOG(LogEWE, Verbose, TEXT("AssignSelectedToQuickSlot: selected slot is out of range"));
        return;
    }

    UEWEWeaponInstance *Instance = Slots[SelectedItemSlotIndex].Get();
    if (!Instance)
    {
        EWE_LOG(LogEWE, Verbose, TEXT("AssignSelectedToQuickSlot: selected slot is empty"));
        return;
    }

    // The selection is kept — assigning does not change the inventory slot.
    Model->Server_AssignToQuickSlot(Instance, QuickSlotIndex);
}

void UEWEInventoryViewModel::OnItemSlotChanged(int32 SlotIndex)
{
    UEWEInventoryComponent *Model = BoundModel.Get();
    if (!Model)
        return;

    // Refresh just this slot's content.
    const TArray<TObjectPtr<UEWEWeaponInstance>> &Slots = Model->GetItemSlots();
    UEWEWeaponInstance *Instance = Slots.IsValidIndex(SlotIndex) ? Slots[SlotIndex].Get() : nullptr;
    if (SlotItems.IsValidIndex(SlotIndex) && SlotItems[SlotIndex])
    {
        SlotItems[SlotIndex]->SetInstance(Instance);
    }

    // The selected slot's content changed (emptied or replaced) — drop the selection.
    if (SelectedItemSlotIndex == SlotIndex)
    {
        ClearSelection();
    }
}

void UEWEInventoryViewModel::ApplySelection()
{
    for (int32 i = 0; i < SlotItems.Num(); ++i)
    {
        UEWESlotItemViewModel *SlotItem = SlotItems[i];
        if (SlotItem)
        {
            SlotItem->SetSelected(i == SelectedItemSlotIndex);
        }
    }
}
