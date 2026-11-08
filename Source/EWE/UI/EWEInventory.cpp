// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEInventory.h"
#include "EWEInventoryViewModel.h"
#include "EWE/EWELog.h"

UEWEInventory::UEWEInventory(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer)
{
    bIsInventoryOpen = false;
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

    // The slot display is driven by the inventory view model; the widget only toggles
    // visibility and the mouse cursor.
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
    SetVisibility(ESlateVisibility::Hidden);
    K2_OnInventoryClosed();
    SetMouseCursorVisible(false);

    // Clear the view model's selection. The view model may not be injected yet.
    if (UEWEInventoryViewModel *VM = ViewModel.Get())
    {
        VM->ClearSelection();
    }

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

void UEWEInventory::SetViewModel(UEWEInventoryViewModel *InViewModel)
{
    ViewModel = InViewModel;
}
