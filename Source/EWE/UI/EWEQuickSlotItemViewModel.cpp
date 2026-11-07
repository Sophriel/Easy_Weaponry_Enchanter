// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEQuickSlotItemViewModel.h"
#include "EasyWeaponryEnchanter/Weapon/EWEWeaponInstance.h"
#include "EasyWeaponryEnchanter/Weapon/EWEWeaponData.h"

void UEWEQuickSlotItemViewModel::BeginDestroy()
{
    UnsubscribeLoadFinished();
    Super::BeginDestroy();
}

void UEWEQuickSlotItemViewModel::SetInstance(UEWEWeaponInstance *InInstance)
{
    if (WeaponInstance == InInstance)
    {
        return;   // Same instance — any existing subscription is still valid.
    }

    // Drop the previous instance's load-finished subscription before switching.
    UnsubscribeLoadFinished();

    if (UE_MVVM_SET_PROPERTY_VALUE(WeaponInstance, InInstance))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsEmpty);
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetWeaponName);
    }

    // If the new instance is still loading, subscribe to its load-finished notification so
    // the name refreshes once the data arrives. An already-finished instance needs no
    // subscription — GetWeaponName above already reflects its loaded (or absent) data.
    if (InInstance && !InInstance->IsLoadFinished())
    {
        LoadSubscriptionInstance = InInstance;
        LoadFinishedHandle = InInstance->OnLoadFinished.AddUObject(this, &UEWEQuickSlotItemViewModel::HandleLoadFinished);
    }
}

void UEWEQuickSlotItemViewModel::HandleLoadFinished(UEWEWeaponInstance *FinishedInstance)
{
    // Only react if this is still the instance shown in this slot.
    if (FinishedInstance != WeaponInstance)
    {
        return;
    }

    UnsubscribeLoadFinished();
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetWeaponName);
}

void UEWEQuickSlotItemViewModel::UnsubscribeLoadFinished()
{
    UEWEWeaponInstance *Subscribed = LoadSubscriptionInstance.Get();
    if (Subscribed)
    {
        Subscribed->OnLoadFinished.Remove(LoadFinishedHandle);
    }
    LoadSubscriptionInstance = nullptr;
    LoadFinishedHandle.Reset();
}

void UEWEQuickSlotItemViewModel::SetSelected(bool bInSelected)
{
    if (UE_MVVM_SET_PROPERTY_VALUE(bSelected, bInSelected))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsSelected);
    }
}

void UEWEQuickSlotItemViewModel::SetSlotIndex(int32 InSlotIndex)
{
    // Fixed once at creation, before any widget is bound. Plain assignment:
    // there is nothing derived from it and no observer to notify.
    SlotIndex = InSlotIndex;
}

FText UEWEQuickSlotItemViewModel::GetWeaponName() const
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
