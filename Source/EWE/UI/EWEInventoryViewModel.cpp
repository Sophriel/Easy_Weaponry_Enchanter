// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEInventoryViewModel.h"
#include "EWE/UI/EWESlotItemViewModel.h"
#include "EWE/Character/EWEInventoryComponent.h"
#include "EasyWeaponryEnchanter/Weapon/EWEWeaponInstance.h"
#include "EWE/EWELog.h"

void UEWEInventoryViewModel::BindModel(UEWEInventoryComponent *Old, UEWEInventoryComponent *New)
{
    // Unsubscribe whatever is currently bound, regardless of what Old is. This keeps the
    // function idempotent: a repeated call with the same Model just re-subscribes.
    UEWEInventoryComponent *Current = BoundModel.Get();
    if (Current)
    {
        Current->OnItemSlotChanged.Remove(ItemSlotChangedHandle);
    }
    ItemSlotChangedHandle.Reset();

    BoundModel = New;

    // A new Model is a fresh inventory — the previous selection no longer applies.
    ClearSelection();

    if (!New)
    {
        EWE_LOG(LogEWE, Verbose, TEXT("BindModel: no model to bind"));
        return;
    }

    ItemSlotChangedHandle = New->OnItemSlotChanged.AddUObject(this, &UEWEInventoryViewModel::OnItemSlotChanged);

    UpdateFromModel();
}

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

void UEWEInventoryViewModel::UpdateFromModel()
{
    UEWEInventoryComponent *Model = BoundModel.Get();
    if (!Model)
        return;

    // Size the item array to the Model's slot count. The count is a developer setting,
    // fixed for the session, so the array is resized (and GetSlotCount broadcast) only
    // on the first bind; later binds reuse the item view models.
    const int32 SlotCount = Model->GetInventorySlotCount();
    const bool bSlotCountChanged = SlotItems.Num() != SlotCount;
    if (bSlotCountChanged)
    {
        SlotItems.SetNum(SlotCount);
    }
    for (int32 i = 0; i < SlotItems.Num(); ++i)
    {
        if (!SlotItems[i])
        {
            SlotItems[i] = NewObject<UEWESlotItemViewModel>(this);
        }
        SlotItems[i]->SetSlotIndex(i);
    }

    // Mirror the slot contents.
    const TArray<TObjectPtr<UEWEWeaponInstance>> &Slots = Model->GetItemSlots();
    for (int32 i = 0; i < SlotItems.Num(); ++i)
    {
        UEWESlotItemViewModel *SlotItem = SlotItems[i];
        if (!SlotItem)
            continue;

        UEWEWeaponInstance *Instance = Slots.IsValidIndex(i) ? Slots[i].Get() : nullptr;
        SlotItem->SetInstance(Instance);
    }

    // Re-apply the selection to the freshly mirrored slots.
    ApplySelection();

    // Broadcast after the item view models exist so subscribers can read GetSlotItem(i)
    // in their callback.
    if (bSlotCountChanged)
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetSlotCount);
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

UEWESlotItemViewModel *UEWEInventoryViewModel::GetSlotItem(int32 SlotIndex) const
{
    if (!SlotItems.IsValidIndex(SlotIndex))
        return nullptr;
    return SlotItems[SlotIndex];
}
