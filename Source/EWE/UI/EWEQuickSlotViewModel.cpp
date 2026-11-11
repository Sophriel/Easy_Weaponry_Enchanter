// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEQuickSlotViewModel.h"
#include "EWE/UI/EWESlotItemViewModel.h"
#include "EWE/Character/EWEInventoryComponent.h"
#include "EasyWeaponryEnchanter/Weapon/EWEWeaponInstance.h"
#include "EWE/EWELog.h"

#pragma region UEWESlotContainerViewModelBase

// --- UEWESlotContainerViewModelBase override ---

int32 UEWEQuickSlotViewModel::GetModelSlotCount(const UEWEInventoryComponent &Model) const
{
    return Model.GetQuickSlotCount();
}

const TArray<TObjectPtr<UEWEWeaponInstance>> &UEWEQuickSlotViewModel::GetModelSlots(const UEWEInventoryComponent &Model) const
{
    return Model.GetQuickSlots();
}

void UEWEQuickSlotViewModel::SubscribeModel(UEWEInventoryComponent &Model)
{
    QuickSlotsChangedHandle = Model.OnQuickSlotsChanged.AddUObject(this, &UEWEQuickSlotViewModel::OnModelQuickSlotsChanged);
    SelectedQuickSlotChangedHandle = Model.OnSelectedQuickSlotChanged.AddUObject(this, &UEWEQuickSlotViewModel::OnModelSelectedQuickSlotChanged);
}

void UEWEQuickSlotViewModel::UnsubscribeModel(UEWEInventoryComponent &Model)
{
    Model.OnQuickSlotsChanged.Remove(QuickSlotsChangedHandle);
    Model.OnSelectedQuickSlotChanged.Remove(SelectedQuickSlotChangedHandle);
    QuickSlotsChangedHandle.Reset();
    SelectedQuickSlotChangedHandle.Reset();
}

void UEWEQuickSlotViewModel::UpdateFromModel()
{
    Super::UpdateFromModel();

    UEWEInventoryComponent *Model = BoundModel.Get();
    if (!Model)
        return;

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

#pragma endregion

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
