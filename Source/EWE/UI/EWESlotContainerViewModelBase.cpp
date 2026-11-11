// Fill out your copyright notice in the Description page of Project Settings.

#include "EWESlotContainerViewModelBase.h"
#include "EWE/UI/EWESlotItemViewModel.h"
#include "EWE/Character/EWEInventoryComponent.h"
#include "EasyWeaponryEnchanter/Weapon/EWEWeaponInstance.h"
#include "EWE/EWELog.h"

void UEWESlotContainerViewModelBase::BindModel(UEWEInventoryComponent *Old, UEWEInventoryComponent *New)
{
    // Unsubscribe whatever is currently bound, regardless of what Old is. This keeps the
    // function idempotent: a repeated call with the same Model just re-subscribes.
    UEWEInventoryComponent *Current = BoundModel.Get();
    if (Current)
    {
        UnsubscribeModel(*Current);
    }

    BoundModel = New;

    OnModelBound();

    if (!New)
    {
        EWE_LOG(LogEWE, Verbose, TEXT("BindModel: no model to bind"));
        return;
    }

    SubscribeModel(*New);
    UpdateFromModel();
}

void UEWESlotContainerViewModelBase::UpdateFromModel()
{
    UEWEInventoryComponent *Model = BoundModel.Get();
    if (!Model)
        return;

    // Size the item array to the Model's slot count. The count is a developer setting,
    // fixed for the session, so the array is resized (and GetSlotCount broadcast) only
    // on the first bind; later binds reuse the item view models.
    const int32 SlotCount = GetModelSlotCount(*Model);
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
    const TArray<TObjectPtr<UEWEWeaponInstance>> &Slots = GetModelSlots(*Model);
    for (int32 i = 0; i < SlotItems.Num(); ++i)
    {
        UEWESlotItemViewModel *SlotItem = SlotItems[i];
        if (!SlotItem)
            continue;

        UEWEWeaponInstance *Instance = Slots.IsValidIndex(i) ? Slots[i].Get() : nullptr;
        SlotItem->SetInstance(Instance);
    }

    // Broadcast after the item view models exist so subscribers can read GetSlotItem(i)
    // in their callback.
    if (bSlotCountChanged)
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetSlotCount);
    }
}

UEWESlotItemViewModel *UEWESlotContainerViewModelBase::GetSlotItem(int32 SlotIndex) const
{
    if (!SlotItems.IsValidIndex(SlotIndex))
        return nullptr;
    return SlotItems[SlotIndex];
}
