// Fill out your copyright notice in the Description page of Project Settings.

#include "EWESlotItem.h"
#include "EWE/UI/EWESlotItemViewModel.h"
#include "View/MVVMView.h"
#include "EWE/EWELog.h"

void UEWESlotItem::SetViewModel(UEWESlotItemViewModel *InViewModel)
{
    if (!InViewModel)
    {
        EWE_LOG(LogEWE, Warning, TEXT("SetViewModel: slot item view model is null"));
        return;
    }

    UMVVMView *MVVMView = GetExtension<UMVVMView>();
    if (!MVVMView)
    {
        EWE_LOG(LogEWE, Warning, TEXT("Slot item widget has no MVVMView extension. Enable ViewModel in WBP Class Settings."));
        return;
    }

    // The name must match the view model declared in the WBP (BP_ItemSlot), or the
    // binding silently does nothing.
    MVVMView->SetViewModel(FName(TEXT("EWESlotItemViewModel")), InViewModel);
}
