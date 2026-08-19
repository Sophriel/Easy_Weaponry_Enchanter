// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEQuickSlotViewModel.h"
#include "EWE/UI/EWEQuickSlotItemViewModel.h"
#include "EasyWeaponryEnchanter/Weapon/EWEWeaponData.h"
#include "EWE/EWELog.h"

void UEWEQuickSlotViewModel::InitializeSlots(int32 InSlotCount)
{
    if (SlotItems.Num() == InSlotCount)
        return;

    SlotItems.SetNum(InSlotCount);

    for (int32 i = 0; i < InSlotCount; ++i)
    {
        SlotItems[i] = NewObject<UEWEQuickSlotItemViewModel>(this);
        SlotItems[i]->SetSlotIndex(i);
    }
}

void UEWEQuickSlotViewModel::SetSlot(int32 SlotIndex, UEWEWeaponData *Weapon)
{
    // Unsized is the expected state before the widget attaches and sizes the view
    // model. Defer silently; a sized-but-out-of-range index still warns below.
    if (SlotItems.Num() == 0)
    {
        EWE_LOG(LogEWE, Verbose, TEXT("SetSlot: view model not sized yet, ignoring"));
        return;
    }

    if (!SlotItems.IsValidIndex(SlotIndex))
    {
        EWE_LOG(LogEWE, Warning, TEXT("SetSlot: index %d out of range (%d slots)"), SlotIndex, SlotItems.Num());
        return;
    }

    UEWEQuickSlotItemViewModel *SlotItem = SlotItems[SlotIndex];
    if (!SlotItem)
        return;

    if (SlotItem->GetWeapon() == Weapon)
        return;

    SlotItem->SetWeapon(Weapon);

    // Changing the contents of the slot the player is holding must re-equip.
    // SelectSlot() cannot do this: the index did not change.
    if (SlotIndex == SelectedIndex)
    {
        OnSlotSelected.Broadcast(SlotIndex, Weapon);
    }
}

int32 UEWEQuickSlotViewModel::AddWeapon(UEWEWeaponData *Weapon)
{
    // Unsized is the expected state before the widget attaches and sizes the view
    // model. Defer silently; the SetSlot() path below has its own guard, so this
    // one is still required to avoid a spurious "no empty slot" warning.
    if (SlotItems.Num() == 0)
    {
        EWE_LOG(LogEWE, Verbose, TEXT("AddWeapon: view model not sized yet, ignoring"));
        return INDEX_NONE;
    }

    for (int32 i = 0; i < SlotItems.Num(); ++i)
    {
        if (SlotItems[i] && SlotItems[i]->IsEmpty())
        {
            // Route through SetSlot so the re-equip path applies here too.
            // Filling the currently selected slot must equip the weapon.
            SetSlot(i, Weapon);
            return i;
        }
    }

    EWE_LOG(LogEWE, Warning, TEXT("AddWeapon: no empty slot available"));
    return INDEX_NONE;
}

void UEWEQuickSlotViewModel::ClearSlot(int32 SlotIndex)
{
    // Route through SetSlot: clearing the currently held slot must broadcast
    // nullptr (unarmed), which only the re-equip path does. The range check and
    // any unsized guard are inherited from SetSlot.
    SetSlot(SlotIndex, nullptr);
}

void UEWEQuickSlotViewModel::SelectSlot(int32 SlotIndex)
{
    // Unsized is the expected state before the widget attaches and sizes the view
    // model. Defer silently; a sized-but-out-of-range index still warns below.
    if (SlotItems.Num() == 0)
    {
        EWE_LOG(LogEWE, Verbose, TEXT("SelectSlot: view model not sized yet, ignoring"));
        return;
    }

    if (!SlotItems.IsValidIndex(SlotIndex))
    {
        EWE_LOG(LogEWE, Warning, TEXT("SelectSlot: index %d out of range (%d slots)"), SlotIndex, SlotItems.Num());
        return;
    }

    // Re-selecting the current slot changes nothing, so there is nothing to broadcast.
    if (SlotIndex == SelectedIndex)
        return;

    // Deselect previous
    if (SlotItems.IsValidIndex(SelectedIndex) && SlotItems[SelectedIndex])
    {
        SlotItems[SelectedIndex]->SetSelected(false);
    }

    if (UE_MVVM_SET_PROPERTY_VALUE(SelectedIndex, SlotIndex))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetSelectedIndex);
    }

    UEWEQuickSlotItemViewModel *NewSlotItem = SlotItems[SlotIndex];
    if (!NewSlotItem)
        return;

    NewSlotItem->SetSelected(true);

    // Empty slot broadcasts nullptr on purpose: selecting an empty slot unequips.
    OnSlotSelected.Broadcast(SlotIndex, NewSlotItem->GetWeapon());
}

void UEWEQuickSlotViewModel::ScrollSlot(float Direction)
{
    if (SlotItems.Num() == 0)
        return;

    // Nothing selected yet: start at the first slot instead of wrapping from INDEX_NONE.
    if (!SlotItems.IsValidIndex(SelectedIndex))
    {
        SelectSlot(0);
        return;
    }

    int32 NextIndex = SelectedIndex;

    if (Direction > 0.0f)
        NextIndex = (SelectedIndex + 1) % SlotItems.Num();
    else if (Direction < 0.0f)
        NextIndex = (SelectedIndex - 1 + SlotItems.Num()) % SlotItems.Num();

    SelectSlot(NextIndex);
}

bool UEWEQuickSlotViewModel::IsEmpty() const
{
    for (const auto &SlotItem : SlotItems)
    {
        if (SlotItem && !SlotItem->IsEmpty())
            return false;
    }
    return true;
}

void UEWEQuickSlotViewModel::SeedFromInventory(const TArray<UEWEWeaponData *> &InWeapons)
{
    // Unsized is the expected state before the widget attaches and sizes the view
    // model. Defer silently; the slots simply do not exist yet.
    if (SlotItems.Num() == 0)
    {
        EWE_LOG(LogEWE, Verbose, TEXT("SeedFromInventory: view model not sized yet, ignoring"));
        return;
    }

    if (!IsEmpty())
        return;

    for (int32 i = 0; i < InWeapons.Num(); ++i)
    {
        if (!SlotItems.IsValidIndex(i))
        {
            EWE_LOG(LogEWE, Warning, TEXT("SeedFromInventory: more weapons (%d) than slots (%d), skipping weapon %d"),
                    InWeapons.Num(), SlotItems.Num(), i);
            break;
        }

        // Route through SetSlot so the re-equip path applies here too.
        SetSlot(i, InWeapons[i]);
    }
}

UEWEQuickSlotItemViewModel *UEWEQuickSlotViewModel::GetSlotItem(int32 SlotIndex) const
{
    if (!SlotItems.IsValidIndex(SlotIndex))
        return nullptr;
    return SlotItems[SlotIndex];
}
