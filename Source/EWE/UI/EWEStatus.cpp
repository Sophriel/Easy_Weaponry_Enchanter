// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEStatus.h"
#include "EWEStatusViewModel.h"
#include "View/MVVMView.h"
#include "EWE/EWELog.h"

void UEWEStatus::SetViewModel(UEWEStatusViewModel *InViewModel)
{
    if (!InViewModel)
    {
        EWE_LOG(LogEWE, Warning, TEXT("SetViewModel: status view model is null"));
        return;
    }

    UMVVMView *MVVMView = GetExtension<UMVVMView>();
    if (!MVVMView)
    {
        EWE_LOG(LogEWE, Warning, TEXT("Status widget has no MVVMView extension. Enable ViewModel in WBP Class Settings."));
        return;
    }

    // The name must match the view model declared in the WBP (BP_Status), or the
    // binding silently does nothing.
    MVVMView->SetViewModel(FName(TEXT("EWEStatusViewModel")), InViewModel);
}
