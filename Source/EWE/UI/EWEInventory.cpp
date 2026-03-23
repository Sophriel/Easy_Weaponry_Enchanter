// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEInventory.h"
#include "EWE/EWELog.h"
#include "EWE/UI/EWELocalUIManageSubsystem.h"

UEWEInventory::UEWEInventory(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer)
{
    bIsInventoryOpen = false;
    SelectedItemIndex = -1;
}

void UEWEInventory::NativeConstruct()
{
    Super::NativeConstruct();

    // Initialize item slots (default 8 slots)
    const int32 InitialSlotCount = 8;

    // Check validation once before loop (efficiency optimization)
    if (ItemSlotClasses.IsValidIndex(0))
    {
        TObjectPtr<UUserWidget> ItemSlot;
        for (int32 i = 0; i < InitialSlotCount; ++i)
        {
            ItemSlot = CreateWidget<UUserWidget>(GetWorld(), ItemSlotClasses[0]);
            if (ItemSlot)
            {
                ItemSlots.Add(ItemSlot);
            }
        }
    }
}

void UEWEInventory::OpenInventory()
{
    if (bIsInventoryOpen)
    {
        return;
    }

    bIsInventoryOpen = true;
    SetVisibility(ESlateVisibility::Visible);
    K2_OnInventoryOpened();
    SetMouseCursorVisible(true);

    EWE_LOG(LogEWEInventory, Log, TEXT("Inventory Opened"));
}

void UEWEInventory::CloseInventory()
{
    if (!bIsInventoryOpen)
    {
        return;
    }

    bIsInventoryOpen = false;
    SelectedItemIndex = -1;
    SetVisibility(ESlateVisibility::Hidden);
    K2_OnInventoryClosed();
    SetMouseCursorVisible(false);

    EWE_LOG(LogEWEInventory, Log, TEXT("Inventory Closed"));
}

void UEWEInventory::SetMouseCursorVisible(bool bShow)
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        PC->SetShowMouseCursor(bShow);
    }
}

void UEWEInventory::SetWeapons(const TArray<UEWEWeaponData *> &InWeapons)
{
    Weapons = InWeapons;

    // Update slot UI (implemented in Blueprint)
}

void UEWEInventory::SelectItem(int32 Index)
{
    if (!Weapons.IsValidIndex(Index))
    {
        return;
    }

    SelectedItemIndex = Index;
    EWE_LOG(LogEWEInventory, Log, TEXT("Selected Item: %s"), *Weapons[Index]->GetName());
}

void UEWEInventory::AssignToQuickSlot(int32 SlotIndex)
{
    if (SelectedItemIndex < 0 || !Weapons.IsValidIndex(SelectedItemIndex))
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("No item selected"));
        return;
    }

    UEWEWeaponData *SelectedWeapon = Weapons[SelectedItemIndex];

    // Get the subsystem directly from the owning player
    if (APlayerController* PC = GetOwningPlayer())
    {
        if (UEWELocalUIManageSubsystem* UIManager = PC->GetLocalPlayer()->GetSubsystem<UEWELocalUIManageSubsystem>())
        {
            UIManager->SetQuickSlot(SlotIndex, SelectedWeapon);
        }
    }
}
