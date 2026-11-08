// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEInventory.h"
#include "EWE/EWELog.h"
#include "EWE/Character/EWEPlayerController.h"
#include "EWE/Character/EWEInventoryComponent.h"

UEWEInventory::UEWEInventory(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer)
{
    bIsInventoryOpen = false;
    SelectedItemIndex = -1;
}

void UEWEInventory::NativeConstruct()
{
    Super::NativeConstruct();

    const int32 InitialSlotCount = 8;

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
    SetMouseCursorVisible(true);

    // Pull the current weapon list from the Model so the display is fresh on open.
    UEWEInventoryComponent *Model = GetOwningInventoryModel();
    if (Model)
    {
        Weapons = Model->GetAllWeapons();
    }

    // Fire the open event after the list is fresh, so listeners see the current weapons.
    K2_OnInventoryOpened();

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
    APlayerController* PC = GetOwningPlayer();
    if (PC)
    {
        PC->SetShowMouseCursor(bShow);
    }
}

void UEWEInventory::SetWeapons(const TArray<UEWEWeaponInstance *> &InWeapons)
{
    Weapons = InWeapons;

    // TODO: Update slot UI (implemented in Blueprint)
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

UEWEInventoryComponent *UEWEInventory::GetOwningInventoryModel()
{
    AEWEPlayerController *PC = Cast<AEWEPlayerController>(GetOwningPlayer());
    if (!PC)
        return nullptr;

    return PC->GetInventoryModel();
}

void UEWEInventory::AssignToQuickSlot(int32 SlotIndex)
{
    if (SelectedItemIndex < 0 || !Weapons.IsValidIndex(SelectedItemIndex))
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("No item selected"));
        return;
    }

    UEWEWeaponInstance *SelectedInstance = Weapons[SelectedItemIndex];
    if (!SelectedInstance)
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("Selected item is not a valid instance"));
        return;
    }

    // The assignment converges on the Model's server gate. The widget does not own the
    // slot contents; it only requests the change.
    UEWEInventoryComponent *Model = GetOwningInventoryModel();
    if (!Model)
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("Cannot assign quick slot: no inventory model"));
        return;
    }

    Model->Server_AssignToQuickSlot(SelectedInstance, SlotIndex);
}
