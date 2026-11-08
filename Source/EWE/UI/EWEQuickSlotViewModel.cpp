// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEQuickSlotViewModel.h"
#include "EWE/UI/EWESlotItemViewModel.h"
#include "EWE/Character/EWEInventoryComponent.h"
#include "EasyWeaponryEnchanter/Weapon/EWEWeaponInstance.h"
#include "EWE/EWELog.h"

void UEWEQuickSlotViewModel::BindModel(UEWEInventoryComponent *Old, UEWEInventoryComponent *New)
{
    // Unsubscribe whatever is currently bound, regardless of what Old is. This keeps the
    // function idempotent: a repeated call with the same Model just re-subscribes.
    UEWEInventoryComponent *Current = BoundModel.Get();
    if (Current)
    {
        Current->OnQuickSlotsChanged.Remove(QuickSlotsChangedHandle);
        Current->OnSelectedQuickSlotChanged.Remove(SelectedQuickSlotChangedHandle);
    }
    QuickSlotsChangedHandle.Reset();
    SelectedQuickSlotChangedHandle.Reset();

    BoundModel = New;

    if (!New)
    {
        EWE_LOG(LogEWE, Verbose, TEXT("BindModel: no model to bind"));
        return;
    }

    QuickSlotsChangedHandle = New->OnQuickSlotsChanged.AddUObject(this, &UEWEQuickSlotViewModel::OnModelQuickSlotsChanged);
    SelectedQuickSlotChangedHandle = New->OnSelectedQuickSlotChanged.AddUObject(this, &UEWEQuickSlotViewModel::OnModelSelectedQuickSlotChanged);

    UpdateFromModel();
}

void UEWEQuickSlotViewModel::SelectSlot(int32 SlotIndex)
{
    UEWEInventoryComponent *Model = BoundModel.Get();
    if (!Model)
    {
        EWE_LOG(LogEWE, Verbose, TEXT("SelectSlot: no model bound"));
        return;
    }
    Model->Server_SelectQuickSlot(SlotIndex);
}

void UEWEQuickSlotViewModel::ScrollSlot(float Direction)
{
    UEWEInventoryComponent *Model = BoundModel.Get();
    if (!Model)
    {
        EWE_LOG(LogEWE, Verbose, TEXT("ScrollSlot: no model bound"));
        return;
    }
    Model->Server_ScrollQuickSlot(Direction);
}

void UEWEQuickSlotViewModel::AssignToSlot(UEWEWeaponInstance *Instance, int32 SlotIndex)
{
    UEWEInventoryComponent *Model = BoundModel.Get();
    if (!Model)
    {
        EWE_LOG(LogEWE, Verbose, TEXT("AssignToSlot: no model bound"));
        return;
    }
    Model->Server_AssignToQuickSlot(Instance, SlotIndex);
}

void UEWEQuickSlotViewModel::ClearSlot(int32 SlotIndex)
{
    UEWEInventoryComponent *Model = BoundModel.Get();
    if (!Model)
    {
        EWE_LOG(LogEWE, Verbose, TEXT("ClearSlot: no model bound"));
        return;
    }
    Model->Server_ClearQuickSlot(SlotIndex);
}

void UEWEQuickSlotViewModel::OnModelQuickSlotsChanged()
{
    // A full mirror is idempotent and cheap for a fixed-size slot bar.
    UpdateFromModel();
}

void UEWEQuickSlotViewModel::OnModelSelectedQuickSlotChanged(int32 SlotIndex)
{
    UpdateFromModel();
}

void UEWEQuickSlotViewModel::UpdateFromModel()
{
    UEWEInventoryComponent *Model = BoundModel.Get();
    if (!Model)
        return;

    // Size the item array to the Model's slot count. The count is a developer setting,
    // fixed for the session, so this runs once per bind.
    const int32 SlotCount = Model->GetQuickSlotCount();
    if (SlotItems.Num() != SlotCount)
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
    const TArray<TObjectPtr<UEWEWeaponInstance>> &Slots = Model->GetQuickSlots();
    for (int32 i = 0; i < SlotItems.Num(); ++i)
    {
        UEWESlotItemViewModel *SlotItem = SlotItems[i];
        if (!SlotItem)
            continue;

        UEWEWeaponInstance *Instance = Slots.IsValidIndex(i) ? Slots[i].Get() : nullptr;
        SlotItem->SetInstance(Instance);
    }

    // Mirror the selection.
    const int32 Selected = Model->GetSelectedQuickSlotIndex();
    for (int32 i = 0; i < SlotItems.Num(); ++i)
    {
        UEWESlotItemViewModel *SlotItem = SlotItems[i];
        if (SlotItem)
        {
            SlotItem->SetSelected(i == Selected);
        }
    }

    if (UE_MVVM_SET_PROPERTY_VALUE(SelectedIndex, Selected))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetSelectedIndex);
    }
}

UEWESlotItemViewModel *UEWEQuickSlotViewModel::GetSlotItem(int32 SlotIndex) const
{
    if (!SlotItems.IsValidIndex(SlotIndex))
        return nullptr;
    return SlotItems[SlotIndex];
}
