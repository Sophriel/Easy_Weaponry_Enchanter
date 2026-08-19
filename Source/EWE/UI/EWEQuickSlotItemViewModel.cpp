// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEQuickSlotItemViewModel.h"
#include "EasyWeaponryEnchanter/Weapon/EWEWeaponData.h"

void UEWEQuickSlotItemViewModel::SetWeapon(UEWEWeaponData *InWeapon)
{
    if (UE_MVVM_SET_PROPERTY_VALUE(Weapon, InWeapon))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsEmpty);
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetWeaponName);
    }
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
    if (Weapon)
    {
        return FText::FromString(Weapon->GetName());
    }
    return FText::GetEmpty();
}
