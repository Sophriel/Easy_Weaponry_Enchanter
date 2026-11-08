// Fill out your copyright notice in the Description page of Project Settings.

#include "EWESlotItemViewModel.h"
#include "EasyWeaponryEnchanter/Weapon/EWEWeaponInstance.h"
#include "EasyWeaponryEnchanter/Weapon/EWEWeaponData.h"

void UEWESlotItemViewModel::BeginDestroy()
{
    UnsubscribeInstance();
    Super::BeginDestroy();
}

void UEWESlotItemViewModel::SetInstance(UEWEWeaponInstance *InInstance)
{
    if (WeaponInstance == InInstance)
    {
        return;   // Same instance — the existing subscriptions are still valid.
    }

    // Drop the previous instance's subscriptions before switching.
    UnsubscribeInstance();

    if (UE_MVVM_SET_PROPERTY_VALUE(WeaponInstance, InInstance))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsEmpty);
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetWeaponName);
    }

    // Stay subscribed to the displayed instance for as long as it is shown, so both a late
    // load finish and any later item-state change refresh the name. An already-finished
    // instance simply never fires OnLoadFinished again — subscribing is harmless.
    if (InInstance)
    {
        SubscribedInstance = InInstance;
        LoadFinishedHandle = InInstance->OnLoadFinished.AddUObject(this, &UEWESlotItemViewModel::HandleLoadFinished);
        ItemChangedHandle = InInstance->OnItemChanged.AddUObject(this, &UEWESlotItemViewModel::HandleItemChanged);
    }
}

void UEWESlotItemViewModel::HandleLoadFinished(UEWEWeaponInstance *FinishedInstance)
{
    // Only react if this is still the instance shown in this slot.
    if (FinishedInstance != WeaponInstance)
    {
        return;
    }

    // The subscription stays for the whole display lifetime; only the name needs refreshing.
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetWeaponName);
}

void UEWESlotItemViewModel::HandleItemChanged(UEWEWeaponInstance *ChangedInstance)
{
    // Only react if this is still the instance shown in this slot.
    if (ChangedInstance != WeaponInstance)
    {
        return;
    }

    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetWeaponName);
}

void UEWESlotItemViewModel::UnsubscribeInstance()
{
    UEWEWeaponInstance *Subscribed = SubscribedInstance.Get();
    if (Subscribed)
    {
        Subscribed->OnLoadFinished.Remove(LoadFinishedHandle);
        Subscribed->OnItemChanged.Remove(ItemChangedHandle);
    }
    SubscribedInstance = nullptr;
    LoadFinishedHandle.Reset();
    ItemChangedHandle.Reset();
}

void UEWESlotItemViewModel::SetSelected(bool bInSelected)
{
    if (UE_MVVM_SET_PROPERTY_VALUE(bSelected, bInSelected))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsSelected);
    }
}

void UEWESlotItemViewModel::SetSlotIndex(int32 InSlotIndex)
{
    // Fixed once at creation, before any widget is bound. Plain assignment:
    // there is nothing derived from it and no observer to notify.
    SlotIndex = InSlotIndex;
}

FText UEWESlotItemViewModel::GetWeaponName() const
{
    // The display name comes from the loaded definition. An instance that has not
    // finished loading (or failed) has no name yet — an empty string is expected.
    UEWEWeaponData *LoadedData = WeaponInstance ? WeaponInstance->GetLoadedWeaponData() : nullptr;
    if (LoadedData)
    {
        return FText::FromString(LoadedData->GetName());
    }
    return FText::GetEmpty();
}
