// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEInventory.h"
#include "EWEInventoryViewModel.h"
#include "INotifyFieldValueChanged.h"
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
    if (!InViewModel)
    {
        EWE_LOG(LogEWEInventory, Warning, TEXT("SetViewModel: inventory view model is null"));
        return;
    }

    // Same view model — already bound. The subsystem injects once, but guard against a
    // duplicate subscription if it is called again.
    if (InViewModel == ViewModel.Get())
    {
        return;
    }

    // Unsubscribe from the previous view model before switching.
    UEWEInventoryViewModel *Previous = ViewModel.Get();
    if (Previous)
    {
        Previous->RemoveFieldValueChangedDelegate(
            UEWEInventoryViewModel::FFieldNotificationClassDescriptor::GetSlotCount,
            SlotCountChangedHandle);
    }
    SlotCountChangedHandle.Reset();

    ViewModel = InViewModel;

    // Bind this widget's own view to the view model (name must match the WBP declaration).
    BindViewModel(InViewModel, TEXT("EWEInventoryViewModel"));

    // Subscribe to the slot count so the grid can be created when the count becomes known.
    SlotCountChangedHandle = InViewModel->AddFieldValueChangedDelegate(
        UEWEInventoryViewModel::FFieldNotificationClassDescriptor::GetSlotCount,
        INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &UEWEInventory::OnSlotCountChanged));
}

void UEWEInventory::NativeDestruct()
{
    // Unsubscribe from the view model's GetSlotCount before it is torn down.
    UEWEInventoryViewModel *VM = ViewModel.Get();
    if (VM)
    {
        VM->RemoveFieldValueChangedDelegate(
            UEWEInventoryViewModel::FFieldNotificationClassDescriptor::GetSlotCount,
            SlotCountChangedHandle);
    }
    SlotCountChangedHandle.Reset();

    Super::NativeDestruct();
}

void UEWEInventory::OnSlotCountChanged(UObject *InViewModel, UE::FieldNotification::FFieldId InFieldId)
{
    // TODO: Build the inventory grid here — create one UEWESlotItem per slot and call
    // SetViewModel(VM->GetSlotItem(i)) on each. Filled in when the grid widget is implemented.
}
